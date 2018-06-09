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

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <toolbox/testutils/TestAssignmentProvider.h>
#include <toolbox/testutils/TestReasonProvider.h>

#define JAM_ENABLE_TEST_FAULTS

#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/utils/FaultInjector.h>

namespace jamsat {
using TrivialClause = TestAssignmentProvider::Clause;

namespace {
using DummyReasonProvider = TestReasonProvider<TrivialClause>;

std::vector<CNFLit> createLiterals(CNFVar::RawVariable min, CNFVar::RawVariable max) {
    std::vector<CNFLit> result;

    for (CNFVar::RawVariable i = min; i <= max; ++i) {
        result.emplace_back(CNFVar{i}, CNFSign::POSITIVE);
    }

    return result;
}

bool equalLits(const std::vector<CNFLit> &lhs, const std::vector<CNFLit> &rhs) {
    std::unordered_set<CNFLit> lhsLits;
    for (CNFLit literal : lhs) {
        lhsLits.insert(literal);
    }

    std::unordered_set<CNFLit> rhsLits;
    for (CNFLit literal : rhs) {
        rhsLits.insert(literal);
    }

    return lhsLits == rhsLits;
}
}

// TODO: cover the following scenarios:
// - the algorithm is used again after failing due to a bad_alloc

TEST(UnitSolver, classInvariantsSatisfiedAfterFirstUIPLearningConstruction) {
    TestAssignmentProvider assignments;
    DummyReasonProvider reasons;
    FirstUIPLearning<TestAssignmentProvider, DummyReasonProvider> underTest(CNFVar{10}, assignments,
                                                                            reasons);
    underTest.test_assertClassInvariantsSatisfied();
}

TEST(UnitSolver, firstUIPIsFoundWhenConflictingClauseHas2LitsOnCurLevel) {
    TestAssignmentProvider assignments;
    DummyReasonProvider reasons;

    TrivialClause dummyReasonClause{~3_Lit, ~1_Lit};
    TrivialClause conflictingClause{3_Lit, ~4_Lit, 6_Lit, ~9_Lit};

    assignments.addAssignment(~conflictingClause[1]);
    assignments.addAssignment(~conflictingClause[3]);
    assignments.addAssignment(~dummyReasonClause[1]);
    assignments.addAssignment(~conflictingClause[2]);
    assignments.addAssignment(~conflictingClause[0]);

    assignments.setAssignmentDecisionLevel(CNFVar{4}, 2);
    assignments.setAssignmentDecisionLevel(CNFVar{1}, 3);
    assignments.setAssignmentDecisionLevel(CNFVar{9}, 3);

    assignments.setAssignmentDecisionLevel(CNFVar{3}, 4);
    assignments.setAssignmentDecisionLevel(CNFVar{6}, 4);

    reasons.setAssignmentReason(CNFVar{3}, dummyReasonClause);

    assignments.setCurrentDecisionLevel(4);

    CNFVar maxVar{9};
    FirstUIPLearning<TestAssignmentProvider, DummyReasonProvider> underTest(maxVar, assignments,
                                                                            reasons);
    std::vector<CNFLit> result;
    underTest.computeConflictClause(conflictingClause, result);
    auto expectedClause = std::vector<CNFLit>{
        CNFLit(CNFVar{4}, CNFSign::NEGATIVE), CNFLit(CNFVar{6}, CNFSign::POSITIVE),
        CNFLit(CNFVar{9}, CNFSign::NEGATIVE), CNFLit(CNFVar{1}, CNFSign::NEGATIVE)};

    // Check that the asserting literal is the first one
    auto assertingLiteral = 6_Lit;
    ASSERT_EQ(result.size(), 4ull);
    EXPECT_EQ(result[0], assertingLiteral);
    EXPECT_TRUE(equalLits(result, expectedClause));

    underTest.test_assertClassInvariantsSatisfied();
}

TEST(UnitSolver, firstUIPLearningCallsSeenVariableCallback) {
    TestAssignmentProvider assignments;
    DummyReasonProvider reasons;

    TrivialClause dummyReasonClause{~3_Lit, ~1_Lit};
    TrivialClause conflictingClause{3_Lit, ~4_Lit, 6_Lit, ~9_Lit};

    assignments.addAssignment(~conflictingClause[1]);
    assignments.addAssignment(~conflictingClause[3]);
    assignments.addAssignment(~dummyReasonClause[1]);
    assignments.addAssignment(~conflictingClause[2]);
    assignments.addAssignment(~conflictingClause[0]);

    assignments.setAssignmentDecisionLevel(CNFVar{4}, 2);
    assignments.setAssignmentDecisionLevel(CNFVar{1}, 3);
    assignments.setAssignmentDecisionLevel(CNFVar{9}, 3);

    assignments.setAssignmentDecisionLevel(CNFVar{3}, 4);
    assignments.setAssignmentDecisionLevel(CNFVar{6}, 4);

    reasons.setAssignmentReason(CNFVar{3}, dummyReasonClause);

    assignments.setCurrentDecisionLevel(4);

    CNFVar maxVar{9};
    FirstUIPLearning<TestAssignmentProvider, DummyReasonProvider> underTest(maxVar, assignments,
                                                                            reasons);
    std::vector<CNFVar> seenVars;
    underTest.setOnSeenVariableCallback(
        [&seenVars](CNFVar seenVar) { seenVars.push_back(seenVar); });
    std::vector<CNFLit> result;
    underTest.computeConflictClause(conflictingClause, result);

    EXPECT_EQ(seenVars.size(), 5ull);
    EXPECT_NE(std::find(seenVars.begin(), seenVars.end(), CNFVar{1}), seenVars.end());
    EXPECT_NE(std::find(seenVars.begin(), seenVars.end(), CNFVar{3}), seenVars.end());
    EXPECT_NE(std::find(seenVars.begin(), seenVars.end(), CNFVar{4}), seenVars.end());
    EXPECT_NE(std::find(seenVars.begin(), seenVars.end(), CNFVar{6}), seenVars.end());
    EXPECT_NE(std::find(seenVars.begin(), seenVars.end(), CNFVar{9}), seenVars.end());
}

TEST(UnitSolver, firstUIPIsFoundWhenAssertingLiteralHasBeenPropagated) {
    CNFLit decisionLit{CNFVar{0}, CNFSign::POSITIVE};
    CNFLit assertingLit{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit prop1{CNFVar{2}, CNFSign::NEGATIVE};
    CNFLit prop2{CNFVar{3}, CNFSign::NEGATIVE};

    std::vector<CNFLit> filler = createLiterals(4, 7);

    TrivialClause clause1{~decisionLit, assertingLit, ~filler[0]};
    TrivialClause clause2{~filler[1], ~assertingLit, prop1};
    TrivialClause clause3{~filler[2], ~filler[3], ~assertingLit, prop2};
    TrivialClause conflictingClause{~prop1, ~prop2};

    TestAssignmentProvider assignments;
    DummyReasonProvider reasons;

    for (auto lit : filler) {
        assignments.addAssignment(lit);
        assignments.setAssignmentDecisionLevel(lit.getVariable(), 1);
    }

    assignments.addAssignment(decisionLit);
    assignments.setAssignmentDecisionLevel(decisionLit.getVariable(), 2);
    assignments.addAssignment(assertingLit);
    assignments.setAssignmentDecisionLevel(assertingLit.getVariable(), 2);
    reasons.setAssignmentReason(assertingLit.getVariable(), clause1);
    assignments.addAssignment(prop1);
    assignments.setAssignmentDecisionLevel(prop1.getVariable(), 2);
    reasons.setAssignmentReason(prop1.getVariable(), clause2);
    assignments.addAssignment(prop2);
    assignments.setAssignmentDecisionLevel(prop2.getVariable(), 2);
    reasons.setAssignmentReason(prop2.getVariable(), clause3);

    assignments.setCurrentDecisionLevel(2);

    CNFVar maxVar{7};
    FirstUIPLearning<TestAssignmentProvider, DummyReasonProvider> underTest(maxVar, assignments,
                                                                            reasons);
    std::vector<CNFLit> result;
    underTest.computeConflictClause(conflictingClause, result);
    auto expectedClause = std::vector<CNFLit>{~filler[1], ~filler[2], ~filler[3], ~assertingLit};

    // Check that the asserting literal is the first one
    ASSERT_EQ(result.size(), 4ull);
    EXPECT_EQ(result[0], ~assertingLit);
    EXPECT_TRUE(equalLits(result, expectedClause));

    underTest.test_assertClassInvariantsSatisfied();
}

namespace {
void test_firstUIPIsFoundWhenAllLiteralsAreOnSameLevel(bool simulateOOM) {
    FaultInjectorResetRAII faultInjectorResetter;

    if (simulateOOM) {
        FaultInjector::getInstance().enableFaults("FirstUIPLearning/low_memory");
    }

    CNFLit decisionLit{CNFVar{0}, CNFSign::POSITIVE};
    CNFLit intermediateLit{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit conflLit{CNFVar{2}, CNFSign::NEGATIVE};

    TrivialClause clause1{~decisionLit, intermediateLit};
    TrivialClause clause2{~intermediateLit, conflLit};
    TrivialClause conflictingClause{~decisionLit, ~intermediateLit, ~conflLit};

    TestAssignmentProvider assignments;
    DummyReasonProvider reasons;

    assignments.addAssignment(decisionLit);
    assignments.setAssignmentDecisionLevel(decisionLit.getVariable(), 1);
    assignments.addAssignment(intermediateLit);
    assignments.setAssignmentDecisionLevel(intermediateLit.getVariable(), 1);
    reasons.setAssignmentReason(intermediateLit.getVariable(), clause1);
    assignments.addAssignment(conflLit);
    assignments.setAssignmentDecisionLevel(conflLit.getVariable(), 1);
    reasons.setAssignmentReason(conflLit.getVariable(), clause2);
    assignments.setCurrentDecisionLevel(1);

    CNFVar maxVar{2};
    FirstUIPLearning<TestAssignmentProvider, DummyReasonProvider> underTest(maxVar, assignments,
                                                                            reasons);

    std::vector<CNFLit> result;
    if (!simulateOOM) {
        underTest.computeConflictClause(conflictingClause, result);
        // Check that the asserting literal is the first one
        ASSERT_EQ(result.size(), 1ull);
        EXPECT_EQ(result[0], ~decisionLit);
    } else {
        try {
            underTest.computeConflictClause(conflictingClause, result);
            FAIL() << "Expected a bad_alloc exception to be thrown";
        } catch (const std::bad_alloc &exception) {
        } catch (...) {
            FAIL() << "Catched exception of unexpected type";
        }
    }

    underTest.test_assertClassInvariantsSatisfied();
}
}

TEST(UnitSolver, firstUIPIsFoundWhenAllLiteralsAreOnSameLevel) {
    test_firstUIPIsFoundWhenAllLiteralsAreOnSameLevel(false);
}

TEST(UnitSolver, firstUIPLearningSatisfiesClassInvariantsAfterOutOfMemory) {
    test_firstUIPIsFoundWhenAllLiteralsAreOnSameLevel(true);
}

TEST(UnitSolver, firstUIPIsFoundWhenAssertingLiteralIsDecisionLiteral) {
    // This is the waerden resolution example found in Knuth, TAOCP, chapter
    // 7.2.2.2

    TestAssignmentProvider assignments;
    DummyReasonProvider reasons;

    TrivialClause waerden1{3_Lit, 9_Lit, 6_Lit};

    TrivialClause waerden2{5_Lit, 4_Lit, 6_Lit};

    TrivialClause waerden3{8_Lit, 4_Lit, 6_Lit};

    TrivialClause waerden4{2_Lit, 4_Lit, 6_Lit};

    TrivialClause waerden5{~7_Lit, ~5_Lit, ~3_Lit};

    TrivialClause waerden6{~2_Lit, ~5_Lit, ~8_Lit};

    assignments.addAssignment(~6_Lit);
    assignments.setAssignmentDecisionLevel(CNFVar{6}, 1);

    assignments.addAssignment(~9_Lit);
    assignments.setAssignmentDecisionLevel(CNFVar{9}, 2);

    assignments.addAssignment(3_Lit);
    assignments.setAssignmentDecisionLevel(CNFVar{3}, 2);
    reasons.setAssignmentReason(CNFVar{3}, waerden1);

    assignments.addAssignment(~4_Lit);
    assignments.setAssignmentDecisionLevel(CNFVar{4}, 3);

    assignments.addAssignment(5_Lit);
    assignments.setAssignmentDecisionLevel(CNFVar{5}, 3);
    reasons.setAssignmentReason(CNFVar{5}, waerden2);

    assignments.addAssignment(8_Lit);
    assignments.setAssignmentDecisionLevel(CNFVar{8}, 3);
    reasons.setAssignmentReason(CNFVar{8}, waerden3);

    assignments.addAssignment(2_Lit);
    assignments.setAssignmentDecisionLevel(CNFVar{2}, 3);
    reasons.setAssignmentReason(CNFVar{2}, waerden4);

    assignments.addAssignment(~7_Lit);
    assignments.setAssignmentDecisionLevel(CNFVar{7}, 3);
    reasons.setAssignmentReason(CNFVar{7}, waerden5);

    assignments.setCurrentDecisionLevel(3);

    CNFVar maxVar{16};
    FirstUIPLearning<TestAssignmentProvider, DummyReasonProvider> underTest(maxVar, assignments,
                                                                            reasons);

    std::vector<CNFLit> conflictClause;
    underTest.computeConflictClause(waerden6, conflictClause);
    auto expectedClause = std::vector<CNFLit>{CNFLit(CNFVar(4), CNFSign::POSITIVE),
                                              CNFLit(CNFVar(6), CNFSign::POSITIVE)};
    ASSERT_EQ(conflictClause.size(), 2ull);
    EXPECT_EQ(conflictClause[0], CNFLit(CNFVar(4), CNFSign::POSITIVE));
    EXPECT_EQ(conflictClause, expectedClause);

    underTest.test_assertClassInvariantsSatisfied();
}
}
