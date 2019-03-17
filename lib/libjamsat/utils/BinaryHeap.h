/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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
 * \file utils/BinaryHeap.h
 * \brief Fast binary heap implementations
 */

#pragma once

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/Casts.h>
#include <libjamsat/utils/Concepts.h>

#include <cstdint>
#include <vector>

namespace jamsat {

/**
 * \brief A max-heap implementation geared towards objects that are cheap to
 * copy.
 *
 * This max-heap implementation facilitates exception-safe usage by performing
 * allocations only during construction and a special resize method. In this
 * implementation, speed is favoured over low memory consumption, allowing e.g.
 * the implementation of `contains()` with a single memory access.
 *
 * \ingroup JamSAT_Utils
 *
 * \tparam K            The type of the values to be stored in the heap.
 *                      `K` must satisfy the STL concepts `DefaultConstructible`,
 *                      `CopyConstructible` and `CopyAssignable`.
 * \tparam Comparator   A type satisfying the STL Compare concept for K.
 *                      `Comparator` must have a constructor accepting the
 *                      maximal element (wrt. `KIndex`) that will be
 *                      compared. Furthermore,
 *                      for an object `c` of type `Comparator` and an object
 *                      `z` of type `K`, the expression
 *                      `c.increaseMaxSizeTo(z)` must be valid, where `z` is
 *                      not smaller than the value passed to the constructor
 *                      or to any previous invocation of `c.increaseMaxSizeTo()`.
 *                      After `c.increaseMaxSizeTo(z)`, all values no larger
 *                      than `z` (wrt. `KIndex`) must be comparable by `c`.
 * \tparam KIndex       A type that is a model of the concept `Index` with indexed
 *                      type `K`.
 */
template <typename K, typename Comparator, typename KIndex = typename K::Index>
class BinaryMaxHeap {
    static_assert(is_index<KIndex, K>::value, "KIndex must satisfy Index for K, but does not");

public:
    using size_type = size_t;

    /**
     * \brief Constructs an empty max-heap.
     *
     * \param maxElement    The maximal element (wrt. KIndex) that will be
     *                      stored in the heap.
     *
     * \par Complexity
     * Worst case: `O(KIndex::getIndex(maxElement))`
     */
    explicit BinaryMaxHeap(K maxElement);

    /**
     * \brief Inserts an element into the heap.
     *
     * \param element   The value to be inserted. `element` must not be larger
     *                  (wrt. KIndex) than the current maximal element.
     *
     * If `element` is already contained in the heap, no insertion is performed.
     *
     * \par Complexity
     * Worst case: `O(log(size()))`
     */
    void insert(K element) noexcept;

    /**
     * \brief Removes the greatest element from the heap and returns it.
     *
     * Precondition: the heap must not be empty.
     *
     * \returns the greatest element contained in the heap.
     *
     * \par Complexity
     * Worst case: `O(log(size()))`
     */
    auto removeMax() noexcept -> K;

    /**
     * \brief Removes all elements from the heap.
     *
     * Precondition: the heap must not be empty.
     *
     * \returns the greatest object contained in the heap.
     *
     * \par Complexity
     * Worst case: `O(1)`
     */
    void clear() noexcept;

    /**
     * \brief Returns the amount of elements currently stored in the heap.
     *
     * \returns the amount of elements currently stored in the heap.
     *
     * \par Complexity
     * Worst case: `O(1)`
     */
    auto size() const noexcept -> size_type;

    /**
     * \brief Determines whether the heap is empty.
     *
     * \returns `true` iff the heap is empty.
     *
     * \par Complexity
     * Worst case: `O(1)`
     */
    auto empty() const noexcept -> bool;

    /**
     * \brief Updates the in-heap position of an element that may have
     * increased wrt. the ordering given by `Comparator`.
     *
     *
     * \param element   The affected element. `element` must not be larger
     *                  (wrt. KIndex) than the current maximal element.
     *
     * \par Complexity
     * Worst case: `O(log(size()))`
     */
    void increasingUpdate(K element) noexcept;

    /**
     * \brief Updates the in-heap position of an element that may have
     * decreased wrt. the ordering given by `Comparator`.
     *
     *
     * \param element   The affected element. `element` must not be larger
     *                  (wrt. KIndex) than the current maximal element.
     *
     * \par Complexity
     * Worst case: `O(log(size()))`
     */
    void decreasingUpdate(K element) noexcept;

