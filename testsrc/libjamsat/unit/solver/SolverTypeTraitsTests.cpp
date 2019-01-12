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

#include <vector>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/SolverTypeTraits.h>

struct EmptyStruct {};

// Tests for jamsat::is_literal_container<T>:
static_assert(
    jamsat::is_literal_container<jamsat::Clause>::value,
    "Type Clause must be marked as a literal container by is_literal_container, but is not");
static_assert(
    jamsat::is_literal_container<jamsat::Clause const>::value,
    "Type Clause const must be marked as a literal container by is_literal_container, but is not");
static_assert(!jamsat::is_literal_container<EmptyStruct>::value,
              "The empty struct is marked to be a literal container, but should not be");

// Tests for jamsat::is_lbd_carrier<T>:
static_assert(jamsat::is_lbd_carrier<jamsat::Clause>::value,
              "Type Clause must be marked as an LBD carrier by is_lbd_carrier, but is not");
static_assert(jamsat::is_lbd_carrier<jamsat::Clause const>::value,
              "Type Clause const must be marked as an LBD carrier by is_lbd_carrier, but is not");
static_assert(!jamsat::is_lbd_carrier<EmptyStruct>::value,
              "The empty struct is marked as an LBD carrier, but should not be");

// Tests for jamsat::is_clause<T>:
