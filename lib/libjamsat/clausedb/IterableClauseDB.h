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

/**
 * \file IterableClauseDB.h
 * \brief A region allocator for indirectionless clauses (see e.g. Clause.h)
 */

#pragma once

#include <libjamsat/concepts/ClauseTraits.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/FlatteningIterator.h>
#include <libjamsat/utils/Logger.h>

#include <boost/range.hpp>

#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>


#if defined(JAM_ENABLE_CLAUSEDB_LOGGING)
#define JAM_LOG_ICDB(x, y) JAM_LOG(x, " icdb ", y)
#else
#define JAM_LOG_ICDB(x, y)
#endif

namespace jamsat {

/**
 * \ingroup JamSAT_ClauseDB
 *
 * \brief Iterator type for Region<T>
 *
 * This iterator satisfies the ForwardIterator concept.
 */
template <typename RegionT>
class RegionIterator {
public:
    using value_type = std::remove_reference_t<typename RegionT::item_type>;
    using difference_type = std::size_t;
    using reference = std::add_lvalue_reference_t<typename RegionT::item_type>;
    using pointer = std::add_pointer_t<value_type>;
    using iterator_category = std::forward_iterator_tag;

    /**
     * Constructs the default RegionIterator, which is typically used
     * as the "end" iterator.
     */
    RegionIterator() noexcept;

    /**
     * Constructs a RegionIterator.
     *
     * \param region    The region object to traverse. Traversal starts at the first item in
     *                  the region and ends with the last item in the region. Once the last
     *                  item of the region object has been traversed, the iterator becomes
     *                  equal to the default-constructed RegionIterator.
     */
    explicit RegionIterator(RegionT& region) noexcept;

    RegionIterator(RegionIterator const& rhs) = default;
    RegionIterator(RegionIterator&& rhs) = default;

    auto operator=(RegionIterator const& rhs) -> RegionIterator& = default;
    auto operator=(RegionIterator&& rhs) -> RegionIterator& = default;

    auto operator++() noexcept -> RegionIterator&;
    auto operator++(int) noexcept -> RegionIterator;

    auto operator*() const noexcept -> reference;
    auto operator->() const noexcept -> reference;

    auto operator==(RegionIterator const& rhs) const noexcept -> bool;
    auto operator!=(RegionIterator const& rhs) const noexcept -> bool;

private:
    typename RegionT::item_type* m_currentItem;
    RegionT* m_region;
};

/**
 * \ingroup JamSAT_ClauseDB
 *
 * \brief Iterable region allocator for types satisfying the VarsizedIntoConstructible
 *        concept
 *
 * \tparam ClauseT A type satisfying the VarsizedIntoConstructible concept.
 */
template <typename ClauseT>
class Region {
    static_assert(is_varsized_into_constructible<ClauseT>::value,
                  "ClauseT must satisfy the VarsizedIntoConstructible concept, but does not");

public:
    using item_type = std::remove_reference_t<ClauseT>;
    using size_type = typename ClauseT::size_type;

    /**
     * \brief Initializes the region.
     *
     * \param size          The region's size, in bytes. `size` must be greater than 0.
     * \throws bad_alloc    The allocation of the region's memory failed.
     */
    explicit Region(std::size_t size);

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
    auto allocate(size_type numLiterals) noexcept -> ClauseT*;

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
     * \brief Returns `true` iff the region is empty.
     */
    auto empty() const noexcept -> bool;

    /**
     * \brief Clones the region.
     *
     * \returns a bitwise clone of this region if allocation succeeded, otherwise nothing.
     */
    auto clone() const noexcept -> std::optional<Region>;

    /**
     * \brief Destroys all clauses in the region.
     */
    void clear() noexcept;

    /**
     * \brief Returns a "begin" iterator of the region.
     *
     * If the region is not empty, the returned iterator points to the first item of the region.
     * Otherwise, an iterator is returned that may not be dereferenced and is equal to all
     * iterators returned by `end()`.
     *
     * After finitely many calls to operator++(), the returned iterator is equal to all
     * iterators returned by `end()`.
     *
     * \returns a RegionIterator as described above.
     */
    auto begin() noexcept -> RegionIterator<Region<ClauseT>>;

