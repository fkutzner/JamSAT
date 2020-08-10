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
 * \file utils/FlatteningIterator.h
 * \brief An iterator for nested structures
 */

#pragma once

#include <cstdint>
#include <iterator>
#include <optional>
#include <type_traits>

#include <libjamsat/utils/Assert.h>

namespace jamsat {

/**
 * \ingroup JamSAT_Utils
 *
 * \brief A const iterator type providing a flat view on a sequence of STL containers.
 *
 * Let
 *
 *  - `C` be a type satisfying the STL Iterable concept,
 *  - `I` be a type satisfying the STP InputIterator concept, with the value type of `I` being `C`,
 *  - `i1` and `i2` be objects of type `I` such that `i1 = i2` can be achieved by incrementing `i1`
 *
 * this iterator traverses the objects iterated over by the iterables in `[i1, i2)`. The nested
 * containers are traversed in the order given by `i1`.
 *
 * Restriction: Only a single level of Iterable nesting is supported by this iterator type.
 *
 * Usage example: Use this iterator to iterate over all the integers contained in an
 *   `std::vector<std::vector<int>>` object.
 *
 * This type satisfies the STL's InputIterator concept. If both `I` and the Iterables iterated by
 * `I` satisfy the STL's ForwardIterator concept, so does `FlatteningIterator<I>`.
 *
 * \tparam I    a type satisfying the STP InputIterator concept, with the value type of `I`
 *              satisfying the STL Iterable concept.
 */
template <typename I>
class FlatteningIterator {
private:
  using OuterIt = I;
  using InnerIt = decltype(std::declval<I>()->begin());

  static_assert(std::is_nothrow_move_assignable_v<InnerIt>);
  static_assert(std::is_nothrow_move_constructible_v<InnerIt>);


public:
  using value_type = typename std::iterator_traits<InnerIt>::value_type;
  using reference = typename std::iterator_traits<InnerIt>::reference;
  using pointer = typename std::iterator_traits<InnerIt>::pointer;
  using iterator_category = std::input_iterator_tag;
  using difference_type = int64_t;

  using const_reference = typename std::add_const<reference>::type;
  using const_pointer = typename std::add_const<pointer>::type;

  /**
   * \brief Constructs a new FlatteningIterator.
   *
   * \param begin     An iterator pointing to the first nested container to be traversed.
   * \param end       An iterator pointing past the last container to be traversed.
   */
  FlatteningIterator(I begin, I end);

  /**
   * \brief Constructs a universal past-the-end FlatteningIterator.
   */
  FlatteningIterator();

  reference operator*();
  pointer operator->();

  const_reference operator*() const;
  const_pointer operator->() const;

  FlatteningIterator<I>& operator++();
  FlatteningIterator<I> operator++(int dummy);

  bool operator==(const FlatteningIterator<I>& rhs) const noexcept;
  bool operator!=(const FlatteningIterator<I>& rhs) const noexcept;

  FlatteningIterator<I>& operator=(const FlatteningIterator<I>& other) = default;
  FlatteningIterator<I>& operator=(FlatteningIterator<I>&& other) noexcept = default;
  FlatteningIterator(const FlatteningIterator<I>& other) = default;
  FlatteningIterator(FlatteningIterator<I>&& other) noexcept = default;
  ~FlatteningIterator() = default;

private:
  // Class invariant A: m_outerIt is either dereferencable or equal to m_outerEndIt
  OuterIt m_outerIt;
  OuterIt m_outerEndIt;

  // Class invariant B: m_outerIt != m_outerEndIt <=> m_innerIt exists
  // Class invariant C: m_innerIt exists <=> m_innerEndIt exists
  // Class invariant D: if m_innerIt exists, *m_innerIt is dereferencable
  std::optional<InnerIt> m_innerIt;
  std::optional<InnerIt> m_innerEndIt;
};
}

namespace std {
template <typename I>
struct iterator_traits<jamsat::FlatteningIterator<I>> {
  using value_type = typename jamsat::FlatteningIterator<I>::value_type;
  using reference = typename jamsat::FlatteningIterator<I>::reference;
  using pointer = typename jamsat::FlatteningIterator<I>::pointer;
  using iterator_category = typename jamsat::FlatteningIterator<I>::iterator_category;
  using difference_type = typename jamsat::FlatteningIterator<I>::difference_type;
};
}

