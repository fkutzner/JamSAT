/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)

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

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

#include <libjamsat/utils/Assert.h>

namespace jamsat {
namespace clausedb_detail {

/**
 * \ingroup JamSAT_ClauseDB
 *
 * \brief Simple allocator for a fixed-size chunk of heap memory.
 */
class Heaplet {
public:
    using size_type = uintptr_t;

    /**
     * \brief Constructs a Heaplet.
     *
     * No memory is allocated during construction. The heaplet can only be
     * used after the initialize() method has been called.
     *
     * \param size  The size of the heaplet in bytes.
     */
    explicit Heaplet(size_type size) noexcept
      : m_memory(nullptr), m_firstFree(nullptr), m_size(size), m_free(0) {}

    ~Heaplet();

    /**
     * \brief Initializes the Heaplet.
     *
     * \throws std::bad_alloc when memory allocation failed.
     */
    void initialize();

    /**
     * \brief Returns true iff the Heaplet has been initialized.
     *
     * \returns true iff the Heaplet has been initialized.
     */
    bool isInitialized() const noexcept;

    /**
     * \brief Empties the Heaplet.
     *
     * This method may only be called for initialized Heaplets. After calling
     * this method, the Heaplet is reset to its state just after initialization.
     */
    void clear() noexcept;

    /**
     * \brief Allocates memory in the Heaplet.
     *
     * If the allocation is successful, the allocated memory is initialized by calling
     * the function T::constructIn, passing a pointer to the allocated memory as well as
     * \p constructionArgs to that function.
     *
     * \param size             The size of the allocated object, in bytes. \p size must
     *                         not be smaller than `sizeof(T)`.
     * \param constructionArgs The arguments to be passed to T::constructIn(...).
     * \returns         If the allocation was successful, a pointer to the first byte
     *                  of the allocated memory is returned. Otherwise, `nullptr` is returned.
     *                  If the returned pointer is not nullptr, it is aligned such that an
     *                  object of type `T` can be stored at that location.
     * \tparam T        The non-void type of the allocated object. T::constructIn must be
     *                  a static function of signature T* T::constructIn(void*, CtorArgs...).
     * \tparam CtorArgs... The argument types of T::constructIn(void*, CtorArgs...).
     */
    template <typename T, typename... CtorArgs>
    T *allocate(size_type size, CtorArgs &&... constructionArgs) noexcept;

    /**
     * \brief Returns the amount of bytes which are available for allocation.
     *
     * \returns The amount of bytes which are available for allocation.
     */
    size_type getFreeSize() const noexcept { return m_free; }

    Heaplet &operator=(const Heaplet &other) = delete;
    Heaplet(const Heaplet &other) = delete;

    Heaplet &operator=(Heaplet &&other) noexcept;
    Heaplet(Heaplet &&other) noexcept;

#if defined(JAM_EXPOSE_INTERNAL_TESTING_INTERFACES)
    // Functions for testing the internal state of heaplets:
    bool test_isRegionInHeaplet(const void *ptr, size_type length) const noexcept;
#endif

private:
    void *m_memory;

    // Pointer to the first free byte of the Heaplet. In case the Heaplet is full,
    // this is a past-the-end pointer.
    // Invariant: m_memory != nullptr => (m_memory <= m_firstFree <= m_memory + m_size)
    void *m_firstFree;

    size_type m_size;

    // Invariant: m_free <= m_size
    size_type m_free;
};
}

/**
 * \ingroup JamSAT_ClauseDB
 *
 * \brief Simple allocator storing clauses in contiguous chunks of memory ("heaplets"),
 *   with their in-chunk positions relative to each other controllable by the user.
 *
 * \tparam ClauseT      The clause type. (TODO: define "full" clause concept)
 */
template <typename ClauseT>
class HeapletClauseDB {
public:
    using ReasonClausePredicateFunc = std::function<bool(const ClauseT &) noexcept>;
    using RelocateReasonClauseFunc = std::function<void(const ClauseT &, const ClauseT &) noexcept>;

    using size_type = uintptr_t;

    /**
     * \brief Constructs a HeapletClauseDB.
     *
     * TODO: document exceptions and exception safety
     *
     * \param heapletSize     The size of a heaplet, in bytes.
     * \param memoryLimit     The maximum amount of memory allocated to heaplets.
     */
    HeapletClauseDB(size_type heapletSize, size_type memoryLimit);

