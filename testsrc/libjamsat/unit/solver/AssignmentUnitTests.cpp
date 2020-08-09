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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>

#include <toolbox/testutils/ClauseUtils.h>
#include <toolbox/testutils/GMockMatchers.h>

#include <vector>

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;


TEST(UnitSolver, emptyAssignmentHasAssgnLevel0)
{
  Assignment under_test{CNFVar{10}};
  EXPECT_EQ(under_test.getCurrentLevel(), 0ull);
}

TEST(UnitSolver, firstnewLevelIs1)
{
  Assignment under_test{CNFVar{10}};
  under_test.newLevel();
  EXPECT_EQ(under_test.getCurrentLevel(), 1ull);
}

TEST(UnitSolver, emptyAssignmentHasNoAssignments)
{
  Assignment under_test{CNFVar{10}};
  EXPECT_EQ(under_test.getNumAssignments(), 0ull);
}

TEST(UnitSolver, assignmentHasSingleAssignmentAfterSingleAdd)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(3_Lit);
  EXPECT_EQ(under_test.getNumAssignments(), 1ull);
}

TEST(UnitSolver, assignmentHasThreeAssignmentsAfterThreeAdds)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(1_Lit);
  under_test.append(~2_Lit);
  under_test.append(~3_Lit);
  EXPECT_EQ(under_test.getNumAssignments(), 3ull);
}

TEST(UnitSolver, initialAddedLiteralsAreOnLevel0)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(1_Lit);
  under_test.append(~2_Lit);
  under_test.append(~3_Lit);
  EXPECT_EQ(under_test.getCurrentLevel(), 0ull);
}

TEST(UnitSolver, whenNoAssignmentsArePresent_thenAssignmentRangeIsEmpty)
{
  Assignment under_test{CNFVar{10}};
  Assignment::AssignmentRange result = under_test.getAssignments();
  EXPECT_THAT(result, range_empty());
}

TEST(UnitSolver, whenSingleAssignmentIsPresent_thenAssignmentRangeHasSingleElement)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(1_Lit);
  Assignment::AssignmentRange result = under_test.getAssignments();
  EXPECT_THAT(result, range_is(std::vector<CNFLit>{1_Lit}));
}

TEST(UnitSolver, whenThreeAssignmentsArePresent_thenAssignmentRangeHasThreeElements)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(1_Lit);
  under_test.append(3_Lit);
  under_test.append(6_Lit);
  Assignment::AssignmentRange result = under_test.getAssignments();
  EXPECT_THAT(result, range_is(std::vector<CNFLit>{1_Lit, 3_Lit, 6_Lit}));
}


TEST(UnitSolver, assignmentSeparatesLiteralsByAssgnLevels)
{
  CNFLit testLiteral1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit testLiteral3{CNFVar{3}, CNFSign::NEGATIVE};

  Assignment under_test{CNFVar{10}};
  under_test.append(testLiteral1);

  under_test.newLevel();
  under_test.append(testLiteral2);
  under_test.append(testLiteral3);

  ASSERT_EQ(under_test.getCurrentLevel(), 1ull);

  auto level_0_iterator = under_test.getLevelAssignments(0);
  EXPECT_EQ(level_0_iterator.end() - level_0_iterator.begin(), 1);
  EXPECT_EQ(*(level_0_iterator.begin()), testLiteral1);

  auto level_1_iterator = under_test.getLevelAssignments(1);
  EXPECT_EQ(level_1_iterator.end() - level_1_iterator.begin(), 2);
  EXPECT_EQ(*(level_1_iterator.begin()), testLiteral2);
  EXPECT_EQ(*(level_1_iterator.begin() + 1), testLiteral3);

  under_test.newLevel();
  ASSERT_EQ(under_test.getCurrentLevel(), 2ull);

  auto level_2_iterator = under_test.getLevelAssignments(2);
  EXPECT_EQ(level_2_iterator.end(), level_2_iterator.begin());

  auto level_3_iterator = under_test.getLevelAssignments(3);
  EXPECT_EQ(level_3_iterator.end(), level_2_iterator.begin());
}

