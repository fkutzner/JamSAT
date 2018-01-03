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

#pragma once

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/Assert.h>

namespace jamsat {
/**
 * \ingroup JamSAT_Solver
 *
 * \brief Destructively marks clauses as "to be deleted".
 *
 * \param clause    A nonempty clause.
 * \tparam ClauseT  A type satisfying the \ref SimpleClause concept.
 */
template <class ClauseT>
void markToBeDeleted(ClauseT &clause) noexcept;

/**
 * \ingroup JamSAT_Solver
 *
 * \brief Checks whether a given clause has been marked as "to be deleted".
 *
 * \param clause    A nonempty clause.
 * \tparam ClauseT  A type satisfying the \ref SimpleClause concept.
 *
 * \returns true iff \p clause has been marked as "to be deleted".
 */
template <class ClauseT>
bool isMarkedToBeDeleted(const ClauseT &clause) noexcept;

/********** Implementation ****************************** */

template <class ClauseT>
void markToBeDeleted(ClauseT &clause) noexcept {
    JAM_ASSERT(!clause.empty(), "clause may not be empty");
    clause[0] = CNFLit::getUndefinedLiteral();
}

template <class ClauseT>
bool isMarkedToBeDeleted(const ClauseT &clause) noexcept {
    JAM_ASSERT(!clause.empty(), "clause may not be empty");
    return clause[0] == CNFLit::getUndefinedLiteral();
}
}
