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
#include <toolbox/testutils/RangeUtils.h>

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;

TEST(UnitSolver, propagateWithoutClausesIsNoop) {
    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);

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
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);

    CNFLit propagatedLit = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
    auto conflictingClause = underTest.propagateUntilFixpoint(propagatedLit);

    EXPECT_EQ(conflictingClause, nullptr);
    EXPECT_FALSE(underTest.hasForcedAssignment(propagatedLit.getVariable()));
}

TEST(UnitSolver, falsingSingleLiteralInBinaryClauseCausesPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    TrivialClause binaryClause{lit1, lit2};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(binaryClause);

    assignments.addAssignment(~lit2);

    size_t amntNewFacts = 0xFFFF;
    auto conflictingClause = underTest.propagate(~lit2, amntNewFacts);
    EXPECT_EQ(conflictingClause, nullptr); // no conflict expected
    EXPECT_EQ(amntNewFacts, 1ull);
    EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBool::FALSE);
}

TEST(UnitSolver, reasonsAreRecordedDuringPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    TrivialClause binaryClause{lit1, lit2};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(binaryClause);

    assignments.addAssignment(~lit2);

    size_t amntNewFacts = 0xFFFF;
    underTest.propagate(~lit2, amntNewFacts);

    EXPECT_EQ(underTest.getAssignmentReason(CNFVar{2}), nullptr);
    EXPECT_EQ(underTest.getAssignmentReason(CNFVar{1}), &binaryClause);
}

TEST(UnitSolver, propagateWithSingleTrueClauseCausesNoPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    TrivialClause binaryClause{lit1, lit2};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(binaryClause);

    assignments.addAssignment(lit1);
    assignments.addAssignment(~lit2);

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
    TrivialClause ternaryClause{lit1, lit2, lit3};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(ternaryClause);

    size_t newFacts = 0xFFFF;
    assignments.addAssignment(~lit1);
    underTest.propagate(~lit1, newFacts);
    EXPECT_EQ(newFacts, 0ull);

    assignments.addAssignment(~lit2);
    underTest.propagate(~lit2, newFacts);
    EXPECT_EQ(newFacts, 1ull);
    EXPECT_EQ(assignments.getAssignment(lit3), TBool::TRUE);
}

TEST(UnitSolver, propagateWithTernaryClausesAfterConflict) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause ternaryClause{lit1, lit2, lit3};
    TrivialClause ternaryClause2{lit1, ~lit2, lit3};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(ternaryClause);
    underTest.registerClause(ternaryClause2);

    size_t newFacts = 0xFFFF;
    assignments.addAssignment(~lit1);
    underTest.propagate(~lit1, newFacts);

    assignments.addAssignment(~lit3);
    auto conflictingClause = underTest.propagate(~lit3, newFacts);
    EXPECT_EQ(newFacts, 1ull);
    EXPECT_NE(conflictingClause, nullptr);
    EXPECT_TRUE(conflictingClause == &ternaryClause || conflictingClause == &ternaryClause2);

    // backtrack
    assignments.popLiteral();
    assignments.popLiteral();

    // propagate something else
    assignments.addAssignment(~lit2);
    newFacts = 0xFFFF;
    conflictingClause = underTest.propagate(~lit2, newFacts);
    EXPECT_EQ(newFacts, 1ull);
    EXPECT_EQ(conflictingClause, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit3), TBool::TRUE);
}

TEST(UnitSolver, registerClauseWithUnassignedLiteralsCausesNoPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause ternaryClause{lit1, lit2, lit3};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(ternaryClause);

    EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBool::INDETERMINATE);
    EXPECT_EQ(assignments.getAssignment(CNFVar{2}), TBool::INDETERMINATE);
    EXPECT_EQ(assignments.getAssignment(CNFVar{3}), TBool::INDETERMINATE);
}

TEST(UnitSolver, registerClauseWithAssignedLiteralsCausesPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause ternaryClause{lit1, lit2, lit3};

    TestAssignmentProvider assignments;
    assignments.addAssignment(~lit2);
    assignments.addAssignment(~lit3);

    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(ternaryClause);

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

    TrivialClause firstForcingClause{lit1, lit2};
    TrivialClause midForcingClause1{~lit3, lit1, ~lit2};
    TrivialClause midForcingClause2{~lit2, lit1, lit4};
    TrivialClause lastForcingClause{lit3, ~lit4, lit5};

    TestAssignmentProvider assignments;

    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(firstForcingClause);
    underTest.registerClause(midForcingClause1);
    underTest.registerClause(midForcingClause2);
    underTest.registerClause(lastForcingClause);

    assignments.addAssignment(~lit1);
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
    TrivialClause binaryClause{lit1, lit2};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(binaryClause);

    assignments.addAssignment(~lit1);
    assignments.addAssignment(~lit2);

    auto conflictingClause = underTest.propagateUntilFixpoint(~lit2);
    EXPECT_EQ(conflictingClause, &binaryClause);
}

