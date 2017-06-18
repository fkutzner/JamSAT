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
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/VariableState.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
class FakeAssignmentProvider {
public:
  FakeAssignmentProvider(VariableState::TruthValue assignment)
      : m_assignment(assignment) {}

  VariableState::TruthValue getAssignment(CNFVar variable) const noexcept {
    (void)variable;
    return m_assignment;
  }

private:
  VariableState::TruthValue m_assignment;
};

TEST(UnitBranching, VSIDSBranchingHeuristic_allAssignedCausesUndefToBePicked) {
  CNFVar maxVar{10};
  FakeAssignmentProvider fakeAssignmentProvider{
      VariableState::TruthValue::TRUE};
  VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{
      maxVar, fakeAssignmentProvider};
  EXPECT_EQ(underTest.pickBranchLiteral(), CNFLit::undefinedLiteral);
}

TEST(UnitBranching, VSIDSBranchingHeuristic_singleVariableGetsPicked) {
  CNFVar maxVar{0};
  FakeAssignmentProvider fakeAssignmentProvider{
      VariableState::TruthValue::INDETERMINATE};
  VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{
      maxVar, fakeAssignmentProvider};

  underTest.setEligibleForDecisions(CNFVar{0}, true);

  CNFLit result = underTest.pickBranchLiteral();
  EXPECT_EQ(result.getVariable(), CNFVar{0});
}

TEST(UnitBranching,
     VSIDSBranchingHeuristic_usingVariablesInConflictCausesReordering) {
  CNFVar maxVar{10};
  FakeAssignmentProvider fakeAssignmentProvider{
      VariableState::TruthValue::INDETERMINATE};
  VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{
      maxVar, fakeAssignmentProvider};

  for (CNFVar::RawVariableType i = 0; i <= 10; ++i) {
    underTest.setEligibleForDecisions(CNFVar{i}, true);
  }

  underTest.seenInConflict(CNFVar{4});

  CNFLit result = underTest.pickBranchLiteral();
  EXPECT_NE(result, CNFLit::undefinedLiteral);
  EXPECT_EQ(result.getVariable(), CNFVar{4});
}
}