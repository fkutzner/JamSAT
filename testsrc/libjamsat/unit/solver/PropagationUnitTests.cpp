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

#include <libjamsat/concepts/SolverTypeTraits.h>
#include <libjamsat/solver/Propagation.h>
#include <toolbox/testutils/RangeUtils.h>
#include <toolbox/testutils/TestAssignmentProvider.h>

namespace jamsat {
using TrivialClause = jamsat::TestAssignmentProvider::Clause;

static_assert(is_reason_provider<Propagation<TestAssignmentProvider>, TrivialClause>::value,
              "Propagation<TestAssignmentProvider> should satisfy ReasonProvider, but does not");

TEST(UnitSolver, propagateWithoutClausesIsNoop) {
    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

    size_t amntNewFacts = 0xFFFF;
    CNFLit propagatedLit = ~2_Lit;
    auto conflictingClause = underTest.propagate(propagatedLit, amntNewFacts);

    EXPECT_EQ(amntNewFacts, 0ull);
    EXPECT_EQ(conflictingClause, nullptr);
    EXPECT_FALSE(underTest.hasForcedAssignment(propagatedLit.getVariable()));
}

TEST(UnitSolver, propagateToFixpointWithoutClausesIsNoop) {
    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

    CNFLit propagatedLit = ~2_Lit;
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
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(binaryClause);

    assignments.addAssignment(~lit2);

    size_t amntNewFacts = 0xFFFF;
    auto conflictingClause = underTest.propagate(~lit2, amntNewFacts);
    EXPECT_EQ(conflictingClause, nullptr); // no conflict expected
    EXPECT_EQ(amntNewFacts, 1ull);
    EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBools::FALSE);
}

TEST(UnitSolver, reasonsAreRecordedDuringPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    TrivialClause binaryClause{lit1, lit2};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
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
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(binaryClause);

    assignments.addAssignment(lit1);
    assignments.addAssignment(~lit2);

    size_t amntNewFacts = 0xFFFF;
    auto conflictingClause = underTest.propagate(~lit2, amntNewFacts);
    EXPECT_EQ(conflictingClause, nullptr); // no conflict expected
    EXPECT_EQ(amntNewFacts, 0ull);
    EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBools::FALSE);
    EXPECT_EQ(assignments.getAssignment(CNFVar{2}), TBools::TRUE);
}

TEST(UnitSolver, propagateWithTernaryClause) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause ternaryClause{lit1, lit2, lit3};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(ternaryClause);

    size_t newFacts = 0xFFFF;
    assignments.addAssignment(~lit1);
    underTest.propagate(~lit1, newFacts);
    EXPECT_EQ(newFacts, 0ull);

    assignments.addAssignment(~lit2);
    underTest.propagate(~lit2, newFacts);
    EXPECT_EQ(newFacts, 1ull);
    EXPECT_EQ(assignments.getAssignment(lit3), TBools::TRUE);
}

TEST(UnitSolver, propagateWithTernaryClausesAfterConflict) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause ternaryClause{lit1, lit2, lit3};
    TrivialClause ternaryClause2{lit1, ~lit2, lit3};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
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
    EXPECT_EQ(assignments.getAssignment(lit3), TBools::TRUE);
}

TEST(UnitSolver, registerClauseWithUnassignedLiteralsCausesNoPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause ternaryClause{lit1, lit2, lit3};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(ternaryClause);

    EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBools::INDETERMINATE);
    EXPECT_EQ(assignments.getAssignment(CNFVar{2}), TBools::INDETERMINATE);
    EXPECT_EQ(assignments.getAssignment(CNFVar{3}), TBools::INDETERMINATE);
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
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(ternaryClause);

    EXPECT_EQ(assignments.getAssignment(lit1), TBools::TRUE);
    EXPECT_EQ(assignments.getAssignment(lit2), TBools::FALSE);
    EXPECT_EQ(assignments.getAssignment(lit3), TBools::FALSE);
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
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(firstForcingClause);
    underTest.registerClause(midForcingClause1);
    underTest.registerClause(midForcingClause2);
    underTest.registerClause(lastForcingClause);

    assignments.addAssignment(~lit1);
    auto conflictingClause = underTest.propagateUntilFixpoint(~lit1);

    EXPECT_EQ(conflictingClause, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit1), TBools::FALSE);
    EXPECT_EQ(assignments.getAssignment(lit2), TBools::TRUE);
    EXPECT_EQ(assignments.getAssignment(lit3), TBools::FALSE);
    EXPECT_EQ(assignments.getAssignment(lit4), TBools::TRUE);
    EXPECT_EQ(assignments.getAssignment(lit5), TBools::TRUE);
}

