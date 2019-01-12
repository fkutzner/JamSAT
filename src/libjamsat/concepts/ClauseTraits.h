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

#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/concepts/TraitUtils.h>
#include <type_traits>

/** \file */

/**
 * \defgroup JamSAT_Concepts  JamSAT concept definitions
 * This module contains definitions of named requirements used in JamSAT, as well as related
 * type trait definitions.
 */

namespace jamsat {

/**
 * \ingroup JamSAT_Concepts
 *
 * \brief Checks whether a type is a literal container type.
 *
 * \tparam T    A type.
 *
 * `is_literal_container<T>::value` is `true` if `T` satisfies the LiteralContainer concept.
 * Otherwise, `is_literal_container<T>::value` is `false`.
 *
 * The LiteralContainer concept is modeled after STL's ContiguousContainer concept,
 * introducing some restrictions in order to enable optimizations.
 *
 * A type satisfies the LiteralContainer concept iff it satisfies the following named requirements,
 * member type and expression requirements:
 *
 * \par Named Requirements
 * - STL's EqualityComparable concept
 *
 * \par Member Types
 *
 * <table>
 *  <tr><th>Member Type</th><th>Definition</th></tr>
 *  <tr>
 *    <td> `value_type` </td>
 *    <td> `CNFLit` </td>
 *  </tr>
 *  <tr>
 *    <td> `size_type` </td>
 *    <td> unsigned integral type; the container's size type. </td>
 *  </tr>
 *  <tr>
 *    <td> `iterator` </td>
 *    <td> a type satisfying STL's `ForwardIterator` concept for `CNFLit` values. </td>
 *  </tr>
 *  <tr>
 *    <td> `const_iterator` </td>
 *    <td> a type satisfying STL's `RandomAccessIterator` concept for `CNFLit const` values. </td>
 *  </tr>
 * </table>
 *
 * \par Expressions
 * Given
 *  - `c`, an object of type `T` with const qualifier removed
 *  - `cc`, an object of type `T` with added const qualifier
 *  - `i`, an object of type `T::size_type`
 *  - `it`, an object of type `T::const_iterator`
 *  - `it2`, an object of type `T::const_iterator`
 *
 * the following expressions must be well-formed:
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *   <td>`cc.size()`</td>
 *   <td>Returns the amount of literals contained in the clause.</td>
 *   <td>`T::size_type`</td>
 *  </tr>
 *  <tr>
 *   <td>`c[i]`</td>
 *   <td>Returns a reference to the `i`'th literal of the clause for `0 <= i < t.size()`</td>
 *   <td>`CNFLit&`</td>
 *  </tr>
 *  <tr>
 *   <td>`cc[i]`</td>
 *   <td>Returns a const reference to the `i`'th literal of the clause for `0 <= i < t.size()`</td>
 *   <td>`CNFLit const&`</td>
 *  </tr>
 *  <tr>
 *   <td>`c.begin()`</td>
 *   <td>Returns the begin iterator for the contained literals.</td>
 *   <td>`T::iterator`</td>
 *  </tr>
 *  <tr>
 *   <td>`cc.begin()`</td>
 *   <td>Returns the begin const_iterator for the contained literals.</td>
 *   <td>`T::const_iterator`</td>
 *  </tr>
 *  <tr>
 *   <td>`c.end()`</td>
 *   <td>Returns the end iterator for the contained literals.</td>
 *   <td>`T::iterator`</td>
 *  </tr>
 *  <tr>
 *   <td>`cc.end()`</td>
 *   <td>Returns the end const_iterator for the contained literals.</td>
 *   <td>`T::const_iterator`</td>
 *  </tr>
 *  <tr>
 *   <td>`c.resize(i)`</td>
 *   <td>Resizes the container, keeping the first `i` literals, for `0 <= i < t.size()`</td>
 *   <td></td>
 *  </tr>
 *  <tr>
 *   <td>`c.erase(it)`</td>
 *   <td>Erases the element at position `it` from the clause, returning an iterator pointing to the
 *       element following the last removed element (or the end iterator if no such element exists).
 *   </td>
 *   <td>`T::iterator`</td>
 *  </tr>
 *  <tr>
 *   <td>`c.erase(it, it2)`</td>
 *   <td>Erases the literals in `[it, it2)`, returning the iterator following the last removed
 *       element (or the end iterator if no such element exists).
 *   </td>
 *   <td>`T::iterator`</td>
 *  </tr>
 *  <tr>
 *   <td>`c = c2`</td>
 *   <td>Copies `c2` with `c2.size() <= c.size()` such that after the assignment, `c == c2` is
 *       true.</td>
 *   <td>`T&`</td>
 *  </tr>
 * </table>
 */
template <typename, typename = j_void_t<>>
struct is_literal_container : public std::false_type {};

// Documentation: see the base case
template <typename T>
struct is_literal_container<
    T,
    j_void_t<
        // Require that T::size_type is integral:
        std::enable_if_t<std::is_integral<typename T::size_type>::value, void>,

        // Require that T::value_type is CNFLit:
        std::enable_if_t<std::is_same<typename T::value_type, jamsat::CNFLit>::value, void>,

