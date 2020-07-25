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

#include <libjamsat/solver/Assignment.h>

#include <gtest/gtest.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>

#include <vector>

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;


TEST(UnitSolver, emptyAssignmentHasAssgnLevel0) {
    assignment under_test{CNFVar{10}};
    EXPECT_EQ(under_test.get_current_level(), 0ull);
}

TEST(UnitSolver, firstnew_levelIs1) {
    assignment under_test{CNFVar{10}};
    under_test.new_level();
    EXPECT_EQ(under_test.get_current_level(), 1ull);
}

TEST(UnitSolver, emptyAssignmentHasNoAssignments) {
    assignment under_test{CNFVar{10}};
    EXPECT_EQ(under_test.get_num_assignments(), 0ull);
}

TEST(UnitSolver, assignmentHasSingleAssignmentAfterSingleAdd) {
    assignment under_test{CNFVar{10}};
    under_test.append(3_Lit);
    EXPECT_EQ(under_test.get_num_assignments(), 1ull);
}

TEST(UnitSolver, assignmentHasThreeAssignmentsAfterThreeAdds) {
    assignment under_test{CNFVar{10}};
    under_test.append(1_Lit);
    under_test.append(~2_Lit);
    under_test.append(~3_Lit);
    EXPECT_EQ(under_test.get_num_assignments(), 3ull);
}

TEST(UnitSolver, initialAddedLiteralsAreOnLevel0) {
    assignment under_test{CNFVar{10}};
    under_test.append(1_Lit);
    under_test.append(~2_Lit);
    under_test.append(~3_Lit);
    EXPECT_EQ(under_test.get_current_level(), 0ull);
}

TEST(UnitSolver, assignmentSeparatesLiteralsByAssgnLevels) {
    CNFLit testLiteral1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit testLiteral3{CNFVar{3}, CNFSign::NEGATIVE};

    assignment under_test{CNFVar{10}};
    under_test.append(testLiteral1);

    under_test.new_level();
    under_test.append(testLiteral2);
    under_test.append(testLiteral3);

    ASSERT_EQ(under_test.get_current_level(), 1ull);

    auto level_0_iterator = under_test.get_level_assignments(0);
    EXPECT_EQ(level_0_iterator.end() - level_0_iterator.begin(), 1);
    EXPECT_EQ(*(level_0_iterator.begin()), testLiteral1);

    auto level_1_iterator = under_test.get_level_assignments(1);
    EXPECT_EQ(level_1_iterator.end() - level_1_iterator.begin(), 2);
    EXPECT_EQ(*(level_1_iterator.begin()), testLiteral2);
    EXPECT_EQ(*(level_1_iterator.begin() + 1), testLiteral3);

    under_test.new_level();
    ASSERT_EQ(under_test.get_current_level(), 2ull);

    auto level_2_iterator = under_test.get_level_assignments(2);
    EXPECT_EQ(level_2_iterator.end(), level_2_iterator.begin());

    auto level_3_iterator = under_test.get_level_assignments(3);
    EXPECT_EQ(level_3_iterator.end(), level_2_iterator.begin());
}

TEST(UnitSolver, assignmentAssgnLevelIteratorsRemainValidAfterAdd) {
    assignment under_test{CNFVar{16384}};
    for (CNFVar::RawVariable v = 0; v < 10; ++v) {
        under_test.append(CNFLit{CNFVar{v}, CNFSign::NEGATIVE});
    }

    under_test.new_level();
    auto dl_0_iterators_pre = under_test.get_level_assignments(0);

    for (CNFVar::RawVariable v = 11; v < 16384; ++v) {
        under_test.append(CNFLit{CNFVar{v}, CNFSign::NEGATIVE});
    }

    auto dl_0_iterators_post = under_test.get_level_assignments(0);

    EXPECT_EQ(dl_0_iterators_pre.begin(), dl_0_iterators_post.begin());
    EXPECT_EQ(dl_0_iterators_pre.end(), dl_0_iterators_post.end());
}

TEST(UnitSolver, emptyAssignmentHasIndeterminateAssignment) {
    assignment under_test{CNFVar{10}};
    for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
        EXPECT_EQ(under_test.get(CNFVar{i}), TBools::INDETERMINATE);
        CNFLit iLit = CNFLit{CNFVar{i}, CNFSign::POSITIVE};
        EXPECT_EQ(under_test.get(iLit), TBools::INDETERMINATE);
        EXPECT_EQ(under_test.get(~iLit), TBools::INDETERMINATE);
    }
}

TEST(UnitSolver, variablesOnAssignmentHaveAssignment) {
    assignment under_test{CNFVar{10}};
    under_test.append(~4_Lit);
    EXPECT_EQ(under_test.get(CNFVar{4}), TBools::FALSE);
    EXPECT_EQ(under_test.get(4_Lit), TBools::FALSE);
    EXPECT_EQ(under_test.get(~4_Lit), TBools::TRUE);

    for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
        if (i != 4) {
            EXPECT_EQ(under_test.get(CNFVar{i}), TBools::INDETERMINATE);
        }
    }
}