TEST(UnitSolver, propagateUntilFixpointReportsImmediateConflicts) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    TrivialClause binaryClause{lit1, lit2};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
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
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
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
    TrivialClause forcingClause{~10_Lit, 6_Lit};
    Propagation<TestAssignmentProvider> underTest(CNFVar{5}, assignments);
    underTest.increaseMaxVarTo(CNFVar{10});
    underTest.registerClause(forcingClause);
    assignments.addAssignment(10_Lit);
    underTest.propagateUntilFixpoint(10_Lit);
    EXPECT_EQ(assignments.getAssignment(CNFVar{6}), TBools::TRUE);
}

TEST(UnitSolver, propagationDetectsAssignmentReasonClause) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause clause1{lit1, lit2, lit3};
    TrivialClause clause2{lit1, ~lit2, lit3};

    TestAssignmentProvider assignments;
    assignments.setCurrentDecisionLevel(1);
    assignments.setAssignmentDecisionLevel(CNFVar{1}, 0);
    assignments.setAssignmentDecisionLevel(CNFVar{2}, 0);
    assignments.setAssignmentDecisionLevel(CNFVar{3}, 0);

    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(clause1);
    underTest.registerClause(clause2);

    assignments.addAssignment(~lit1);
    underTest.propagateUntilFixpoint(~lit1);
    assignments.addAssignment(~lit2);
    underTest.propagateUntilFixpoint(~lit2);

    auto lit3Reason = underTest.getAssignmentReason(CNFVar{3});
    ASSERT_NE(lit3Reason, nullptr);
    EXPECT_TRUE(underTest.isAssignmentReason(clause1, assignments));
    EXPECT_FALSE(underTest.isAssignmentReason(clause2, assignments));
}


TEST(UnitSolver, propagationDoesNotDetectImpliedFactAssignmentReasonClauseAfterBacktrack) {
    TrivialClause testData{1_Lit, 2_Lit, 3_Lit};

    TestAssignmentProvider assignments;
    Propagation<TestAssignmentProvider> underTest(CNFVar{3}, assignments);
    underTest.registerClause(testData);

    assignments.setCurrentDecisionLevel(0);
    assignments.addAssignment(~1_Lit);
    underTest.propagateUntilFixpoint(~1_Lit);
    assignments.addAssignment(~2_Lit);
    underTest.propagateUntilFixpoint(~2_Lit);

    ASSERT_EQ(assignments.getAssignment(3_Lit), TBools::TRUE);
    EXPECT_EQ(underTest.getAssignmentReason(CNFVar{3}), &testData);
    EXPECT_TRUE(underTest.isAssignmentReason(testData, assignments));

    assignments.popLiteral();
    assignments.popLiteral();
    assignments.popLiteral();

    EXPECT_FALSE(underTest.isAssignmentReason(testData, assignments));
}

namespace {
void test_clearClausesInPropagation() {
    using PropagationTy = Propagation<TestAssignmentProvider>;

    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::POSITIVE};
    CNFLit lit5{CNFVar{5}, CNFSign::POSITIVE};
    CNFLit lit6{CNFVar{6}, CNFSign::POSITIVE};
    TrivialClause clause1{lit1, lit2, lit3, lit4};
    TrivialClause clause2{lit1, lit2, ~lit4};
    TrivialClause clause3{lit5, lit6};

    TestAssignmentProvider assignments;
    CNFVar maxVar{6};
    PropagationTy underTest(maxVar, assignments);
    underTest.registerClause(clause1);
    underTest.registerClause(clause2);
    underTest.registerClause(clause3);


    assignments.addAssignment(~lit5);
    underTest.propagateUntilFixpoint(~lit5);
    ASSERT_EQ(underTest.getAssignmentReason(lit6.getVariable()), &clause3);

    underTest.clear();

    EXPECT_EQ(underTest.getAssignmentReason(lit6.getVariable()), &clause3);

    assignments.addAssignment(~lit1);
    underTest.propagateUntilFixpoint(~lit1);
    assignments.addAssignment(~lit3);
    underTest.propagateUntilFixpoint(~lit3);
    assignments.addAssignment(~lit2);
    auto conflicting = underTest.propagateUntilFixpoint(~lit2);
    EXPECT_EQ(assignments.getAssignment(lit4), TBools::INDETERMINATE);
    EXPECT_EQ(conflicting, nullptr);
}
}

TEST(UnitSolver, clearClausesInPropagation_withReasonsKept) {
    test_clearClausesInPropagation();
}


TEST(UnitSolver, binaryClausesCanBeQueriedInPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};

    TrivialClause c1{lit1, lit2};
    TrivialClause c2{~lit2, lit3};
    TrivialClause c3{~lit2, lit4};

    TestAssignmentProvider assignments;

    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(c1);
    underTest.registerClause(c2);
    underTest.registerClause(c3);

    auto binaryMap = underTest.getBinariesMap();
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

