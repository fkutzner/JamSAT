/* Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)

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

#include "FactCleaner.h"

#include <boost/range/algorithm_ext/erase.hpp>

#include <libjamsat/solver/Statistics.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/RangeUtils.h>

#include <iostream>

namespace jamsat {
namespace {

class InconsistentFactsException {};

std::vector<CNFLit> withConsequences(Assignment& assignment, std::vector<CNFLit> const& facts) {
    assignment.undoAll();
    OnExitScope undoAssignments{[&assignment]() { assignment.undoAll(); }};

    for (CNFLit fact : facts) {
        TBool factValue = assignment.getAssignment(fact);
        bool conflicting = false;
        if (!isDeterminate(factValue)) {
            conflicting = (assignment.append(fact) != nullptr);
        }

        if (conflicting || factValue == TBools::FALSE) {
            throw InconsistentFactsException{};
        }
    }
    auto factsAndConsequences = assignment.getAssignments();
    return std::vector<CNFLit>{factsAndConsequences.begin(), factsAndConsequences.end()};
}

void deleteClause(SharedOptimizerState& optState, Clause& clause) {
    optState.getUnsatCertificate().deleteClause(clause.span());
    clause.setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
    optState.getAssignment().registerClauseModification(clause);
    optState.getOccurrenceMap().remove(clause);
    optState.getStats().amntClausesRemoved += 1;
}

class FactCleaner : public ProblemOptimizer {
public:
    auto wantsExecution(StatisticsEra const& currentStats) const noexcept -> bool override {
        return currentStats.m_conflictCount == 0 ||
               currentStats.m_unitLemmas > m_learntFactsAfterLastCall;
    }

    auto optimize(SharedOptimizerState sharedOptimizerState, StatisticsEra const& currentStats)
        -> SharedOptimizerState override {
        if (sharedOptimizerState.hasDetectedUnsat()) {
            return sharedOptimizerState;
        }

        m_learntFactsAfterLastCall = currentStats.m_unitLemmas;

        if (sharedOptimizerState.getFacts().size() == m_factsAfterLastCall) {
            return sharedOptimizerState;
        }

        SharedOptimizerState::OccMap& occurrences = sharedOptimizerState.getOccurrenceMap();
        Assignment& assignment = sharedOptimizerState.getAssignment();
        OptimizationStats& stats = sharedOptimizerState.getStats();
        DRATCertificate& unsatCert = sharedOptimizerState.getUnsatCertificate();

        std::vector<CNFLit>& facts = sharedOptimizerState.getFacts();
        std::size_t const amntFactsBeforePropagation = facts.size();
        try {
            facts = withConsequences(assignment, facts);
        } catch (InconsistentFactsException const&) {
            sharedOptimizerState.setDetectedUnsat();
            return sharedOptimizerState;
        }
        std::size_t const additionalFacts = facts.size() - amntFactsBeforePropagation;
        stats.amntFactsDerived += additionalFacts;
        m_learntFactsAfterLastCall += additionalFacts;

        m_factsAfterLastCall = sharedOptimizerState.getFacts().size();

        // Deleting clauses in a separate loop to avoid strengthening
        // operations later on
        for (CNFLit fact : facts) {
            for (Clause* toDelete : occurrences[fact]) {
                deleteClause(sharedOptimizerState, *toDelete);
            }
        }

        for (CNFLit fact : facts) {
            for (Clause* toStrengthen : occurrences[~fact]) {
                if (toStrengthen->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION)) {
                    continue;
                }

                if (toStrengthen->size() == 2) {
                    // Assuming that all facts have been propagated without conflict,
                    // the literal that would remain is a fact
                    deleteClause(sharedOptimizerState, *toStrengthen);
                } else {
                    swapWithLastElement(*toStrengthen, ~fact);
                    unsatCert.addATClause(
                        toStrengthen->span().subspan(0, toStrengthen->size() - 1));
                    unsatCert.deleteClause(toStrengthen->span());
                    toStrengthen->resize(toStrengthen->size() - 1);

                    toStrengthen->setFlag(Clause::Flag::MODIFIED);
                    toStrengthen->clauseUpdated();
                    occurrences.setModified(
                        *toStrengthen, std::array<CNFLit, 0>{}, std::array<CNFLit, 1>{~fact});
                    assignment.registerClauseModification(*toStrengthen);
                    stats.amntLitsRemoved += 1;
                }
            }
        }
        return sharedOptimizerState;
    }

private:
    uint64_t m_learntFactsAfterLastCall = 0;
    uint64_t m_factsAfterLastCall = 0;
};
}

std::unique_ptr<ProblemOptimizer> createFactCleaner() {
    return std::make_unique<FactCleaner>();
}
}