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
 * \defgroup JamSAT_Utils_Concepts  Concepts
 * \ingroup JamSAT_Utils
 */

/**
 * \defgroup Index
 * \ingroup JamSAT_Utils_Concepts
 * \{
 *
 * The `Index` describes types that can be used to associate objects with
 * integral indices.
 *
 * \par Notation
 *
 * - `X`: A type that is a model of `Index`
 *
 * \par Associated types
 *
 * <table>
 *  <tr>
 *    <td> Indexed type </td>
 *    <td> `X::Type` </td>
 *    <td> The type of the objects to be indexed. </td>
 *  </tr>
 * </table>
 *
 * \par Valid expressions
 * <table>
 *  <tr><th>Name</th><th>Expression</th><th>Type requirements</th><th>Return type</th></tr>
 *  <tr>
 *    <td> Index </td>
 *    <td> `X::getIndex(a)` </td>
 *    <td> `a` is of type `X::Type`</td>
 *    <td> `std::size_t` </td>
 *  </tr>
 * </table>
 *
 * \par Expression semantics
 *
 * <table>
 *  <tr><th>Name</th><th>Expression</th><th>Precondition</th><th>Semantics</th><th>Postcondition</th></tr>
 *  <tr>
 *    <td> Index </td>
 *    <td> `X::getIndex(a)` </td>
 *    <td> </td>
 *    <td> Returns a unique index value for `a`. If `X::Type` is a model of `Comparable`, then for any
 *         two objects `a,b` of type `X::Type`, the following must hold: `a < b => X::getIndex(a) < X::getIndex(b)`.</td>
 *    <td> </td>
 *  </tr>
 * </table>
 * \}
 */
