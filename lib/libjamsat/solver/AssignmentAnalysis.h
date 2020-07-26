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
 * \file AssignmentAnalysis.h
 * \brief Assignment analysis routines
 */

#pragma once

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/concepts/ClauseTraits.h>
#include <libjamsat/concepts/SolverTypeTraits.h>
#include <libjamsat/utils/StampMap.h>

#include <vector>

namespace jamsat {

/**
 * \brief Collect the reason-less literals on the current decision level (i.e.
 *        literals representing a variable assignment) that led to the assignment
 *        of a given literal \p query.
 *
 * Usage example: use this function to analyze conflicts on the decision level where
 * assumption literals are stored (and propagated) to obtain a superset of the assumptions
 * that were used to obtain an UNSAT result.
 *
 * Note: \p query is always included in the result, even when \p query has
 * an assignment reason.
 *
 * \param reasonProvider    A provider of reason clauses.
 * \param dlProvider        A provider of decision levels for literals.
 * \param stamps            A StampMap suitable for stamping CNFVar objects. \p stamps must
 *                          support stamping the greatest variable occuring on \p trail.
 * \param query             The literal as described above.
 *
 * \result The set of assignment-representing literals as described above.
 *
 * \tparam ReasonProviderT      A type satisfying the ReasonProvider concept, with the reason type
 *                              satisfying the LiteralContainer concept.
 * \tparam DLProviderT          A type satisfying the DecisionLevelProvider concept.
 * \tparam StampMapT            A specialization of StampMap<...> supporting stamping of
 *                              CNFVar objects.
 */
template <typename ReasonProviderT, typename DLProviderT, typename StampMapT>
std::vector<CNFLit> analyzeAssignment(ReasonProviderT& reasonProvider,
                                      DLProviderT& trail,
                                      StampMapT& stamps,
                                      CNFLit query);

/********** Implementation ****************************** */

template <typename ReasonProviderT, typename DLProviderT, typename StampMapT>
std::vector<CNFLit> analyzeAssignment(ReasonProviderT& reasonProvider,
                                      DLProviderT& dlProvider,
                                      StampMapT& stamps,
                                      CNFLit query) {
    static_assert(is_reason_provider<ReasonProviderT, typename ReasonProviderT::Reason>::value,
                  "Template arg. ReasonProviderT doesn't satisfy the ReasonProvider concept");
    static_assert(is_literal_container<typename ReasonProviderT::Reason>::value,
                  "Template arg. ReasonProviderT::Reason doesn't satisfy the"
                  " LiteralContainer concept");
    static_assert(is_decision_level_provider<DLProviderT>::value,
                  "Template arg. DLProviderT doesn't satisfy the DecisionLevelProvider concept");
    static_assert(is_stamp_map<StampMapT, CNFVar>::value,
                  "Template arg. StampMapT is not a StampMap specialization supporting CNFVar");

    const auto stampContext = stamps.createContext();
    const auto stamp = stampContext.getStamp();

    std::vector<CNFLit> result{query};

    if (reasonProvider.get_reason(query.getVariable()) == nullptr) {
        return result;
    }

    auto currentDecisionLevel = dlProvider.get_current_level();

    std::vector<CNFVar> toAnalyze{query.getVariable()};
    while (!toAnalyze.empty()) {
        CNFVar currentVar = toAnalyze.back();
        toAnalyze.pop_back();
        stamps.setStamped(currentVar, stamp, true);
        JAM_ASSERT(reasonProvider.get_reason(currentVar) != nullptr,
                   "Expected only literals with reasons in the work queue");
        auto assignmentReason = reasonProvider.get_reason(currentVar);
        for (CNFLit lit : *assignmentReason) {
            if (stamps.isStamped(lit.getVariable(), stamp)) {
                continue;
            }
            stamps.setStamped(lit.getVariable(), stamp, true);
            if (dlProvider.get_level(lit.getVariable()) == currentDecisionLevel) {
                if (reasonProvider.get_reason(lit.getVariable()) != nullptr) {
                    toAnalyze.push_back(lit.getVariable());
                } else {
                    // ~lit must be on the trail: the only positive-assigned literal of
                    // the clause is the one whose assignment it has forced. The algorithm arrived
                    // here via that positive-assigned literal, so its variable must have been
                    // stamped further up the outer loop.
                    result.push_back(~lit);
                }
            }
        }
    }

    return result;
}
}
