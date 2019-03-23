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
 * \file LightweightSimplifier.h
 * \brief A CNF problem simiplifier that can be used in CDCL search for
 *        preprocessing as well as inprocessing.
 */

#pragma once

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/simplification/FailedLiteralAnalysis.h>
#include <libjamsat/simplification/SSRWithHyperBinaryResolution.h>
#include <libjamsat/simplification/SimplificationStats.h>
#include <libjamsat/simplification/UnaryOptimizations.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Casts.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/OccurrenceMap.h>

#include <boost/optional.hpp>

#include <stdexcept>


#if defined(JAM_ENABLE_INFLIGHTSIMP_LOGGING)
#define JAM_LOG_LIGHTWEIGHTSIMP(x, y) JAM_LOG(x, "lwsimp", y)
#else
#define JAM_LOG_LIGHTWEIGHTSIMP(x, y)
#endif

namespace jamsat {
/**
 * \defgroup JamSAT_Simplification  JamSAT Simplification Library
 *
 * This module contains SAT problem instance simplifiers. Except for
 * the functions defined in the \ref JamSAT_Simplification_Minimizer
 * submodule, the functions and classes contained in this module's
 * submodules should only be used by classes defined in this module
 * (i.e. \ref JamSAT_Simplification).
 *
 * This simplification library is primarily used by the \ref JamSAT_Solver
 * to perform _inprocessing_, i.e. simplification of the problem during
 * search.
 */

/**
 * \ingroup JamSAT_Simplification
 *
 * \brief A problem simplifier for performing lightweight simplifications.
 *
 * Intended usage: simplify a problem before and during search
 *
 * If `F` is a SAT problem instance and `G` is a SAT problem
 * instance derived from `F` by applying methods of `LightweightSimplifier`,
 * `G` is equivalent to `F`.
 *
 * \tparam PropagationT         A type that is a model of the Propagation concept
 * \tparam AssignmentProviderT  A type that is a model of the AssignmentProvider
 *                              concept
 */
template <typename PropagationT, typename AssignmentProviderT>
class LightweightSimplifier {
public:
    static_assert(
        std::is_same<typename PropagationT::Clause, typename AssignmentProviderT::Clause>::value,
        "PropagationT and AssignmentProviderT must have the same Clause type");


    using Propagation = PropagationT;
    using AssignmentProvider = typename Propagation::AssignmentProvider;
    using Clause = typename Propagation::Clause;

    /**
     * \brief Constructs a LightweightSimplifier
     *
     * \param maxVar             The maximum variable occurring in the problem instance.
     * \param propagation        A propagator where the problem's clauses are registered.
     * \param assignmentProvider An assignment provider in sync with \p propagation.
     */
    LightweightSimplifier(CNFVar maxVar,
                          PropagationT& propagation,
                          AssignmentProviderT& assignmentProvider) noexcept;

    /**
     * \brief Performs lightweight simplification
     *
     * - removes clauses satisfied because of assignments forced by unary clauses
     * - strengthens clauses using assignments forced by unary clauses
     * - removes and strengthens clauses using hyper-binary resolution
     * - performs failed literal elimination, restricted in the sense that failed literals
     *   are detected using only the clauses in \p possiblyIrredundantClauses (this
     *   is a by-product of the third item in this list)
     *
     * Precondition: all unary clauses have been propagated using the propagation object
     * and the assignment provider passed to the simplifier
     *
     * If a new unary clause is deduced during simplification, it is added to
     * \p unaryClauses. If the problem instance is detected to be unsatisfiable
     * via simplification, the derived contradictory unary clauses are placed in
     * \p unaryClauses.
     *
     * No assumptions may be made about the current literals assignments when this
     * function returns.
     *
     * \param unaryClauses                  The current set of unary clauses.
     * \param problemClauses                A range of pointers to the problem clauses.
     * \param tempStamps                    A StampMap capable of stamping any CNFLit
     *                                      object occurring in the clauses passed via
     *                                      \p unaryClauses, \p possiblyIrredundantClauses
     *                                      and \p redundantClauses or during propagation.
     *
     * \returns Statistics about the applied simplifications.
     */
    template <typename ClausePtrRange, typename StampMapT>
    auto simplify(std::vector<CNFLit>& unaryClauses,
                  ClausePtrRange problemClauses,
                  StampMapT& tempStamps) -> SimplificationStats;