TEST(UnitSolver, variablesOnAssignmentHaveNullReasonsByDefault) {
    assignment under_test{CNFVar{10}};
    under_test.append(~4_Lit);
    EXPECT_EQ(under_test.get_reason(CNFVar{4}), nullptr);

    assignment const& constUnderTest = under_test;
    EXPECT_EQ(constUnderTest.get_reason(CNFVar{4}), nullptr);
}

TEST(UnitSolver, assignmentsBecomeIndeterminateOnRevisit) {
    assignment under_test{CNFVar{10}};
    under_test.append(~4_Lit);
    under_test.new_level();
    under_test.append(5_Lit);
    under_test.new_level();
    under_test.append(6_Lit);
    under_test.new_level();
    under_test.append(7_Lit);

    under_test.undo_to_level(1);
    EXPECT_EQ(under_test.get(CNFVar{4}), TBools::FALSE);
    EXPECT_EQ(under_test.get(CNFVar{5}), TBools::TRUE);
    EXPECT_EQ(under_test.get(CNFVar{6}), TBools::INDETERMINATE);
    EXPECT_EQ(under_test.get(CNFVar{7}), TBools::INDETERMINATE);
}

TEST(UnitSolver, undiscardedAssgnLevelsRemainIntactAfterRevisit) {
    assignment under_test{CNFVar{10}};
    under_test.append(~4_Lit);
    under_test.new_level();
    under_test.append(5_Lit);
    under_test.append(6_Lit);
    under_test.new_level();
    under_test.append(7_Lit);

    under_test.undo_to_level(1);
    EXPECT_EQ(under_test.get_level(CNFVar{4}), 0ul);
    EXPECT_EQ(under_test.get_level(CNFVar{5}), 1ul);
    EXPECT_EQ(under_test.get_level(CNFVar{6}), 1ul);
}

TEST(UnitSolver, variablePhaseIsNegativeByDefault) {
    assignment under_test{CNFVar{16384}};
    EXPECT_EQ(under_test.get_phase(CNFVar{1024}), TBools::FALSE);
}

TEST(UnitSolver, variablePhaseIsSavedInAssignment) {
    assignment under_test{CNFVar{24}};
    under_test.new_level();
    under_test.append(10_Lit);
    // The phase should not have changed from the default until backtracking
    EXPECT_EQ(under_test.get_phase(CNFVar{10}), TBools::FALSE);
    under_test.undo_to_level(0);
    EXPECT_EQ(under_test.get_num_assignments(), 0ull);
    EXPECT_EQ(under_test.get_phase(CNFVar{10}), TBools::TRUE);
}

TEST(UnitSolver, sizeOneAssignmentWithoutAssignmentHasNoCompleteAssignment) {
    assignment under_test{CNFVar{0}};
    EXPECT_FALSE(under_test.is_complete());
}

TEST(UnitSolver, sizeOneAssignmentWithSingleAssignmentHasCompleteAssignment) {
    assignment under_test{CNFVar{0}};
    under_test.append(0_Lit);
    EXPECT_TRUE(under_test.is_complete());
}

TEST(UnitSolver, sizeThreeAssignmentWithThreeAssignmentsHasCompleteAssignment) {
    assignment under_test{CNFVar{2}};
    under_test.append(0_Lit);
    EXPECT_FALSE(under_test.is_complete());
    under_test.append(2_Lit);
    EXPECT_FALSE(under_test.is_complete());
    under_test.append(1_Lit);
    EXPECT_TRUE(under_test.is_complete());
}

TEST(UnitSolver, assignmentAssignmentIsIncompleteAfterBacktrack) {
    assignment under_test{CNFVar{5}};
    under_test.new_level();
    under_test.new_level();
    under_test.append(0_Lit);
    under_test.append(2_Lit);
    under_test.append(1_Lit);
    under_test.new_level();
    under_test.append(4_Lit);
    under_test.append(3_Lit);
    under_test.append(5_Lit);
    ASSERT_TRUE(under_test.is_complete());
    // Removes all assignments on current decision level:
    under_test.undo_to_level(0);
    EXPECT_FALSE(under_test.is_complete());
}

TEST(UnitSolver, assignmentMaxVariableCanBeIncreased) {
    assignment under_test{CNFVar{5}};
    under_test.new_level();

    under_test.append(5_Lit);
    ASSERT_EQ(under_test.get(CNFVar{5}), TBools::TRUE);
    under_test.inc_max_var(CNFVar{7});
    ASSERT_EQ(under_test.get(CNFVar{5}), TBools::TRUE);

    EXPECT_EQ(under_test.get(CNFVar{7}), TBools::INDETERMINATE);
    EXPECT_EQ(under_test.get_phase(CNFVar{7}), TBools::FALSE);
    under_test.append(7_Lit);
    ASSERT_EQ(under_test.get_num_assignments(), 2ull);
    EXPECT_EQ(under_test.get(CNFVar{7}), TBools::TRUE);
    EXPECT_EQ(under_test.get_level(CNFVar{7}), 1ull);
    EXPECT_EQ(under_test.get_phase(CNFVar{7}), TBools::FALSE);
}
}