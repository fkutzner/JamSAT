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
#include <libjamsat/concepts/SolverTypeTraits.h>
#include <libjamsat/solver/Trail.h>

#include <vector>

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;

static_assert(is_decision_level_provider<Trail<TrivialClause>>::value,
              "Trail<TrivialClause> must satisfy the DecisionLevelProvider concept, but does not");

TEST(UnitSolver, emptyTrailHasDecisionLevel0) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    EXPECT_EQ(underTest.getCurrentDecisionLevel(), 0ull);
}

TEST(UnitSolver, firstNewDecisionLevelIs1) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.newDecisionLevel();
    EXPECT_EQ(underTest.getCurrentDecisionLevel(), 1ull);
}

TEST(UnitSolver, emptyTrailHasNoAssignments) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    EXPECT_EQ(underTest.getNumberOfAssignments(), 0ull);
}

TEST(UnitSolver, trailHasSingleAssignmentAfterSingleAdd) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(3_Lit);
    EXPECT_EQ(underTest.getNumberOfAssignments(), 1ull);
}

TEST(UnitSolver, trailHasThreeAssignmentsAfterThreeAdds) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(1_Lit);
    underTest.addAssignment(~2_Lit);
    underTest.addAssignment(~3_Lit);
    EXPECT_EQ(underTest.getNumberOfAssignments(), 3ull);
}

TEST(UnitSolver, initialAddedLiteralsAreOnLevel0) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(1_Lit);
    underTest.addAssignment(~2_Lit);
    underTest.addAssignment(~3_Lit);
    EXPECT_EQ(underTest.getCurrentDecisionLevel(), 0ull);
}

TEST(UnitSolver, trailSeparatesLiteralsByDecisionLevels) {
    CNFLit testLiteral1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit testLiteral3{CNFVar{3}, CNFSign::NEGATIVE};

    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(testLiteral1);

    underTest.newDecisionLevel();
    underTest.addAssignment(testLiteral2);
    underTest.addAssignment(testLiteral3);

    ASSERT_EQ(underTest.getCurrentDecisionLevel(), 1ull);

    auto level0Iterator = underTest.getDecisionLevelAssignments(0);
    EXPECT_EQ(level0Iterator.end() - level0Iterator.begin(), 1);
    EXPECT_EQ(*(level0Iterator.begin()), testLiteral1);

    auto level1Iterator = underTest.getDecisionLevelAssignments(1);
    EXPECT_EQ(level1Iterator.end() - level1Iterator.begin(), 2);
    EXPECT_EQ(*(level1Iterator.begin()), testLiteral2);
    EXPECT_EQ(*(level1Iterator.begin() + 1), testLiteral3);

    underTest.newDecisionLevel();
    ASSERT_EQ(underTest.getCurrentDecisionLevel(), 2ull);

    auto level2Iterator = underTest.getDecisionLevelAssignments(2);
    EXPECT_EQ(level2Iterator.end(), level2Iterator.begin());

    auto level3Iterator = underTest.getDecisionLevelAssignments(3);
    EXPECT_EQ(level3Iterator.end(), level2Iterator.begin());
}

TEST(UnitSolver, trailIsEmptyAfterShrinkToLevel0) {
    CNFLit testLiteral1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit testLiteral3{CNFVar{3}, CNFSign::NEGATIVE};

    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(testLiteral1);
    underTest.newDecisionLevel();
    underTest.addAssignment(testLiteral2);
    underTest.addAssignment(testLiteral3);
    underTest.newDecisionLevel();

    underTest.shrinkToDecisionLevel(0);
    EXPECT_EQ(underTest.getCurrentDecisionLevel(), 0ull);
    EXPECT_EQ(underTest.getNumberOfAssignments(), 0ull);
}

TEST(UnitSolver, trailDecisionLevelIteratorsRemainValidAfterAdd) {
    Trail<TrivialClause> underTest{CNFVar{16384}};
    for (CNFVar::RawVariable v = 0; v < 10; ++v) {
        underTest.addAssignment(CNFLit{CNFVar{v}, CNFSign::NEGATIVE});
    }

    underTest.newDecisionLevel();
    auto dl0IteratorsPre = underTest.getDecisionLevelAssignments(0);

    for (CNFVar::RawVariable v = 11; v < 16384; ++v) {
        underTest.addAssignment(CNFLit{CNFVar{v}, CNFSign::NEGATIVE});
    }

    auto dl0IteratorsPost = underTest.getDecisionLevelAssignments(0);

    EXPECT_EQ(dl0IteratorsPre.begin(), dl0IteratorsPost.begin());
    EXPECT_EQ(dl0IteratorsPre.end(), dl0IteratorsPost.end());
}

