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

#include <bitset>

namespace jamsat {

/**
 * \class OverApproximatingSet
 *
 * \brief An over-approximating set
 *
 * A space-efficient set data structure allowing false positives in queries.
 *
 * \tparam Size     The size of the set, in bits.
 * \tparam Key      The key type of the values to be stored. The values to be
 *                  stored in the set must be of type `Key::Type`. `Key::getIndex(x)`
 *                  must be a valid expression with return type `size_t` for all
 *                  `x` of type `Key::Type`.
 */
template <size_t Size, typename Key>
class OverApproximatingSet {
public:
    static constexpr size_t size = Size;
    using Type = typename Key::Type;

    /**
     * \brief Inserts a value into the set.
     * \param toInsert      The value to be inserted into the set.
     */
    void insert(typename Key::Type toInsert) noexcept;

    /**
     * \brief Checks whether a given value might be contained in the set.
     *
     * \param toLookup      The value to be looked up.
     * \return              If \p toLookup had been added to the set, `true`
     *                      is returned. Otherwise, either `true` or `false`
     *                      is returned. If `false` is returned, \p toLookup
     *                      had definitely not been added to the set.
     *
     * For any given value of \p toLookup, the return value of this method
     * is either always `true` or always `false` until the next call to
     * `insert`.
     */
    auto mightContain(typename Key::Type toLookup) const noexcept -> bool;

    /**
     * \brief Checks whether the set might be a superset of this set.
     *
     * \param set           An arbitrary over-approximating set
     * \return              `false` iff the approximation allows the conclusion
     *                      that this set is definitely not a subset of `set`;
     *                      `true` otherwise.
     */
    auto mightBeSubsetOf(OverApproximatingSet<Size, Key> const &set) const noexcept -> bool;

private:
    std::bitset<Size> m_approximatedSet;
};

/********** Implementation ****************************** */

template <size_t Size, typename Key>
void OverApproximatingSet<Size, Key>::insert(Type toInsert) noexcept {
    auto index = Key::getIndex(toInsert);
    m_approximatedSet[index % Size] = 1;
}

template <size_t Size, typename Key>
auto OverApproximatingSet<Size, Key>::mightContain(Type toLookup) const noexcept -> bool {
    auto index = Key::getIndex(toLookup);
    return (m_approximatedSet[index % Size] == 1);
}

template <size_t Size, typename Key>
auto OverApproximatingSet<Size, Key>::mightBeSubsetOf(
    OverApproximatingSet<Size, Key> const &set) const noexcept -> bool {
    // Need to check if m_approximatedSet -> set.m_approximatedSet)
    // Applying deMorgan's law to enable better code generation by the
    // compiler:
    return (m_approximatedSet & ~set.m_approximatedSet).none();
}
}