TEST(UnitSolver, assignmentAssgnLevelIteratorsRemainValidAfterAdd)
{
  Assignment under_test{CNFVar{16384}};
  for (CNFVar::RawVariable v = 0; v < 10; ++v) {
    under_test.append(CNFLit{CNFVar{v}, CNFSign::NEGATIVE});
  }

  under_test.newLevel();
  auto dl_0_iterators_pre = under_test.getLevelAssignments(0);

  for (CNFVar::RawVariable v = 11; v < 16384; ++v) {
    under_test.append(CNFLit{CNFVar{v}, CNFSign::NEGATIVE});
  }

  auto dl_0_iterators_post = under_test.getLevelAssignments(0);

  EXPECT_EQ(dl_0_iterators_pre.begin(), dl_0_iterators_post.begin());
  EXPECT_EQ(dl_0_iterators_pre.end(), dl_0_iterators_post.end());
}

TEST(UnitSolver, emptyAssignmentHasIndeterminateAssignment)
{
  Assignment under_test{CNFVar{10}};
  for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
    EXPECT_EQ(under_test.getAssignment(CNFVar{i}), TBools::INDETERMINATE);
    CNFLit iLit = CNFLit{CNFVar{i}, CNFSign::POSITIVE};
    EXPECT_EQ(under_test.getAssignment(iLit), TBools::INDETERMINATE);
    EXPECT_EQ(under_test.getAssignment(~iLit), TBools::INDETERMINATE);
  }
}

TEST(UnitSolver, variablesOnAssignmentHaveAssignment)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(~4_Lit);
  EXPECT_EQ(under_test.getAssignment(CNFVar{4}), TBools::FALSE);
  EXPECT_EQ(under_test.getAssignment(4_Lit), TBools::FALSE);
  EXPECT_EQ(under_test.getAssignment(~4_Lit), TBools::TRUE);

  for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
    if (i != 4) {
      EXPECT_EQ(under_test.getAssignment(CNFVar{i}), TBools::INDETERMINATE);
    }
  }
}

TEST(UnitSolver, variablesOnAssignmentHaveNullReasonsByDefault)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(~4_Lit);
  EXPECT_EQ(under_test.getReason(CNFVar{4}), nullptr);

  Assignment const& constunder_test = under_test;
  EXPECT_EQ(constunder_test.getReason(CNFVar{4}), nullptr);
}

TEST(UnitSolver, assignmentsBecomeIndeterminateOnRevisit)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(~4_Lit);
  under_test.newLevel();
  under_test.append(5_Lit);
  under_test.newLevel();
  under_test.append(6_Lit);
  under_test.newLevel();
  under_test.append(7_Lit);

  under_test.undoToLevel(1);
  EXPECT_EQ(under_test.getAssignment(CNFVar{4}), TBools::FALSE);
  EXPECT_EQ(under_test.getAssignment(CNFVar{5}), TBools::TRUE);
  EXPECT_EQ(under_test.getAssignment(CNFVar{6}), TBools::INDETERMINATE);
  EXPECT_EQ(under_test.getAssignment(CNFVar{7}), TBools::INDETERMINATE);
}

TEST(UnitSolver, undiscardedAssgnLevelsRemainIntactAfterRevisit)
{
  Assignment under_test{CNFVar{10}};
  under_test.append(~4_Lit);
  under_test.newLevel();
  under_test.append(5_Lit);
  under_test.append(6_Lit);
  under_test.newLevel();
  under_test.append(7_Lit);

  under_test.undoToLevel(1);
  EXPECT_EQ(under_test.getLevel(CNFVar{4}), 0ul);
  EXPECT_EQ(under_test.getLevel(CNFVar{5}), 1ul);
  EXPECT_EQ(under_test.getLevel(CNFVar{6}), 1ul);
}

TEST(UnitSolver, variablePhaseIsNegativeByDefault)
{
  Assignment under_test{CNFVar{16384}};
  EXPECT_EQ(under_test.getPhase(CNFVar{1024}), TBools::FALSE);
}

