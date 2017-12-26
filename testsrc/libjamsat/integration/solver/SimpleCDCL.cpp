/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)

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

#include <gtest/gtest.h>

#include <libjamsat/branching/VSIDSBranchingHeuristic.h>
#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/HeapClauseDB.h>
#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/solver/ClauseMinimization.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/Trail.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
namespace {
class SimpleCDCL {
public:
    SimpleCDCL(CNFVar maxVar);
    void addClause(const CNFClause &clause);
    TBool solve();

private:
    using TrailType = Trail;
    using PropagationType = Propagation<Trail, Clause>;
    using ConflictAnalysisType = FirstUIPLearning<TrailType, PropagationType, Clause>;
    using ClauseDBType = HeapClauseDB<Clause>;
    using BranchingHeuristicType = VSIDSBranchingHeuristic<TrailType>;

    bool propagateUnitClauses();
    void backtrackToLevel(TrailType::DecisionLevel level);

    CNFVar m_maxVar;
    TrailType m_trail;
    PropagationType m_propagation;
    ConflictAnalysisType m_conflictAnalyzer;
    ClauseDBType m_clauseDB;
    BranchingHeuristicType m_branchingHeuristic;

    std::vector<CNFLit> m_unitClauses;
};

// TODO: resizable data structures
SimpleCDCL::SimpleCDCL(CNFVar maxVar)
  : m_maxVar(maxVar)
  , m_trail(maxVar)
  , m_propagation(maxVar, m_trail)
  , m_conflictAnalyzer(maxVar, m_trail, m_propagation)
  , m_clauseDB()
  , m_branchingHeuristic(maxVar, m_trail) {}

void SimpleCDCL::addClause(const CNFClause &clause) {
    JAM_ASSERT(clause.size() > 0, "Can't add empty clauses for solving");
    if (clause.size() > 1) {
        auto &newClause = m_clauseDB.insertClause(clause);
        std::cout << "Added clause " << &newClause << ": ";
        for (auto lit : newClause) {
            std::cout << " " << lit;
        }
        std::cout << std::endl;
        m_propagation.registerClause(newClause);
    } else if (clause.size() == 1) {
        m_unitClauses.push_back(clause[0]);
    }
}

// TODO: Sign <-> TBool
bool SimpleCDCL::propagateUnitClauses() {
    for (auto unitClauseLit : m_unitClauses) {
        std::cout << "Tack" << std::endl;
        if (m_trail.getAssignment(unitClauseLit) != TBool::INDETERMINATE) {
            auto assignment = m_trail.getAssignment(unitClauseLit);
            auto litIsPositive = toTBool(unitClauseLit.getSign() == CNFSign::POSITIVE);
            if (assignment == litIsPositive) {
                continue;
            } else {
                return true;
            }
        }

        m_branchingHeuristic.setEligibleForDecisions(unitClauseLit.getVariable(), false);
        m_trail.addAssignment(unitClauseLit);
        auto conflictingClause = m_propagation.propagateUntilFixpoint(unitClauseLit);
        std::cout << "No. of assignments after unit propagate of " << unitClauseLit << ": "
                  << m_trail.getNumberOfAssignments() << std::endl;

        if (conflictingClause != nullptr) {
            return true;
        }
    }
    return false;
}

void SimpleCDCL::backtrackToLevel(TrailType::DecisionLevel level) {
    for (auto currentDL = m_trail.getCurrentDecisionLevel(); currentDL >= level; --currentDL) {
        for (auto lit : m_trail.getDecisionLevelAssignments(currentDL)) {
            std::cout << "Undoing assignment: " << lit << std::endl;
            m_branchingHeuristic.reset(lit.getVariable());
        }
        if (currentDL == 0) {
            break;
        }
    }
    m_trail.shrinkToDecisionLevel(level);
}

TBool SimpleCDCL::solve() {
    // TODO: easy variable increasing (or make range)
    for (CNFVar v = CNFVar{0}; v <= m_maxVar; v = CNFVar{v.getRawValue() + 1}) {
        m_branchingHeuristic.setEligibleForDecisions(v, true);
    }

    while (m_trail.getNumberOfAssignments() <= (m_maxVar.getRawValue())) {
        if (propagateUnitClauses()) {
            return toTBool(false);
        }
        m_trail.newDecisionLevel();

        // TODO: isAllAssigned in trail
        while (m_trail.getNumberOfAssignments() <= (m_maxVar.getRawValue())) {
            auto branchingLit = m_branchingHeuristic.pickBranchLiteral();
            std::cout << "Picking a branching variable: " << branchingLit << std::endl;

            JAM_ASSERT(branchingLit != CNFLit::getUndefinedLiteral(),
                       "branching should always return a defined literal");
            m_trail.addAssignment(branchingLit);

            auto conflictingClause = m_propagation.propagateUntilFixpoint(branchingLit);

            bool doRestart = false;
            while (conflictingClause != nullptr) {
                std::cout << "Handling a conflict..." << std::endl;
                m_branchingHeuristic.beginHandlingConflict();
                auto learntClause = m_conflictAnalyzer.computeConflictClause(*conflictingClause);
                if (learntClause.size() == 0) {

                    return toTBool(false);
                }

                if (learntClause.size() == 1) {
                    std::cout << "Learnt unit clause: " << learntClause[0] << std::endl;
                    m_unitClauses.push_back(learntClause[0]);
                    backtrackToLevel(0);
                    m_branchingHeuristic.endHandlingConflict();
                    // restart:
                    doRestart = true;
                    break;
                } else {
                    auto &newClause = m_clauseDB.insertClause(learntClause);
                    std::cout << "Learnt clause: " << newClause << std::endl;
                    auto targetLevel =
                        m_trail.getAssignmentDecisionLevel(learntClause[1].getVariable());
                    std::cout << "Backtracking to level:" << targetLevel << std::endl;
                    backtrackToLevel(targetLevel);
                    if (targetLevel == 0) {
                        doRestart = true;
                    }
                    conflictingClause = m_propagation.registerClause(newClause);
                    m_branchingHeuristic.endHandlingConflict();
                }
            }
            if (doRestart) {
                break;
            }
        }
    }
    std::cout << m_trail.getNumberOfAssignments() << std::endl;
    std::cout << m_maxVar.getRawValue() << std::endl;
    return toTBool(true);
}
}