namespace {
void test_shortenedClausesArePropagatedCorrectly(bool withChangeInWatchedLits,
                                                 bool shortenedBeforeFirstPropagation) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
    CNFLit lit5{CNFVar{5}, CNFSign::NEGATIVE};

    TrivialClause c1{lit1, lit2, lit3, lit4};
    TrivialClause c2{lit1, lit2, lit4, lit5};

    TestAssignmentProvider assignments;
    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

    underTest.registerClause(c1);
    underTest.registerClause(c2);

    if (shortenedBeforeFirstPropagation) {
        underTest.notifyClauseModificationAhead(c1);
        c1.resize(3);
        if (withChangeInWatchedLits) {
            std::swap(c1[1], c1[2]);
        }
    }

    assignments.addAssignment(~lit1);
    auto conflict = underTest.propagateUntilFixpoint(~lit1);
    ASSERT_EQ(conflict, nullptr);

    if (!shortenedBeforeFirstPropagation) {
        underTest.notifyClauseModificationAhead(c1);
        c1.resize(3);
        if (withChangeInWatchedLits) {
            std::swap(c1[1], c1[2]);
        }
    }

    assignments.addAssignment(~lit2);
    conflict = underTest.propagateUntilFixpoint(~lit2);
    ASSERT_EQ(conflict, nullptr);

    // The shortened clause now forces the assignment of lit3:
    EXPECT_EQ(assignments.getAssignment(lit3), TBools::TRUE);

    // Check that c2 remains unchanged:
    assignments.addAssignment(~lit4);
    conflict = underTest.propagateUntilFixpoint(~lit4);
    ASSERT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit5), TBools::TRUE);
}
}

TEST(UnitSolver,
     shortenedClausesArePropagatedCorrectly_noChangeInWatchedLits_shortenedAfterRegistration) {
    test_shortenedClausesArePropagatedCorrectly(false, true);
}

TEST(UnitSolver,
     shortenedClausesArePropagatedCorrectly_noChangeInWatchedLits_shortenedAfterPropagation) {
    test_shortenedClausesArePropagatedCorrectly(false, false);
}

TEST(UnitSolver,
     shortenedClausesArePropagatedCorrectly_withChangeInWatchedLits_shortenedAfterRegistration) {
    test_shortenedClausesArePropagatedCorrectly(true, true);
}

TEST(UnitSolver,
     shortenedClausesArePropagatedCorrectly_withChangeInWatchedLits_shortenedAfterPropagation) {
    test_shortenedClausesArePropagatedCorrectly(true, true);
}

TEST(UnitSolver, shortenedClausesArePropagatedCorrectly_withBothWatchedLitsRemoved) {
    TrivialClause testData{1_Lit, 2_Lit, 3_Lit, 4_Lit};

    TestAssignmentProvider assignments;
    CNFVar maxVar{4};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

    underTest.registerClause(testData);
    underTest.notifyClauseModificationAhead(testData);
    testData = TrivialClause{3_Lit, 4_Lit};

    assignments.addAssignment(~3_Lit);
    auto conflict = underTest.propagateUntilFixpoint(~3_Lit);
    ASSERT_EQ(conflict, nullptr);

    EXPECT_EQ(assignments.getAssignment(4_Lit), TBools::TRUE);
}

TEST(UnitSolver, shortenedClausesArePropagatedCorrectly_shortenToBinary) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
    CNFLit lit5{CNFVar{5}, CNFSign::NEGATIVE};

    TrivialClause c1{lit1, lit2, lit3, lit4};
    TrivialClause c2{lit1, ~lit2, lit4, lit5};

    TestAssignmentProvider assignments;
    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

    underTest.registerClause(c1);
    underTest.registerClause(c2);

    underTest.notifyClauseModificationAhead(c1);
    c1.resize(2);

    assignments.addAssignment(~lit1);
    auto conflict = underTest.propagateUntilFixpoint(~lit1);
    ASSERT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit2), TBools::TRUE);

    // Check that c2 remains unchanged:
    assignments.addAssignment(~lit4);
    conflict = underTest.propagateUntilFixpoint(~lit4);
    ASSERT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit5), TBools::TRUE);
}

