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
 */
template <typename PropagationT, typename AssignmentProviderT>
class LightweightSimplifier {
public:
    static_assert(
        std::is_same<typename PropagationT::Clause, typename AssignmentProviderT::Clause>::value,
        "PropagationT and AssignmentProviderT must have the same Clause type");

    using Propagation = PropagationT;
    using AssignmentProvider = AssignmentProviderT;
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
     *
     * \param unaryClauses                  The current set of unary clauses.
     * \param possiblyIrredundantClauses    The current set of clauses that are possibly
     *                                      not redundant.
     * \param redundantClauses              The current set of clauses that are redundant.
     * \param tempStamps                    TODO
     */
    template <typename StampMapT>
    auto simplify(std::vector<CNFLit> const &unaryClauses,
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

    PropagationT &m_propagation;
    AssignmentProviderT &m_assignmentProvider;

    CNFVar m_maxVar;
    size_t m_lastSeenAmntUnaries;
    OccurrenceMap<Clause, ClauseDeletedQuery> m_occurrenceMap;
};

template <typename PropagationT, typename AssignmentProviderT>
LightweightSimplifier<PropagationT, AssignmentProviderT>::LightweightSimplifier(
    CNFVar maxVar, PropagationT &propagation, AssignmentProviderT &assignmentProvider) noexcept
  : m_propagation{propagation}
  , m_assignmentProvider{assignmentProvider}
  , m_maxVar{maxVar}
  , m_lastSeenAmntUnaries{0}
  , m_occurrenceMap{getMaxLit(m_maxVar)} {}

template <typename PropagationT, typename AssignmentProviderT>
template <typename StampMapT>
auto LightweightSimplifier<PropagationT, AssignmentProviderT>::simplify(
    std::vector<CNFLit> const &unaryClauses,
    std::vector<Clause *> const &possiblyIrredundantClauses,
    std::vector<Clause *> const &redundantClauses, StampMapT &tempStamps) -> SimplificationStats {

    SimplificationStats result;
    auto currentDecisionLevel = m_assignmentProvider.getCurrentDecisionLevel();

    if (unaryClauses.size() > m_lastSeenAmntUnaries) {

        m_occurrenceMap.clear();
        m_occurrenceMap.insert(possiblyIrredundantClauses.begin(),
                               possiblyIrredundantClauses.end());
        m_occurrenceMap.insert(redundantClauses.begin(), redundantClauses.end());

        auto delMarker = [this](Clause *cla) { m_propagation.notifyClauseModificationAhead(*cla); };
        result +=
            scheduleClausesSubsumedByUnariesForDeletion(m_occurrenceMap, delMarker, unaryClauses);
        result += strengthenClausesWithUnaries(m_occurrenceMap, delMarker, unaryClauses);

        for (CNFVar i{0}; i <= m_maxVar; i = nextCNFVar(i)) {
            try {
                result += ssrWithHyperBinaryResolution(m_occurrenceMap, delMarker, m_propagation,
                                                       m_assignmentProvider, tempStamps,
                                                       CNFLit{i, CNFSign::NEGATIVE});
            } catch (FailedLiteralException<Clause> &e) {
                // TODO: failed literal handling: do first-uip-learning, ...
                m_assignmentProvider.revisitDecisionLevel(e.getDecisionLevelToRevisit());
            }

            try {
                result += ssrWithHyperBinaryResolution(m_occurrenceMap, delMarker, m_propagation,
                                                       m_assignmentProvider, tempStamps,
                                                       CNFLit{i, CNFSign::POSITIVE});
            } catch (FailedLiteralException<Clause> &e) {
                // TODO: failed literal handling
                m_assignmentProvider.revisitDecisionLevel(e.getDecisionLevelToRevisit());
            }
        }


        m_lastSeenAmntUnaries = unaryClauses.size();
    }

    JAM_ASSERT(m_assignmentProvider.getCurrentDecisionLevel() == currentDecisionLevel,
               "Illegal decision level modification");
    return result;
}

template <typename PropagationT, typename AssignmentProviderT>
void LightweightSimplifier<PropagationT, AssignmentProviderT>::increaseMaxVarTo(CNFVar newMaxVar) {
    JAM_ASSERT(isRegular(newMaxVar), "Argument newMaxVar must be a regular variable.");
    JAM_ASSERT(newMaxVar >= m_maxVar,
               "Argument newMaxVar must not be smaller than the current maximum variable");
    m_maxVar = newMaxVar;
    m_occurrenceMap.increaseMaxElementTo(getMaxLit(newMaxVar));
}
}
