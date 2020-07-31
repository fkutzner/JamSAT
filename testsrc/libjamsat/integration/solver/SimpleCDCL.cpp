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

#include <sstream>

#include <libjamsat/branching/VSIDSBranchingHeuristic.h>
#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/simplification/ClauseMinimization.h>
#include <libjamsat/solver/Assignment.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/Truth.h>

#include <toolbox/cnfgenerators/GateStructure.h>
#include <toolbox/cnfgenerators/Rule110.h>

#include "HeapClauseDB.h"

#if defined(JAM_ENABLE_CDCLITEST_LOGGING)
#define JAM_LOG_CDCLITEST(x, y) JAM_LOG(x, "cdclitest", y)
#else
#define JAM_LOG_CDCLITEST(x, y)
#endif

/*
 * This file contains a simple implementation of a CDCL SAT solver, serving as an integration test
 * for the solver's classes, and to drive the implementation. Testing is done on the level of
 * checking whether correct sat/unsat answers can be obtained using the tested subsystems.
 */

namespace jamsat {
namespace {
class SimpleCDCL {
public:
    SimpleCDCL();
    void addClause(const CNFClause& clause);
    TBool isProblemSatisfiable();

private:
    using ConflictAnalysisType = FirstUIPLearning<Assignment, Assignment>;
    using ClauseDBType = HeapClauseDB<Clause>;
    using BranchingHeuristicType = VSIDSBranchingHeuristic<Assignment>;

    enum class UnitPropagationResult { CONFLICTING, CONSISTENT };
    UnitPropagationResult propagateUnitClauses();

    void backtrackToLevel(Assignment::level level);
    void backtrackAll();
    void resetBranchingStates(Assignment::level minLevel);

    CNFVar m_maxVar;
    Assignment m_assignment;
    ConflictAnalysisType m_conflictAnalyzer;
    ClauseDBType m_clauseDB;
    BranchingHeuristicType m_branchingHeuristic;