    /**
     * \brief Returns the "end" iterator of the region.
     *
     * See `begin()`.
     *
     * \returns the "end" iterator of the region, which may not be dereferenced.
     */
    auto end() noexcept -> RegionIterator<Region<ClauseT>>;

    // NB: const versions of begin(), end() omitted until there is an actual use case for them
    // within JamSAT.

    auto operator=(Region&& rhs) noexcept -> Region&;
    Region(Region&& rhs) noexcept;

    // Deleting copy operations to guard against programming errors
    // See clone()
    auto operator=(Region const& rhs) -> Region& = delete;
    Region(Region const& rhs) = delete;

private:
    friend class RegionIterator<Region<ClauseT>>;

    /**
     * \brief Gets the pointer to the first clause stored in this region.
     *
     * \returns If the region contains a clause, the pointer to the first clause stored in this
     *          region is returned. Otherwise, nothing is returned.
     */
    auto getFirstClause() noexcept -> ClauseT*;

    /**
     * \brief Gets the pointer to the clause physically succeeding the given clause in this region.
     *
     * \param clause  Pointer to a clause allocated in this region.
     *
     * \returns If the given clause is succeeded by a clause within this region, the pointer to
     *          the succeeding clause is returned. Otherwise, nothing is returned.
     */
    auto getNextClause(ClauseT const* clause) noexcept -> ClauseT*;

    void* m_memory;
    void* m_nextFreeCell;

    std::size_t m_size;
    std::size_t m_free;
};


/**
 * \ingroup JamSAT_ClauseDB
 *
 * \brief Iterable clause database
 *
 * This data structure affords fast allocation of clauses and iteration over all allocated
 * clauses.
 *
 * \tparam ClauseT A type satisfying the VarsizedIntoConstructible concept and the Clause concept.
 */
template <typename ClauseT>
class IterableClauseDB {
    static_assert(is_varsized_into_constructible<ClauseT>::value,
                  "ClauseT must satisfy the VarsizedIntoConstructible concept, but does not");

    static_assert(std::is_copy_assignable<ClauseT>::value,
                  "ClauseT must be copy-assignable, but is not");

    static_assert(is_clause_flaggable<ClauseT>::value,
                  "ClauseT must satisfy the ClauseFlaggable concept, but does not");

public:
    using size_type = std::size_t;

    /**
     * \brief Constructs an IterableClauseDB instance.
     *
     * \param regionSize    The size of the memory chunks allocated by this clause database.
     */
    explicit IterableClauseDB(size_type regionSize) noexcept;

    /**
     * \brief Creates a new clause.
     *
     * \param size      The clause's size, in literals.
     *
     * Creating a new clause may fail due to allocation errors, which may occur when the
     * clause is too large to be stored in a single memory chunk or when no memory could be
     * allocated.
     *
     * NB: Allocation failures due to clauses could be avoided by not allocating such large
     * clauses using the `Region<T>` data structure. However, this is a remote pathological
     * case, as region sizes will be in the range of dozens of megabytes, and current SAT
     * problems have few enough variables such that clauses not fitting in a Region<T> would
     * need to contain duplicate literals, which are however eliminated by the solver.
     *
     * \returns A pointer to the new clause. If allocation failed, nullptr is returned instead.
     */
    auto createClause(size_type size) noexcept -> ClauseT*;

    /**
     * \brief Compresses the database by removing all clauses scheduled for deletion.
     *
     * A clause is scheduled for deletion iff its SCHEDULED_FOR_DELETION flag is set.
     *
     * This operation invalidates all pointers to clauses stored in this database.
     */
    void compress() noexcept;

    /** Iterator type for iterating over the database's clauses */
    using iterator = FlatteningIterator<typename std::vector<Region<ClauseT>>::iterator>;

