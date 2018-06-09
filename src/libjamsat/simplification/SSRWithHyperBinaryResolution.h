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

#include <iostream>
#include <libjamsat/simplification/UnaryOptimizations.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/Printers.h>
#include <libjamsat/utils/Truth.h>

#if defined(JAM_ENABLE_INFLIGHTSIMP_LOGGING)
#define JAM_LOG_SSRWITHHBR(x, y) JAM_LOG(x, "ssrhbr", y)
#else
#define JAM_LOG_SSRWITHHBR(x, y)
#endif

namespace jamsat {
/**
 * \brief Performs self-subsuming resolution and strengthening with hyper-binary
 *        resolution.
 *
 * Precondition: All assignments forced by unary clauses (wrt. \p propagation)
 * have been propagated to fixpoint.
 *
 * Computes the set `A` of assignments implied by an assignment represented
 * by `resolveAt` and for each clause containing `resolveAt`, applies the following:
 *
 * - (a) if the intersection of `A` and `C` is not empty, `C` is scheduled for deletion
 *   since it is redundant.
 * - (b) for each `c in C`: if `~c in A`, `c` is removed from `C`.
 *
 * When this function returns, \p assignments contains exactly the assignments it
 * contians at the corresponding call to this function.
 *
 * \ingroup JamSAT_Simplification
 *
 * \tparam OccurrenceMap            An OccurrenceMap specialization for `Propagation::Clause`
 * \tparam ModFn                    A type such that for an object `o` of type `ModFn` and
 *                                  an object `c` of type `Clause`, `o(&c)` is a valid
 *                                  expression.
 * \tparam Propagation              A type that is a model of the Propagation concept, with
 *                                  `Propagation::Clause` being the same type as
 *                                  `OccurrenceMap::Container` and `AssignmentProvider::Clause`.
 * \tparam AssignmentProvider       A type that is a model of the AssignmentProvider concept,
 *                                  with `AssignmentProvider::Clause` being the same type
 *                                  as `Propagation::Clause`.
 * \tparam StampMap                 A StampMap type for CNFLit
 *
 * \param occMap                    An OccurrenceMap containing the non-unary problem clauses
 *                                  to be optimized. Clauses occurring in \p occMap may be
 *                                  shortened or scheduled for deletion.
 * \param notifyModificationAhead   A callable object to which a pointer to each clause to be
 *                                  modified get passed before the modification.
 * \param propagation               A propagation object.
 * \param assignments               An assignment provider in sync with \p propagation.
 * \param tempStamps                A StampMap capable of stamping all CNFLit objects occurring
 *                                  in any clause contained in \p occMap, and during propagation
 *                                  of \p resolveAt with \p propagation.
 * \param resolveAt                 The pivot literal.
 *
 * \returns                         A statistics object indicating how many clauses have been
 *                                  scheduled for deletion and how many literals have been
 *                                  removed from clauses via strengthening.
 */
template <typename OccurrenceMap, typename ModFn, typename Propagation, typename AssignmentProvider,
          typename StampMap>
auto ssrWithHyperBinaryResolution(OccurrenceMap &occMap, ModFn const &notifyModificationAhead,
                                  Propagation &propagation, AssignmentProvider &assignments,
                                  StampMap &tempStamps, CNFLit resolveAt) -> SimplificationStats;

/********** Implementation ****************************** */

template <typename OccurrenceMap, typename ModFn, typename Propagation, typename AssignmentProvider,
          typename StampMap>
auto ssrWithHyperBinaryResolution(OccurrenceMap &occMap, ModFn const &notifyModificationAhead,
                                  Propagation &propagation, AssignmentProvider &assignments,
                                  StampMap &tempStamps, CNFLit resolveAt) -> SimplificationStats {
    static_assert(
        std::is_same<typename Propagation::Clause, typename AssignmentProvider::Clause>::value,
        "Propagation and AssignmentProvider must have the same Clause type");

    using Clause = typename Propagation::Clause;

    SimplificationStats result;

    if (assignments.getAssignment(resolveAt) != TBools::INDETERMINATE) {
        // The assignment of resolveAt is already forced by a unary clause
        return result;
    }

    auto backtrackLevel = assignments.getCurrentDecisionLevel();
    assignments.newDecisionLevel();
    OnExitScope backtrackOnExit{
        [&assignments, backtrackLevel]() { assignments.revisitDecisionLevel(backtrackLevel); }};

    assignments.addAssignment(~resolveAt);
    auto confl = propagation.propagateUntilFixpoint(
        ~resolveAt, Propagation::PropagationMode::EXCLUDE_REDUNDANT_CLAUSES);
    if (confl) {
        // Found a failed literal! TODO: find the first UIP & learn the failed literal.
        return result;
    }

    auto stampingContext = tempStamps.createContext();
    auto stamp = stampingContext.getStamp();

    auto currentDL = assignments.getDecisionLevelAssignments(assignments.getCurrentDecisionLevel());
    auto currentDLEnd = currentDL.end();

    bool resolveAtImpliedAnything = false;
    for (auto lit = currentDL.begin() + 1; lit != currentDLEnd; ++lit) {
        tempStamps.setStamped(*lit, stamp, true);
        resolveAtImpliedAnything = true;
    }

    if (!resolveAtImpliedAnything) {
        return result;
    }

    for (auto clause : occMap[resolveAt]) {
        if (clause->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION) ||
            propagation.isAssignmentReason(*clause, assignments)) {
            continue;
        }

        // If resolveAt has been removed from the clause earlier,
        // optimizing this clause on the ground of resolveAt's presence
        // would not be sound, so in that case skip the clause:
        if (!clause->mightContain(resolveAt) ||
            std::find(clause->begin(), clause->end(), resolveAt) == clause->end()) {
            continue;
        }

        bool clauseModified = false;
        bool strengthened = false;
        for (auto litIt = clause->begin(); litIt != clause->end();) {
            if (tempStamps.isStamped(*litIt, stamp)) {
                // remove by subsumption: the clause contains some literal
                // b such that (resolveAt b) is a "virtual" binary
                if (!clauseModified) {
                    notifyModificationAhead(clause);
                }
                ++result.amntClausesRemovedBySubsumption;
                clause->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
                break;
            } else if (tempStamps.isStamped(~(*litIt), stamp)) {
                // strengthen the clause: the clause contains some
                // literal b such that (resolveAt ~b) is a "virtual" binary,
                // therefore b can be removed via resolution:
                if (!clauseModified) {
                    notifyModificationAhead(clause);
                    clauseModified = true;
                }
                ++result.amntLiteralsRemovedByStrengthening;
                litIt = clause->erase(litIt);
                strengthened = true;
            } else {
                ++litIt;
            }
        }

        if (clause->size() <= 1) {
            JAM_ASSERT(assignments.getAssignmentDecisionLevel((*clause)[0].getVariable()) == 0ULL,
                       "Not expecting to find new unaries here :O");
            clause->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
            JAM_LOG_SSRWITHHBR(info, "Deleting clause "
                                         << std::addressof(*clause)
                                         << " (redundancy detected by strenghtening)");
        } else if (strengthened) {
            JAM_LOG_SSRWITHHBR(info, "Strenghtened " << std::addressof(*clause) << " to "
                                                     << toString(clause->begin(), clause->end()));
            clause->clauseUpdated();
        }
    }
    return result;
}
}