TEST(UnitSolver, variablePhaseIsSavedInAssignment)
{
  Assignment under_test{CNFVar{24}};
  under_test.newLevel();
  under_test.append(10_Lit);
  // The phase should not have changed from the default until backtracking
  EXPECT_EQ(under_test.getPhase(CNFVar{10}), TBools::FALSE);
  under_test.undoToLevel(0);
  EXPECT_EQ(under_test.getNumAssignments(), 0ull);
  EXPECT_EQ(under_test.getPhase(CNFVar{10}), TBools::TRUE);
}

TEST(UnitSolver, sizeOneAssignmentWithoutAssignmentHasNoCompleteAssignment)
{
  Assignment under_test{CNFVar{0}};
  EXPECT_FALSE(under_test.isComplete());
}

TEST(UnitSolver, sizeOneAssignmentWithSingleAssignmentHasCompleteAssignment)
{
  Assignment under_test{CNFVar{0}};
  under_test.append(0_Lit);
  EXPECT_TRUE(under_test.isComplete());
}

TEST(UnitSolver, sizeThreeAssignmentWithThreeAssignmentsHasCompleteAssignment)
{
  Assignment under_test{CNFVar{2}};
  under_test.append(0_Lit);
  EXPECT_FALSE(under_test.isComplete());
  under_test.append(2_Lit);
  EXPECT_FALSE(under_test.isComplete());
  under_test.append(1_Lit);
  EXPECT_TRUE(under_test.isComplete());
}

TEST(UnitSolver, assignmentAssignmentIsIncompleteAfterBacktrack)
{
  Assignment under_test{CNFVar{5}};
  under_test.newLevel();
  under_test.newLevel();
  under_test.append(0_Lit);
  under_test.append(2_Lit);
  under_test.append(1_Lit);
  under_test.newLevel();
  under_test.append(4_Lit);
  under_test.append(3_Lit);
  under_test.append(5_Lit);
  ASSERT_TRUE(under_test.isComplete());
  // Removes all assignments on current decision level:
  under_test.undoToLevel(0);
  EXPECT_FALSE(under_test.isComplete());
}

TEST(UnitSolver, assignmenMaxVariableCanBeIncreased)
{
  Assignment under_test{CNFVar{5}};
  under_test.newLevel();

  under_test.append(5_Lit);
  ASSERT_EQ(under_test.getAssignment(CNFVar{5}), TBools::TRUE);
  under_test.increaseMaxVar(CNFVar{7});
  ASSERT_EQ(under_test.getAssignment(CNFVar{5}), TBools::TRUE);

  EXPECT_EQ(under_test.getAssignment(CNFVar{7}), TBools::INDETERMINATE);
  EXPECT_EQ(under_test.getPhase(CNFVar{7}), TBools::FALSE);
  under_test.append(7_Lit);
  ASSERT_EQ(under_test.getNumAssignments(), 2ull);
  EXPECT_EQ(under_test.getAssignment(CNFVar{7}), TBools::TRUE);
  EXPECT_EQ(under_test.getLevel(CNFVar{7}), 1ull);
  EXPECT_EQ(under_test.getPhase(CNFVar{7}), TBools::FALSE);
}

TEST(UnitSolver, propagateWithoutClausesIsNoop)
{
  CNFVar max_var{4};
  Assignment under_test{max_var};

  size_t amnt_new_facts = 0xFFFF;
  CNFLit propagatedLit = ~2_Lit;
  auto conflicting_clause = under_test.propagate(propagatedLit, amnt_new_facts);

  EXPECT_EQ(amnt_new_facts, 0ull);
  EXPECT_EQ(conflicting_clause, nullptr);
  EXPECT_FALSE(under_test.isForced(propagatedLit.getVariable()));
}

TEST(UnitSolver, propagateToFixpointWithoutClausesIsNoop)
{
  CNFVar max_var{4};
  Assignment under_test{max_var};

  CNFLit propagatedLit = ~2_Lit;
  auto conflicting_clause = under_test.append(propagatedLit);

  EXPECT_EQ(conflicting_clause, nullptr);
  EXPECT_FALSE(under_test.isForced(propagatedLit.getVariable()));
}