TEST(UnitSolver, emptyTrailHasIndeterminateAssignment) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
        EXPECT_EQ(underTest.getAssignment(CNFVar{i}), TBools::INDETERMINATE);
        CNFLit iLit = CNFLit{CNFVar{i}, CNFSign::POSITIVE};
        EXPECT_EQ(underTest.getAssignment(iLit), TBools::INDETERMINATE);
        EXPECT_EQ(underTest.getAssignment(~iLit), TBools::INDETERMINATE);
    }
}

TEST(UnitSolver, variablesOnTrailHaveAssignment) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(~4_Lit);
    EXPECT_EQ(underTest.getAssignment(CNFVar{4}), TBools::FALSE);
    EXPECT_EQ(underTest.getAssignment(4_Lit), TBools::FALSE);
    EXPECT_EQ(underTest.getAssignment(~4_Lit), TBools::TRUE);

    for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
        if (i != 4) {
            EXPECT_EQ(underTest.getAssignment(CNFVar{i}), TBools::INDETERMINATE);
        }
    }
}

TEST(UnitSolver, variablesOnTrailHaveNullReasonsByDefault) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(~4_Lit);
    EXPECT_EQ(underTest.getAssignmentReason(CNFVar{4}), nullptr);
}

TEST(UnitSolver, variablesOnTrailHaveCorrectReasonClauses) {
    TrivialClause cl1, cl2;
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(~4_Lit, cl1);
    underTest.addAssignment(~5_Lit);
    underTest.addAssignment(~6_Lit, cl2);
    EXPECT_EQ(underTest.getAssignmentReason(CNFVar{4}), &cl1);
    EXPECT_EQ(underTest.getAssignmentReason(CNFVar{5}), nullptr);
    EXPECT_EQ(underTest.getAssignmentReason(CNFVar{6}), &cl2);
}

TEST(UnitSolver, variablesOnTrailHaveCorrectDecisionLevel) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(~4_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(5_Lit);
    underTest.addAssignment(6_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(7_Lit);

    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{4}), 0ul);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{5}), 1ul);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{6}), 1ul);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{7}), 2ul);
}

TEST(UnitSolver, assignmentsBecomeIndeterminateOnShrink) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(~4_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(5_Lit);
    underTest.addAssignment(6_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(7_Lit);

    underTest.shrinkToDecisionLevel(1);
    EXPECT_EQ(underTest.getAssignment(CNFVar{4}), TBools::FALSE);
    EXPECT_EQ(underTest.getAssignment(CNFVar{5}), TBools::INDETERMINATE);
    EXPECT_EQ(underTest.getAssignment(CNFVar{6}), TBools::INDETERMINATE);
    EXPECT_EQ(underTest.getAssignment(CNFVar{7}), TBools::INDETERMINATE);
}

TEST(UnitSolver, unshrinkedDecisionLevelsRemainIntactAfterShrink) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(~4_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(5_Lit);
    underTest.addAssignment(6_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(7_Lit);

    underTest.shrinkToDecisionLevel(2);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{4}), 0ul);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{5}), 1ul);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{6}), 1ul);
}

TEST(UnitSolver, assignmentsBecomeIndeterminateOnRevisit) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(~4_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(5_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(6_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(7_Lit);

    underTest.revisitDecisionLevel(1);
    EXPECT_EQ(underTest.getAssignment(CNFVar{4}), TBools::FALSE);
    EXPECT_EQ(underTest.getAssignment(CNFVar{5}), TBools::TRUE);
    EXPECT_EQ(underTest.getAssignment(CNFVar{6}), TBools::INDETERMINATE);
    EXPECT_EQ(underTest.getAssignment(CNFVar{7}), TBools::INDETERMINATE);
}

TEST(UnitSolver, undiscardedDecisionLevelsRemainIntactAfterRevisit) {
    Trail<TrivialClause> underTest{CNFVar{10}};
    underTest.addAssignment(~4_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(5_Lit);
    underTest.addAssignment(6_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(7_Lit);

    underTest.revisitDecisionLevel(1);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{4}), 0ul);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{5}), 1ul);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{6}), 1ul);
}

TEST(UnitSolver, assignmentRangeMatchesAssignment) {
    Trail<TrivialClause> underTest{CNFVar{8}};
    CNFLit lit1{CNFVar{4}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{5}, CNFSign::POSITIVE};
    CNFLit lit3{CNFVar{8}, CNFSign::POSITIVE};

    underTest.addAssignment(lit1);
    underTest.addAssignment(lit2);
    underTest.addAssignment(lit3);

    auto assignmentRange = underTest.getAssignments(1ull);
    ASSERT_EQ(assignmentRange.end() - assignmentRange.begin(), 2);
    auto begin = assignmentRange.begin();
    EXPECT_EQ(*begin, lit2);
    EXPECT_EQ(*(begin + 1), lit3);
    EXPECT_EQ(begin + 2, assignmentRange.end());
}

