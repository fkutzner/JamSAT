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
 * \file utils/RangeUtils.h
 * \brief Utility functions for iterator ranges
 */

#pragma once

#include <type_traits>
#include <vector>

namespace jamsat {

/**
 * \brief Returns a vector containing the elements of the given sequence, without redundancies.
 *
 * \ingroup JamSAT_Utils
 *
 * \param begin   The input sequence's begin iterator.
 * \param end     The input sequence's end iterator.
 * \returns A vector containing the elements in `[begin, end)`, without redundancies.
 *
 * \tparam InputIt  A type satisfying the InputIterator concept. `InputIterator::value_type` must
 *                  be copy-constructible, copy-assignable and comparable.
 */
template <typename InputIt>
auto withoutRedundancies(InputIt begin, InputIt end) -> std::vector<typename InputIt::value_type>;


/**
 * \brief Exchanges the first occurrence of an item with the last element of a
 *        vector-like object
 *
 * \ingroup JamSAT_Utils
 *
 * \param vector    A container
 * \param element   The element whose first occurrence in \p{vector} (if any)
 *                  shall be swapped with the last element of \p{vector}.
 *
 * \tparam VectorLike   A type with objects `o` of `VectorLike` supporting
 *                      `VectorLike::value_type& x = o[i]` and `o[i] = z` for
 *                      a `VectorLike::value_type` object `z` and all
 *                      `VectorLike::size_type i` with  `0 <= i < o.size()`.
 *                      `VectorLike::value_type` must be nothrow-swappable
 *                      and comparable.
 *
 * \returns The amount of items moved to the end.
 */
template <typename VectorLike>
auto swapWithLastElement(VectorLike&& vector,
                         typename std::decay_t<VectorLike>::value_type const& element) noexcept
    -> std::size_t;

/********** Implementation ****************************** */

template <typename InputIt>
auto withoutRedundancies(InputIt begin, InputIt end) -> std::vector<typename InputIt::value_type>
{
  using CompressionBuf = std::vector<typename InputIt::value_type>;
  CompressionBuf compressionBuf;

  auto compressionBufSize = static_cast<typename CompressionBuf::size_type>(end - begin);
  compressionBuf.resize(compressionBufSize);
  std::copy(begin, end, compressionBuf.begin());
  std::sort(compressionBuf.begin(), compressionBuf.end());
  auto compressionBufEnd = std::unique(compressionBuf.begin(), compressionBuf.end());

  compressionBufSize =
      static_cast<typename CompressionBuf::size_type>(compressionBufEnd - compressionBuf.begin());
  compressionBuf.resize(compressionBufSize);
  return compressionBuf;
}

template <typename VectorLike>
auto swapWithLastElement(VectorLike&& vector,
                         typename std::decay_t<VectorLike>::value_type const& element) noexcept
    -> std::size_t
{
  using DecayedVL = std::decay_t<VectorLike>;
  using SizeTy = typename DecayedVL::size_type;
  using ValueTy = typename DecayedVL::value_type;

  static_assert(std::is_nothrow_swappable_v<ValueTy>);

  using std::swap;

  SizeTy const size = vector.size();
  for (SizeTy i = 0; i < size; ++i) {
    if (vector[i] == element) {
      swap(vector[i], vector[size - 1]);
      return 1;
    }
  }

  return 0;
}
}