    /**
     * \brief Determines whether the heap contains a given element.
     *
     *
     * \param element   The element to be looked up. `element` must not be larger
     *                  (wrt. KIndex) than the current maximal element.
     *
     * \returns true iff the heap contains `element`.
     *
     * \par Complexity
     * Worst case: `O(1)`
     */
    auto contains(K element) const noexcept -> bool;

    /**
     * \brief Returns the comparator object.
     *
     * \returns the heap's comparator object.
     *
     * \par Complexity
     * Worst case: `O(1)`
     */
    auto getComparator() noexcept -> Comparator&;

    /**
     * \brief Returns the comparator object.
     *
     * \returns the heap's comparator object.
     *
     * \par Complexity
     * Worst case: `O(1)`
     */
    auto getComparator() const noexcept -> Comparator const&;

    /**
     * \brief Increases the maximal element storable in the heap.
     *
     * \param newMaxElement     The new maximal element. `newMaxElement`
     *                          must not be smaller than the current maximal
     *                          element.
     *
     * This method invokes `increaseMaxSizeTo(newMaxElement)` on the heap's
     * comparator.
     *
     * \par Complexity
     * Worst case: `O(KIndex::getIndex(newMaxElement))`
     */
    void increaseMaxSizeTo(K newMaxElement);

    /**
     * \brief Checks the heap's internal consistency.
     *
     * This method should only be called by tests.
     *
     * \returns true iff the heap is internally consistent.
     *
     * \par Complexity
     * Worst case: `O(size())`
     */
    auto test_satisfiesHeapProperty() const noexcept -> bool;

private:
    using Storage = std::vector<K>;
    using StorageIdx = typename Storage::size_type;

    constexpr auto getParentIdx(StorageIdx index) const noexcept -> StorageIdx;
    constexpr auto getLeftChildIdx(StorageIdx index) const noexcept -> StorageIdx;
    constexpr auto getRightChildIdx(StorageIdx index) const noexcept -> StorageIdx;

    using Index = int32_t;

    /// Maps stored objects to their rsp. index in m_heap.
    /// Objects that are not contained in m_heap have the index -1.
    BoundedMap<K, Index, KIndex> m_indices;

    /// An array for which the following invariant holds: for all
    /// elements `i` in `[0, m_size)`,
    ///
    /// - if `getLeftChildIdx(i) < m_size`, then
    ///   `m_heap[getLeftChildIdx(i)] < m_heap[i]` wrt. `m_lessThan`
    /// - if `getRightChildIdx(i) < m_size`, then
    ///   `m_heap[getRightChildIdx(i)] < m_heap[i]` wrt. `m_lessThan`
    ///
    /// `m_heap` with a size suitable to store all insertable elements
    /// including the current maximum element.
    Storage m_heap;

    /// The amount of elements currently residing in the heap.
    size_type m_size;