    /**
     * \brief Allocates a new clause.
     *
     * TODO: document exceptions and exception safety
     *
     * This method does not reallocate existing clauses.
     *
     * \param size      The size of the clause, in literals.
     * \returns         A reference to the newly constructed clause. Ownership of the clause
     *                  remains with the allocator.
     */
    ClauseT &allocate(typename ClauseT::size_type size);

    /**
     * \brief Deletes all clauses except the specified ones.
     *
     * Deletes all clauses not iterated over by \p clausePointers and reallocates the
     * clauses iterated over by \p clausePointers such that for all clause pointers c1, c2, c3
     * in \p clausePointers with c1 < c2 < c3 (wrt. order of iteration), the following holds:
     *
     *  - if the clauses to which c1 and c3 point are contained in the same heaplet,
     *    then the clause pointed to c1 occurs at a lower address than the clause pointed to
     *    by c3.
     *  - if the clauses to which c1 and c3 point are contained in the same heaplet,
     *    the clause to which c2 points is also contained in that heaplet.
     *
     * This method relocates all clauses which are retained in the clause database. Pointers
     * to the new clauses are stored in the range beginning by \p relocedReceiver if the user
     * supplies \p{relocedReceiver}.
     *
     * When this method finishes execution without throwing an exception, all pointers over
     * which \p clausePointers iterates are invalid. Otherwise, the state before calling
     * `retain()` is restored and the pointers in \p clausePointers remain valid.
     *
     * TODO: document exceptions
     *
     * \param clausePointers    An object which is iterable over clause pointers. For each
     *                          clause pointer `p` iterated by \p clausePointers, the following
     *                          must hold: the clause to which `p` points must contain more
     *                          than 2 literals, `p` must be valid and must have been allocated
     *                          by the same allocator objet where `retain` is called.
     *                          \p clausePointers must not contain any pointer to empty clauses.
     *                          \p clausePointers may contain duplicates; if a clause pointer has
     *                          already been processed, it is ignored.
     *
     * \param reasonPredicate   A function determining whether a given clause is a reason clause.
     *
     * \param reasonRelocator   A function that is called whenever a reason clause has been moved in
     *                          memory. A reference to the "old" clause is passed as the first
     *                          argument, a reference to the "new" clause as the second one. When
     *                          \p reasonRelocator is called, both clauses may be
     *                          dereferenced; however, the "old" clause will have been resized
     *                          to contain 0 literals.
     *
     * \param relocedReceiver   An optional output iterator of a destination range where the
     *                          pointers to the new clauses should to be stored.
     *
     * \tparam ClauseTIterable  A type satisfying the InputIterator concept for pointers to
     *                          `ClauseT`.
     */
    template <typename ClauseTIterable, typename ClauseTPtrOutputIter>
    void retain(const ClauseTIterable &clausePointers, ReasonClausePredicateFunc m_isReasonClause,
                RelocateReasonClauseFunc m_relocateReasonClause,
                boost::optional<ClauseTPtrOutputIter> relocedReceiver);

#if defined(JAM_EXPOSE_INTERNAL_TESTING_INTERFACES)
    // Functions for testing the internal state of the clause database:
    bool test_isRegionInHeapletList(const std::vector<clausedb_detail::Heaplet> &heaplets,
                                    const void *ptr, size_type length) const noexcept;
    bool test_isRegionInActiveHeaplet(const void *ptr, size_type length) const noexcept;
    bool test_isRegionInBinaryHeaplet(const void *ptr, size_type length) const noexcept;
    size_type
    test_getAvailableSpaceInHeapletList(const std::vector<clausedb_detail::Heaplet> &heaplets) const
        noexcept;
    size_type test_getAvailableSpaceInActiveHeaplets() const noexcept;
    size_type test_getAvailableSpaceInBinaryHeaplets() const noexcept;
    size_type test_getAvailableSpaceInFreeHeaplets() const noexcept;
#endif

private:
    using HeapletList = std::vector<clausedb_detail::Heaplet>;

    ClauseT &allocateIn(typename ClauseT::size_type size, HeapletList &target, HeapletList &from);

    size_type m_heapletSize;
    size_type m_memoryLimit;

