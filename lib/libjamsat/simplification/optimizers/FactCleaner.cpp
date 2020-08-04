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

#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/ControlFlow.h>


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

        if (conflicting || factValue != toTBool(fact.getSign() == CNFSign::POSITIVE)) {
            throw InconsistentFactsException{};
        }
    }
    auto factsAndConsequences = assignment.getAssignments();
    return std::vector<CNFLit>{factsAndConsequences.begin(), factsAndConsequences.end()};
}

class FactCleaner : public ProblemOptimizer {
public:
    auto wantsExecution(uint64_t conflictsSinceInvocation) const noexcept -> bool override {
        return conflictsSinceInvocation >= 5000;
    }

    auto optimize(SharedOptimizerState sharedOptimizerState) -> SharedOptimizerState override {
        if (sharedOptimizerState.hasDetectedUnsat()) {
            return sharedOptimizerState;
        }

        if (sharedOptimizerState.getFacts().size() == m_unariesAfterLastCall) {
            return sharedOptimizerState;
        }

        SharedOptimizerState::OccMap& occurrences = sharedOptimizerState.getOccurrenceMap();
        Assignment& assignment = sharedOptimizerState.getAssignment();
        OptimizationStats& stats = sharedOptimizerState.getStats();

        std::vector<CNFLit>& facts = sharedOptimizerState.getFacts();
        try {
            facts = withConsequences(assignment, facts);
        } catch (InconsistentFactsException const&) {
            sharedOptimizerState.setDetectedUnsat();
            return sharedOptimizerState;
        }

        for (CNFLit fact : facts) {
            for (Clause* toDelete : occurrences[fact]) {
                toDelete->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
                assignment.registerClauseModification(*toDelete);
                occurrences.remove(*toDelete);
                stats.amntClausesRemoved += 1;
            }

            for (Clause* toStrengthen : occurrences[~fact]) {
                if (toStrengthen->size() == 2) {
                    // Assuming that all facts have been propagated without conflict,
                    // the literal that would remain is a fact
                    toStrengthen->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
                    occurrences.remove(*toStrengthen);
                    stats.amntClausesRemoved += 1;
                } else {
                    boost::remove_erase(*toStrengthen, ~fact);
                    toStrengthen->setFlag(Clause::Flag::MODIFIED);
                    occurrences.setModified(
                        *toStrengthen, std::array<CNFLit, 0>{}, std::array<CNFLit, 1>{~fact});
                    stats.amntLitsRemoved += 1;
                }

                assignment.registerClauseModification(*toStrengthen);
            }
        }

        m_unariesAfterLastCall = sharedOptimizerState.getFacts().size();
        return sharedOptimizerState;
    }

private:
    uint64_t m_unariesAfterLastCall = 0;
};
}

std::unique_ptr<ProblemOptimizer> createFactCleaner() {
    return std::make_unique<FactCleaner>();
}
}