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

#include "TestAssignmentProvider.h"
#include <libjamsat/solver/Propagation.h>

namespace jamsat {
TEST(UnitSolver, propagateWithoutClausesIsNoop) {
  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

  size_t amntNewFacts = 0xFFFF;
  CNFLit propagatedLit = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
  auto conflictingClause = underTest.propagate(propagatedLit, amntNewFacts);

  EXPECT_EQ(amntNewFacts, 0ull);
  EXPECT_EQ(conflictingClause, nullptr);
  EXPECT_FALSE(underTest.hasForcedAssignment(propagatedLit.getVariable()));
}

TEST(UnitSolver, propagateToFixpointWithoutClausesIsNoop) {
  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

  CNFLit propagatedLit = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
  auto conflictingClause = underTest.propagateUntilFixpoint(propagatedLit);

  EXPECT_EQ(conflictingClause, nullptr);
  EXPECT_FALSE(underTest.hasForcedAssignment(propagatedLit.getVariable()));
}

TEST(UnitSolver, falsingSingleLiteralInBinaryClauseCausesPropagation) {
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binaryClause = createHeapClause(2);
  (*binaryClause)[0] = lit1;
  (*binaryClause)[1] = lit2;

  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*binaryClause);

  assignments.addLiteral(~lit2);

  size_t amntNewFacts = 0xFFFF;
  auto conflictingClause = underTest.propagate(~lit2, amntNewFacts);
  EXPECT_EQ(conflictingClause, nullptr); // no conflict expected
  EXPECT_EQ(amntNewFacts, 1ull);
  EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBool::FALSE);
}

TEST(UnitSolver, reasonsAreRecordedDuringPropagation) {
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binaryClause = createHeapClause(2);
  (*binaryClause)[0] = lit1;
  (*binaryClause)[1] = lit2;

  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*binaryClause);

  assignments.addLiteral(~lit2);

  size_t amntNewFacts = 0xFFFF;
  underTest.propagate(~lit2, amntNewFacts);

  EXPECT_EQ(underTest.getAssignmentReason(CNFVar{2}), nullptr);
  EXPECT_EQ(underTest.getAssignmentReason(CNFVar{1}), binaryClause.get());
}

TEST(UnitSolver, propagateWithSingleTrueClauseCausesNoPropagation) {
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binaryClause = createHeapClause(2);
  (*binaryClause)[0] = lit1;
  (*binaryClause)[1] = lit2;

  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*binaryClause);

  assignments.addLiteral(lit1);
  assignments.addLiteral(~lit2);

  size_t amntNewFacts = 0xFFFF;
  auto conflictingClause = underTest.propagate(~lit2, amntNewFacts);
  EXPECT_EQ(conflictingClause, nullptr); // no conflict expected
  EXPECT_EQ(amntNewFacts, 0ull);
  EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBool::FALSE);
  EXPECT_EQ(assignments.getAssignment(CNFVar{2}), TBool::TRUE);
}

TEST(UnitSolver, propagateWithTernaryClause) {
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};

  auto ternaryClause = createHeapClause(3);
  (*ternaryClause)[0] = lit1;
  (*ternaryClause)[1] = lit2;
  (*ternaryClause)[2] = lit3;

  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*ternaryClause);

  size_t newFacts = 0xFFFF;
  assignments.addLiteral(~lit1);
  underTest.propagate(~lit1, newFacts);
  EXPECT_EQ(newFacts, 0ull);

  assignments.addLiteral(~lit2);
  underTest.propagate(~lit2, newFacts);
  EXPECT_EQ(newFacts, 1ull);
  EXPECT_EQ(assignments.getAssignment(lit3), TBool::TRUE);
}

TEST(UnitSolver, propagateWithTernaryClausesAfterConflict) {
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};

  auto ternaryClause = createHeapClause(3);
  (*ternaryClause)[0] = lit1;
  (*ternaryClause)[1] = lit2;
  (*ternaryClause)[2] = lit3;

  auto ternaryClause2 = createHeapClause(3);
  (*ternaryClause2)[0] = lit1;
  (*ternaryClause2)[1] = ~lit2;
  (*ternaryClause2)[2] = lit3;

  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*ternaryClause);

  size_t newFacts = 0xFFFF;
  assignments.addLiteral(~lit1);
  underTest.propagate(~lit1, newFacts);
  EXPECT_EQ(newFacts, 0ull);

  assignments.addLiteral(~lit3);
  auto conflictingClause = underTest.propagate(~lit2, newFacts);

  EXPECT_TRUE(conflictingClause == ternaryClause.get() ||
              conflictingClause == ternaryClause2.get());

  // backtrack
  assignments.clearLiteral(~lit3);
  assignments.clearLiteral(lit2);

  // propagate something else
  assignments.addLiteral(~lit2);
  newFacts = 0xFFFF;
  conflictingClause = underTest.propagate(~lit2, newFacts);
  EXPECT_EQ(newFacts, 1ull);
  EXPECT_EQ(assignments.getAssignment(lit3), TBool::TRUE);
}

