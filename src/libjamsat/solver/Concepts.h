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

/**
 * \defgroup JamSAT_Solver_Concepts  Concepts
 * \ingroup JamSAT_Solver
 */

/* Template:
 *
 * \defgroup ConceptName
 * \ingroup JamSAT_Solver_Concepts
 * \{
 *
 * Description
 *
 * \par Requirements
 *
 * Given
 *  - `D`, a \ref ConceptName type
 *  - `d`, an object of type `d`
 *  - ...
 *
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *    <td> </td>
 *    <td> </td>
 *    <td> </td>
 *  </tr>
 * </table>
 * \}
 */


/**
 * \defgroup AssignmentProvider
 * \ingroup JamSAT_Solver_Concepts
 * \{
 *
 * Objects of types satisfying AssignmentProvider can be used to modify and access the
 * solver's current variable assignment.
 *
 * \par Requirements
 *
 * Given
 *  - `A`, an \ref AssignmentProvider type
 *  - `L`, a Random Access Range type with CNFLit const iterators
 *  - `a`, an object of type `A`
 *  - `l`, an object of type CNFLit
 *  - `v`, an object of type CNFVar
 *  - `s`, an object of type `A::size_type`
 *
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *    <td> `A::size_type` </td>
 *    <td> `A::size_type` is an integral type that can represent the size of the
 *         largest amount of variable assignments an object of A can hold. </td>
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
 *    <td> `A::size_type` </td>
 *  </tr>
 * </table>
 * \}
 */

/**
 * \defgroup DecisionLevelProvider
 * \ingroup JamSAT_Solver_Concepts
 * \{
 *
 * Objects of types satisfying DecisionLevelProvider can be used to obtain the
 * decision level of variables and to obtain the assignments made in decision
 * levels.
 *
 * \par Requirements
 *
 * Given
 *  - `D`, a \ref DecisionLevelProvider type
 *  - `L`, a Random Access Range type with CNFLit const iterators
 *  - `d`, an object of type `D`
 *  - `v`, an object of type `CNFVar`
 *  - `e`, an object of type `D::DecisionLevel`
 *
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *    <td> `D::DecisionLevel` </td>
 *    <td> `D::DecisionLevel` is an integral type that can represent the largest
 *         decision level index which an object of type `D` can store. </td>
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
 *    <td> Returns the range of literals which have been assigned on level `e`.
 *         If `e` is larger than the current decision level, an empty range is
 *         returned. </td>
 *    <td> `L` </td>
 *  </tr>
 * </table>
 * \}
 */

/**
 * \defgroup ReasonProvider
 * \ingroup JamSAT_Solver_Concepts
 * \{
 *
 * Objects of types satisfying the \ref ReasonProvider concept can be used
 * to access the assignment reason of a variable, ie. the clause having forced
 * its assignment.
 *
 * \par Requirements
 *
 * Given
 *  - `R`, a \ref ReasonProvider type
 *  - `r`, an object of type `R`
 *  - `v`, an object of type `CNFVar`
 *
 * <table>
 *  <tr><th>Expression</th><th>Requirements</th><th>Return value</th></tr>
 *  <tr>
 *    <td> `R::ClauseType`</td>
 *    <td> The reason clause type. </td>
 *    <td> </td>
 *  </tr>
 *  <tr>
 *    <td> `r.getAssignmentReason(v)`</td>
 *    <td> Returns a pointer to the assignment reason clause for `v` if `v`
 *         has been assigned via propagation; returns `nullptr` otherwise. </td>
 *    <td> `R::ClauseType*` </td>
 *  </tr>
 * </table>
 * \}
 */