TEST(UnitSolver, propagateUntilFixpointReportsEnsuingConflicts) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
    CNFLit lit5{CNFVar{5}, CNFSign::POSITIVE};

    TrivialClause firstForcingClause{lit1, lit2};
    TrivialClause midForcingClause1{~lit3, lit1, ~lit2};
    TrivialClause midForcingClause2{~lit2, lit1, lit4};
    TrivialClause lastForcingClause{lit3, ~lit4, lit5};

    TestAssignmentProvider assignments;

    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(firstForcingClause);
    underTest.registerClause(midForcingClause1);
    underTest.registerClause(midForcingClause2);
    underTest.registerClause(lastForcingClause);

    assignments.addAssignment(~lit5);
    auto conflictingClause = underTest.propagateUntilFixpoint(~lit5);
    EXPECT_EQ(conflictingClause, nullptr);

    assignments.addAssignment(~lit1);
    conflictingClause = underTest.propagateUntilFixpoint(~lit1);
    EXPECT_TRUE(conflictingClause == &midForcingClause1 ||
                conflictingClause == &midForcingClause2 || conflictingClause == &lastForcingClause);
}

TEST(UnitSolver, propagateAfterIncreasingMaximumVariable) {
    TestAssignmentProvider assignments;
    TrivialClause forcingClause{CNFLit{CNFVar{10}, CNFSign::NEGATIVE},
                                CNFLit{CNFVar{6}, CNFSign::POSITIVE}};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(CNFVar{5}, assignments);
    underTest.increaseMaxVarTo(CNFVar{10});
    underTest.registerClause(forcingClause);
    assignments.addAssignment(CNFLit{CNFVar{10}, CNFSign::POSITIVE});
    underTest.propagateUntilFixpoint(CNFLit{CNFVar{10}, CNFSign::POSITIVE});
    EXPECT_EQ(assignments.getAssignment(CNFVar{6}), TBool::TRUE);
}

TEST(UnitSolver, propagateAfterEraseToBeDeletedDoesNotPropagateDeletedClauses) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
    CNFLit lit5{CNFVar{5}, CNFSign::POSITIVE};

    TrivialClause c1{lit1, lit2, lit3};
    TrivialClause c2{~lit3, lit1, ~lit5};
    TrivialClause c3{~lit2, lit1, lit4};

    TestAssignmentProvider assignments;

    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider, TrivialClause> underTest(maxVar, assignments);
    underTest.registerClause(c1);
    underTest.registerClause(c2);
    underTest.registerClause(c3);

    markToBeDeleted(c1);
    underTest.eraseClausesToBeDeleted();

    assignments.addAssignment(~lit1);
    assignments.addAssignment(~lit2);
    auto conflictingClause = underTest.propagateUntilFixpoint(~lit1);
    ASSERT_EQ(conflictingClause, nullptr);
    conflictingClause = underTest.propagateUntilFixpoint(~lit2);
    ASSERT_EQ(conflictingClause, nullptr);
    EXPECT_EQ(assignments.getAssignment(CNFVar{3}), TBool::INDETERMINATE)
        << "Clause c1 has not been erased";

    assignments.addAssignment(lit3);
    conflictingClause = underTest.propagateUntilFixpoint(lit3);
    ASSERT_EQ(conflictingClause, nullptr);
    EXPECT_NE(assignments.getAssignment(CNFVar{5}), TBool::INDETERMINATE)
        << "More clauses erased than expected";
}

TEST(UnitSolver, propagationClauseRangeEmptyWhenNoClausesAdded) {
    TestAssignmentProvider assignments;
    Propagation<TestAssignmentProvider, TrivialClause> underTest(CNFVar{5}, assignments);
    auto clauses = underTest.getClausesInPropagationOrder();
    EXPECT_TRUE(clauses.begin() == clauses.end());
}

TEST(UnitSolver, propagationClauseRangeHasCorrectOrder) {
    TrivialClause c1{CNFLit{CNFVar{2}, CNFSign::POSITIVE}, CNFLit{CNFVar{10}, CNFSign::POSITIVE}};
    TrivialClause c2{CNFLit{CNFVar{0}, CNFSign::NEGATIVE}, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TrivialClause c3{CNFLit{CNFVar{1}, CNFSign::POSITIVE}, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};

    TestAssignmentProvider assignments;
    Propagation<TestAssignmentProvider, TrivialClause> underTest(CNFVar{15}, assignments);
    underTest.registerClause(c1);
    underTest.registerClause(c2);
    underTest.registerClause(c3);

    auto clauses = underTest.getClausesInPropagationOrder();
    // Note: the concrete ordering of clauses is a "hidden" implementation detail because
    // it depends on implementation details of the Watcher implementation.
    expectRangeElementsSequencedEqual(clauses,
                                      std::vector<TrivialClause *>{&c2, &c3, &c1, &c2, &c1, &c3});
}

// TODO: test watcher restoration
}
