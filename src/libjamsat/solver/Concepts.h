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

* \
}
* /
