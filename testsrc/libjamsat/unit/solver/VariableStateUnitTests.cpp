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

#include <libjamsat/solver/VariableState.h>

namespace jamsat {
TEST(UnitSolver, unsettedVariablesAreIndeterminate) {
  VariableState underTest{CNFVar{10}};
  EXPECT_EQ(underTest.getAssignment(CNFVar{4}),
            VariableState::TruthValue::INDETERMINATE);
}

TEST(UnitSolver, variableTruthValuesAreStored) {
  VariableState underTest{CNFVar{10}};
  underTest.setAssignment(CNFVar{8}, VariableState::TruthValue::FALSE);
  EXPECT_EQ(underTest.getAssignment(CNFVar{8}),
            VariableState::TruthValue::FALSE);
}

TEST(UnitSolver, variablesArentEligibleForDecisionByDefault) {
  VariableState underTest{CNFVar{10}};
  EXPECT_EQ(underTest.isEligibleForDecisions(CNFVar{3}), false);
}

TEST(UnitSolver, decisionVariableEligibilityIsStored) {
  VariableState underTest{CNFVar{10}};
  underTest.setEligibleForDecisions(CNFVar{3}, true);
  EXPECT_EQ(underTest.isEligibleForDecisions(CNFVar{3}), true);
}

TEST(UnitSolver, variablesAreNotEliminatedByDefault) {
  VariableState underTest{CNFVar{10}};
  EXPECT_EQ(underTest.isEliminated(CNFVar{3}), false);
}

TEST(UnitSolver, variableEliminationIsStored) {
  VariableState underTest{CNFVar{10}};
  underTest.setEliminated(CNFVar{3});
  EXPECT_EQ(underTest.isEliminated(CNFVar{3}), true);
}

TEST(UnitSolver, variableDecisionLevelsAreStored) {
  VariableState underTest{CNFVar{10}};
  underTest.setAssignmentDecisionLevel(CNFVar{5}, 100ul);
  EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{5}), 100ul);
}

TEST(UnitSolver, variablReasonsAreStored) {
  VariableState underTest{CNFVar{10}};
  Clause *dummy = reinterpret_cast<Clause *>(0xFF);
  underTest.setAssignmentReason(CNFVar{5}, dummy);
  EXPECT_EQ(underTest.getAssignmentReason(CNFVar{5}), dummy);
}
}
