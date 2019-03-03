/* Copyright (c) 2017, 2018, 2019 Felix Kutzner (github.com/fkutzner)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

*/

#pragma once

#include <libjamsat/utils/Assert.h>

#include <boost/optional.hpp>

#include <cstdint>
#include <stdexcept>

namespace jamsat {

/**
 * \ingroup JamSAT_ClauseDB
 *
 * \brief Iterable clause-memory region
 * \tparam ClauseT TODO
 */
template <typename ClauseT>
class Region {
public:
    /**
     * \brief Initializes the region.
     *
     * If the allocation of the region's memory fails, the region is initialized with size 0.
     *
     * \param size      The region's size, in bytes.
     */
    explicit Region(std::size_t size) noexcept;

    /**
     * \brief Destroys the region and releases its associated memory.
     */
    ~Region();

    /**
     * \brief Allocates a clause.
     *
     * \param numLiterals       The size of the clause to be allocated, in literals.
     * \returns The allocated clause, or nullptr in case the allocation failed.
     */
    auto allocate(typename ClauseT::size_type numLiterals) noexcept -> ClauseT*;

    /**
     * \brief Returns the amount of bytes used up by allocations.
     *
     * \returns the amount of bytes used up by allocations.
     */
    auto getUsedSize() const noexcept -> std::size_t;

    /**
     * \brief Returns the amount of bytes available for allocations.
     *
     * \returns the amount of bytes available for allocations.
     */
    auto getFreeSize() const noexcept -> std::size_t;

    /**
     * \brief Clones the region.
     *
     * \returns a bitwise clone of this region if allocation succeeded, otherwise nothing.
     */
    auto clone() const noexcept -> boost::optional<Region>;

    /**
     * \brief Destroys all clauses in the region.
     */
    void clear() noexcept;

    /**
     * \brief Gets the pointer to the first clause stored in this region.
     *
     * \returns If the region contains a clause, the pointer to the first clause stored in this
     *          region is returned. Otherwise, nothing is returned.
     */
    auto getFirstClause() noexcept -> boost::optional<ClauseT*>;

    /**
     * \brief Gets the pointer to the first clause stored in this region (const version).
     *
     * \returns If the region contains a clause, the pointer to the first clause stored in this
     *          region is returned. Otherwise, nothing is returned.
     */
    auto getFirstClause() const noexcept -> boost::optional<ClauseT const*>;

    /**
     * \brief Gets the pointer to the clause physically succeeding the given clause in this region.
     *
     * \param clause  Pointer to a clause allocated in this region.
     *
     * \returns If the given clause is succeeded by a clause within this region, the pointer to
     *          the succeeding clause is returned. Otherwise, nothing is returned.
     */
    auto getNextClause(ClauseT const* clause) noexcept -> boost::optional<ClauseT*>;

    /**
     * \brief Gets the pointer to the clause physically succeeding the given clause in this region
     *        (const version).
     *
     * \param clause  Pointer to a clause allocated in this region.
     *
     * \returns If the given clause is succeeded by a clause within this region, the pointer to the
     *          succeeding clause is returned. Otherwise, nothing is returned.
     */
    auto getNextClause(ClauseT const* clause) const noexcept -> boost::optional<ClauseT const*>;

    auto operator=(Region&& rhs) noexcept -> Region&;
    Region(Region&& rhs) noexcept;

    // Deleting copy operations to guard against programming errors
    // See clone()
    auto operator=(Region const& rhs) -> Region& = delete;
    Region(Region const& rhs) = delete;

private:
    void* m_memory;
    void* m_nextFreeCell;