    std::vector<CNFLit> m_unitClauses;
};

// TODO: resizable data structures
SimpleCDCL::SimpleCDCL()
  : m_maxVar(CNFVar{1})
  , m_assignment(m_maxVar)
  , m_conflictAnalyzer(m_maxVar, m_assignment, m_assignment)
  , m_clauseDB()
  , m_branchingHeuristic(m_maxVar, m_assignment) {}

void SimpleCDCL::addClause(const CNFClause& clause) {
    auto oldMaxVar = m_maxVar;
    for (auto lit : clause) {
        m_maxVar = std::max(m_maxVar, lit.getVariable());
    }
    if (m_maxVar > oldMaxVar) {
        JAM_LOG_CDCLITEST(info,
                          "Increasing max. variable from " << oldMaxVar << " to " << m_maxVar);
        m_assignment.increaseMaxVar(m_maxVar);
        m_conflictAnalyzer.increaseMaxVarTo(m_maxVar);
        m_branchingHeuristic.increaseMaxVarTo(m_maxVar);
    }

    JAM_ASSERT(clause.size() > 0, "Can't add empty clauses for solving");
    if (clause.size() > 1) {
        auto& newClause = m_clauseDB.insertClause(clause);
        JAM_LOG_CDCLITEST(info, "Added clause " << &clause << " " << clause);
        m_assignment.registerClause(newClause);
    } else if (clause.size() == 1) {
        m_unitClauses.push_back(clause[0]);
        JAM_LOG_CDCLITEST(info, "Added unit clause " << clause[0]);
    }
}

SimpleCDCL::UnitPropagationResult SimpleCDCL::propagateUnitClauses() {
    JAM_LOG_CDCLITEST(info, "Propagating unit clauses...");
    for (auto unitClauseLit : m_unitClauses) {
        if (isDeterminate(m_assignment.getAssignment(unitClauseLit))) {
            auto assignment = m_assignment.getAssignment(unitClauseLit);
            auto litIsPositive = toTBool(unitClauseLit.getSign() == CNFSign::POSITIVE);
            if (assignment == litIsPositive) {
                continue;
            } else {
                return UnitPropagationResult::CONFLICTING;
            }
        }

        m_branchingHeuristic.setEligibleForDecisions(unitClauseLit.getVariable(), false);
        auto conflictingClause = m_assignment.append(unitClauseLit);
        if (conflictingClause != nullptr) {
            JAM_LOG_CDCLITEST(info, "Detected a conflict within the unit clauses.");
            return UnitPropagationResult::CONFLICTING;
        }
    }
    return UnitPropagationResult::CONSISTENT;
}

void SimpleCDCL::resetBranchingStates(Assignment::level minLevel) {
    for (auto currentDL = m_assignment.getCurrentLevel(); currentDL >= minLevel; --currentDL) {
        for (auto lit : m_assignment.getLevelAssignments(currentDL)) {
            JAM_LOG_CDCLITEST(info, "  Undoing assignment: " << lit);
            m_branchingHeuristic.reset(lit.getVariable());
        }
        if (currentDL == 0) {
            break;
        }
    }
}

void SimpleCDCL::backtrackToLevel(Assignment::level level) {
    JAM_LOG_CDCLITEST(info, "Backtracking, revisiting level " << level);
    resetBranchingStates(level + 1);
    m_assignment.undoToLevel(level);
}

void SimpleCDCL::backtrackAll() {
    JAM_LOG_CDCLITEST(info, "Backtracking all");
    resetBranchingStates(0);
    m_assignment.undoAll();
}

TBool SimpleCDCL::isProblemSatisfiable() {
    for (CNFVar v = CNFVar{0}; v <= m_maxVar; v = nextCNFVar(v)) {
        m_branchingHeuristic.setEligibleForDecisions(v, true);
    }

    // Set up VSIDS-style variable activity bumping:
    m_conflictAnalyzer.setOnSeenVariableCallback(
        [this](CNFVar seenVar) { m_branchingHeuristic.seenInConflict(seenVar); });

    // Leave the solver with an empty trail:
    OnExitScope backtrackToLevel0{[this]() { this->backtrackAll(); }};

    while (!m_assignment.isComplete()) {
        JAM_LOG_CDCLITEST(info, "Performing a restart.");
        JAM_ASSERT(m_assignment.getCurrentLevel() == 0ull, "Illegal restart: not on DL 0");
        if (propagateUnitClauses() != UnitPropagationResult::CONSISTENT) {
            return toTBool(false);
        }

        bool doRestart = false;
        // Breaking out of this inner loop causes a restart.
        while (!m_assignment.isComplete() && !doRestart) {
            doRestart = false;
            m_assignment.newLevel();
            auto branchingLit = m_branchingHeuristic.pickBranchLiteral();
            JAM_LOG_CDCLITEST(info, "Decided branching variable: " << branchingLit);

            JAM_ASSERT(branchingLit != CNFLit::getUndefinedLiteral(),
                       "branching should always return a defined literal");
            auto conflictingClause = m_assignment.append(branchingLit);

            while (conflictingClause != nullptr) {
                JAM_LOG_CDCLITEST(info, "Handling a conflict...");
                m_branchingHeuristic.beginHandlingConflict();
                OnExitScope notifyEndOfConflictHandling{
                    [this]() { this->m_branchingHeuristic.endHandlingConflict(); }};

                std::vector<CNFLit> learntClause;
                m_conflictAnalyzer.computeConflictClause(*conflictingClause, learntClause);

                // Put a literal with second-highest decision level next to the asserting literal
                // (will backtrack there later)
                auto backtrackLevel = m_assignment.getLevel(learntClause[1].getVariable());
                auto litWithMaxDecisionLevel = learntClause.begin() + 1;
                for (auto lit = learntClause.begin() + 1; lit != learntClause.end(); ++lit) {
                    auto currentBacktrackLevel =
                        std::max(backtrackLevel, m_assignment.getLevel(lit->getVariable()));
                    if (currentBacktrackLevel > backtrackLevel) {
                        litWithMaxDecisionLevel = lit;
                        backtrackLevel = currentBacktrackLevel;
                    }
                }
                std::swap(*litWithMaxDecisionLevel, learntClause[1]);

                // Learning clauses until the solver learns a contradiction on the unit clause
                // level (or finds a satisfying variable assignment)
                if (learntClause.size() == 1 ||
                    m_assignment.getLevel(learntClause[1].getVariable()) == 0) {
                    JAM_LOG_CDCLITEST(info, "Learnt a unit clause: " << learntClause[0]);
                    m_unitClauses.push_back(learntClause[0]);
                    backtrackToLevel(0);
                    // Restarting, since unit clauses need to be put on the first decision
                    // level.
                    conflictingClause = nullptr;
                    doRestart = true;
                } else {
                    auto& newClause = m_clauseDB.insertClause(learntClause);
                    JAM_LOG_CDCLITEST(info, "Learnt a clause: " << newClause);
                    auto targetLevel = m_assignment.getLevel(learntClause[1].getVariable());
                    backtrackToLevel(targetLevel);
                    conflictingClause = m_assignment.registerLemma(newClause);
                    // If conflictingClause != nullptr, another round of conflict resolution is
                    // performed
                }
            }
        }
    }
    // All variables assigned without conflict => the CNFProblem is satisfiable.
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

    SimpleCDCL underTest;
    for (auto& clause : testData.getClauses()) {
        underTest.addClause(clause);
    }

    EXPECT_EQ(underTest.isProblemSatisfiable(), TBools::FALSE);
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

    SimpleCDCL underTest;
    for (auto& clause : testData.getClauses()) {
        underTest.addClause(clause);
    }

    EXPECT_EQ(underTest.isProblemSatisfiable(), TBools::FALSE);
}

TEST(IntegrationSolver, SimpleCDCL_complexUnsatisfiableFormula) {
    CNFProblem testData;
    std::vector<CNFLit> lines;
    for (CNFVar i{0}; i < CNFVar{16}; i = nextCNFVar(i)) {
        lines.push_back(CNFLit{i, CNFSign::POSITIVE});
    }

    insertXOR({lines[0], lines[1], lines[2], lines[9]}, lines[15], testData);
    insertXOR({lines[0], lines[1], lines[2], lines[9]}, ~lines[15], testData);
    insertXOR({lines[3], lines[4]}, lines[0], testData);
    insertXOR({lines[5], lines[6]}, lines[1], testData);
    insertXOR({lines[7], lines[8]}, lines[2], testData);

    CNFClause unit{15_Lit};
    testData.addClause(std::move(unit));

    SimpleCDCL underTest;
    for (auto& clause : testData.getClauses()) {
        underTest.addClause(clause);
    }

    EXPECT_EQ(underTest.isProblemSatisfiable(), TBools::FALSE);
}

TEST(IntegrationSolver, SimpleCDCL_rule110_reachable) {
    Rule110PredecessorStateProblem problem{"xx1xx", "x1xxx", 7};
    auto rule110Encoding = problem.getCNFEncoding();

    SimpleCDCL underTest;
    for (auto& clause : rule110Encoding.cnfProblem.getClauses()) {
        underTest.addClause(clause);
    }
    EXPECT_EQ(underTest.isProblemSatisfiable(), TBools::TRUE);
}
}
