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

#include <libjamsat/solver/Propagation.h>
#include <toolbox/testutils/RangeUtils.h>
#include <toolbox/testutils/TestAssignmentProvider.h>

namespace jamsat {
using TrivialClause = jamsat::TestAssignmentProvider::Clause;

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
    TrivialClause forcingClause{CNFLit{CNFVar{10}, CNFSign::NEGATIVE},
                                CNFLit{CNFVar{6}, CNFSign::POSITIVE}};
    Propagation<TestAssignmentProvider> underTest(CNFVar{5}, assignments);
    underTest.increaseMaxVarTo(CNFVar{10});
    underTest.registerClause(forcingClause);
    assignments.addAssignment(CNFLit{CNFVar{10}, CNFSign::POSITIVE});
    underTest.propagateUntilFixpoint(CNFLit{CNFVar{10}, CNFSign::POSITIVE});
    EXPECT_EQ(assignments.getAssignment(CNFVar{6}), TBools::TRUE);
}

TEST(UnitSolver, propagationClauseRangeEmptyWhenNoClausesAdded) {
    TestAssignmentProvider assignments;
    Propagation<TestAssignmentProvider> underTest(CNFVar{5}, assignments);
    auto clauses = underTest.getClausesInPropagationOrder();
    EXPECT_TRUE(clauses.begin() == clauses.end());
}

TEST(UnitSolver, propagationClauseRangeHasCorrectOrderForNonBinaryClauses) {
    TrivialClause c1{CNFLit{CNFVar{2}, CNFSign::POSITIVE}, CNFLit{CNFVar{10}, CNFSign::POSITIVE},
                     CNFLit{CNFVar{5}, CNFSign::POSITIVE}};
    TrivialClause c2{CNFLit{CNFVar{0}, CNFSign::NEGATIVE}, CNFLit{CNFVar{10}, CNFSign::NEGATIVE},
                     CNFLit{CNFVar{5}, CNFSign::POSITIVE}};
    TrivialClause c3{CNFLit{CNFVar{1}, CNFSign::POSITIVE}, CNFLit{CNFVar{11}, CNFSign::NEGATIVE},
                     CNFLit{CNFVar{5}, CNFSign::POSITIVE}};

    TestAssignmentProvider assignments;
    Propagation<TestAssignmentProvider> underTest(CNFVar{15}, assignments);
    underTest.registerClause(c1);
    underTest.registerClause(c2);
    underTest.registerClause(c3);

    auto clauses = underTest.getClausesInPropagationOrder();
    // Note: the concrete ordering of clauses is a "hidden" implementation detail because
    // it depends on implementation details of the Watcher implementation.
    expectRangeElementsSequencedEqual(clauses,
                                      std::vector<TrivialClause *>{&c2, &c3, &c1, &c2, &c1, &c3});
}