    /**
     * \brief Performs failed literal elimination
     *
     * Precondition: all unary clauses have been propagated using the propagation object
     * and the assignment provider passed to the simplifier
     *
     * If a new unary clause is deduced during this procedure, it is added to
     * \p unaryClauses. If the problem instance is detected to be unsatisfiable
     * via simplification, the derived contradictory unary clauses are placed in
     * \p unaryClauses.
     *
     * No assumptions may be made about the current literals assignments when this
     * function returns.
     *
     * \param unaryClauses      The unary clauses currently
     *
     * \return Statistics about the applied simplifications.
     */
    auto eliminateFailedLiterals(std::vector<CNFLit>& unaryClauses) -> SimplificationStats;

    /**
     * \brief Increases the maximum variable which may occur in the problem instance..
     *
     * \param newMaxVar     The new maximum variable. Must not be smaller than the previous
     *                      maximum variable, and must be a regular variable.
     */
    void increaseMaxVarTo(CNFVar newMaxVar);

    auto operator=(LightweightSimplifier const& other) -> LightweightSimplifier& = delete;
    auto operator=(LightweightSimplifier&& other) -> LightweightSimplifier& = delete;
    LightweightSimplifier(LightweightSimplifier const& other) = delete;
    LightweightSimplifier(LightweightSimplifier&& other) = delete;

private:
    class ClauseDeletedQuery {
    public:
        bool operator()(Clause const* x) const noexcept {
            return x->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
        }
    };

    class DetectedUNSATException final : public std::exception {
    public:
        virtual ~DetectedUNSATException(){};
    };

    /**
     * \brief Propagates the given facts in m_propagator
     *
     * \param[in,out] the facts to propagate. All new facts found during propagation
     *                are added to `facts`. The relative order of elements in `facts`
     *                is not preserved.
     *
     * \returns SimplificationStats containing the number of learnt facts
     */
    auto propagateFacts(std::vector<CNFLit>& facts) -> SimplificationStats;

    /**
     * \brief Subsumption and self-subsuming resolution using unary clauses.
     *
     * \param unaryClauses  A vector of literals representing unary clauses.
     * 
     * Precondition: m_assignmentProvider must have no assignments.
     *
     * \returns Simplification statistics
     */
    auto runUnaryOptimizations(std::vector<CNFLit> const& unaryClauses) -> SimplificationStats;

    /**
     * \brief Runs ssrWithHyperBinaryResolution() for all literals and performs failed
     * literal elimination for all encountered failed literals.
     *
     *
     * \param tempStamps    A StampMap capable of stamping any CNFLit
     *                      with a variable no greater than m_maxLit.
     * \param unaryClauses  A vector of literals representing unary clauses. Unary
     *                      clauses derived via failed literal elimination are added
     *                      to this vector.
     *
     * \returns Simplification statistics
     */
    template <typename StampMapT>
    auto runSSRWithHBR(StampMapT& tempStamps, std::vector<CNFLit>& unaryClauses)
        -> SimplificationStats;

    enum FLEPostProcessing { NONE, FULL };

    /**
     * \brief Performs failed literal elimination for a failed literal.
     *
     * `m_assignmentProvider` and `m_propagation` must be in the state just after
     * the propagation (to fixpoint) of \p failedLiteral (with only the assignments
     * forced by unary clauses being set before the propagation).
     *
     * Unless `postProcMode == FLEPostProcessing::FULL`, `m_occurrenceMap` does not
     * need to be in a valid state during the execution of this method.
     *
     * \param failedLiteral      A literal whose propagation until fixpoint (after
     *                           propagation of assignments forced by unary clauses)
     *                           results in a conflict with conflicting clause
     *                           \p conflictingClause.
     * \param conflictingClause  A conflicting clause for \p failedLiteral.
     * \param unaries            A vector of literals representing unary clauses.
     *                           Unary clauses derived via failed literal elimination
     *                           are added to this vector.
     * \param unaryLevel         The decision level on which assignments forced by
     *                           unary clauses are propagated.
     * \param postProcMode       If FULL, subsumption checks and strengthening is
     *                           performed for the newly found unaries. If NONE,
     *                           the problem clauses are not modified by this method.
     */
    auto eliminateFailedLiteral(CNFLit failedLiteral,
                                Clause* conflictingClause,
                                std::vector<CNFLit>& unaries,
                                FLEPostProcessing postProcMode) -> SimplificationStats;

    PropagationT& m_propagation;
    AssignmentProviderT& m_assignmentProvider;
    CNFVar m_maxVar;
    boost::optional<size_t> m_lastSeenAmntUnaries;
    OccurrenceMap<Clause, ClauseDeletedQuery> m_occurrenceMap;