TEST(UnitSolver, falsingSingleLiteralInbinary_clauseCausesPropagation)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binary_clause = createClause({lit1, lit2});

  CNFVar max_var{4};
  Assignment under_test{max_var};
  under_test.registerClause(*binary_clause);

  under_test.assign(~lit2, nullptr);

  size_t amnt_new_facts = 0xFFFF;
  auto conflicting_clause = under_test.propagate(~lit2, amnt_new_facts);
  EXPECT_EQ(conflicting_clause, nullptr); // no conflict expected
  EXPECT_EQ(amnt_new_facts, 1ull);
  EXPECT_EQ(under_test.getAssignment(CNFVar{1}), TBools::FALSE);
}

TEST(UnitSolver, reasonsAreRecordedDuringPropagation)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binary_clause = createClause({lit1, lit2});

  CNFVar max_var{4};
  Assignment under_test{max_var};
  under_test.registerClause(*binary_clause);

  under_test.assign(~lit2, nullptr);

  size_t amnt_new_facts = 0xFFFF;
  under_test.propagate(~lit2, amnt_new_facts);

  EXPECT_EQ(under_test.getReason(CNFVar{2}), nullptr);
  EXPECT_EQ(under_test.getReason(CNFVar{1}), binary_clause.get());
}

TEST(UnitSolver, propagateWithSingleTrueClauseCausesNoPropagation)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binary_clause = createClause({lit1, lit2});

  CNFVar max_var{4};
  Assignment under_test{max_var};
  under_test.registerClause(*binary_clause);

  under_test.assign(lit1, nullptr);
  under_test.assign(~lit2, nullptr);

  size_t amnt_new_facts = 0xFFFF;
  auto conflicting_clause = under_test.propagate(~lit2, amnt_new_facts);
  EXPECT_EQ(conflicting_clause, nullptr); // no conflict expected
  EXPECT_EQ(amnt_new_facts, 0ull);
  EXPECT_EQ(under_test.getAssignment(CNFVar{1}), TBools::FALSE);
  EXPECT_EQ(under_test.getAssignment(CNFVar{2}), TBools::TRUE);
}

TEST(UnitSolver, propagateWithTernaryClause)
{
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  auto ternary_clause = createClause({lit1, lit2, lit3});

  CNFVar max_var{4};
  Assignment under_test{max_var};
  under_test.registerClause(*ternary_clause);

  size_t newFacts = 0xFFFF;
  under_test.assign(~lit1, nullptr);
  under_test.propagate(~lit1, newFacts);
  EXPECT_EQ(newFacts, 0ull);

  under_test.assign(~lit2, nullptr);
  under_test.propagate(~lit2, newFacts);
  EXPECT_EQ(newFacts, 1ull);
  EXPECT_EQ(under_test.getAssignment(lit3), TBools::TRUE);
}

TEST(UnitSolver, propagateWithTernaryClausesAfterConflict)
{
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  auto ternary_clause = createClause({lit1, lit2, lit3});
  auto ternary_clause2 = createClause({lit1, ~lit2, lit3});

  CNFVar max_var{4};
  Assignment under_test{max_var};
  under_test.registerClause(*ternary_clause);
  under_test.registerClause(*ternary_clause2);

  size_t newFacts = 0xFFFF;
  under_test.assign(~lit1, nullptr);
  under_test.newLevel();

  under_test.propagate(~lit1, newFacts);

  under_test.assign(~lit3, nullptr);
  auto conflicting_clause = under_test.propagate(~lit3, newFacts);
  EXPECT_EQ(newFacts, 1ull);
  EXPECT_NE(conflicting_clause, nullptr);
  EXPECT_TRUE(conflicting_clause == ternary_clause.get() ||
              conflicting_clause == ternary_clause2.get());

  // backtrack
  under_test.undoToLevel(0);

  // propagate something else
  under_test.assign(~lit2, nullptr);
  newFacts = 0xFFFF;
  conflicting_clause = under_test.propagate(~lit2, newFacts);
  EXPECT_EQ(newFacts, 1ull);
  EXPECT_EQ(conflicting_clause, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit3), TBools::TRUE);
}