TEST(UnitSolver, assignmentRangeIteratorsRemainValidAfterAdd) {
    Trail<TrivialClause> underTest{CNFVar{16384}};
    for (CNFVar::RawVariable v = 0; v < 10; ++v) {
        underTest.addAssignment(CNFLit{CNFVar{v}, CNFSign::NEGATIVE});
    }

    underTest.newDecisionLevel();
    auto dl0IteratorsPre = underTest.getAssignments(10);

    for (CNFVar::RawVariable v = 11; v < 16384; ++v) {
        underTest.addAssignment(CNFLit{CNFVar{v}, CNFSign::NEGATIVE});
    }

    auto dl0IteratorsPost = underTest.getAssignments(10);

    EXPECT_EQ(dl0IteratorsPre.begin(), dl0IteratorsPost.begin());
}

TEST(UnitSolver, variablePhaseIsNegativeByDefault) {
    Trail<TrivialClause> underTest{CNFVar{16384}};
    EXPECT_EQ(underTest.getPhase(CNFVar{1024}), TBools::FALSE);
}

TEST(UnitSolver, variablePhaseIsSavedInTrail) {
    Trail<TrivialClause> underTest{CNFVar{24}};
    underTest.addAssignment(10_Lit);
    // The phase should not have changed from the default until backtracking
    EXPECT_EQ(underTest.getPhase(CNFVar{10}), TBools::FALSE);
    underTest.shrinkToDecisionLevel(0);
    EXPECT_EQ(underTest.getNumberOfAssignments(), 0ull);
    EXPECT_EQ(underTest.getPhase(CNFVar{10}), TBools::TRUE);
}

TEST(UnitSolver, sizeOneTrailWithoutAssignmentHasNoCompleteAssignment) {
    Trail<TrivialClause> underTest{CNFVar{0}};
    EXPECT_FALSE(underTest.isVariableAssignmentComplete());
}

TEST(UnitSolver, sizeOneTrailWithSingleAssignmentHasCompleteAssignment) {
    Trail<TrivialClause> underTest{CNFVar{0}};
    underTest.addAssignment(0_Lit);
    EXPECT_TRUE(underTest.isVariableAssignmentComplete());
}

TEST(UnitSolver, sizeThreeTrailWithThreeAssignmentsHasCompleteAssignment) {
    Trail<TrivialClause> underTest{CNFVar{2}};
    underTest.addAssignment(0_Lit);
    EXPECT_FALSE(underTest.isVariableAssignmentComplete());
    underTest.addAssignment(2_Lit);
    EXPECT_FALSE(underTest.isVariableAssignmentComplete());
    underTest.addAssignment(1_Lit);
    EXPECT_TRUE(underTest.isVariableAssignmentComplete());
}

TEST(UnitSolver, trailAssignmentIsIncompleteAfterBacktrack) {
    Trail<TrivialClause> underTest{CNFVar{5}};
    underTest.addAssignment(0_Lit);
    underTest.addAssignment(2_Lit);
    underTest.addAssignment(1_Lit);
    underTest.newDecisionLevel();
    underTest.addAssignment(4_Lit);
    underTest.addAssignment(3_Lit);
    underTest.addAssignment(5_Lit);
    ASSERT_TRUE(underTest.isVariableAssignmentComplete());
    // Removes all assignments on current decision level:
    underTest.shrinkToDecisionLevel(underTest.getCurrentDecisionLevel());
    EXPECT_FALSE(underTest.isVariableAssignmentComplete());
}

TEST(UnitSolver, trailMaxVariableCanBeIncreased) {
    Trail<TrivialClause> underTest{CNFVar{5}};
    underTest.newDecisionLevel();

    underTest.addAssignment(5_Lit);
    ASSERT_EQ(underTest.getAssignment(CNFVar{5}), TBools::TRUE);
    underTest.increaseMaxVarTo(CNFVar{7});
    ASSERT_EQ(underTest.getAssignment(CNFVar{5}), TBools::TRUE);

    EXPECT_EQ(underTest.getAssignment(CNFVar{7}), TBools::INDETERMINATE);
    EXPECT_EQ(underTest.getPhase(CNFVar{7}), TBools::FALSE);
    underTest.addAssignment(7_Lit);
    ASSERT_EQ(underTest.getNumberOfAssignments(), 2ull);
    EXPECT_EQ(underTest.getAssignment(CNFVar{7}), TBools::TRUE);
    EXPECT_EQ(underTest.getAssignmentDecisionLevel(CNFVar{7}), 1ull);
    EXPECT_EQ(underTest.getPhase(CNFVar{7}), TBools::FALSE);
}
}
