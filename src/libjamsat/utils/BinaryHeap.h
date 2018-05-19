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

#pragma once

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>

#include <cstdint>
#include <vector>

namespace jamsat {

/**
 * \brief Max-heap
 *
 * \ingroup JamSAT_Utils
 *
 * \tparam K            The type of the values to be stored in the heap.
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
 * \tparam KIndex       A key descriptor for `K`. TODO: document concept -
 *                      for now, see the key descriptor parameter documentation
 *                      of `StampMap`
 */
template <typename K, typename Comparator, typename KIndex = typename K::Index>
class BinaryMaxHeap {
public:
    using size_type = size_t;

    /**
     * \brief Constructs an empty max-heap.
     *
     * \param maxElement    The maximal element (wrt. KIndex) that will be
     *                      stored in the heap.
     */
    BinaryMaxHeap(K maxElement);

    /**
     * \brief Inserts an element into the heap.
     *
     * \param element   The value to be inserted. `element` must not be larger
     *                  (wrt. KIndex) than the current maximal element.
     */
    void insert(K element) noexcept;

    /**
     * \brief Removes the greatest element from the heap and returns it.
     *
     * Precondition: the heap must not be empty.
     *
     * \returns the greatest element contained in the heap.
     */
    auto removeMax() noexcept -> K;

    /**
     * \brief Removes all elements from the heap.
     *
     * Precondition: the heap must not be empty.
     *
     * \returns the greatest object contained in the heap.
     */
    void clear() noexcept;

    /**
     * \brief Returns the amount of elements currently stored in the heap.
     *
     * \returns the amount of elements currently stored in the heap.
     */
    auto size() const noexcept -> size_type;

    /**
     * \brief Determines whether the heap is empty.
     *
     * \returns `true` iff the heap is empty.
     */
    auto empty() const noexcept -> bool;

    /**
     * \brief Updates the in-heap position of an element that may have
     * increased wrt. the ordering given by `Comparator`.
     *
     *
     * \param element   The affected element. `element` must not be larger
     *                  (wrt. KIndex) than the current maximal element.
     */
    void increasingUpdate(K element) noexcept;

    /**
     * \brief Updates the in-heap position of an element that may have
     * decreased wrt. the ordering given by `Comparator`.
     *
     *
     * \param element   The affected element. `element` must not be larger
     *                  (wrt. KIndex) than the current maximal element.
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
     */
    auto contains(K element) const noexcept -> bool;

    /**
     * \brief Returns the comparator object.
     *
     * \returns the heap's comparator object.
     */
    auto getComparator() noexcept -> Comparator &;

    /**
     * \brief Returns the comparator object.
     *
     * \returns the heap's comparator object.
     */
    auto getComparator() const noexcept -> Comparator const &;

    /**
     * \brief Increases the maximal element storable in the heap.
     *
     * \param newMaxElement     The new maximal element. `newMaxElement`
     *                          must not be smaller than the current maximal
     *                          element.
     *
     * This method invokes `increaseMaxSizeTo(newMaxElement)` on the heap's
     * comparator.
     */
    void increaseMaxSizeTo(K newMaxElement);

    /**
     * \brief Checks the heap's internal consistency.
     *
     * This method should only be called by tests.
     *
     * \returns true iff the heap is internally consistent.
     */
    auto test_satisfiesHeapProperty() -> bool;

private:
    using Storage = std::vector<K>;
    using StorageIdx = typename Storage::size_type;

    auto getParent(StorageIdx index) -> StorageIdx;
    auto getLeftChild(StorageIdx index) -> StorageIdx;
    auto getRightChild(StorageIdx index) -> StorageIdx;

    using Index = int32_t;
    BoundedMap<K, Index, KIndex> m_indices;
    Storage m_heap;
    size_type m_size;
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
auto BinaryMaxHeap<K, Comparator, KIndex>::getParent(StorageIdx index) -> StorageIdx {
    return (index - 1) / 2;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::getLeftChild(StorageIdx index) -> StorageIdx {
    return 2 * index + 1;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::getRightChild(StorageIdx index) -> StorageIdx {
    return 2 * index + 2;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::getComparator() noexcept -> Comparator & {
    return m_lessThan;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::getComparator() const noexcept -> Comparator const & {
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
    m_indices[element] = insertionIndex;

    // The new element might be larger than its parent ~> restore
    // heap property by moving it up:
    increasingUpdate(element);
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::increasingUpdate(K element) noexcept {
    Index cursorIdx = m_indices[element];
    Index parentIndex = getParent(cursorIdx);

    if (cursorIdx == 0) {
        return;
    }

    while (cursorIdx != 0 && !m_lessThan(element, m_heap[parentIndex])) {
        // element is greater than the current parent -> move down the parent
        K parent = m_heap[parentIndex];
        m_heap[cursorIdx] = parent;
        m_indices[parent] = cursorIdx;
        cursorIdx = parentIndex;
        parentIndex = getParent(cursorIdx);
    }

    JAM_ASSERT(cursorIdx >= 0, "Cursor index must be non-negative");
    m_heap[cursorIdx] = element;
    m_indices[element] = cursorIdx;
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::decreasingUpdate(K element) noexcept {
    Index cursorIdx = m_indices[element];

    while (getLeftChild(cursorIdx) < m_size) {
        Index leftChildIdx = getLeftChild(cursorIdx);
        Index rightChildIdx = getRightChild(cursorIdx);
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

    JAM_ASSERT(static_cast<size_type>(cursorIdx) < m_size, "Cursor index must be non-negative");
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
auto BinaryMaxHeap<K, Comparator, KIndex>::test_satisfiesHeapProperty() -> bool {
    bool heapPropertyOK = true;
    for (size_t i = 0; i < m_size; ++i) {
        if (getLeftChild(i) < m_size && m_lessThan(m_heap[i], m_heap[getLeftChild(i)])) {
            heapPropertyOK = false;
        }
        if (getRightChild(i) < m_size && m_lessThan(m_heap[i], m_heap[getRightChild(i)])) {
            heapPropertyOK = false;
        }
    }
    return heapPropertyOK;
}
}
