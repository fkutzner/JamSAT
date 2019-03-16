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
 * \file SolverTypeTraits.h
 * \brief Concepts and concept-checking traits related to CDCL algorithms and data structures
 */

#pragma once

#include <libjamsat/concepts/ClauseTraits.h>
#include <libjamsat/utils/TraitUtils.h>
#include <libjamsat/utils/Truth.h>
#include <type_traits>

namespace jamsat {

/**
 * \ingroup JamSAT_Concepts
 *
 * \brief Checks whether a type is an assignment-reason provider type.
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
 * A type satisfies the ReasonProvider concept iff it satisfies the following requirements:
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


// clang-format off

template<typename, typename, typename = j_void_t<>>
struct is_const_range : public std::false_type {};

template<typename T, typename O>
struct is_const_range<T, O, j_void_t<
    std::enable_if_t<
      std::is_same<typename std::iterator_traits<decltype(std::declval<T>().begin())>::reference,
                   std::add_lvalue_reference_t<std::add_const_t<O>>>::value,
      void>,

    std::enable_if_t<
      std::is_same<typename std::iterator_traits<decltype(std::declval<T>().end())>::reference,
         std::add_lvalue_reference_t<std::add_const_t<O>>>::value,
      void>

  >> : public std::true_type {};

/**
 * \ingroup JamSAT_Concepts
 *
 * \brief Checks whether a type is a decision level provider type.
 *
 * \tparam T    A type.
 *
 * `is_decision_level_provider<T>::value` is `true` if `T` satisfies the
 * DecisionLevelProvider concept defined below. Otherwise,
 * `is_decision_level_provider<T>::value` is `false`.
 *
 * Objects of types satisfying DecisionLevelProvider can be used to obtain the
 * decision level of variables and to obtain the assignments made in decision
 * levels.
 *
 * A type satisfies the DecisionLevelProvider concept iff it satisfies the following requirements:
 *
 * \par Requirements
 *
 * Given
 *  - `d`, an object of type const `T`
 *  - `L`, a Random Access Range type with CNFLit const iterators
 *  - `v`, an object of type `CNFVar`
 *  - `e`, an object of type `D::DecisionLevel`
 *
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *    <td> `T::DecisionLevel` </td>
 *    <td> `T::DecisionLevel` is an integral type that can represent the largest
 *         decision level index which an object of type `T` can store. </td>
 *    <td> </td>
 *  </tr>
 *  <tr>
 *    <td> `d.getCurrentDecisionLevel()` </td>
 *    <td> </td>
 *    <td> `D::DecisionLevel` </td>
 *  </tr>
 *  <tr>
 *    <td> `d.getAssignmentDecisionLevel(v)` </td>
 *    <td> Returns the decision level on which `v` has been assigned. `v` must
 *         be a variable with an assignment.</td>
 *    <td> `D::DecisionLevel` </td>
 *  </tr>
 *  <tr>
 *    <td> `d.getDecisionLevelAssignments(e)` </td>
 *    <td> Returns the const range of literals which have been assigned on level `e`.
 *         If `e` is larger than the current decision level, an empty range is
 *         returned. </td>
 *    <td> `L` </td>
 *  </tr>
 * </table>
 */
template<typename, typename = j_void_t<>>
struct is_decision_level_provider : public std::false_type {};

template<typename T>
struct is_decision_level_provider<T,
                                  j_void_t<
    // Require that T::DecisionLevel is an integral type:
    std::enable_if_t<std::is_integral<typename T::DecisionLevel>::value, void>,

    // For t of type const T, require that t.getCurrentDecisionLevel() returns a decision level:
    JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().getCurrentDecisionLevel(),
                     typename T::DecisionLevel),

    // For t of type const T and v of type CNFVar, require that t.getAssignmentDecisionLevel(v)
    // returns a decision level:
    JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().getAssignmentDecisionLevel(std::declval<CNFVar>()),
                     typename T::DecisionLevel),

    // For t of type const T and l of type T::DecisionLevel, require that
    // t.getDecisionLevelAssignments(l) returns a range over CNFLit const:
    std::enable_if_t<is_const_range<decltype(std::declval<T>().getDecisionLevelAssignments(std::declval<typename T::DecisionLevel>())),
                     CNFLit>::value, void>

  // end requirements
  >> : public std::true_type {};


/**
 * \ingroup JamSAT_Concepts
 *
 * \brief Checks whether a type is an assignment provider type.
 *
 * \tparam T    A type.
 *
 * `is_assignment_provider<T>::value` is `true` if `T` satisfies the
 * AssignmentProvider concept defined below. Otherwise,
 * `is_assignment_provider<T>::value` is `false`.
 *
 * Objects of types satisfying AssignmentProvider can be used to access the
 * solver's current variable assignment.
 *
 * A type satisfies the AssignmentProvider concept iff it satisfies the following requirements:
 *
 * \par Requirements
 *
 * Given
 *  - `L`, a Random Access Range type with CNFLit const iterators
 *  - `a`, an object of type `T`
 *  - `l`, an object of type CNFLit
 *  - `v`, an object of type CNFVar
 *  - `s`, an object of type `A::size_type`
 *
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *    <td> `T::size_type` </td>
 *    <td> `T::size_type` is an integral type that can represent the size of the
 *         largest amount of variable assignments an object of T can hold. </td>
 *    <td> </td>
 *  </tr>
 *  <tr>
 *    <td> `a.getAssignment(l)` </td>
 *    <td> Returns the assignment of `l`'s variable.</td>
 *    <td> `TBool` </td>
 *  </tr>
 *  <tr>
 *    <td> `a.getAssignment(v)` </td>
 *    <td> Returns the assignment of `v`.</td>
 *    <td> `TBool` </td>
 *  </tr>
 *  <tr>
 *    <td> `a.getAssignments(s)` </td>
 *    <td> Returns a range of literals sorted in chronological order of
 *         assignment, beginning with the `s`th assignment (counted from 0).
 *         </td>
 *    <td> `L` </td>
 *  </tr>
 *  <tr>
 *    <td> `a.getNumberOfAssignments()` </td>
 *    <td> Returns the total number of variable assignments the object holds.
 *         </td>
 *    <td> `T::size_type` </td>
 *  </tr>
 * </table>
 */
template<typename, typename = j_void_t<>>
struct is_assignment_provider : public std::false_type {};

template<typename T>
struct is_assignment_provider<T, j_void_t<

    // Require that T::size_type is an integral type:
    std::enable_if_t<std::is_integral<typename T::size_type>::value, void>,

    // For t of type const T and l of type CNFLit, require that t.getAssignment(l) returns
    // a TBool:
    JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().getAssignment(std::declval<CNFLit>()),
                     TBool),

    // For t of type const T and v of type CNFVar, require that t.getAssignment(l) returns
    // a TBool:
    JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().getAssignment(std::declval<CNFVar>()),
                     TBool),

    // For t of type const T, require that t.getNumberOfAssignments() returns a value of type
    // T::size_type:
    JAM_REQUIRE_EXPR(std::declval<std::add_const_t<T>>().getNumberOfAssignments(),
                     typename T::size_type),

    // For t of type const T and l of type T::size_type, require that t.getAssignments(s)
    // returns a const range of CNFLit objects:
    std::enable_if_t<is_const_range<decltype(std::declval<std::add_const_t<T>>()
                                            .getAssignments(std::declval<typename T::size_type>())),
                     CNFLit>::value, void>


    // end requirements
    >> : public std::true_type {};

// clang-format on
}
