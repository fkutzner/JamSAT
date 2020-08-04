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

std::unique_ptr<ProblemOptimizer> createFactCleaner() {
    return std::make_unique<FactCleaner>();
}
}