TEST(UnitSolver, registerClauseWithUnassignedLiteralsCausesNoPropagation)
{
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  auto ternary_clause = createClause({lit1, lit2, lit3});

  CNFVar max_var{4};
  Assignment under_test{max_var};
  under_test.registerClause(*ternary_clause);

  EXPECT_EQ(under_test.getAssignment(CNFVar{1}), TBools::INDETERMINATE);
  EXPECT_EQ(under_test.getAssignment(CNFVar{2}), TBools::INDETERMINATE);
  EXPECT_EQ(under_test.getAssignment(CNFVar{3}), TBools::INDETERMINATE);
}

TEST(UnitSolver, registerClauseWithAssignedLiteralsCausesPropagation)
{
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  auto ternary_clause = createClause({lit1, lit2, lit3});

  CNFVar max_var{4};
  Assignment under_test{max_var};
  under_test.assign(~lit2, nullptr);
  under_test.assign(~lit3, nullptr);

  under_test.registerLemma(*ternary_clause);

  EXPECT_EQ(under_test.getAssignment(lit1), TBools::TRUE);
  EXPECT_EQ(under_test.getAssignment(lit2), TBools::FALSE);
  EXPECT_EQ(under_test.getAssignment(lit3), TBools::FALSE);
}

TEST(UnitSolver, propagateUntilFixpointPropagatesTransitively)
{
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::POSITIVE};

  auto first_forcing_clause = createClause({lit1, lit2});
  auto mid_forcing_clause1 = createClause({~lit3, lit1, ~lit2});
  auto mid_forcing_clause2 = createClause({~lit2, lit1, lit4});
  auto last_forcing_clause = createClause({lit3, ~lit4, lit5});

  CNFVar max_var{5};
  Assignment under_test{max_var};
  under_test.registerClause(*first_forcing_clause);
  under_test.registerClause(*mid_forcing_clause1);
  under_test.registerClause(*mid_forcing_clause2);
  under_test.registerClause(*last_forcing_clause);

  Clause* const conflicting_clause = under_test.append(~lit1);

  EXPECT_EQ(conflicting_clause, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit1), TBools::FALSE);
  EXPECT_EQ(under_test.getAssignment(lit2), TBools::TRUE);
  EXPECT_EQ(under_test.getAssignment(lit3), TBools::FALSE);
  EXPECT_EQ(under_test.getAssignment(lit4), TBools::TRUE);
  EXPECT_EQ(under_test.getAssignment(lit5), TBools::TRUE);
}

TEST(UnitSolver, propagateUntilFixpointReportsImmediateConflicts)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binary_clause = createClause({lit1, lit2});

  CNFVar max_var{4};
  Assignment under_test{max_var};
  under_test.registerClause(*binary_clause);

  under_test.assign(~lit1, nullptr);

  auto conflicting_clause = under_test.append(~lit2);
  EXPECT_EQ(conflicting_clause, binary_clause.get());
}

TEST(UnitSolver, propagateUntilFixpointReportsEnsuingConflicts)
{
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::POSITIVE};

  auto first_forcing_clause = createClause({lit1, lit2});
  auto mid_forcing_clause1 = createClause({~lit3, lit1, ~lit2});
  auto mid_forcing_clause2 = createClause({~lit2, lit1, lit4});
  auto last_forcing_clause = createClause({lit3, ~lit4, lit5});


  CNFVar max_var{5};
  Assignment under_test{max_var};
  under_test.registerClause(*first_forcing_clause);
  under_test.registerClause(*mid_forcing_clause1);
  under_test.registerClause(*mid_forcing_clause2);
  under_test.registerClause(*last_forcing_clause);

  Clause* conflicting_clause = under_test.append(~lit5);
  EXPECT_EQ(conflicting_clause, nullptr);

  conflicting_clause = under_test.append(~lit1);
  EXPECT_TRUE(conflicting_clause == mid_forcing_clause1.get() ||
              conflicting_clause == mid_forcing_clause2.get() ||
              conflicting_clause == last_forcing_clause.get());
}