    std::size_t m_size;
    std::size_t m_free;
};

/********** Implementation ****************************** */

template <typename ClauseT>
Region<ClauseT>::Region(std::size_t size) noexcept
  : m_memory(nullptr), m_nextFreeCell(nullptr), m_size(size), m_free(size) {
    if (m_size > 0) {
        m_memory = std::malloc(size);
    }

    if (m_memory == nullptr) {
        m_size = 0;
        m_free = 0;
    }
    m_nextFreeCell = m_memory;
}

template <typename ClauseT>
Region<ClauseT>::~Region() {
    if (m_memory != nullptr) {
        std::free(m_memory);
        m_memory = nullptr;
    }
    m_nextFreeCell = nullptr;
}

template <typename ClauseT>
auto Region<ClauseT>::allocate(typename ClauseT::size_type numLiterals) noexcept -> ClauseT* {
    auto size_in_bytes = ClauseT::getAllocationSize(numLiterals);

    if (getFreeSize() < size_in_bytes) {
        return nullptr;
    }

    void* result = std::align(alignof(ClauseT), size_in_bytes, m_nextFreeCell, m_free);
    // m_nextFreeCell and m_free now have been updated by std::align
    if (result == nullptr) {
        return nullptr;
    }
    m_free -= size_in_bytes;
    m_nextFreeCell =
        reinterpret_cast<void*>(reinterpret_cast<char*>(m_nextFreeCell) + size_in_bytes);
    return ClauseT::constructIn(result, numLiterals);
}

template <typename ClauseT>
auto Region<ClauseT>::getUsedSize() const noexcept -> std::size_t {
    return m_size - m_free;
}

template <typename ClauseT>
auto Region<ClauseT>::getFreeSize() const noexcept -> std::size_t {
    return m_free;
}

template <typename ClauseT>
auto Region<ClauseT>::clone() const noexcept -> boost::optional<Region> {
    Region<ClauseT> result{m_size};
    if (result.getFreeSize() == 0) {
        // Allocation failed, or cloning a size-0 region
        return {};
    }
    std::memcpy(result.m_memory, this->m_memory, getUsedSize());
    result.m_nextFreeCell =
        reinterpret_cast<void*>(reinterpret_cast<char*>(result.m_memory) + getUsedSize());
    result.m_free = this->m_free;
    return result;
}

template <typename ClauseT>
void Region<ClauseT>::clear() noexcept {
    auto currentClause = getFirstClause();
    while (currentClause.has_value()) {
        ClauseT* currentClausePtr = *currentClause;
        currentClausePtr->~ClauseT();
        currentClause = getNextClause(currentClausePtr);
    }

    m_free = m_size;
    m_nextFreeCell = m_memory;
}

template <typename ClauseT>
auto Region<ClauseT>::operator=(Region&& rhs) noexcept -> Region& {
    std::swap(this->m_memory, rhs.m_memory);
    std::swap(this->m_nextFreeCell, rhs.m_nextFreeCell);
    std::swap(this->m_size, rhs.m_size);
    std::swap(this->m_free, rhs.m_free);
}

template <typename ClauseT>
Region<ClauseT>::Region(Region&& rhs) noexcept {
    this->m_memory = rhs.m_memory;
    this->m_nextFreeCell = rhs.m_nextFreeCell;
    this->m_free = rhs.m_free;
    this->m_size = rhs.m_size;

    rhs.m_memory = nullptr;
    rhs.m_nextFreeCell = nullptr;
    rhs.m_size = 0;
    rhs.m_free = 0;
}


template <typename ClauseT>
auto Region<ClauseT>::getFirstClause() noexcept -> boost::optional<ClauseT*> {
    // Delegating to the const version of this method:
    auto result = static_cast<Region<ClauseT> const*>(this)->getFirstClause();
    if (!result.has_value()) {
        return {};
    }

    ClauseT const* firstClause = *result;
    // This is OK since clause points to memory that is not actually constant:
    return const_cast<ClauseT*>(firstClause);
}

template <typename ClauseT>
auto Region<ClauseT>::getFirstClause() const noexcept -> boost::optional<ClauseT const*> {
    // The following code contains dangerous casts, which are required for computing offsets
    // and using std::align. The dubious pointers are only used for pointer computations:
    // They do not escape this method, and neither the state of the given clause nor the state
    // of the Region is modified in this method.

    if (getUsedSize() == 0) {
        return {};
    }

    // Unfortunately, there is no const version of std::align :(
    // The following is OK since std::align does not write to the pointed-to memory:
    void* memory = const_cast<void*>(m_memory);
    std::size_t dummy_free_size = m_size;
    void* firstClause = std::align(alignof(ClauseT), sizeof(ClauseT), memory, dummy_free_size);
    JAM_ASSERT(firstClause != nullptr, "An alignment operation failed which previously succeeded");
    return reinterpret_cast<ClauseT const*>(firstClause);
}

template <typename ClauseT>
auto Region<ClauseT>::getNextClause(ClauseT const* clause) noexcept -> boost::optional<ClauseT*> {
    // Delegating to the const version of this method:
    auto result = static_cast<Region<ClauseT> const*>(this)->getNextClause(clause);
    if (!result.has_value()) {
        return {};
    }

    ClauseT const* resultClause = *result;
    // This is OK since clause points to memory that is not actually constant:
    return const_cast<ClauseT*>(resultClause);
}

template <typename ClauseT>
auto Region<ClauseT>::getNextClause(ClauseT const* clause) const noexcept
    -> boost::optional<ClauseT const*> {
    // The following code contains dangerous casts, which are required for computing offsets
    // and using std::align. The dubious pointers are only used for pointer computations:
    // They do not escape this method, and neither the state of the given clause nor the state
    // of the Region is modified in this method.

    char const* clauseBytes = reinterpret_cast<char const*>(clause);
    char const* regionBytes = reinterpret_cast<char const*>(m_memory);
    std::size_t clauseSize = ClauseT::getAllocationSize(clause->initialSize());

    // Check if the clause is the last one in this region:
    if (std::distance(regionBytes, clauseBytes) == static_cast<long>(getUsedSize() - clauseSize)) {
        return {};
    }

    // Unfortunately, there is no const version of std::align :(
    // The following is OK since std::align does not write to the pointed-to memory:
    char const* candidatePtr = reinterpret_cast<char const*>(clause) + clauseSize;

    std::size_t dummy_free_size = m_size;
    void* candidatePtrAsVoid = reinterpret_cast<void*>(const_cast<char*>(candidatePtr));
    void* nextClause =
        std::align(alignof(ClauseT), sizeof(ClauseT), candidatePtrAsVoid, dummy_free_size);
    JAM_ASSERT(nextClause != nullptr, "An alignment operation failed which previously succeeded");
    return reinterpret_cast<ClauseT const*>(nextClause);
}

}