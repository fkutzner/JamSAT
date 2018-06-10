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

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/simplification/SSRWithHyperBinaryResolution.h>
#include <libjamsat/simplification/UnaryOptimizations.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/OccurrenceMap.h>

#if defined(JAM_ENABLE_INFLIGHTSIMP_LOGGING)
#define JAM_LOG_LIGHTWEIGHTSIMP(x, y) JAM_LOG(x, "lwsimp", y)
#else
#define JAM_LOG_LIGHTWEIGHTSIMP(x, y)
#endif

namespace jamsat {
/**
 * \ingroup JamSAT_Simplification
 *
 * \brief A problem simplifier for performing lightweight simplifications.
 *
 * Intended usage: simplify a problem during search
 *
 * \tparam PropagationT         A type that is a model of the Propagation concept
 * \tparam AssignmentProviderT  TODO
 * \tparam ConflictAnalyzerT
 */
template <typename PropagationT, typename AssignmentProviderT, typename ConflictAnalyzerT>
class LightweightSimplifier {
public:
    static_assert(
        std::is_same<typename PropagationT::Clause, typename AssignmentProviderT::Clause>::value,
        "PropagationT and AssignmentProviderT must have the same Clause type");

    static_assert(
        std::is_same<typename PropagationT::Clause, typename ConflictAnalyzerT::Clause>::value,
        "PropagationT and ConflictAnalyzerT must have the same Clause type");


    using Propagation = PropagationT;
    using AssignmentProvider = AssignmentProviderT;
    using ConflictAnalyzer = ConflictAnalyzerT;
    using Clause = typename Propagation::Clause;

    /**
     * \brief Constructs a LightweightSimplifier
     *
     * \param maxVar             The maximum variable occurring in the problem instance
     * \param propagation        A propagator where the problem's clauses are registered
     * \param assignmentProvider TODO
     */
    LightweightSimplifier(CNFVar maxVar, PropagationT &propagation,
                          AssignmentProviderT &assignmentProvider) noexcept;

    /**
     * \brief Performs lightweight simplification
     *
     * - removes clauses satisfied because of assignments forced by unary clauses
     * - strengthens clauses using assignments forced by unary clauses
     * - performs failed literal elimination, restricted in the sense that failed literals
     *   are detected using only the clauses in \p possiblyIrredundantClauses
     *
     * Precondition: all unary clauses have been propagated using the propagation object
     * and the assignment provider passed to the simplifier
     *
     * If a new unary clause is deduced during simplification, it is added to
     * \p unaryClauses.
     *
     * \param unaryClauses                  The current set of unary clauses.
     * \param possiblyIrredundantClauses    The current set of clauses that are possibly
     *                                      not redundant.
     * \param redundantClauses              The current set of clauses that are redundant.
     * \param tempStamps                    A StampMap capable of stamping any CNFLit
     *                                      object occurring in the clauses passed via
     *                                      \p unaryClauses, \p possiblyIrredundantClauses
     *                                      and \p redundantClauses or during propagation.
     */
    template <typename StampMapT>
    auto simplify(std::vector<CNFLit> &unaryClauses,
                  std::vector<Clause *> const &possiblyIrredundantClauses,
                  std::vector<Clause *> const &redundantClauses, StampMapT &tempStamps)
        -> SimplificationStats;

    /**
     * \brief Increases the maximum variable which may occur in the problem instance..
     *
     * \param newMaxVar     The new maximum variable. Must not be smaller than the previous
     *                      maximum variable, and must be a regular variable.
     */
    void increaseMaxVarTo(CNFVar newMaxVar);

    auto operator=(LightweightSimplifier const &other) -> LightweightSimplifier & = delete;
    auto operator=(LightweightSimplifier &&other) -> LightweightSimplifier & = delete;
    LightweightSimplifier(LightweightSimplifier const &other) = delete;
    LightweightSimplifier(LightweightSimplifier &&other) = delete;

private:
    class ClauseDeletedQuery {
    public:
        bool operator()(Clause const *x) const noexcept {
            return x->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
        }
    };

    class DetectedUNSATException {};

    auto eliminateFailedLiteral(CNFLit failedLiteral, Clause *conflictingClause,
                                std::vector<CNFLit> &unaries,
                                typename AssignmentProviderT::DecisionLevel unaryLevel)
        -> SimplificationStats;

