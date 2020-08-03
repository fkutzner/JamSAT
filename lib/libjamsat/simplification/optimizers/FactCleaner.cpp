#include "FactCleaner.h"

#include <boost/range/algorithm_ext/erase.hpp>

#include <libjamsat/utils/BoundedMap.h>

namespace jamsat {
namespace {

std::vector<CNFLit> withConsequences(Assignment& assignment, std::vector<CNFLit> const& facts) {
    assignment.undoAll();
    for (CNFLit fact : facts) {
        assignment.append(fact);
    }
    auto factsAndConsequences = assignment.getAssignments();
    return std::vector<CNFLit>{factsAndConsequences.begin(), factsAndConsequences.end()};
}

class FactCleaner {
public:
    auto wantsExecution(uint64_t conflictsSinceInvocation) const noexcept -> bool {
        return conflictsSinceInvocation >= 5000;
    }

    auto optimize(SharedOptimizerState sharedOptimizerState) -> SharedOptimizerState {
        if (sharedOptimizerState.hasDetectedUnsat()) {
            return sharedOptimizerState;
        }

        if (sharedOptimizerState.getFacts().size() == m_unariesAfterLastCall) {
            return sharedOptimizerState;
        }

        SharedOptimizerState::OccMap& occurrences = sharedOptimizerState.getOccurrenceMap();
        Assignment& assignment = sharedOptimizerState.getAssignment();

        std::vector<CNFLit>& facts = sharedOptimizerState.getFacts();
        facts = withConsequences(assignment, facts);

        for (CNFLit fact : facts) {
            for (Clause* toDelete : occurrences[fact]) {
                toDelete->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
                assignment.registerClauseModification(*toDelete);
                occurrences.remove(*toDelete);
            }

            for (Clause* toStrengthen : occurrences[~fact]) {
                if (toStrengthen->size() == 2) {
                    // Assuming that all facts have been propagated without conflict,
                    // the literal that would remain is a fact
                    toStrengthen->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
                    occurrences.remove(*toStrengthen);
                } else {
                    boost::remove_erase(*toStrengthen, ~fact);
                    toStrengthen->setFlag(Clause::Flag::MODIFIED);
                    occurrences.setModified(
                        *toStrengthen, std::array<CNFLit, 0>{}, std::array<CNFLit, 1>{~fact});
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

ProblemOptimizer createFactCleaner() {
    return ProblemOptimizer{FactCleaner{}};
}
}