TEST(UnitSolver, registerClauseWithUnassignedLiteralsCausesNoPropagation) {
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};

  auto ternaryClause = createHeapClause(3);
  (*ternaryClause)[0] = lit1;
  (*ternaryClause)[1] = lit2;
  (*ternaryClause)[2] = lit3;

  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*ternaryClause);

  EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBool::INDETERMINATE);
  EXPECT_EQ(assignments.getAssignment(CNFVar{2}), TBool::INDETERMINATE);
  EXPECT_EQ(assignments.getAssignment(CNFVar{3}), TBool::INDETERMINATE);
}

TEST(UnitSolver, registerClauseWithAssignedLiteralsCausesPropagation) {
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};

  auto ternaryClause = createHeapClause(3);
  (*ternaryClause)[0] = lit1;
  (*ternaryClause)[1] = lit2;
  (*ternaryClause)[2] = lit3;

  TestAssignmentProvider assignments;
  assignments.addLiteral(~lit2);
  assignments.addLiteral(~lit3);

  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*ternaryClause);

  EXPECT_EQ(assignments.getAssignment(lit1), TBool::TRUE);
  EXPECT_EQ(assignments.getAssignment(lit2), TBool::FALSE);
  EXPECT_EQ(assignments.getAssignment(lit3), TBool::FALSE);
}

TEST(UnitSolver, propagateUntilFixpointPropagatesTransitively) {
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::POSITIVE};

  auto firstForcingClause = createHeapClause(2);
  (*firstForcingClause)[0] = lit1;
  (*firstForcingClause)[1] = lit2;

  auto midForcingClause1 = createHeapClause(3);
  (*midForcingClause1)[0] = ~lit3;
  (*midForcingClause1)[1] = lit1;
  (*midForcingClause1)[2] = ~lit2;

  auto midForcingClause2 = createHeapClause(3);
  (*midForcingClause2)[0] = ~lit2;
  (*midForcingClause2)[1] = lit1;
  (*midForcingClause2)[2] = lit4;

  auto lastForcingClause = createHeapClause(3);
  (*lastForcingClause)[0] = lit3;
  (*lastForcingClause)[1] = ~lit4;
  (*lastForcingClause)[2] = lit5;

  TestAssignmentProvider assignments;

  CNFVar maxVar{5};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*firstForcingClause);
  underTest.registerClause(*midForcingClause1);
  underTest.registerClause(*midForcingClause2);
  underTest.registerClause(*lastForcingClause);

  assignments.addLiteral(~lit1);
  auto conflictingClause = underTest.propagateUntilFixpoint(~lit1);

  EXPECT_EQ(conflictingClause, nullptr);
  EXPECT_EQ(assignments.getAssignment(lit1), TBool::FALSE);
  EXPECT_EQ(assignments.getAssignment(lit2), TBool::TRUE);
  EXPECT_EQ(assignments.getAssignment(lit3), TBool::FALSE);
  EXPECT_EQ(assignments.getAssignment(lit4), TBool::TRUE);
  EXPECT_EQ(assignments.getAssignment(lit5), TBool::TRUE);
}

TEST(UnitSolver, propagateUntilFixpointReportsImmediateConflicts) {
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binaryClause = createHeapClause(2);
  (*binaryClause)[0] = lit1;
  (*binaryClause)[1] = lit2;

  TestAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*binaryClause);

  assignments.addLiteral(~lit1);
  assignments.addLiteral(~lit2);

  auto conflictingClause = underTest.propagateUntilFixpoint(~lit2);
  EXPECT_EQ(conflictingClause, binaryClause.get());
}

TEST(UnitSolver, propagateUntilFixpointReportsEnsuingConflicts) {
  CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
  CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
  CNFLit lit5{CNFVar{5}, CNFSign::POSITIVE};

  auto firstForcingClause = createHeapClause(2);
  (*firstForcingClause)[0] = lit1;
  (*firstForcingClause)[1] = lit2;

  auto midForcingClause1 = createHeapClause(3);
  (*midForcingClause1)[0] = ~lit3;
  (*midForcingClause1)[1] = lit1;
  (*midForcingClause1)[2] = ~lit2;

  auto midForcingClause2 = createHeapClause(3);
  (*midForcingClause2)[0] = ~lit2;
  (*midForcingClause2)[1] = lit1;
  (*midForcingClause2)[2] = lit4;

  auto lastForcingClause = createHeapClause(3);
  (*lastForcingClause)[0] = lit3;
  (*lastForcingClause)[1] = ~lit4;
  (*lastForcingClause)[2] = lit5;

  TestAssignmentProvider assignments;

  CNFVar maxVar{5};
  Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*firstForcingClause);
  underTest.registerClause(*midForcingClause1);
  underTest.registerClause(*midForcingClause2);
  underTest.registerClause(*lastForcingClause);

  assignments.addLiteral(~lit5);
  auto conflictingClause = underTest.propagateUntilFixpoint(~lit5);
  EXPECT_EQ(conflictingClause, nullptr);

  assignments.addLiteral(~lit1);
  conflictingClause = underTest.propagateUntilFixpoint(~lit1);
  EXPECT_TRUE(conflictingClause == midForcingClause1.get() ||
              conflictingClause == midForcingClause2.get() ||
              conflictingClause == lastForcingClause.get());
}

// TODO: test watcher restoration
}