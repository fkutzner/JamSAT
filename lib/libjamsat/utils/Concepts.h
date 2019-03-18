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
 * \file utils/Concepts.h
 * \brief Concepts and concept-checking traits for the utilities package
 */

/**
 * \defgroup JamSAT_Utils_Concepts  Concepts
 * \ingroup JamSAT_Utils
 */

#pragma once

#include <libjamsat/utils/TraitUtils.h>

#include <cstdint>
#include <type_traits>

namespace jamsat {

/**
 * \ingroup JamSAT_Utils_Concepts
 *
 * \brief Checks whether a type I is an Index type for a type T.
 *
 * \tparam I    The index type.
 * \tparam T    The indexed type.
 *
 * `is_index<I, T>::value` is `true` if `T` satisfies the Index concept
 * defined below, with I as the index type and T as the indexed type. Otherwise,
 * `is_index<I, T>::value` is `false`.
 *
 * Objects of types satisfying the Index concept can be used to assign unsigned integer
 * values to other objects, e.g. to obtain the "raw" value of a CNF literal.
 *
 * A type satisfies the Index concept iff it satisfies the following requirements:
 *
 * \par Requirements
 *
 * Given
 *  - `idx`, an object of type `I`
 *  - `t`, an object of type `T const`
 *
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *    <td> `I::Type`</td>
 *    <td> The indexed type. Must be the same type as `T`.</td>
 *    <td> </td>
 *  </tr>
 *  <tr>
 *    <td> `I::getIndex(t)`</td>
 *    <td> Returns a unique index value for `a`. If `X::Type` is a model of `Comparable`, then for
 *         any two objects `a,b` of type `X::Type`, the following must hold:
 *         `a < b => X::getIndex(a) < X::getIndex(b)`. </td>
 *    <td> `std::size_t` </td>
 *  </tr>
 * </table>
 */
template <typename, typename, typename = j_void_t<>>
struct is_index : public std::false_type {};

template <typename I, typename T>
struct is_index<
    I,
    T,
    j_void_t<std::enable_if_t<std::is_same<typename I::Type, T>::value, void>,
             JAM_REQUIRE_EXPR(I::getIndex(std::declval<std::add_const_t<T>>()), std::size_t)>>
  : public std::true_type {};

}
