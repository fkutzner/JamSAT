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

#include <vector>

namespace jamsat {
/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::BoundedMap
 *
 * \brief A noniterable map with a bounded index range with and O(1) access
 * times.
 *
 * \tparam K        The key type.
 * \tparam V        The value type.
 * \tparam KIndex   A type having the static function T KIndex::index(const K&)
 * where T is an integral type not bigger than std::vector<V>::size_type. This
 * function is used to obtain indices for keys.
 * \tparam Allocator  The allocator used by the backing vector.
 */
template <typename K, typename V, typename KIndex = typename K::Index,
          typename Allocator = std::allocator<V>>
class BoundedMap {
private:
  using BackingType = std::vector<V, Allocator>;

public:
  /// The size type.
  using size_type = typename BackingType::size_type;

  /**
   * \brief Constructs an ArrayBackedMap with the given maximum key.
   *
   * All keys from the one mapped to 0 up to the maximum key are initially
   * associated with an individual default-constructed value of V.
   *
   * \param maxKey    The key with the maximum index storable in the map
   * instance. The instance will have a constant size in O(\param maxKey).
   */
  explicit BoundedMap(K maxKey) : m_values(KIndex::getIndex(maxKey) + 1) {}

  /**
   * \brief Constructs an ArrayBackedMap with the given default value and
   * maximum key.
   *
   * All keys from the one mapped to 0 up to the maximum key are initially
   * associated with \p defaultValue.
   *
   * \param maxKey        The key with the maximum index supported by the map
   * instance.
   * \param defaultValue  The default value of this map.
   */
  BoundedMap(K maxKey, V defaultValue)
      : m_values(KIndex::getIndex(maxKey) + 1, defaultValue) {}

  /**
   * \brief Gets a reference to the specified element.
   *
   * \param index   The index of the element to be looked up. \p index must not
   * be larger than the maximum index storable in this map.
   * \returns       A reference to the looked-up element.
   */
  V &operator[](const K &index) noexcept {
    return m_values[KIndex::getIndex(index)];
  }

  /**
   * \brief Gets a reference to the specified element (const version).
   *
   * \param index   The index of the element to be looked up. \p index must not
   * be larger than the maximum index storable in this map.
   * \returns       A reference to the looked-up element.
   */
  const V &operator[](const K &index) const noexcept {
    return m_values[KIndex::getIndex(index)];
  }

  /**
   * \brief Gets the total size of this map (including default values).
   *
   * \returns       The total size of this map.
   */
  size_type size() const noexcept { return m_values.size(); }

private:
  BackingType m_values;
};
}
