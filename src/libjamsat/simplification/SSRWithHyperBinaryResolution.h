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
 * \brief Performs self-subsuming resolution with hyper-binary resolution
 *
 * \ingroup JamSAT_Simplification
 *
 * \tparam OccurrenceMap
 * \tparam ModFn
 * \tparam Propagation
 * \tparam AssignmentProvider
 * \tparam StampMap
 *
 * \param occMap
 * \param notifyModificationAhead
 * \param propagation
 * \param assignments
 * \param tempStamps
 * \param resolveAt
 *
 * \returns
 *
 * TODO: documentation
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

    // propagate ~resolveAt (excluding redundant clauses)
    // Now: all virtual binaries (resolveAt z) present on level D
    // Remove all -z from clauses containing resolveAt (using tempStamps)

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

    bool stamped = false;
    for (auto lit = currentDL.begin() + 1; lit != currentDLEnd; ++lit) {
        tempStamps.setStamped(*lit, stamp, true);
        stamped = true;
    }

    if (!stamped) {
        return result;
    }

    for (auto clause : occMap[resolveAt]) {
        if (clause->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION) ||
            propagation.isAssignmentReason(*clause, assignments)) {
            continue;
        }

        bool foundPivot = false;
        for (auto litIt = clause->begin(); litIt != clause->end(); ++litIt) {
            if (*litIt == resolveAt) {
                foundPivot = true;
                break;
            }
        }

        if (!foundPivot) {
            continue;
        }

        bool clauseModified = false;
        bool strengthened = false;
        for (auto litIt = clause->begin(); litIt != clause->end();) {
            if (tempStamps.isStamped(*litIt, stamp)) {
                if (!clauseModified) {
                    notifyModificationAhead(clause);
                }
                // remove by subsumption:
                ++result.amntClausesRemovedBySubsumption;
                clause->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
                break;
                //++litIt;
            } else if (tempStamps.isStamped(~(*litIt), stamp)) {
                // strengthen
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
                                         << &clause << " (redundancy detected by strenghtening)");
        } else if (strengthened) {
            JAM_LOG_SSRWITHHBR(info, "Strenghtened " << std::addressof(*clause) << " to "
                                                     << toString(clause->begin(), clause->end()));
        }
    }
    return result;
}
}
