/* Copyright (c) 2019 Felix Kutzner (github.com/fkutzner)

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
 * \file FailedLiteralAnalysis.h
 * \brief Failed literal analysis functions
 */

#pragma once

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/simplification/SimplificationStats.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/utils/ControlFlow.h>

#include <vector>

#if defined(JAM_ENABLE_INFLIGHTSIMP_LOGGING)
#define JAM_LOG_FLE(x, y) JAM_LOG(x, "flelim", y)
#else
#define JAM_LOG_FLE(x, y)
#endif

namespace jamsat {

/**
 * \ingroup JamSAT_Simplification
 *
 * \brief Failed literal analyzer
 *
 * A failed literal is a literal L such that propagating L after propagating the facts
 * directly leads to a conflict. This class is responsible for analyzing the current
 * assignment and propagator state right after the detection of a failed literal, which
 * may easily reveal more failed literals or even the problem's unsatisfiability.
 *
 * \tparam DLProviderT      A type that is a model of the DecisionLevelProvider concept.
 * \tparam PropagationT     A type that is a model of the Propagation concept.
 */
template <typename DLProviderT, typename PropagationT>
class FailedLiteralAnalyzer {
public:
    using Clause = typename PropagationT::Clause;
    using AssignmentProvider = typename PropagationT::AssignmentProvider;

    /**
     * \brief Constructs a FailedLiteralAnalyzer.
     *
     * \param maxVar                    The maximum variable occuring in the problem.
     * \param propagator                The propagator to analyze.
     * \param assignmentProvider        The assignment provider associated with `propagator`.
     * \param decisionLevelProvider     The decision level provider associated with `propagator`.
     * \param factLevel                 The decision level on which facts are propagated.
     */
    FailedLiteralAnalyzer(CNFVar maxVar,
                          PropagationT& propagator,
                          AssignmentProvider& assignmentProvider,
                          DLProviderT& decisionLevelProvider,
                          typename DLProviderT::DecisionLevel factLevel);

    struct Analysis {
        /**
         * The new facts found by analyzing the failed literal. Includes the failed literal or
         * - in case unsatisfiability has been detected - a contradictory pair of facts.
         */
        std::vector<CNFLit> newFacts;

        /**
         *  `true` iff the problem has been detected to be unsatisfiable. Iff this field is set to
         *  `true`, the field `newFacts` contains a contradiction.
         */
        bool detectedUnsat = false;

        /**
         *  Simplification statistics.
         */
        SimplificationStats stats;
    };

    /**
     * \brief Analyzes a conflict induced by propagating a failed literal.
     *
     * \param failedLiteral         A failed literal.
     * \param conflictingClause     A clause that is falsified by propagating `failedLiteral` on
     *                              the fact decision level.
     * \returns                     An Analysis object describing the analysis.
     *
     * Precondition: The state of the propagator, assignment provider and decision level provider
     *               passed to the analyzer's constructor must not have been changed after detecting
     *               the conflict with clause `conflictingClause`.
     *
     * Postcondition: The decision level has been reduced to the fact level, without the new facts
     *                having been propagated.
     */
    auto analyze(CNFLit failedLiteral, Clause* conflictingClause) -> Analysis;