        // Require that T::iterator exists:
        typename T::iterator,

        // Require that T::const_iterator exists:
        typename T::const_iterator,

        // Require that T is comparable (== and !=):
        JAM_REQUIRE_EXPR(std::declval<T>() == std::declval<T>(), bool),
        JAM_REQUIRE_EXPR(std::declval<T>() != std::declval<T>(), bool),

        // Require that non-const T has a bracket operator returning CNFLit&:
        JAM_REQUIRE_EXPR(
            std::declval<std::remove_const_t<T>>()[std::declval<typename T::size_type>()], CNFLit&),

        // Require that const T has a bracket operator returning CNFLit&:
        JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>()[std::declval<typename T::size_type>()],
                         CNFLit const&),

        // For t of non-const type T and x of T::size_type, require that t.resize(x) is a valid
        // expression:
        JAM_REQUIRE_EXPR(
            std::declval<std::remove_const_t<T>>().resize(std::declval<typename T::size_type>()),
            void),

        // For t of non-const type T, require that t.begin() returns a T::iterator:
        JAM_REQUIRE_EXPR(std::declval<std::remove_const_t<T>>().begin(), typename T::iterator),

        // For t of const type T, require that t.begin() returns a T::const_iterator:
        JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().begin(), typename T::const_iterator),

        // For t of non-const type T, require that t.end() returns a T::iterator:
        JAM_REQUIRE_EXPR(std::declval<std::remove_const_t<T>>().end(), typename T::iterator),

        // For t of const type T, require that t.end() returns a T::const_iterator:
        JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().end(), typename T::const_iterator),

        // For t of non-const type T and i of T::const_iterator, require that t.erase(i) returns a
        // T::iterator:
        JAM_REQUIRE_EXPR(std::declval<std::remove_const_t<T>>().erase(
                             std::declval<typename T::const_iterator>()),
                         typename T::iterator),

        // For t of non-const type T and i, j of T::const_iterator, require that t.erase(i) returns
        // a T::iterator:
        JAM_REQUIRE_EXPR(std::declval<std::remove_const_t<T>>().erase(
                             std::declval<typename T::const_iterator>(),
                             std::declval<typename T::const_iterator>()),
                         typename T::iterator),

        // Require that non-const T is assignable:
        JAM_REQUIRE_EXPR(std::declval<std::remove_const_t<T>>() =
                             std::declval<std::add_const_t<T>>(),
                         std::add_lvalue_reference_t<std::remove_const_t<T>>),

        // For t of type T, require that t.size() exists and has type T::size_type:
        JAM_REQUIRE_EXPR(std::declval<T>().size(), typename T::size_type)>>
  : public std::true_type {};


/**
 * \ingroup JamSAT_Concepts
 *
 * \brief Checks whether a type is an LBD value carrier
 *
 * \tparam T    A type.
 *
 * `is_lbd_carrier<T>::value` is `true` if `T` satisfies the LBDCarrier concept.
 * Otherwise, `is_lbd_carrier<T>::value` is `false`.
 *
 * A type satisfies the LBDCarrier concept iff it satisfies the following expression
 * requirements:
 *
 * \par Expressions
 * Given
 *  - `x`, an object of type `T` with const qualifier removed
 *  - `L`, an integral type
 *  - `l`, an object of type `L`
 *
 * the following expressions must be well-formed:
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *   <td>`x.template getLBD<L>()`</td>
 *   <td>Returns LBD value.</td>
 *   <td>`L`</td>
 *  </tr>
 *  <tr>
 *   <td>`x.template setLBD<L>(l)`</td>
 *   <td>Sets the LBD value.</td>
 *   <td></td>
 *  </tr>
 * </table>
 */
template <typename, typename = j_void_t<>>
struct is_lbd_carrier : public std::false_type {};

template <typename T>
struct is_lbd_carrier<T,
                      j_void_t<
                          // For t of type T, require that t.template getLBD<int>() returns an int:
                          JAM_REQUIRE_EXPR(std::declval<T>().template getLBD<int>(), int),

                          // For t of non-const type T and int i, require that t.template
                          // setLBD<int>(i) has type void:
                          JAM_REQUIRE_EXPR(std::declval<std::remove_const_t<T>>()
                                               .template setLBD<int>(std::declval<int>()),
                                           void)>> : public std::true_type {};

/**
 * \ingroup JamSAT_Concepts
 *
 * \brief Checks whether a type is a clause flag type
 *
 * \tparam T    A type.
 *
 * `is_clause_flag<T>::value` is `true` if `T` satisfies the ClauseFlag concept.
 * Otherwise, `is_clause_flag<T>::value` is `false`.
 *
 * A type satisfies the ClauseFlag concept iff it is a regular type and satisfies the following
 * expression requirements:
 *
 * \par Expressions
 * The following expressions must be well-formed:
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *   <td>`T::SCHEDULED_FOR_DELETION`</td>
 *   <td></td>
 *   <td>`T`</td>
 *  </tr>
 *  <tr>
 *   <td>`T::REDUNDANT`</td>
 *   <td></td>
 *   <td>`T`</td>
 *  </tr>
 * </table>
 */