    /**
     * \brief Gets a range of clauses stored in this database.
     *
     * The returned range is invalidated by any call to `compress()`.
     *
     * \returns a range of clauses stored in this database, referencing the clauses in the
     *          order of addition.
     */
    auto getClauses() noexcept -> boost::iterator_range<iterator>;

private:
    auto createActiveRegion() -> Region<ClauseT>&;

    size_type m_regionSize;
    std::vector<Region<ClauseT>> m_activeRegions;
    std::vector<Region<ClauseT>> m_spareRegions;
};
}

namespace std {
template <typename T>
struct iterator_traits<typename jamsat::RegionIterator<T>> {
    using value_type = typename jamsat::RegionIterator<T>::value_type;
    using difference_type = typename jamsat::RegionIterator<T>::difference_type;
    using reference = typename jamsat::RegionIterator<T>::reference;
    using pointer = typename jamsat::RegionIterator<T>::pointer;
    using iterator_category = typename jamsat::RegionIterator<T>::iterator_category;
};
}


/********** Implementation ****************************** */

namespace jamsat {

template <typename ClauseT>
Region<ClauseT>::Region(std::size_t size)
  : m_memory(nullptr), m_nextFreeCell(nullptr), m_size(size), m_free(size) {
    JAM_ASSERT(size > 0, "Region<T> must be initialized with a size greater than 0");
    m_memory = std::malloc(size);
    m_nextFreeCell = m_memory;

    if (m_memory == nullptr) {
        throw std::bad_alloc{};
    }
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
auto Region<ClauseT>::allocate(size_type numLiterals) noexcept -> ClauseT* {
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
auto Region<ClauseT>::empty() const noexcept -> bool {
    return m_free == m_size;
}

template <typename ClauseT>
auto Region<ClauseT>::clone() const noexcept -> std::optional<Region> {
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
    while (currentClause != nullptr) {
        currentClause->~ClauseT();
        currentClause = getNextClause(currentClause);
    }

    m_free = m_size;
    m_nextFreeCell = m_memory;
}

template <typename ClauseT>
auto Region<ClauseT>::begin() noexcept -> RegionIterator<Region<ClauseT>> {
    return RegionIterator<Region<ClauseT>>{*this};
}

template <typename ClauseT>
auto Region<ClauseT>::end() noexcept -> RegionIterator<Region<ClauseT>> {
    return RegionIterator<Region<ClauseT>>{};
}

template <typename ClauseT>
auto Region<ClauseT>::operator=(Region&& rhs) noexcept -> Region& {
    std::swap(this->m_memory, rhs.m_memory);
    std::swap(this->m_nextFreeCell, rhs.m_nextFreeCell);
    std::swap(this->m_size, rhs.m_size);
    std::swap(this->m_free, rhs.m_free);
    return *this;
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
auto Region<ClauseT>::getFirstClause() noexcept -> ClauseT* {
    // The following code contains dangerous casts, which are required for computing offsets
    // and using std::align. The dubious pointers are only used for pointer computations:
    // They do not escape this method, and neither the state of the given clause nor the state
    // of the Region is modified in this method.

    if (getUsedSize() == 0) {
        return nullptr;
    }

    std::size_t dummy_free_size = m_size;
    void* memory = m_memory;
    void* firstClause = std::align(alignof(ClauseT), sizeof(ClauseT), memory, dummy_free_size);
    JAM_ASSERT(firstClause != nullptr, "An alignment operation failed which previously succeeded");
    return reinterpret_cast<ClauseT*>(firstClause);
}

template <typename ClauseT>
auto Region<ClauseT>::getNextClause(ClauseT const* clause) noexcept -> ClauseT* {
    // The following code contains dangerous casts, which are required for computing offsets
    // and using std::align. The dubious pointers are only used for pointer computations:
    // They do not escape this method, and neither the state of the given clause nor the state
    // of the Region is modified in this method.

    char const* clauseBytes = reinterpret_cast<char const*>(clause);
    char const* regionBytes = reinterpret_cast<char const*>(m_memory);
    std::size_t clauseSize = ClauseT::getAllocationSize(clause->initialSize());

    // Check if the clause is the last one in this region:
    if (std::distance(regionBytes, clauseBytes) == static_cast<long>(getUsedSize() - clauseSize)) {
        return nullptr;
    }

    // Unfortunately, there is no const version of std::align :(
    // The following is OK since std::align does not write to the pointed-to memory:
    char const* candidatePtr = reinterpret_cast<char const*>(clause) + clauseSize;

    std::size_t dummy_free_size = m_size;
    void* candidatePtrAsVoid = reinterpret_cast<void*>(const_cast<char*>(candidatePtr));
    void* nextClause =
        std::align(alignof(ClauseT), sizeof(ClauseT), candidatePtrAsVoid, dummy_free_size);
    JAM_ASSERT(nextClause != nullptr, "An alignment operation failed which previously succeeded");
    return reinterpret_cast<ClauseT*>(nextClause);
}


template <typename RegionT>
RegionIterator<RegionT>::RegionIterator() noexcept : m_currentItem(nullptr), m_region(nullptr) {}

template <typename RegionT>
RegionIterator<RegionT>::RegionIterator(RegionT& region) noexcept
  : m_currentItem(region.getFirstClause()), m_region(&region) {}

template <typename RegionT>
auto RegionIterator<RegionT>::operator++() noexcept -> RegionIterator& {
    if (m_currentItem != nullptr) {
        m_currentItem = m_region->getNextClause(m_currentItem);
    }
    return *this;
}

template <typename RegionT>
auto RegionIterator<RegionT>::operator++(int) noexcept -> RegionIterator {
    RegionIterator result = *this;
    operator++();
    return result;
}

template <typename RegionT>
auto RegionIterator<RegionT>::operator*() const noexcept -> reference {
    JAM_ASSERT(m_currentItem != nullptr, "Illegally dereferencing RegionIterator");
    return *m_currentItem;
}

template <typename RegionT>
auto RegionIterator<RegionT>::operator->() const noexcept -> reference {
    JAM_ASSERT(m_currentItem != nullptr, "Illegally dereferencing RegionIterator");
    return *m_currentItem;
}

template <typename RegionT>
auto RegionIterator<RegionT>::operator==(RegionIterator const& rhs) const noexcept -> bool {
    return (this == &rhs) || (this->m_currentItem == nullptr && rhs.m_currentItem == nullptr) ||
           (this->m_currentItem == rhs.m_currentItem && this->m_region == rhs.m_region);
}

template <typename RegionT>
auto RegionIterator<RegionT>::operator!=(RegionIterator const& rhs) const noexcept -> bool {
    return !(*this == rhs);
}


template <typename ClauseT>
IterableClauseDB<ClauseT>::IterableClauseDB(size_type regionSize) noexcept
  : m_regionSize(regionSize), m_activeRegions(), m_spareRegions() {}

template <typename ClauseT>
auto IterableClauseDB<ClauseT>::createClause(size_type size) noexcept -> ClauseT* {

    // Check if a clause of the requested size can be constructed:
    uintmax_t maxClauseSize = std::numeric_limits<typename ClauseT::size_type>::max();
    auto newClauseSize = static_cast<typename ClauseT::size_type>(size);
    if (static_cast<uintmax_t>(size) > static_cast<uintmax_t>(maxClauseSize) ||
        ClauseT::getAllocationSize(newClauseSize) > m_regionSize) {
        return nullptr;
    }

    try {
        Region<ClauseT>* targetRegion =
            m_activeRegions.empty() ? &createActiveRegion() : &(m_activeRegions.back());
        ClauseT* clause = targetRegion->allocate(newClauseSize);
        if (clause) {
            return clause;
        }

        // Allocation failed, create new region and retry:
        clause = createActiveRegion().allocate(newClauseSize);
        return clause;
    } catch (std::bad_alloc&) {
        return nullptr;
    }
}


template <typename ClauseT>
void IterableClauseDB<ClauseT>::compress() noexcept {
    JAM_LOG_ICDB(info,
                 "Compressing the clause DB (" << m_activeRegions.size() << " active regions, "
                                               << m_spareRegions.size() << " spare regions)");
    OnExitScope printInfo{[this]() {
        JAM_LOG_ICDB(info,
                     "Finished compressing the clause DB ("
                         << m_activeRegions.size() << " active regions, " << m_spareRegions.size()
                         << " spare regions)");
    }};

    if (m_activeRegions.empty()) {
        return;
    }

    JAM_ASSERT(!m_spareRegions.empty(), "There must be at least 1 spare region");

    Region<ClauseT> currentSpare = std::move(m_spareRegions.back());
    m_spareRegions.pop_back();
    JAM_ASSERT(currentSpare.empty(), "Spare regions must be empty");

    std::size_t swapInIndex = 0;
    for (auto& region : m_activeRegions) {
        // Let idx be the index of `region` in `m_activeRegions`.
        // Loop invariant A: (swapInIndex < idx) || (currentSpare.getFreeSize() >=
        // region.getUsedSize())
        JAM_ASSERT(&region != &m_activeRegions[swapInIndex] ||
                       (currentSpare.getFreeSize() >= region.getUsedSize()),
                   "Loop invariant A violated");

        for (ClauseT& clause : region) {
            if (clause.getFlag(ClauseT::Flag::SCHEDULED_FOR_DELETION)) {
                continue;
            }

            ClauseT* copy = currentSpare.allocate(clause.size());
            if (copy == nullptr) {
                // currentSpare is full.
                // Invariant: all clauses in m_activeRegions[swapInIndex] have already been copied
                // either to currentSpare or to m_activeRegions[i] for some 0 <= i < swapInIndex.

                JAM_ASSERT(&region != &m_activeRegions[swapInIndex], "Loop invariant A violated");
                std::swap(currentSpare, m_activeRegions[swapInIndex]);
                swapInIndex += 1;
                JAM_ASSERT(currentSpare.getUsedSize() == 0, "Spare regions must be empty");
                copy = currentSpare.allocate(clause.size());
            }
            *copy = clause;
        }

        region.clear();
    }
    std::swap(currentSpare, m_activeRegions[swapInIndex]);

    // Collect "retired" regions for reuse
    currentSpare.clear();
    m_spareRegions.push_back(std::move(currentSpare));
    while (m_activeRegions.size() > 1 && m_activeRegions.back().getUsedSize() == 0) {
        m_spareRegions.push_back(std::move(m_activeRegions.back()));
        m_activeRegions.pop_back();
    }
}

template <typename ClauseT>
auto IterableClauseDB<ClauseT>::createActiveRegion() -> Region<ClauseT>& {
    // Make sure that at least two regions are in the list of spares
    // (One for immediate use as an active region, one as a spare for compress())
    for (auto i = m_spareRegions.size(); i <= 2; ++i) {
        m_spareRegions.push_back(Region<ClauseT>{m_regionSize});
    }

    m_activeRegions.push_back(std::move(m_spareRegions.back()));
    m_spareRegions.pop_back();

    // Throwing bad_alloc exceptions only here, to keep compress() exception-safe.
    // Note that region objects are small objects pointing to large regions, so this isn't
    // very expensive:
    m_spareRegions.reserve(m_spareRegions.size() + m_activeRegions.size());
    m_activeRegions.reserve(m_spareRegions.size() + m_activeRegions.size());

    return m_activeRegions.back();
}

template <typename ClauseT>
auto IterableClauseDB<ClauseT>::getClauses() noexcept -> boost::iterator_range<iterator> {
    return boost::make_iterator_range(iterator{m_activeRegions.begin(), m_activeRegions.end()},
                                      iterator{});
}

}