TEST(UnitSolver, propagateAfterIncreasingMaximumVariable)
{
  auto forcingClause = createClause({~10_Lit, 6_Lit});
  Assignment under_test(CNFVar{5});
  under_test.increaseMaxVar(CNFVar{10});
  under_test.registerClause(*forcingClause);
  under_test.append(10_Lit);
  EXPECT_EQ(under_test.getAssignment(CNFVar{6}), TBools::TRUE);
}

TEST(UnitSolver, propagationDetectsAssignmentReasonClause)
{
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  auto clause1 = createClause({lit1, lit2, lit3});
  auto clause2 = createClause({lit1, ~lit2, lit3});

  CNFVar max_var{4};
  Assignment under_test{max_var};

  under_test.registerClause(*clause1);
  under_test.registerClause(*clause2);

  under_test.append(~lit1);
  under_test.append(~lit2);

  auto lit3_reason = under_test.getReason(CNFVar{3});
  ASSERT_NE(lit3_reason, nullptr);
  EXPECT_TRUE(under_test.isReason(*clause1));
  EXPECT_FALSE(under_test.isReason(*clause2));
}

TEST(UnitSolver, propagationDoesNotDetectImpliedFactAssignmentReasonClauseAfterBacktrack)
{
  auto testData = createClause({1_Lit, 2_Lit, 3_Lit});

  Assignment under_test(CNFVar{3});
  under_test.registerClause(*testData);

  under_test.newLevel();
  under_test.append(~1_Lit);
  under_test.append(~2_Lit);

  ASSERT_EQ(under_test.getAssignment(3_Lit), TBools::TRUE);
  EXPECT_EQ(under_test.getReason(CNFVar{3}), testData.get());
  EXPECT_TRUE(under_test.isReason(*testData));

  under_test.undoToLevel(0);

  EXPECT_FALSE(under_test.isReason(*testData));
}

TEST(UnitSolver, clearClausesInPropagation_withReasonsKept)
{
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::POSITIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::POSITIVE};
  CNFLit lit6{CNFVar{6}, CNFSign::POSITIVE};
  auto clause1 = createClause({lit1, lit2, lit3, lit4});
  auto clause2 = createClause({lit1, lit2, ~lit4});
  auto clause3 = createClause({lit5, lit6});

  CNFVar max_var{6};
  Assignment under_test{max_var};
  under_test.registerClause(*clause1);
  under_test.registerClause(*clause2);
  under_test.registerClause(*clause3);

  under_test.append(~lit5);
  ASSERT_EQ(under_test.getReason(lit6.getVariable()), clause3.get());

  under_test.clearClauses();

  EXPECT_EQ(under_test.getReason(lit6.getVariable()), clause3.get());

  under_test.append(~lit1);
  under_test.append(~lit3);
  auto conflicting = under_test.append(~lit2);
  EXPECT_EQ(under_test.getAssignment(lit4), TBools::INDETERMINATE);
  EXPECT_EQ(conflicting, nullptr);
}

/*

Temporarily deactivated: not offering getBinariesMap in assignment yet

TEST(UnitSolver, binary_clausesCanBeQueriedInPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};

    auto c1 = createClause({lit1, lit2});
    auto c2 = createClause({~lit2, lit3});
    auto c3 = createClause({~lit2, lit4});

    CNFVar max_var{5};
    Assignment under_test{max_var};
    under_test.registerClause(*c1);
    under_test.registerClause(*c2);
    under_test.registerClause(*c3);

    auto binaryMap = under_test.getBinariesMap();
    EXPECT_TRUE(binaryMap[~lit1].empty());
    EXPECT_TRUE(binaryMap[~lit3].empty());

    std::vector<CNFLit> expectedForNLit2{lit3, lit4};
    auto binariesWithNLit2 = binaryMap[~lit2];
    ASSERT_EQ(binariesWithNLit2.size(), expectedForNLit2.size());

    std::vector<CNFLit> binariesWithNLit2FwdRange{binariesWithNLit2.begin(),
                                                  binariesWithNLit2.end()};
    EXPECT_TRUE(std::is_permutation(binariesWithNLit2FwdRange.begin(),
                                    binariesWithNLit2FwdRange.end(),
                                    expectedForNLit2.begin()));

    auto binariesWithPLit4 = binaryMap[lit4];
    ASSERT_EQ(binariesWithPLit4.size(), 1ULL);
    EXPECT_EQ(*(binariesWithPLit4.begin()), ~lit2);
}
*/


