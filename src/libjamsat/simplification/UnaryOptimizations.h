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

#include <libjamsat/cnfproblem/CNFLiteral.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <type_traits>
#include <vector>

namespace jamsat {

/**
 * \defgroup JamSAT_Simplification  JamSAT simplification library
 */

/**
 * \brief Schedules all clauses subsumed by a unary clause for deletion.
 *
 * \tparam OccurrenceMap    A specialization of OccurrenceMap with
 *                          OccurrenceMap::Container::value_type being equal to CNFlit.
 *
 * \tparam ModFn            A type that is a model of the Callable concept, with `x(y)` being
 *                          a valid expression for objects `x` of type `ModFn` and `y` of type
 *                          `OccurrenceMap::Container*`.
 *
 * \tparam CNFLitRange      A type that is a model of Boost's Single Pass Range.
 *
 * \param occMap              The occurrence map containing the set of clauses to be optimized.
 * \param notifyDeletionAhead The object receiving (via invocation) pointers to clauses that are
 *                            about to be scheduled for deletion.
 * \param unaries             The list of literals contained in unary clauses.
 *
 * Before a clause `c` is deleted, `occMap(&c)` is called.
 */
template <typename OccurrenceMap, typename ModFn, typename CNFLitRange>
void scheduleClausesSubsumedByUnariesForDeletion(OccurrenceMap &occMap,
                                                 ModFn const &notifyDeletionAhead,
                                                 CNFLitRange const &unaries) {
    using Clause = typename OccurrenceMap::Container;

    for (auto unaryLit : unaries) {
        for (Clause *clause : occMap[unaryLit]) {
            // Subsumption by unaryLit
            notifyDeletionAhead(clause);
            clause->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
            occMap.remove(*clause);
        }
    }
}

/**
 * \brief For each unary clause (a), removes ~a from all clauses.
 *
 * \tparam OccurrenceMap    A specialization of OccurrenceMap with
 *                          OccurrenceMap::Container::value_type being equal to CNFlit.
 *
 * \tparam ModFn            A type that is a model of the Callable concept, with `x(y)` being
 *                          a valid expression for objects `x` of type `ModFn` and `y` of type
 *                          `OccurrenceMap::Container*`.
 *
 * \tparam CNFLitRange      A type that is a model of Boost's Single Pass Range.
 *
 * \param occMap                  The occurrence map containing the set of clauses to be optimized.
 * \param notifyModificationAhead The object receiving (via invocation) pointers to clauses that are
 *                                about to be modified.
 * \param unaries                 The list of literals contained in unary clauses.
 *
 * Preconditions:
 *  - No propagation of any unary clause yields new assignments.
 *  - No clause contained in \p occMap is subsumed by a unary clause.
 */
template <typename OccurrenceMap, typename ModFn, typename CNFLitRange>
void strengthenClausesWithUnaries(OccurrenceMap &occMap, ModFn const &notifyModificationAhead,
                                  CNFLitRange const &unaries) {
    using Clause = typename OccurrenceMap::Container;

    for (auto unaryLit : unaries) {
        for (Clause *clause : occMap[~unaryLit]) {
            notifyModificationAhead(clause);
            boost::remove_erase(*clause, ~unaryLit);
        }
    }
}
}