    /// The comparator used to establish the ordering in `m_heap`.
    Comparator m_lessThan;
};

/********** Implementation ****************************** */

template <typename K, typename Comparator, typename KIndex>
BinaryMaxHeap<K, Comparator, KIndex>::BinaryMaxHeap(K maxElement)
  : m_indices{maxElement, -1}, m_heap(), m_size(0), m_lessThan(maxElement) {
    m_heap.resize(KIndex::getIndex(maxElement) + 1);
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::increaseMaxSizeTo(K newMaxElement) {
    JAM_ASSERT(KIndex::getIndex(newMaxElement) + 1 >= m_heap.size(), "Unable to shrink the heap");
    m_heap.resize(KIndex::getIndex(newMaxElement) + 1);
    m_indices.increaseSizeTo(newMaxElement);
    m_lessThan.increaseMaxSizeTo(newMaxElement);
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::size() const noexcept -> size_type {
    return m_size;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::empty() const noexcept -> bool {
    return m_size == 0;
}

template <typename K, typename Comparator, typename KIndex>
constexpr auto BinaryMaxHeap<K, Comparator, KIndex>::getParentIdx(StorageIdx index) const noexcept
    -> StorageIdx {
    return (index - 1) / 2;
}

template <typename K, typename Comparator, typename KIndex>
constexpr auto BinaryMaxHeap<K, Comparator, KIndex>::getLeftChildIdx(StorageIdx index) const
    noexcept -> StorageIdx {
    return 2 * index + 1;
}

template <typename K, typename Comparator, typename KIndex>
constexpr auto BinaryMaxHeap<K, Comparator, KIndex>::getRightChildIdx(StorageIdx index) const
    noexcept -> StorageIdx {
    return 2 * index + 2;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::getComparator() noexcept -> Comparator& {
    return m_lessThan;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::getComparator() const noexcept -> Comparator const& {
    return m_lessThan;
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::insert(K element) noexcept {
    if (contains(element)) {
        return;
    }
    JAM_ASSERT(m_size != m_heap.size(), "Heap out of space");

    auto insertionIndex = m_size;
    ++m_size;
    m_heap[insertionIndex] = element;
    m_indices[element] = static_checked_cast<Index>(insertionIndex);

    // The new element might be larger than its parent ~> restore
    // heap property by moving it up:
    increasingUpdate(element);
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::increasingUpdate(K element) noexcept {
    Index cursorIdx = m_indices[element];
    Index parentIndex = getParentIdx(cursorIdx);

    if (cursorIdx == 0) {
        return;
    }

    while (cursorIdx != 0 && !m_lessThan(element, m_heap[parentIndex])) {
        // element is greater than the current parent -> move down the parent
        K parent = m_heap[parentIndex];
        m_heap[cursorIdx] = parent;
        m_indices[parent] = cursorIdx;
        cursorIdx = parentIndex;
        parentIndex = getParentIdx(cursorIdx);
    }

    JAM_ASSERT(cursorIdx >= 0, "Cursor index out of range");
    m_heap[cursorIdx] = element;
    m_indices[element] = cursorIdx;
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::decreasingUpdate(K element) noexcept {
    Index cursorIdx = m_indices[element];

    while (getLeftChildIdx(cursorIdx) < m_size) {
        Index leftChildIdx = getLeftChildIdx(cursorIdx);
        Index rightChildIdx = getRightChildIdx(cursorIdx);
        // Invariant: leftChildIdx < rightChildIdx

        // If  element is smaller than any of its children, move up
        // the largest child c with element < c:
        bool rl = (static_cast<size_type>(rightChildIdx) < m_size &&
                   m_lessThan(m_heap[leftChildIdx], m_heap[rightChildIdx]));
        Index maxChildIdx = rl ? rightChildIdx : leftChildIdx;

        K maxChild = m_heap[maxChildIdx];
        if (m_lessThan(element, maxChild)) {
            // Child is smaller than element -> move it upwards
            m_heap[cursorIdx] = maxChild;
            m_indices[maxChild] = cursorIdx;
            cursorIdx = maxChildIdx;
        } else {
            // Insert child at cursor
            break;
        }
    }

    JAM_ASSERT(static_cast<size_type>(cursorIdx) < m_size, "Cursor index out of range");
    m_heap[cursorIdx] = element;
    m_indices[element] = cursorIdx;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::removeMax() noexcept -> K {
    JAM_ASSERT(m_size > 0, "Cannot remove from an empty heap");
    auto result = m_heap[0];
    m_indices[result] = -1;

    if (m_size > 1) {
        // First element has been removed, so restore the heap property
        K replacement = m_heap[m_size - 1];
        m_heap[0] = m_heap[m_size - 1];
        m_indices[replacement] = 0;
        --m_size;
        // The new element at m_heap[0] is guaranteed not to be larger
        // than the one that has just been removed ~> move it down
        decreasingUpdate(replacement);
    } else {
        m_size = 0;
    }

    return result;
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::clear() noexcept {
    m_size = 0;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::contains(K element) const noexcept -> bool {
    return m_indices[element] >= 0;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::test_satisfiesHeapProperty() const noexcept -> bool {
    bool heapPropertyOK = true;
    for (size_t i = 0; i < m_size; ++i) {
        heapPropertyOK = heapPropertyOK && !(getLeftChildIdx(i) < m_size &&
                                             m_lessThan(m_heap[i], m_heap[getLeftChildIdx(i)]));
        heapPropertyOK = heapPropertyOK && !(getLeftChildIdx(i) < m_size &&
                                             m_lessThan(m_heap[i], m_heap[getLeftChildIdx(i)]));
    }
    return heapPropertyOK;
}
}