namespace {
void test_shortenedClausesArePropagatedCorrectly(bool withChangeInWatchedLits,
                                                 bool shortenedBeforeFirstPropagation)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::NEGATIVE};

  auto c1 = createClause({lit1, lit2, lit3, lit4});
  auto c2 = createClause({lit1, lit2, lit4, lit5});

  CNFVar max_var{5};
  Assignment under_test{max_var};

  under_test.registerClause(*c1);
  under_test.registerClause(*c2);

  if (shortenedBeforeFirstPropagation) {
    under_test.registerClauseModification(*c1);
    c1->resize(3);
    if (withChangeInWatchedLits) {
      std::swap((*c1)[1], (*c1)[2]);
    }
  }

  auto conflict = under_test.append(~lit1);
  ASSERT_EQ(conflict, nullptr);

  if (!shortenedBeforeFirstPropagation) {
    under_test.registerClauseModification(*c1);
    c1->resize(3);
    if (withChangeInWatchedLits) {
      std::swap((*c1)[1], (*c1)[2]);
    }
  }

  conflict = under_test.append(~lit2);
  ASSERT_EQ(conflict, nullptr);

  // The shortened clause now forces the assignment of lit3:
  EXPECT_EQ(under_test.getAssignment(lit3), TBools::TRUE);

  // Check that c2 remains unchanged:
  conflict = under_test.append(~lit4);
  ASSERT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit5), TBools::TRUE);
}
}

TEST(UnitSolver,
     shortenedClausesArePropagatedCorrectly_noChangeInWatchedLits_shortenedAfterRegistration)
{
  test_shortenedClausesArePropagatedCorrectly(false, true);
}

TEST(UnitSolver,
     shortenedClausesArePropagatedCorrectly_noChangeInWatchedLits_shortenedAfterPropagation)
{
  test_shortenedClausesArePropagatedCorrectly(false, false);
}

TEST(UnitSolver,
     shortenedClausesArePropagatedCorrectly_withChangeInWatchedLits_shortenedAfterRegistration)
{
  test_shortenedClausesArePropagatedCorrectly(true, true);
}

TEST(UnitSolver,
     shortenedClausesArePropagatedCorrectly_withChangeInWatchedLits_shortenedAfterPropagation)
{
  test_shortenedClausesArePropagatedCorrectly(true, true);
}

TEST(UnitSolver, shortenedClausesArePropagatedCorrectly_withBothWatchedLitsRemoved)
{
  auto testData = createClause({1_Lit, 2_Lit, 3_Lit, 4_Lit});

  CNFVar max_var{4};
  Assignment under_test{max_var};

  under_test.registerClause(*testData);
  under_test.registerClauseModification(*testData);
  testData->resize(2);
  (*testData)[0] = 3_Lit;
  (*testData)[1] = 4_Lit;

  auto conflict = under_test.append(~3_Lit);
  ASSERT_EQ(conflict, nullptr);

  EXPECT_EQ(under_test.getAssignment(4_Lit), TBools::TRUE);
}

TEST(UnitSolver, shortenedClausesArePropagatedCorrectly_shortenToBinary)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::NEGATIVE};

  auto c1 = createClause({lit1, lit2, lit3, lit4});
  auto c2 = createClause({lit1, ~lit2, lit4, lit5});

  CNFVar max_var{5};
  Assignment under_test{max_var};

  under_test.registerClause(*c1);
  under_test.registerClause(*c2);

  under_test.registerClauseModification(*c1);
  c1->resize(2);

  auto conflict = under_test.append(~lit1);
  ASSERT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit2), TBools::TRUE);

  // Check that c2 remains unchanged:
  conflict = under_test.append(~lit4);
  ASSERT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit5), TBools::TRUE);
}

