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

#pragma once

#include <vector>

#include <boost/range.hpp>

namespace jamsat {
/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::BoundedMap
 *
 * \brief A map with a bounded index range with and O(1) access times.
 *
 * \tparam K        The key type.
 * \tparam V        The value type.
 * \tparam KIndex   A type that is a model of the concept `Index` with indexed type `K`.
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

    /// The const iterator type for value ranges.
    using const_iterator = typename BackingType::const_iterator;

    /// The const value range type.
    using const_value_range = boost::iterator_range<const_iterator>;

    /// The iterator type for value ranges.
    using iterator = typename BackingType::iterator;

    /// The const value range type.
    using value_range = boost::iterator_range<iterator>;

    /**
     * \brief Constructs an ArrayBackedMap with the given maximum key.
     *
     * All keys from the one mapped to 0 up to the maximum key are initially
     * associated with an individual default-constructed value of V.
     *
     * \param maxKey    The key with the maximum index storable in the map
     * instance. The instance will have a constant size in O(\param maxKey).
     */
    explicit BoundedMap(K maxKey) : m_values(KIndex::getIndex(maxKey) + 1), m_defaultValue(V{}) {}

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
      : m_values(KIndex::getIndex(maxKey) + 1, defaultValue), m_defaultValue(defaultValue) {}

    /**
     * \brief Gets a reference to the specified element.
     *
     * \param index   The index of the element to be looked up. \p index must not
     * be larger than the maximum index storable in this map.
     * \returns       A reference to the looked-up element.
     */
    V &operator[](const K &index) noexcept { return m_values[KIndex::getIndex(index)]; }

    /**
     * \brief Gets a reference to the specified element (const version).
     *
     * \param index   The index of the element to be looked up. \p index must not
     * be larger than the maximum index storable in this map.
     * \returns       A reference to the looked-up element.
     */
    const V &operator[](const K &index) const noexcept { return m_values[KIndex::getIndex(index)]; }

    /**
     * \brief Gets the total size of this map (including default values).
     *
     * \returns       The total size of this map.
     */
    size_type size() const noexcept { return m_values.size(); }

    /**
     * \brief Gets the const range of values contained in this map.
     *
     * The range contains exactly `size()` elements, which are not required to be unique within
     * the range. The returned range is valid until the map is destroyed or resized.
     *
     * \returns       The range of values contained in this map.
     */
    const_value_range values() const noexcept {
        return const_value_range{m_values.cbegin(), m_values.cend()};
    }

    /**
     * \brief Gets the range of values contained in this map.
     *
     * The range contains exactly `size()` elements, which are not required to be unique within
     * the range. The returned range is valid until the map is destroyed or resized.
     *
     * \returns       The range of values contained in this map.
     */
    value_range values() noexcept { return value_range{m_values.begin(), m_values.end()}; }

    /**
     * \brief Increases the map's size.
     *
     * \param maxKey    the new maximum key which can be mapped to a value. \p maxKey must not
     *                  be smaller than the previous maximum key.
     */
    void increaseSizeTo(K maxKey) {
        auto newMaxKey = KIndex::getIndex(maxKey) + 1;
        if (newMaxKey >= m_values.size()) {
            m_values.resize(newMaxKey, m_defaultValue);
        }
    }

private:
    BackingType m_values;
    V m_defaultValue;
};
}