    using FailedLiteralAnalyzerT = FailedLiteralAnalyzer<AssignmentProviderT, PropagationT>;
    FailedLiteralAnalyzerT m_failedLitAnalyzer;
};

template <typename PropagationT, typename AssignmentProviderT>
LightweightSimplifier<PropagationT, AssignmentProviderT>::LightweightSimplifier(
    CNFVar maxVar, PropagationT& propagation, AssignmentProviderT& assignmentProvider) noexcept
  : m_propagation{propagation}
  , m_assignmentProvider{assignmentProvider}
  , m_maxVar{maxVar}
  , m_lastSeenAmntUnaries{}
  , m_occurrenceMap{getMaxLit(m_maxVar)}
  , m_failedLitAnalyzer{m_maxVar, m_propagation, m_assignmentProvider, m_assignmentProvider, 0} {}

template <typename PropagationT, typename AssignmentProviderT>
template <typename ClausePtrRange, typename StampMapT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT>::simplify(
    std::vector<CNFLit>& unaryClauses, ClausePtrRange problemClauses, StampMapT& tempStamps)
    -> SimplificationStats {
    JAM_LOG_LIGHTWEIGHTSIMP(info, "Starting problem simplification");

    JAM_ASSERT(m_assignmentProvider.getNumberOfAssignments() == 0,
               "LightweightSimplifier may only be invoked when the solver has no assignments");
    OnExitScope undoAllAssignments{[this]() {
        JAM_LOG_LIGHTWEIGHTSIMP(info, "Finished problem simplification");
        m_assignmentProvider.shrinkToDecisionLevel(0);
    }};

    SimplificationStats result;
    try {
        result = propagateFacts(unaryClauses);
    } catch (DetectedUNSATException&) {
        return SimplificationStats{};
    }

    if (m_lastSeenAmntUnaries && unaryClauses.size() <= *m_lastSeenAmntUnaries) {
        return result;
    }

    m_occurrenceMap.clear();
    m_occurrenceMap.insert(problemClauses.begin(), problemClauses.end());

    m_assignmentProvider.shrinkToDecisionLevel(0);
    result += runUnaryOptimizations(unaryClauses);

    propagateFacts(unaryClauses);
    result += runSSRWithHBR(tempStamps, unaryClauses);

    m_lastSeenAmntUnaries = unaryClauses.size();
    return result;
}


template <typename PropagationT, typename AssignmentProviderT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT>::propagateFacts(
    std::vector<CNFLit>& facts) -> SimplificationStats {
    JAM_LOG_LIGHTWEIGHTSIMP(info, "Propagating facts...");

    SimplificationStats result;
    auto initialFactCount = facts.size();

    for (CNFLit fact : facts) {
        TBool previousAssignment = m_assignmentProvider.getAssignment(fact);
        if (!isDeterminate(previousAssignment)) {
            m_assignmentProvider.addAssignment(fact);
            if (m_propagation.propagateUntilFixpoint(fact) != nullptr) {
                JAM_LOG_LIGHTWEIGHTSIMP(info, "Detected unsatisfiability by propagation");
                throw DetectedUNSATException{};
            }
        } else if (isFalse(previousAssignment)) {
            JAM_LOG_LIGHTWEIGHTSIMP(info, "Detected unsatisfiability by propagation");
            throw DetectedUNSATException{};
        }
    }

    result.amntUnariesLearnt = m_assignmentProvider.getNumberOfAssignments() - initialFactCount;
    facts.clear();
    auto newFacts = m_assignmentProvider.getDecisionLevelAssignments(0);
    facts.insert(facts.begin(), newFacts.begin(), newFacts.end());

    JAM_LOG_LIGHTWEIGHTSIMP(info, "Finished propagating facts, no conflict detected");
    return result;
}

template <typename PropagationT, typename AssignmentProviderT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT>::eliminateFailedLiterals(
    std::vector<CNFLit>& unaryClauses) -> SimplificationStats {
    JAM_LOG_LIGHTWEIGHTSIMP(info, "Performing full failed literal elimination");

    SimplificationStats result;
    auto currentDL = m_assignmentProvider.getCurrentDecisionLevel();

    for (CNFVar i{0}; i <= m_maxVar; i = nextCNFVar(i)) {
        for (CNFSign sign : {CNFSign::NEGATIVE, CNFSign::POSITIVE}) {
            if (m_assignmentProvider.getAssignment(i) != TBools::INDETERMINATE) {
                continue;
            }

            CNFLit candidate{i, sign};
            m_assignmentProvider.newDecisionLevel();
            m_assignmentProvider.addAssignment(candidate);
            auto conflictingClause = m_propagation.propagateUntilFixpoint(candidate);
            if (!conflictingClause) {
                m_assignmentProvider.revisitDecisionLevel(currentDL);
                continue;
            }

            try {
                JAM_ASSERT(currentDL == 0, "Must perform FLE on level 0");
                result += eliminateFailedLiteral(
                    candidate, conflictingClause, unaryClauses, FLEPostProcessing::NONE);
                JAM_ASSERT(
                    m_assignmentProvider.getCurrentDecisionLevel() == currentDL,
                    "eliminateFailedLiteral() should have returned to currentDL, but didn't");
            } catch (DetectedUNSATException&) {
                // The unaries are contradictory now, so simplifying the problem
                // further would be redundant
                return result;
            }
        }
    }

    JAM_LOG_LIGHTWEIGHTSIMP(info, "Finished performing full failed literal elimination");
    return result;
}

template <typename PropagationT, typename AssignmentProviderT>
void LightweightSimplifier<PropagationT, AssignmentProviderT>::increaseMaxVarTo(CNFVar newMaxVar) {
    JAM_ASSERT(isRegular(newMaxVar), "Argument newMaxVar must be a regular variable.");
    JAM_ASSERT(newMaxVar >= m_maxVar,
               "Argument newMaxVar must not be smaller than the current maximum variable");
    m_maxVar = newMaxVar;
    m_occurrenceMap.increaseMaxElementTo(getMaxLit(newMaxVar));
    m_failedLitAnalyzer.increaseMaxVarTo(newMaxVar);
}


template <typename PropagationT, typename AssignmentProviderT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT>::runUnaryOptimizations(
    std::vector<CNFLit> const& unaryClauses) -> SimplificationStats {

    SimplificationStats result;
    auto delMarker = [this](Clause* cla) { m_propagation.notifyClauseModificationAhead(*cla); };
    result += scheduleClausesSubsumedByUnariesForDeletion(m_occurrenceMap, delMarker, unaryClauses);
    result += strengthenClausesWithUnaries(m_occurrenceMap, delMarker, unaryClauses);
    return result;
}

template <typename PropagationT, typename AssignmentProviderT>
template <typename StampMapT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT>::runSSRWithHBR(
    StampMapT& tempStamps, std::vector<CNFLit>& unaryClauses) -> SimplificationStats {

    SimplificationStats result;
    auto delMarker = [this](Clause* cla) { m_propagation.notifyClauseModificationAhead(*cla); };
    auto ssrWithHBRParams = createSSRWithHBRParams(
        m_occurrenceMap, delMarker, m_propagation, m_assignmentProvider, tempStamps);

    for (CNFVar i{0}; i <= m_maxVar; i = nextCNFVar(i)) {
        for (CNFSign sign : {CNFSign::NEGATIVE, CNFSign::POSITIVE}) {
            CNFLit resolveAt{i, sign};
            try {
                result += ssrWithHyperBinaryResolution(ssrWithHBRParams, resolveAt);
            } catch (FailedLiteralException<Clause>& e) {
                try {
                    JAM_ASSERT(e.getDecisionLevelToRevisit() == 0, "Must revisit level 0");
                    result += eliminateFailedLiteral(~resolveAt,
                                                     e.getConflictingClause(),
                                                     unaryClauses,
                                                     FLEPostProcessing::FULL);
                } catch (DetectedUNSATException&) {
                    // The unaries are contradictory now, so simplifying the problem
                    // further would be redundant
                    return result;
                }
                // The unaries decision level is revisited during failed literal elimination
            }
        }
    }
    return result;
}


template <typename PropagationT, typename AssignmentProviderT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT>::eliminateFailedLiteral(
    CNFLit failedLiteral,
    Clause* conflictingClause,
    std::vector<CNFLit>& unaries,
    FLEPostProcessing postProcMode) -> SimplificationStats {
    auto analysis = m_failedLitAnalyzer.analyze(failedLiteral, conflictingClause);

    // Add the newly found unary to the unaries vector and perform subsumption/strengthening
    // with the new unaries:
    std::copy(analysis.newFacts.begin(), analysis.newFacts.end(), std::back_inserter(unaries));
    if (analysis.detectedUnsat) {
        throw DetectedUNSATException{};
    }

    SimplificationStats result = analysis.stats;
    if (postProcMode == FLEPostProcessing::FULL) {
        m_assignmentProvider.shrinkToDecisionLevel(0);
        result += runUnaryOptimizations(analysis.newFacts);
        propagateFacts(unaries);
    } else {
        // Propagate the facts to keep the propagator in a consistent state:
        for (CNFLit fact : analysis.newFacts) {
            if (!isDeterminate(m_assignmentProvider.getAssignment(fact))) {
                m_assignmentProvider.addAssignment(fact);
                m_propagation.propagateUntilFixpoint(fact);
            }
        }
    }

    return result;
}
}