TEST(UnitSolver, shortenedClausesArePropagatedCorrectly_shortenToBinary_watchersUpdatedCorrectly)
{
  // Regression test: Check that the "binarized" clause is
  // inserted into the correct watcher list

  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  auto c1 = createClause({lit1, lit2, lit3});

  CNFVar max_var{5};
  Assignment under_test{max_var};

  under_test.registerClause(*c1);

  under_test.registerClauseModification(*c1);
  (*c1)[1] = (*c1)[2];
  c1->resize(2);

  // lit2 has been removed from the clause, check that its assignment
  // does not cause propagations:
  auto conflict = under_test.append(~lit2);
  EXPECT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit1), TBools::INDETERMINATE);

  under_test.newLevel();

  // Check that the clause forces assignments as expected:
  conflict = under_test.append(~lit1);
  EXPECT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit3), TBools::TRUE);

  under_test.undoToLevel(0);

  conflict = under_test.append(~lit3);
  EXPECT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit1), TBools::TRUE);
}

TEST(UnitSolver, deletedBinariesAreRemovedFromPropagationAfterAnnounce)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
  auto c1 = createClause({lit1, lit2});

  CNFVar max_var{5};
  Assignment under_test{max_var};

  under_test.registerClause(*c1);

  under_test.registerClauseModification(*c1);
  c1->resize(1ULL);
  c1->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);

  auto conflict = under_test.append(~lit2);
  EXPECT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit1), TBools::INDETERMINATE);
}

TEST(UnitSolver, deletedNonbinary_clausesAreRemovedFromPropagationAfterAnnounce)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::NEGATIVE};

  auto c1 = createClause({lit1, lit2, lit3});
  auto c2 = createClause({lit1, lit2, lit4, lit5});

  CNFVar max_var{5};
  Assignment under_test{max_var};

  under_test.registerClause(*c1);
  under_test.registerClause(*c2);

  under_test.registerClauseModification(*c1);
  c1->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);

  auto conflict = under_test.append(~lit1);
  ASSERT_EQ(conflict, nullptr);

  conflict = under_test.append(~lit2);
  ASSERT_EQ(conflict, nullptr);

  // c1 should be removed from propagation now:
  EXPECT_EQ(under_test.getAssignment(lit3), TBools::INDETERMINATE);

  // Check that c2 remains unchanged:
  conflict = under_test.append(~lit4);
  ASSERT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit5), TBools::TRUE);
}

TEST(UnitSolver, redundantClausesAreNotPropagatedInExcludeRedundantMode)
{
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::NEGATIVE};
  CNFLit lit6{CNFVar{6}, CNFSign::NEGATIVE};

  auto c1 = createClause({lit1, lit2});
  auto c2 = createClause({lit1, lit3, lit4});
  auto c3 = createClause({lit1, lit5, lit6});

  c1->setFlag(Clause::Flag::REDUNDANT);
  c2->setFlag(Clause::Flag::REDUNDANT);

  Assignment under_test(CNFVar{6});

  under_test.registerClause(*c1);
  under_test.registerClause(*c2);
  under_test.registerClause(*c3);


  // Check that binaries are propagated no matter what their redundancy status is:
  auto conflict = under_test.append(~lit1, Assignment::up_mode::exclude_lemmas);
  ASSERT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit2), TBools::TRUE);

  // Check that redundant non-binary clauses are not propagated:
  conflict = under_test.append(~lit3, Assignment::up_mode::exclude_lemmas);
  ASSERT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit4), TBools::INDETERMINATE);

  // Check that the third (non-redundant) clause is not ignored:
  conflict = under_test.append(~lit5, Assignment::up_mode::exclude_lemmas);
  ASSERT_EQ(conflict, nullptr);
  EXPECT_EQ(under_test.getAssignment(lit6), TBools::TRUE);
}
}