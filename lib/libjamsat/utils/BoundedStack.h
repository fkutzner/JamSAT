/* Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)

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
 * \file utils/BoundedStack.h
 * \brief Implementation of BoundedStack
 */

#pragma once

#include "Assert.h"
#include <vector>

namespace jamsat {

/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::BoundedStack
 *
 * \brief A stack with O(1) (non-amortized) push and pop operations.
 *
 * This efficient stack implementation provides iterators whose validity is not
 * affected by push operations (and neither by pop operations, as long as the
 * pointed-to element has not been removed from the stack).
 *
 * \tparam T          The element type.
 * \tparam Allocator  The allocator used by the backing vector.
 */
template <typename T, typename Allocator = std::allocator<T>>
class BoundedStack {
private:
  using BackingType = std::vector<T, Allocator>;

public:
  // The non-constant random-access iterator type.
  using iterator = typename BackingType::iterator;

  // The constant random-access iterator type.
  using const_iterator = typename BackingType::const_iterator;

  // The stack's size type.
  using size_type = typename BackingType::size_type;

  // The type of references to elements of the stack.
  using reference = T&;

  // The type of const references to elements of the stakc.
  using const_reference = const T&;

  /**
   * \brief Constructs a new BoundedStack instance.
   *
   * \param size    The stack's maximum size.
   */
  explicit BoundedStack(size_type size) : m_stack(size), m_currentSize(0) {}

  /**
   * \brief Returns the topmost element of the stack.
   *
   * This method may only be called when the stack is empty.
   */
  reference back() noexcept
  {
    JAM_ASSERT(!empty(), "Cannot access an empty stack");
    return m_stack[m_currentSize - 1];
  }

  /**
   * \brief Returns the topmost element of the stack (const version).
   *
   * This method may only be called when the stack is empty.
   */
  const_reference back() const noexcept
  {
    JAM_ASSERT(!empty(), "Cannot access an empty stack");
    return m_stack[m_currentSize - 1];
  }

  /**
   * \brief Removes the topmost element from the stack.
   *
   * This method may only be called when the stack is empty.
   */
  void pop() noexcept
  {
    JAM_ASSERT(!empty(), "Cannot pop() an empty stack");
    --m_currentSize;
  }

  /**
   * \brief Removes multiple elements from the stack.
   *
   * \param newSize   The amount of elements remaining in the stack. \p newSize
   *                  must not be larger than the current size of the stack.
   */
  void pop_to(size_type newSize) noexcept
  {
    JAM_ASSERT(newSize <= m_currentSize, "Cannot pop() an empty stack");
    m_currentSize = newSize;
  }

  /**
   * \brief Adds an element to the top of the stack by copying.
   *
   * This method may only be called when the stack is not full.
   *
   * \param item    The item to be copied to the stack.
   */
  void push_back(const_reference item) noexcept
  {
    JAM_ASSERT(m_currentSize != m_stack.size(), "Exceeded stack bound");
    m_stack[m_currentSize] = item;
    m_currentSize += 1;
  }

  /**
   * \brief Returns an iterator pointing to the bottom of the stack.
   *
   * The iterator remains valid until the stack is destroyed or its maximum
   * size is changed.
   *
   * \returns An iterator pointing to the least recently added element of the stack.
   */
  iterator begin() noexcept { return m_stack.begin(); }

  /**
   * \brief Returns an iterator pointing to the bottom of the stack. (const
   *        version)
   *
   * The iterator remains valid until the stack is destroyed or its maximum
   * size is changed.
   *
   * \returns An iterator pointing to the least recently added element of the stack.
   */
  const_iterator begin() const noexcept { return m_stack.begin(); }

  /**
   * \brief Returns an iterator pointing to the top of the stack.
   *
   * The iterator remains valid until the stack is destroyed, its maximum
   * size is changed or the pop() method is called.
   *
   * \returns An iterator pointing the most recently added element of the stack.
   */
  iterator end() noexcept { return m_stack.begin() + m_currentSize; }

  /**
   * \brief Returns an iterator pointing to the top of the stack. (const
   *        version)
   *
   * The iterator remains valid until the stack is destroyed, its maximum
   * size is changed or the pop() method is called.
   *
   * \returns An iterator pointing the most recently added element of the stack.
   */
  const_iterator end() const noexcept { return m_stack.begin() + m_currentSize; }

  /**
   * \brief Gets the amount of elements currently stored in the stack.
   *
   * \returns The amount of elements currently stored in the stack.
   */
  size_type size() const noexcept { return m_currentSize; }

  /**
   * \brief Determines whether the stack is empty.
   *
   * \returns \p true iff the stack is empty.
   */
  bool empty() const noexcept { return m_currentSize == 0; }

  /**
   * \brief Increases the stack's maximum size.
   *
   * \param amount    The size by which the maximum size should be raised.
   */
  void increaseMaxSizeBy(size_type amount)
  {
    if (amount > 0) {
      m_stack.resize(m_stack.size() + amount);
    }
  }

private:
  BackingType m_stack;
  size_type m_currentSize;
};
}