template <typename, typename = j_void_t<>>
struct is_clause_flag : public std::false_type {};

template <typename T>
struct is_clause_flag<T, j_void_t<decltype(T::SCHEDULED_FOR_DELETION), decltype(T::REDUNDANT)>>
  : public std::true_type {};


/**
 * \ingroup JamSAT_Concepts
 *
 * \brief Checks whether a type is a clause type
 *
 * \tparam T    A type.
 *
 * `is_clause<T>::value` is `true` if `T` satisfies the Clause concept.
 * Otherwise, `is_clause<T>::value` is `false`.
 *
 * A type satisfies the Clause concept iff it satisfies the following concepts, member type
 * requirements and expression requirements:
 *
 * \par Named Requirements
 * - `LiteralContainer` (i.e. `is_literal_container<T>::value` is `true`)
 *
 * \par Member Types
 *
 * <table>
 *  <tr><th>Member Type</th><th>Definition</th></tr>
 *  <tr>
 *    <td> `Flag` </td>
 *    <td> A type satisfying the `ClauseFlag` concept, i.e.
 *         `is_clause_flag<typename T::Flag>::value` is `true`</td>
 *  </tr>
 * </table>
 *
 * \par Expressions
 * Given
 * - `c`, an object of type `T` with const qualifiers removed,
 * - `cc`, an object of type `T` with const qualifier added,
 * - `cc2`, an object of type `T` with const qualifier added,
 * - `f`, an object of type `T::Flag`,
 * - `l`, an object of type `CNFLit`
 *
 * the following expressions must be well-formed:
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *   <td>`cc.getFlag(f)`</td>
 *   <td>Returns `true` iff the flag `f` is set for `cc`.</td>
 *   <td>`bool`</td>
 *  </tr>
 *  <tr>
 *   <td>`c.setFlag(f)`</td>
 *   <td>Sets the flag `f` for `c`. Afterwards, `c.getFlag(f)` must be `true`.</td>
 *   <td></td>
 *  </tr>
 *  <tr>
 *   <td>`c.clearFlag(f)`</td>
 *   <td>Clears the flag `f` for `c`. Afterwards, `c.getFlag(f)` must be `false`.</td>
 *   <td></td>
 *  </tr>
 *  <tr>
 *   <td>`cc.mightContain(l)`</td>
 *   <td>If `cc.mightContain(l)` returns `false`, `cc` does not contain `l`. If
 *       `cc.mightContain(l)` returns true, `cc` might contain `l`.</td>
 *   <td>`bool`</td>
 *  </tr>
 *  <tr>
 *   <td>`cc.mightBeSubsetOf(cc2)`</td>
 *   <td>If `cc.mightContain(cc2)` returns `false`, `cc` is not a subset of `cc2`. If
 *       `cc.mightBeSubsetOf(cc2)` returns true, `cc` might be a subset of `cc2`.</td>
 *   <td>`bool`</td>
 *  </tr>
 *  <tr>
 *   <td>`c.clauseUpdated()`</td>
 *   <td>Notifies the clause that one of its literals has been changed.</td>
 *   <td></td>
 *  </tr>
 * </table>
 */
template <typename, typename = j_void_t<>>
struct is_clause : public std::false_type {};

template <typename T>
struct is_clause<
    T,
    j_void_t<
        // Require that T is a literal container:
        std::enable_if_t<is_literal_container<T>::value, void>,

        // Require that T is an LBD carrier:
        std::enable_if_t<is_lbd_carrier<T>::value, void>,

        // Require that T::Flag is a clause flag type:
        std::enable_if_t<is_clause_flag<typename T::Flag>::value, void>,

        // For t of type T, require that t.getFlag() has type T::Flag:
        JAM_REQUIRE_EXPR(std::declval<T>().getFlag(std::declval<typename T::Flag>()), bool),

        // For t of type non-const type T and f of type T::Flag, require that t.setFlag(f) is a
        // valid expression:
        JAM_REQUIRE_EXPR(
            std::declval<std::remove_const_t<T>>().setFlag(std::declval<typename T::Flag>()), void),

        // For t of type non-const type T and f of type T::Flag, require that t.clearFlag(f) is a
        // valid expression:
        JAM_REQUIRE_EXPR(
            std::declval<std::remove_const_t<T>>().clearFlag(std::declval<typename T::Flag>()),
            void),

        // For t of type const T and l of type CNFLit, require that `t.mightContain(l)` is a valid
        // expression of type `bool`:
        JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().mightContain(std::declval<CNFLit>()),
                         bool),

        // For t of type const T and t2 of type T const&, require that `t.mightBeSubsetOf(t2)`
        // is a valid expression of type `bool`:
        JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().mightBeSubsetOf(
                             std::declval<std::add_lvalue_reference_t<std::add_const_t<T>>>()),
                         bool),

        // For t of type T, require that `t.clauseUpdated()` is a valid expression:
        JAM_REQUIRE_EXPR(std::declval<std::remove_const_t<T>>().clauseUpdated(), void)

        // end requirements
        >> : public std::true_type {};
}
