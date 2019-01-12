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

#include <libjamsat/concepts/ClauseTraits.h>
#include <libjamsat/concepts/TraitUtils.h>
#include <type_traits>

namespace jamsat {

/**
 * \ingroup JamSAT_Concepts
 *
 * \brief Checks whether a type is an assignment-reason provider.
 *
 * \tparam T    A type.
 *
 * `is_reason_provider<T, Reason>::value` is `true` if `T` satisfies the ReasonProvider concept
 * defined below, with Reason as the reason object type. Otherwise,
 * `is_reason_provider<T, Reason>::value` is `false`.
 *
 * Objects of types satisfying the ReasonProvider concept can be used
 * to access the assignment reason of a variable, eg. the clause having forced
 * its assignment.
 *
 * A type satisfies the ReasonProvider concepts iff it satisfies the following requirements:
 *
 * \par Requirements
 *
 * Given
 *  - `r`, an object of type `T`
 *  - `v`, an object of type `CNFVar`
 *
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *    <td> `R::Reason`</td>
 *    <td> The reason clause type. Must be the same type as `Reason`.</td>
 *    <td> </td>
 *  </tr>
 *  <tr>
 *    <td> `r.getAssignmentReason(v)`</td>
 *    <td> Returns a pointer to the assignment reason clause for `v` if `v`
 *         has been assigned via propagation; returns `nullptr` otherwise. </td>
 *    <td> `R::Reason const*` </td>
 *  </tr>
 * </table>
 */
template <typename, typename Reason, typename = j_void_t<>>
struct is_reason_provider : public std::false_type {};

template <typename T, typename Reason>
struct is_reason_provider<
    T,
    Reason,
    j_void_t<
        // Require that T::Reason is the same type as Reason:
        std::enable_if_t<std::is_same<Reason, typename T::Reason>::value, void>,

        // For t of type const T and v of type CNFVar, require that t.getAssignmentReason(v)
        // is a pointer to a const T::Clause:
        JAM_REQUIRE_EXPR(
            std::declval<std::add_const_t<T>>().getAssignmentReason(std::declval<CNFVar>()),
            Reason const*)

        // end requirements
        >> : public std::true_type {};
}