    HeapletList m_activeHeaplets;
    HeapletList m_binaryHeaplets;
    HeapletList m_freeHeapletPool;
};


/********** Implementation ****************************** */

namespace clausedb_detail {
inline Heaplet::~Heaplet() {
    if (m_memory != nullptr) {
        std::free(m_memory);
        m_memory = nullptr;
    }
}

inline void Heaplet::initialize() {
    JAM_ASSERT(m_memory == nullptr, "Cannot initialize a heaplet twice");
    m_memory = std::malloc(m_size);
    if (m_memory == nullptr) {
        throw std::bad_alloc{};
    }
    clear();
}

inline bool Heaplet::isInitialized() const noexcept {
    return m_memory != nullptr;
}

inline void Heaplet::clear() noexcept {
    JAM_ASSERT(isInitialized(), "Cannot reset an uninitialized Heaplet");
    m_firstFree = m_memory;
    m_free = m_size;
}

template <typename T, typename... CtorArgs>
T *Heaplet::allocate(size_type size, CtorArgs &&... constructionArgs) noexcept {
    JAM_ASSERT(isInitialized(), "Cannot allocate on an uninitialized Heaplet");
    JAM_ASSERT(size >= sizeof(T), "Fewer bytes allocated than required by type");

    void *result = std::align(alignof(T), size, m_firstFree, m_free);
    if (result == nullptr) {
        return nullptr;
    }
    m_free -= size;
    m_firstFree = reinterpret_cast<char *>(m_firstFree) + size;
    return T::constructIn(result, std::forward<CtorArgs>(constructionArgs)...);
}

inline Heaplet &Heaplet::operator=(Heaplet &&other) noexcept {
    if (m_memory != nullptr) {
        std::free(m_memory);
    }

    m_memory = other.m_memory;
    m_firstFree = other.m_firstFree;
    m_size = other.m_size;
    m_free = other.m_free;
    other.m_memory = nullptr;
    other.m_free = other.m_size;
    return *this;
}

inline Heaplet::Heaplet(Heaplet &&other) noexcept {
    m_memory = other.m_memory;
    m_firstFree = other.m_firstFree;
    m_size = other.m_size;
    m_free = other.m_free;
    other.m_memory = nullptr;
    other.m_free = other.m_size;
}

#if defined(JAM_EXPOSE_INTERNAL_TESTING_INTERFACES)
// Functions for testing the internal state of heaplets:
inline bool Heaplet::test_isRegionInHeaplet(const void *ptr, size_type length) const noexcept {
    if (!isInitialized()) {
        return false;
    }
    uintptr_t ptrAsInt = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t memAsInt = reinterpret_cast<uintptr_t>(m_memory);
    return ptrAsInt >= memAsInt && (ptrAsInt + length) < (memAsInt + m_size);
}
#endif
}

template <typename ClauseT>
HeapletClauseDB<ClauseT>::HeapletClauseDB(size_type heapletSize, size_type memoryLimit)
  : m_heapletSize(heapletSize)
  , m_memoryLimit(memoryLimit)
  , m_activeHeaplets()
  , m_binaryHeaplets()
  , m_freeHeapletPool() {

    size_type numHeaplets = memoryLimit / heapletSize;

    JAM_ASSERT(numHeaplets >= 2, "Insufficient memoryLimit");
    for (size_type i = 0; i < (numHeaplets - 2); ++i) {
        m_freeHeapletPool.emplace_back(heapletSize);
    }
    m_activeHeaplets.emplace_back(heapletSize);
    m_binaryHeaplets.emplace_back(heapletSize);

    // needed for exception safety:
    m_activeHeaplets.reserve(numHeaplets);
    m_binaryHeaplets.reserve(numHeaplets);
    m_freeHeapletPool.reserve(numHeaplets);
}

template <typename ClauseT>
ClauseT &HeapletClauseDB<ClauseT>::allocateIn(typename ClauseT::size_type size,
                                              HeapletList &targetPool, HeapletList &freePool) {
    auto &currentHeaplet = targetPool.back();
    if (!currentHeaplet.isInitialized()) {
        currentHeaplet.initialize();
    }

    ClauseT *newClause = currentHeaplet.allocate<ClauseT>(ClauseT::getAllocationSize(size), size);
    if (newClause == nullptr) {
        if (freePool.empty()) {
            throw std::bad_alloc{};
        }
        targetPool.push_back(std::move(m_freeHeapletPool.back()));
        freePool.pop_back();

        auto &freeHeaplet = targetPool.back();
        if (!freeHeaplet.isInitialized()) {
            freeHeaplet.initialize();
        }

        newClause = freeHeaplet.allocate<ClauseT>(ClauseT::getAllocationSize(size), size);
        if (newClause == nullptr) {
            throw std::bad_alloc{};
        }
    }
    return *newClause;
}

template <typename ClauseT>
ClauseT &HeapletClauseDB<ClauseT>::allocate(typename ClauseT::size_type size) {
    JAM_ASSERT(size >= 2ull, "Can't allocate clauses of size 0 or 1");
    auto &pool = (size == 2) ? m_binaryHeaplets : m_activeHeaplets;
    return allocateIn(size, pool, m_freeHeapletPool);
}

template <typename ClauseT>
template <typename ClauseTIterable, typename ClauseTPtrOutputIter>
void HeapletClauseDB<ClauseT>::retain(const ClauseTIterable &clausePointers,
                                      ReasonClausePredicateFunc isReasonClauseFn,
                                      RelocateReasonClauseFunc relocateReasonClauseFn,
                                      boost::optional<ClauseTPtrOutputIter> relocedReceiver) {
    if (m_freeHeapletPool.empty()) {
        throw std::bad_alloc{};
    }

    std::vector<clausedb_detail::Heaplet> newActiveHeaplets;
    newActiveHeaplets.push_back(std::move(m_freeHeapletPool.back()));
    m_freeHeapletPool.pop_back();

    // Postponing the announcement of reason clause replacements for exception safety
    std::vector<std::pair<const ClauseT *, const ClauseT *>> reasonClauses;
    for (auto oldClauseConst : clausePointers) {
        // This const_cast is safe since the clause memory is guaranteed to be non-constant:
        ClauseT *oldClause = const_cast<ClauseT *>(oldClauseConst);
        // TODO: eliminate the const_cast? It's safe, but rather ugly.

        auto size = oldClause->size();

        if (size == 0ull) {
            // the clause has already been relocated
            continue;
        }

        auto &replacement = allocateIn(size, newActiveHeaplets, m_freeHeapletPool);
        replacement = *oldClause;

        if (isReasonClauseFn(*oldClause)) {
            reasonClauses.emplace_back(oldClause, &replacement);
        }
        if (relocedReceiver) {
            (**relocedReceiver) = &replacement;
            ++(*relocedReceiver);
        }

        oldClause->resize(0);
    }

    for (auto reasonReplacement : reasonClauses) {
        relocateReasonClauseFn(*reasonReplacement.first, *reasonReplacement.second);
    }

    std::swap(newActiveHeaplets, m_activeHeaplets);
    for (auto &freeHeaplet : newActiveHeaplets) {
        if (freeHeaplet.isInitialized()) {
            freeHeaplet.clear();
        }
        m_freeHeapletPool.push_back(std::move(freeHeaplet));
    }
}

#if defined(JAM_EXPOSE_INTERNAL_TESTING_INTERFACES)

template <typename ClauseT>
bool HeapletClauseDB<ClauseT>::test_isRegionInHeapletList(
    const std::vector<clausedb_detail::Heaplet> &heaplets, const void *ptr, size_type length) const
    noexcept {
    for (auto &heaplet : heaplets) {
        if (heaplet.test_isRegionInHeaplet(ptr, length)) {
            return true;
        }
    }
    return false;
}

template <typename ClauseT>
bool HeapletClauseDB<ClauseT>::test_isRegionInActiveHeaplet(const void *ptr, size_type length) const
    noexcept {
    return test_isRegionInHeapletList(m_activeHeaplets, ptr, length);
}

template <typename ClauseT>
bool HeapletClauseDB<ClauseT>::test_isRegionInBinaryHeaplet(const void *ptr, size_type length) const
    noexcept {
    return test_isRegionInHeapletList(m_binaryHeaplets, ptr, length);
}

template <typename ClauseT>
typename HeapletClauseDB<ClauseT>::size_type
HeapletClauseDB<ClauseT>::test_getAvailableSpaceInHeapletList(
    const std::vector<clausedb_detail::Heaplet> &heaplets) const noexcept {
    size_type result = 0;
    for (auto &heaplet : heaplets) {
        if (heaplet.isInitialized()) {
            result += heaplet.getFreeSize();
        }
    }
    return result;
}

template <typename ClauseT>
typename HeapletClauseDB<ClauseT>::size_type
HeapletClauseDB<ClauseT>::test_getAvailableSpaceInActiveHeaplets() const noexcept {
    return test_getAvailableSpaceInHeapletList(m_activeHeaplets);
}

template <typename ClauseT>
typename HeapletClauseDB<ClauseT>::size_type
HeapletClauseDB<ClauseT>::test_getAvailableSpaceInBinaryHeaplets() const noexcept {
    return test_getAvailableSpaceInHeapletList(m_binaryHeaplets);
}

template <typename ClauseT>
typename HeapletClauseDB<ClauseT>::size_type
HeapletClauseDB<ClauseT>::test_getAvailableSpaceInFreeHeaplets() const noexcept {
    return test_getAvailableSpaceInHeapletList(m_freeHeapletPool);
}

#endif
}
