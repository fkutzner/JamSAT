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

template <typename K, typename Comparator, typename KIndex = typename K::Index>
class BinaryMaxHeap {
public:
    using size_type = size_t;

    BinaryMaxHeap(K maxElement);

    void insert(K key) noexcept;

    void increasingUpdate(K key) noexcept;
    void decreasingUpdate(K key) noexcept;

    auto contains(K key) const noexcept -> bool;

    auto getComparator() noexcept -> Comparator &;
    auto getComparator() const noexcept -> Comparator const &;

    auto removeMax() noexcept -> K;
    void clear() noexcept;

    auto size() const noexcept -> size_type;
    auto empty() const noexcept -> bool;

    void increaseMaxSizeTo(K newMaxElement);

    auto assertHeapProperty() -> bool;

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
void BinaryMaxHeap<K, Comparator, KIndex>::insert(K key) noexcept {
    if (contains(key)) {
        return;
    }
    JAM_ASSERT(m_size != m_heap.size(), "Heap out of space");

    auto insertionIndex = m_size;
    ++m_size;
    m_heap[insertionIndex] = key;
    m_indices[key] = insertionIndex;

    // The new element might be larger than its parent ~> restore
    // heap property by moving it up:
    increasingUpdate(key);
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::increasingUpdate(K key) noexcept {
    Index cursorIdx = m_indices[key];
    Index parentIndex = getParent(cursorIdx);

    if (cursorIdx == 0) {
        return;
    }

    while (cursorIdx != 0 && !m_lessThan(key, m_heap[parentIndex])) {
        // key is greater than the current parent -> move down the parent
        K parent = m_heap[parentIndex];
        m_heap[cursorIdx] = parent;
        m_indices[parent] = cursorIdx;
        cursorIdx = parentIndex;
        parentIndex = getParent(cursorIdx);
    }

    JAM_ASSERT(cursorIdx >= 0, "Cursor index must be non-negative");
    m_heap[cursorIdx] = key;
    m_indices[key] = cursorIdx;
}

template <typename K, typename Comparator, typename KIndex>
void BinaryMaxHeap<K, Comparator, KIndex>::decreasingUpdate(K key) noexcept {
    Index cursorIdx = m_indices[key];

    while (getLeftChild(cursorIdx) < m_size) {
        Index leftChildIdx = getLeftChild(cursorIdx);
        Index rightChildIdx = getRightChild(cursorIdx);
        // Invariant: leftChildIdx < rightChildIdx

        // If the key is smaller than any of its children, move up
        // the largest child c with key < c:
        bool rl = (static_cast<size_type>(rightChildIdx) < m_size &&
                   m_lessThan(m_heap[leftChildIdx], m_heap[rightChildIdx]));
        Index maxChildIdx = rl ? rightChildIdx : leftChildIdx;

        K maxChild = m_heap[maxChildIdx];
        if (m_lessThan(key, maxChild)) {
            // Child is smaller than key -> move it upwards
            m_heap[cursorIdx] = maxChild;
            m_indices[maxChild] = cursorIdx;
            cursorIdx = maxChildIdx;
        } else {
            // Insert child at cursor
            break;
        }
    }

    JAM_ASSERT(static_cast<size_type>(cursorIdx) < m_size, "Cursor index must be non-negative");
    m_heap[cursorIdx] = key;
    m_indices[key] = cursorIdx;
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
auto BinaryMaxHeap<K, Comparator, KIndex>::contains(K key) const noexcept -> bool {
    return m_indices[key] >= 0;
}

template <typename K, typename Comparator, typename KIndex>
auto BinaryMaxHeap<K, Comparator, KIndex>::assertHeapProperty() -> bool {
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
