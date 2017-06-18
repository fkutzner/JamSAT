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
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/Trail.h>

namespace jamsat {
TEST(UnitSolver, emptyTrailHasDecisionLevel0) {
  Trail underTest{CNFVar{10}};
  EXPECT_EQ(underTest.getCurrentDecisionLevel(), 0ull);
}

TEST(UnitSolver, firstNewDecisionLevelIs1) {
  Trail underTest{CNFVar{10}};
  underTest.newDecisionLevel();
  EXPECT_EQ(underTest.getCurrentDecisionLevel(), 1ull);
}

TEST(UnitSolver, emptyTrailHasNoAssignments) {
  Trail underTest{CNFVar{10}};
  EXPECT_EQ(underTest.getNumberOfAssignments(), 0ull);
}

TEST(UnitSolver, trailHasSingleAssignmentAfterSingleAdd) {
  Trail underTest{CNFVar{10}};
  underTest.addLiteral(CNFLit{CNFVar{3}, CNFSign::POSITIVE});
  EXPECT_EQ(underTest.getNumberOfAssignments(), 1ull);
}

TEST(UnitSolver, trailHasThreeAssignmentsAfterThreeAdds) {
  Trail underTest{CNFVar{10}};
  underTest.addLiteral(CNFLit{CNFVar{1}, CNFSign::POSITIVE});
  underTest.addLiteral(CNFLit{CNFVar{2}, CNFSign::NEGATIVE});
  underTest.addLiteral(CNFLit{CNFVar{3}, CNFSign::NEGATIVE});
  EXPECT_EQ(underTest.getNumberOfAssignments(), 3ull);
}

TEST(UnitSolver, initialAddedLiteralsAreOnLevel0) {
  Trail underTest{CNFVar{10}};
  underTest.addLiteral(CNFLit{CNFVar{1}, CNFSign::POSITIVE});
  underTest.addLiteral(CNFLit{CNFVar{2}, CNFSign::NEGATIVE});
  underTest.addLiteral(CNFLit{CNFVar{3}, CNFSign::NEGATIVE});
  EXPECT_EQ(underTest.getCurrentDecisionLevel(), 0ull);
}

TEST(UnitSolver, trailSeparatesLiteralsByDecisionLevels) {
  CNFLit testLiteral1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit testLiteral3{CNFVar{3}, CNFSign::NEGATIVE};

  Trail underTest{CNFVar{10}};
  underTest.addLiteral(testLiteral1);

  underTest.newDecisionLevel();
  underTest.addLiteral(testLiteral2);
  underTest.addLiteral(testLiteral3);

  underTest.newDecisionLevel();

  ASSERT_EQ(underTest.getCurrentDecisionLevel(), 2ull);

  auto level0Iterator = underTest.getDecisionLevelLiterals(0);
  EXPECT_EQ(level0Iterator.second - level0Iterator.first, 1);
  EXPECT_EQ(*(level0Iterator.first), testLiteral1);

  auto level1Iterator = underTest.getDecisionLevelLiterals(1);
  EXPECT_EQ(level1Iterator.second - level1Iterator.first, 2);
  EXPECT_EQ(*(level1Iterator.first), testLiteral2);
  EXPECT_EQ(*(level1Iterator.first + 1), testLiteral3);

  auto level2Iterator = underTest.getDecisionLevelLiterals(3);
  EXPECT_EQ(level2Iterator.second, level2Iterator.first);
}

TEST(UnitSolver, trailIsEmptyAfterShrinkToLevel0) {
  CNFLit testLiteral1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit testLiteral3{CNFVar{3}, CNFSign::NEGATIVE};

  Trail underTest{CNFVar{10}};
  underTest.addLiteral(testLiteral1);
  underTest.newDecisionLevel();
  underTest.addLiteral(testLiteral2);
  underTest.addLiteral(testLiteral3);
  underTest.newDecisionLevel();

  underTest.shrinkToDecisionLevel(0);
  EXPECT_EQ(underTest.getCurrentDecisionLevel(), 0ull);
  EXPECT_EQ(underTest.getNumberOfAssignments(), 0ull);
}

TEST(UnitSolver, emptyTrailHasIndeterminateAssignment) {
  Trail underTest{CNFVar{10}};
  for (CNFVar::RawVariableType i = 0; i <= 10; ++i) {
    EXPECT_EQ(underTest.getAssignment(CNFVar{i}), TBool::INDETERMINATE);
  }
}

TEST(UnitSolver, variablesOnTrailHaveAssignment) {
  Trail underTest{CNFVar{10}};
  underTest.addLiteral(CNFLit{CNFVar{4}, CNFSign::NEGATIVE});
  EXPECT_EQ(underTest.getAssignment(CNFVar{4}), TBool::FALSE);
  for (CNFVar::RawVariableType i = 0; i <= 10; ++i) {
    if (i != 4) {
      EXPECT_EQ(underTest.getAssignment(CNFVar{i}), TBool::INDETERMINATE);
    }
  }
}

TEST(UnitSolver, variablesOnTrailHaveCorrectDecisionLevel) {
  Trail underTest{CNFVar{10}};
  underTest.addLiteral(CNFLit{CNFVar{4}, CNFSign::NEGATIVE});
  underTest.newDecisionLevel();
  underTest.addLiteral(CNFLit{CNFVar{5}, CNFSign::POSITIVE});
  underTest.addLiteral(CNFLit{CNFVar{6}, CNFSign::POSITIVE});
  underTest.newDecisionLevel();
  underTest.addLiteral(CNFLit{CNFVar{7}, CNFSign::POSITIVE});

  EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{4}), 0ul);
  EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{5}), 1ul);
  EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{6}), 1ul);
  EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{7}), 2ul);
}

TEST(UnitSolver, assignmentsBecomeIndeterminateOnShrink) {
  Trail underTest{CNFVar{10}};
  underTest.addLiteral(CNFLit{CNFVar{4}, CNFSign::NEGATIVE});
  underTest.newDecisionLevel();
  underTest.addLiteral(CNFLit{CNFVar{5}, CNFSign::POSITIVE});
  underTest.addLiteral(CNFLit{CNFVar{6}, CNFSign::POSITIVE});
  underTest.newDecisionLevel();
  underTest.addLiteral(CNFLit{CNFVar{7}, CNFSign::POSITIVE});

  underTest.shrinkToDecisionLevel(1);
  EXPECT_EQ(underTest.getAssignment(CNFVar{4}), TBool::FALSE);
  EXPECT_EQ(underTest.getAssignment(CNFVar{5}), TBool::INDETERMINATE);
  EXPECT_EQ(underTest.getAssignment(CNFVar{6}), TBool::INDETERMINATE);
  EXPECT_EQ(underTest.getAssignment(CNFVar{7}), TBool::INDETERMINATE);
}

TEST(UnitSolver, unshrinkedDecisionLevelsRemainIntactAfterShrink) {
  Trail underTest{CNFVar{10}};
  underTest.addLiteral(CNFLit{CNFVar{4}, CNFSign::NEGATIVE});
  underTest.newDecisionLevel();
  underTest.addLiteral(CNFLit{CNFVar{5}, CNFSign::POSITIVE});
  underTest.addLiteral(CNFLit{CNFVar{6}, CNFSign::POSITIVE});
  underTest.newDecisionLevel();
  underTest.addLiteral(CNFLit{CNFVar{7}, CNFSign::POSITIVE});

  underTest.shrinkToDecisionLevel(2);
  EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{4}), 0ul);
  EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{5}), 1ul);
  EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{6}), 1ul);
}
}