    PropagationT &m_propagation;
    AssignmentProviderT &m_assignmentProvider;

    CNFVar m_maxVar;
    size_t m_lastSeenAmntUnaries;
    OccurrenceMap<Clause, ClauseDeletedQuery> m_occurrenceMap;

    // Keeping a separate conflict analyzer here to avoid disturbing
    // heuristics
    ConflictAnalyzerT m_firstUIPAnalyzer;
};

template <typename PropagationT, typename AssignmentProviderT, typename ConflictAnalyzerT>
LightweightSimplifier<PropagationT, AssignmentProviderT, ConflictAnalyzerT>::LightweightSimplifier(
    CNFVar maxVar, PropagationT &propagation, AssignmentProviderT &assignmentProvider) noexcept
  : m_propagation{propagation}
  , m_assignmentProvider{assignmentProvider}
  , m_maxVar{maxVar}
  , m_lastSeenAmntUnaries{0}
  , m_occurrenceMap{getMaxLit(m_maxVar)}
  , m_firstUIPAnalyzer{m_maxVar, m_assignmentProvider, m_propagation} {}

template <typename PropagationT, typename AssignmentProviderT, typename ConflictAnalyzerT>
template <typename StampMapT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT, ConflictAnalyzerT>::simplify(
    std::vector<CNFLit> &unaryClauses, std::vector<Clause *> const &possiblyIrredundantClauses,
    std::vector<Clause *> const &redundantClauses, StampMapT &tempStamps) -> SimplificationStats {

    SimplificationStats result;


    if (unaryClauses.size() <= m_lastSeenAmntUnaries) {
        return result;
    }

    auto currentDecisionLevel = m_assignmentProvider.getCurrentDecisionLevel();
    m_occurrenceMap.clear();
    m_occurrenceMap.insert(possiblyIrredundantClauses.begin(), possiblyIrredundantClauses.end());
    m_occurrenceMap.insert(redundantClauses.begin(), redundantClauses.end());

    auto delMarker = [this](Clause *cla) { m_propagation.notifyClauseModificationAhead(*cla); };
    result += scheduleClausesSubsumedByUnariesForDeletion(m_occurrenceMap, delMarker, unaryClauses);
    result += strengthenClausesWithUnaries(m_occurrenceMap, delMarker, unaryClauses);

    for (CNFVar i{0}; i <= m_maxVar; i = nextCNFVar(i)) {
        for (CNFSign sign : {CNFSign::NEGATIVE, CNFSign::POSITIVE}) {
            CNFLit resolveAt{i, sign};
            try {
                result += ssrWithHyperBinaryResolution(m_occurrenceMap, delMarker, m_propagation,
                                                       m_assignmentProvider, tempStamps, resolveAt);
            } catch (FailedLiteralException<Clause> &e) {
                try {
                    result += eliminateFailedLiteral(~resolveAt, e.getConflictingClause(),
                                                     unaryClauses, e.getDecisionLevelToRevisit());
                } catch (DetectedUNSATException &e) {
                    // The unaries are contradictory now, so simplifying the problem
                    // further would be redundant
                    return result;
                }
                // The unaries decision level is revisited during failed literal elimination
            }
        }
    }

    m_lastSeenAmntUnaries = unaryClauses.size();


    JAM_ASSERT(m_assignmentProvider.getCurrentDecisionLevel() == currentDecisionLevel,
               "Illegal decision level modification");
    return result;
}

template <typename PropagationT, typename AssignmentProviderT, typename ConflictAnalyzerT>
void LightweightSimplifier<PropagationT, AssignmentProviderT, ConflictAnalyzerT>::increaseMaxVarTo(
    CNFVar newMaxVar) {
    JAM_ASSERT(isRegular(newMaxVar), "Argument newMaxVar must be a regular variable.");
    JAM_ASSERT(newMaxVar >= m_maxVar,
               "Argument newMaxVar must not be smaller than the current maximum variable");
    m_maxVar = newMaxVar;
    m_occurrenceMap.increaseMaxElementTo(getMaxLit(newMaxVar));
    m_firstUIPAnalyzer.increaseMaxVarTo(newMaxVar);
}

template <typename PropagationT, typename AssignmentProviderT, typename ConflictAnalyzerT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT, ConflictAnalyzerT>::
    eliminateFailedLiteral(CNFLit failedLiteral, Clause *conflictingClause,
                           std::vector<CNFLit> &unaries,
                           typename AssignmentProviderT::DecisionLevel unaryLevel)
        -> SimplificationStats {
    JAM_LOG_LIGHTWEIGHTSIMP(info, "Performing failed literal elimination for failed literal "
                                      << failedLiteral);
    SimplificationStats result;

    // The propagation of the assignment represented by failedLiteral resulted in a conflict.
    // Suppose there are clauses encoding the implications failedLiteral -> x, x -> y,
    // y -> z, y -> ~z. The solver should not only learn ~failedLiteral, but in this case
    // also ~x - more generally, the negation of the asserting literal obtained by resolution
    // until the first UIP.
    //
    // Thus:
    std::vector<CNFLit> pseudoLemma;
    m_firstUIPAnalyzer.computeConflictClause(*conflictingClause, pseudoLemma);
    JAM_LOG_LIGHTWEIGHTSIMP(
        info, "FLE pseudolemma: " << toString(pseudoLemma.begin(), pseudoLemma.end()));
    CNFLit assertingLit = pseudoLemma[0];
    JAM_LOG_LIGHTWEIGHTSIMP(info, "Negate of asserting literal " << assertingLit
                                                                 << " is also a failed literal.");

    // Now learn assertingLit and all its consequences:
    m_assignmentProvider.revisitDecisionLevel(unaryLevel);
    auto firstNewUnaryIdx = m_assignmentProvider.getNumberOfAssignments();
    m_assignmentProvider.addAssignment(assertingLit);
    auto newConflict = m_propagation.propagateUntilFixpoint(assertingLit);

    if (newConflict) {
        JAM_LOG_LIGHTWEIGHTSIMP(info, "Both " << assertingLit << " and " << ~assertingLit
                                              << " are failed literals. Detected UNSAT");
        unaries.push_back(assertingLit);
        unaries.push_back(~assertingLit);
        throw DetectedUNSATException{};
    }

    // If propagating assertingLit did not imply an assignment for the failed
    // literal's variable, propagate ~failedLiteral, too - at this point, it
    // is known that ~failedLiteral is unary.
    if (m_assignmentProvider.getAssignment(failedLiteral) == TBools::INDETERMINATE) {
        JAM_LOG_LIGHTWEIGHTSIMP(info, "Propagating the asserting lit did not imply an assignment "
                                      "for the failed literal's variable");
        m_assignmentProvider.addAssignment(~failedLiteral);
        newConflict = m_propagation.propagateUntilFixpoint(~failedLiteral);
        if (newConflict) {
            JAM_LOG_LIGHTWEIGHTSIMP(info, "Both " << failedLiteral << " and " << ~failedLiteral
                                                  << " are failed literals. Detected UNSAT");
            unaries.push_back(failedLiteral);
            unaries.push_back(~failedLiteral);
            throw DetectedUNSATException{};
        }
    }

    // Add the newly found unary to the unaries vector and perform subsumption/strengthening
    // with the new unaries:
    auto newUnariesRange = m_assignmentProvider.getAssignments(firstNewUnaryIdx);
    std::copy(newUnariesRange.begin(), newUnariesRange.end(), std::back_inserter(unaries));
    result.amntUnariesLearnt += newUnariesRange.size();
    JAM_LOG_LIGHTWEIGHTSIMP(info, "Detected new unaries "
                                      << toString(newUnariesRange.begin(), newUnariesRange.end()));

    auto delMarker = [this](Clause *cla) { m_propagation.notifyClauseModificationAhead(*cla); };
    result +=
        scheduleClausesSubsumedByUnariesForDeletion(m_occurrenceMap, delMarker, newUnariesRange);
    result += strengthenClausesWithUnaries(m_occurrenceMap, delMarker, newUnariesRange);

    JAM_LOG_LIGHTWEIGHTSIMP(info, "Finished failed literal elimination for failed literal "
                                      << failedLiteral);
    return result;
}
}