/********** Implementation ****************************** */

namespace jamsat {

template <typename I>
FlatteningIterator<I>::FlatteningIterator(I begin, I end)
  : m_outerIt(begin), m_outerEndIt(end), m_innerIt(), m_innerEndIt()
{
  while (m_outerIt != m_outerEndIt && m_outerIt->empty()) {
    ++m_outerIt;
  }

  if (m_outerIt != m_outerEndIt) {
    m_innerIt = std::optional<InnerIt>{m_outerIt->begin()};
    m_innerEndIt = std::optional<InnerIt>{m_outerIt->end()};
  }
}

template <typename I>
FlatteningIterator<I>::FlatteningIterator()
  : m_outerIt(), m_outerEndIt(), m_innerIt(), m_innerEndIt()
{
}

template <typename I>
typename FlatteningIterator<I>::reference FlatteningIterator<I>::operator*()
{
  JAM_ASSERT(m_innerIt, "Nested iterator pointing past the end dereferenced");
  return **m_innerIt;
}

template <typename I>
typename FlatteningIterator<I>::pointer FlatteningIterator<I>::operator->()
{
  return &(this->operator*());
}

template <typename I>
typename FlatteningIterator<I>::const_reference FlatteningIterator<I>::operator*() const
{
  JAM_ASSERT(m_innerIt, "Nested iterator pointing past the end dereferenced");
  return **m_innerIt;
}

template <typename I>
typename FlatteningIterator<I>::const_pointer FlatteningIterator<I>::operator->() const
{
  return &(this->operator*());
}

template <typename I>
FlatteningIterator<I>& FlatteningIterator<I>::operator++()
{
  if (!m_innerIt) {
    // Due to class invariant B, there are no further elements to traverse
    return *this;
  }

  // Try to advance to the next element of the nested container:
  if (*m_innerIt != *m_innerEndIt) {
    auto newInnerIt = *m_innerIt;
    ++newInnerIt;
    if (newInnerIt != *m_innerEndIt) {
      m_innerIt = std::optional<InnerIt>{newInnerIt};
      return *this;
    }
  }

  // Try to advance to the first element of the next nonempty nested container:
  // m_outerIt can be advanced at least once due to class invariant B
  do {
    ++m_outerIt;
  } while (m_outerIt != m_outerEndIt && m_outerIt->empty());

  if (m_outerIt != m_outerEndIt) {
    m_innerIt = std::optional<InnerIt>{m_outerIt->begin()};
    m_innerEndIt = std::optional<InnerIt>{m_outerIt->end()};
    return *this;
  }

  // No more elements to traverse
  m_innerIt = std::optional<InnerIt>{};
  m_innerEndIt = std::optional<InnerIt>{};
  return *this;
}

template <typename I>
FlatteningIterator<I> FlatteningIterator<I>::operator++(int dummy)
{
  (void)dummy;
  FlatteningIterator<I> copy = *this;
  this->operator++();
  return copy;
}

template <typename I>
bool FlatteningIterator<I>::operator==(const FlatteningIterator<I>& rhs) const noexcept
{
  if (this == &rhs) {
    return true;
  }

  // Optimize for comparing to end iterators:
  bool isLhsEnd = !static_cast<bool>(this->m_innerIt);
  bool isRhsEnd = !static_cast<bool>(rhs.m_innerIt);
  if (isLhsEnd != isRhsEnd) {
    return false;
  }
  if (isLhsEnd) {
    // Special case for end iterators (m_outerIt may differ, but all end iterators are equal)
    return true;
  }

  return (this->m_outerIt == rhs.m_outerIt && this->m_innerIt == rhs.m_innerIt &&
          this->m_outerEndIt == rhs.m_outerEndIt && this->m_innerEndIt == rhs.m_innerEndIt);
}

template <typename I>
bool FlatteningIterator<I>::operator!=(const FlatteningIterator<I>& rhs) const noexcept
{
  return !this->operator==(rhs);
}
}