TEST(UnitSolver, propagationClauseRangeHasCorrectOrderForMixedNonBinaryAndBinaryClauses) {
    TrivialClause c1{CNFLit{CNFVar{2}, CNFSign::POSITIVE}, CNFLit{CNFVar{10}, CNFSign::POSITIVE},
                     CNFLit{CNFVar{5}, CNFSign::POSITIVE}};
    TrivialClause c2{CNFLit{CNFVar{0}, CNFSign::NEGATIVE}, CNFLit{CNFVar{10}, CNFSign::NEGATIVE},
                     CNFLit{CNFVar{5}, CNFSign::POSITIVE}};
    TrivialClause c3{CNFLit{CNFVar{1}, CNFSign::POSITIVE}, CNFLit{CNFVar{11}, CNFSign::NEGATIVE},
                     CNFLit{CNFVar{5}, CNFSign::POSITIVE}};

    TrivialClause c4{CNFLit{CNFVar{1}, CNFSign::POSITIVE}, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
    TrivialClause c5{CNFLit{CNFVar{2}, CNFSign::POSITIVE}, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};

    TestAssignmentProvider assignments;
    Propagation<TestAssignmentProvider> underTest(CNFVar{15}, assignments);
    underTest.registerClause(c1);
    underTest.registerClause(c2);
    underTest.registerClause(c3);
    underTest.registerClause(c4);
    underTest.registerClause(c5);

    auto clauses = underTest.getClausesInPropagationOrder();
    // Note: the concrete ordering of clauses is a "hidden" implementation detail because
    // it depends on implementation details of the Watcher implementation.
    expectRangeElementsSequencedEqual(
        clauses, std::vector<TrivialClause *>{&c2, &c3, &c1, &c2, &c1, &c3, &c4, &c5, &c5, &c4});
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

TEST(UnitSolver, replacementOfReasonClauseInPropagationSucceeds) {
    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::POSITIVE};
    TrivialClause clause1{lit1, lit2, lit3};

    TestAssignmentProvider assignments;

    CNFVar maxVar{5};
    Propagation<TestAssignmentProvider> underTest(maxVar, assignments);
    underTest.registerClause(clause1);

    assignments.addAssignment(~lit2);
    underTest.propagateUntilFixpoint(~lit2);
    assignments.addAssignment(~lit1);
    underTest.propagateUntilFixpoint(~lit1);

    TrivialClause clause1Replacement = clause1;
    underTest.updateAssignmentReason(clause1, clause1Replacement);
    EXPECT_EQ(underTest.getAssignmentReason(CNFVar{3}), &clause1Replacement);
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

namespace {
enum class SubstitutionClauseReinsertionTestMode { TEST_NO_PROPAGATION, TEST_PRESENCE };

void substitutionClauseReinsertionTest(SubstitutionClauseReinsertionTestMode mode) {
    using PropagationTy = Propagation<TestAssignmentProvider>;

    CNFLit lit1{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit lit2{CNFVar{2}, CNFSign::POSITIVE};
    CNFLit lit3{CNFVar{3}, CNFSign::NEGATIVE};
    CNFLit lit4{CNFVar{4}, CNFSign::POSITIVE};
    TrivialClause clause1{lit2, lit1, lit3, lit4};
    TrivialClause clause2{lit1, lit2, ~lit4};

    TestAssignmentProvider assignments;
    CNFVar maxVar{6};
    PropagationTy underTest(maxVar, assignments);

    assignments.addAssignment(lit4);
    assignments.addAssignment(~lit2);
    underTest.registerClause(clause1);
    underTest.registerClause(clause2);

    // clause2 should have forced the assignment of lit1:
    ASSERT_EQ(assignments.getAssignment(CNFVar{lit1.getVariable()}), TBools::TRUE);

    // Simulate backtracking. Later, it is checked that lit1 has no forced assignment.
    assignments.popLiteral();
    underTest.clear();
    underTest.registerEquivalentSubstitutingClause(clause2);

    if (mode == SubstitutionClauseReinsertionTestMode::TEST_NO_PROPAGATION) {
        EXPECT_EQ(assignments.getAssignment(CNFVar{lit1.getVariable()}), TBools::INDETERMINATE);
        return;
    }

    ASSERT_EQ(assignments.getAssignment(CNFVar{lit1.getVariable()}), TBools::INDETERMINATE);
    // Test that the clause is present: provoke a conflict
    assignments.addAssignment(~lit1);
    auto result = underTest.propagateUntilFixpoint(~lit1);
    EXPECT_EQ(result, &clause2);
}
}

TEST(UnitSolver, reregisteringEquivalentClauseDoesNotCausePropagation) {
    substitutionClauseReinsertionTest(SubstitutionClauseReinsertionTestMode::TEST_NO_PROPAGATION);
}

TEST(UnitSolver, clausesArePresentAfterReregistration) {
    substitutionClauseReinsertionTest(SubstitutionClauseReinsertionTestMode::TEST_PRESENCE);
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
                                    binariesWithNLit2FwdRange.end(), expectedForNLit2.begin()));

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

// TODO: test watcher restoration
}