    /**
     * \brief Increases the maximum variable which may occur in the problem instance.
     *
     * \param newMaxVar     The new maximum variable. Must not be smaller than the previous
     *                      maximum variable, and must be a regular variable.
     */
    void increaseMaxVarTo(CNFVar newMaxVar);

private:
    PropagationT& m_propagator;
    AssignmentProvider& m_assignmentProvider;
    DLProviderT& m_decisionLevelProvider;
    typename DLProviderT::DecisionLevel m_factLevel;
    FirstUIPLearning<DLProviderT, PropagationT> m_conflictAnalyzer;
};

/********** Implementation ****************************** */

template <typename DLProviderT, typename PropagationT>
FailedLiteralAnalyzer<DLProviderT, PropagationT>::FailedLiteralAnalyzer(
    CNFVar maxVar,
    PropagationT& propagator,
    AssignmentProvider& assignmentProvider,
    DLProviderT& decisionLevelProvider,
    typename DLProviderT::DecisionLevel factLevel)
  : m_propagator{propagator}
  , m_assignmentProvider{assignmentProvider}
  , m_decisionLevelProvider{decisionLevelProvider}
  , m_factLevel{factLevel}
  , m_conflictAnalyzer{maxVar, decisionLevelProvider, propagator} {}


template <typename DLProviderT, typename PropagationT>
auto FailedLiteralAnalyzer<DLProviderT, PropagationT>::analyze(CNFLit failedLiteral,
                                                               Clause* conflictingClause)
    -> Analysis {
    JAM_LOG_FLE(info, "Performing failed literal elimination for failed lit. " << failedLiteral);

    // The propagation of the assignment represented by failedLiteral resulted in a conflict.
    // Suppose there are clauses encoding the implications failedLiteral -> x, x -> y,
    // y -> z, y -> ~z. The solver should not only learn ~failedLiteral, but in this case
    // also ~x - more generally, the negation of the asserting literal obtained by resolution
    // until the first UIP.
    //
    // Thus:
    std::vector<CNFLit> pseudoLemma;
    m_conflictAnalyzer.computeConflictClause(*conflictingClause, pseudoLemma);
    JAM_LOG_FLE(info, "FLE pseudolemma: " << toString(pseudoLemma.begin(), pseudoLemma.end()));
    CNFLit assertingLit = pseudoLemma[0];
    JAM_LOG_FLE(info, "Neg. of asserting lit. " << assertingLit << " is also a failed literal.");

    // Now learn assertingLit and all its consequences:
    m_assignmentProvider.revisitDecisionLevel(m_factLevel);
    m_decisionLevelProvider.newDecisionLevel();
    OnExitScope returnTofactLevel{
        [this]() { m_assignmentProvider.revisitDecisionLevel(m_factLevel); }};

    auto firstNewUnaryIdx = m_assignmentProvider.getNumberOfAssignments();
    m_assignmentProvider.addAssignment(assertingLit);
    Clause* newConflict = m_propagator.propagateUntilFixpoint(assertingLit);

    if (newConflict) {
        JAM_LOG_FLE(info, "Detected UNSAT: can't assign var." << assertingLit.getVariable());
        return Analysis{{assertingLit, ~assertingLit}, true, SimplificationStats{}};
    }

    // If propagating assertingLit did not imply an assignment for the failed
    // literal's variable, propagate ~failedLiteral, too - at this point, it
    // is known that ~failedLiteral is unary.
    if (m_assignmentProvider.getAssignment(failedLiteral) == TBools::INDETERMINATE) {
        JAM_LOG_FLE(info,
                    "Propagating the asserting lit did not imply an assignment "
                    "for the failed literal's variable");
        m_assignmentProvider.addAssignment(~failedLiteral);
        newConflict = m_propagator.propagateUntilFixpoint(~failedLiteral);
        if (newConflict) {
            JAM_LOG_FLE(info, "Detected UNSAT: can't assign var." << failedLiteral.getVariable());
            return Analysis{{assertingLit, ~failedLiteral}, true, SimplificationStats{}};
        }
    }

    // Add the newly found unary to the unaries vector and perform subsumption/strengthening
    // with the new unaries:
    auto newUnariesRange = m_assignmentProvider.getAssignments(firstNewUnaryIdx);
    Analysis result;
    result.detectedUnsat = false;
    result.newFacts = std::vector<CNFLit>{newUnariesRange.begin(), newUnariesRange.end()};
    result.stats.amntUnariesLearnt = static_checked_cast<uint32_t>(result.newFacts.size());
    JAM_LOG_FLE(info,
                "Detected facts: " << toString(result.newFacts.begin(), result.newFacts.end()));

    return result;
}


template <typename DLProviderT, typename PropagationT>
void FailedLiteralAnalyzer<DLProviderT, PropagationT>::increaseMaxVarTo(CNFVar newMaxVar) {
    m_conflictAnalyzer.increaseMaxVarTo(newMaxVar);
}

}