TEST(UnitSolver, shortenedClausesArePropagatedCorrectly_shortenToBinary_watchersUpdatedCorrectly) {
    // Regression test: Check that the "binarized" clause is
    // inserted into the correct watcher list

    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause c1{lit1, lit2, lit3};

    TestAssignmentProvider assignments;
    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

    underTest.registerClause(c1);

    underTest.notifyClauseModificationAhead(c1);
    c1[1] = c1[2];
    c1.resize(2);

    // lit2 has been removed from the clause, check that its assignment
    // does not cause propagations:
    assignments.addAssignment(~lit2);
    auto conflict = underTest.propagateUntilFixpoint(~lit2);
    EXPECT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit1), TBools::INDETERMINATE);

    // Check that the clause forces assignments as expected:
    assignments.addAssignment(~lit1);
    conflict = underTest.propagateUntilFixpoint(~lit1);
    EXPECT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit3), TBools::TRUE);

    assignments.popLiteral();
    assignments.popLiteral();

    assignments.addAssignment(~lit3);
    conflict = underTest.propagateUntilFixpoint(~lit3);
    EXPECT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit1), TBools::TRUE);
}

TEST(UnitSolver, deletedBinariesAreRemovedFromPropagationAfterAnnounce) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
    TrivialClause c1{lit1, lit2};

    TestAssignmentProvider assignments;
    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

    underTest.registerClause(c1);

    underTest.notifyClauseModificationAhead(c1);
    c1.resize(1ULL);
    c1.setFlag(TrivialClause::Flag::SCHEDULED_FOR_DELETION);

    assignments.addAssignment(~lit2);
    auto conflict = underTest.propagateUntilFixpoint(~lit2);
    EXPECT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit1), TBools::INDETERMINATE);
}

TEST(UnitSolver, deletedNonbinaryClausesAreRemovedFromPropagationAfterAnnounce) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
    CNFLit lit5{CNFVar{5}, CNFSign::NEGATIVE};

    TrivialClause c1{lit1, lit2, lit3};
    TrivialClause c2{lit1, lit2, lit4, lit5};

    TestAssignmentProvider assignments;
    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);

    underTest.registerClause(c1);
    underTest.registerClause(c2);

    underTest.notifyClauseModificationAhead(c1);
    c1.setFlag(TrivialClause::Flag::SCHEDULED_FOR_DELETION);

    assignments.addAssignment(~lit1);
    auto conflict = underTest.propagateUntilFixpoint(~lit1);
    ASSERT_EQ(conflict, nullptr);

    assignments.addAssignment(~lit2);
    conflict = underTest.propagateUntilFixpoint(~lit2);
    ASSERT_EQ(conflict, nullptr);

    // c1 should be removed from propagation now:
    EXPECT_EQ(assignments.getAssignment(lit3), TBools::INDETERMINATE);

    // Check that c2 remains unchanged:
    assignments.addAssignment(~lit4);
    conflict = underTest.propagateUntilFixpoint(~lit4);
    ASSERT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit5), TBools::TRUE);
}

TEST(UnitSolver, redundantClausesAreNotPropagatedInExcludeRedundantMode) {
    CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::NEGATIVE};
    CNFLit lit5{CNFVar{5}, CNFSign::NEGATIVE};
    CNFLit lit6{CNFVar{6}, CNFSign::NEGATIVE};

    TrivialClause c1{lit1, lit2};
    TrivialClause c2{lit1, lit3, lit4};
    TrivialClause c3{lit1, lit5, lit6};

    c1.setFlag(TrivialClause::Flag::REDUNDANT);
    c2.setFlag(TrivialClause::Flag::REDUNDANT);

    TestAssignmentProvider assignments;
    Propagation<TestAssignmentProvider> underTest(CNFVar{6}, assignments);

    underTest.registerClause(c1);
    underTest.registerClause(c2);
    underTest.registerClause(c3);


    // Check that binaries are propagated no matter what their redundancy status is:
    assignments.addAssignment(~lit1);
    auto conflict = underTest.propagateUntilFixpoint(
        ~lit1, Propagation<TestAssignmentProvider>::PropagationMode::EXCLUDE_REDUNDANT_CLAUSES);
    ASSERT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit2), TBools::TRUE);

    // Check that redundant non-binary clauses are not propagated:
    assignments.addAssignment(~lit3);
    conflict = underTest.propagateUntilFixpoint(
        ~lit3, Propagation<TestAssignmentProvider>::PropagationMode::EXCLUDE_REDUNDANT_CLAUSES);
    ASSERT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit4), TBools::INDETERMINATE);

    // Check that the third (non-redundant) clause is not ignored:
    assignments.addAssignment(~lit5);
    conflict = underTest.propagateUntilFixpoint(
        ~lit5, Propagation<TestAssignmentProvider>::PropagationMode::EXCLUDE_REDUNDANT_CLAUSES);
    ASSERT_EQ(conflict, nullptr);
    EXPECT_EQ(assignments.getAssignment(lit6), TBools::TRUE);
}

// TODO: test watcher restoration
}