TEST(IntegrationSolver, SimpleCDCL_unsatOnConflictInUnitPropagation) {
    std::stringstream conduit;

    conduit << "p cnf 7 5" << std::endl;
    conduit << "1 2 3 0" << std::endl;
    conduit << "1 -3 -4 0" << std::endl;
    conduit << "1 -4 -2 0" << std::endl;
    conduit << "4 0" << std::endl;
    conduit << "-1 0" << std::endl;

    CNFProblem testData;
    conduit >> testData;

    ASSERT_FALSE(conduit.fail());

    SimpleCDCL underTest{testData.getMaxVar()};
    for (auto &clause : testData.getClauses()) {
        underTest.addClause(clause);
    }

    EXPECT_EQ(underTest.solve(), TBool::FALSE);
}

TEST(IntegrationSolver, SimpleCDCL_smallUnsatisfiableProblem) {
    std::stringstream conduit;

    conduit << "p cnf 7 8" << std::endl;
    conduit << "1 2 3 0" << std::endl;
    conduit << "1 -3 -4 0" << std::endl;
    conduit << "1 -4 -2 0" << std::endl;
    conduit << "4 0" << std::endl;
    conduit << "-1 6 7 0" << std::endl;
    conduit << "-1 -6 -5 0" << std::endl;
    conduit << "-1 -5 -7 0" << std::endl;
    conduit << "5 0" << std::endl;

    CNFProblem testData;
    conduit >> testData;

    ASSERT_FALSE(conduit.fail());

    SimpleCDCL underTest{testData.getMaxVar()};
    for (auto &clause : testData.getClauses()) {
        underTest.addClause(clause);
    }

    EXPECT_EQ(underTest.solve(), TBool::FALSE);
}
}
