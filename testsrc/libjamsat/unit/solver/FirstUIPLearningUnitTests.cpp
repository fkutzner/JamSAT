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

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "TestAssignmentProvider.h"
#include <libjamsat/solver/Clause.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/utils/FaultInjector.h>

namespace jamsat {
namespace {
class TestReasonProvider {
public:
  void setAssignmentReason(CNFVar variable, Clause &reason) noexcept {
    m_reasons[variable] = &reason;
  }

  const Clause *getAssignmentReason(CNFVar variable) const noexcept {
    auto reason = m_reasons.find(variable);
    if (reason != m_reasons.end()) {
      return reason->second;
    }
    return nullptr;
  }

private:
  std::unordered_map<CNFVar, const Clause *> m_reasons;
};

std::unique_ptr<Clause> createClause(const std::vector<CNFLit> &literals) {
  auto result = createHeapClause(literals.size());
  for (Clause::size_type i = 0; i < literals.size(); ++i) {
    (*result)[i] = literals[i];
  }
  return result;
}

std::vector<CNFLit> createLiterals(CNFVar::RawVariable min,
                                   CNFVar::RawVariable max) {
  std::vector<CNFLit> result;

  for (CNFVar::RawVariable i = min; i <= max; ++i) {
    result.push_back(CNFLit{CNFVar{i}, CNFSign::POSITIVE});
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
  TestReasonProvider reasons;
  FirstUIPLearning<TestAssignmentProvider, TestReasonProvider, Clause>
      underTest(CNFVar{10}, assignments, reasons);
  underTest.test_assertClassInvariantsSatisfied();
}

TEST(UnitSolver, firstUIPIsFoundWhenConflictingClauseHas2LitsOnCurLevel) {
  TestAssignmentProvider assignments;
  TestReasonProvider reasons;

  auto dummyReasonClause = createHeapClause(2);
  (*dummyReasonClause)[0] = CNFLit{CNFVar{3}, CNFSign::NEGATIVE};
  (*dummyReasonClause)[1] = CNFLit{CNFVar{1}, CNFSign::NEGATIVE};

  auto conflictingClause = createHeapClause(4);
  (*conflictingClause)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
  (*conflictingClause)[1] = CNFLit{CNFVar{4}, CNFSign::NEGATIVE};
  (*conflictingClause)[2] = CNFLit{CNFVar{6}, CNFSign::POSITIVE};
  (*conflictingClause)[3] = CNFLit{CNFVar{9}, CNFSign::NEGATIVE};

  assignments.addAssignment(~(*conflictingClause)[1]);
  assignments.addAssignment(~(*conflictingClause)[3]);
  assignments.addAssignment(~(*dummyReasonClause)[1]);
  assignments.addAssignment(~(*conflictingClause)[0]);
  assignments.addAssignment(~(*conflictingClause)[2]);

  assignments.setAssignmentDecisionLevel(CNFVar{4}, 2);
  assignments.setAssignmentDecisionLevel(CNFVar{1}, 3);
  assignments.setAssignmentDecisionLevel(CNFVar{9}, 3);

  assignments.setAssignmentDecisionLevel(CNFVar{3}, 4);
  assignments.setAssignmentDecisionLevel(CNFVar{6}, 4);

  reasons.setAssignmentReason(CNFVar{3}, *dummyReasonClause);

  assignments.setCurrentDecisionLevel(4);

  CNFVar maxVar{9};
  FirstUIPLearning<TestAssignmentProvider, TestReasonProvider, Clause>
      underTest(maxVar, assignments, reasons);
  auto result = underTest.computeConflictClause(*conflictingClause);
  auto expectedClause =
      std::vector<CNFLit>{CNFLit(CNFVar{4}, CNFSign::NEGATIVE),
                          CNFLit(CNFVar{6}, CNFSign::POSITIVE),
                          CNFLit(CNFVar{9}, CNFSign::NEGATIVE),
                          CNFLit(CNFVar{1}, CNFSign::NEGATIVE)};

  // Check that the asserting literal is the first one
  auto assertingLiteral = CNFLit{CNFVar{6}, CNFSign::POSITIVE};
  ASSERT_EQ(result.size(), 4ull);
  EXPECT_EQ(result[0], assertingLiteral);
  EXPECT_TRUE(equalLits(result, expectedClause));

  underTest.test_assertClassInvariantsSatisfied();
}

TEST(UnitSolver, firstUIPIsFoundWhenAssertingLiteralHasBeenPropagated) {
  CNFLit decisionLit{CNFVar{0}, CNFSign::POSITIVE};
  CNFLit assertingLit{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit prop1{CNFVar{2}, CNFSign::NEGATIVE};
  CNFLit prop2{CNFVar{3}, CNFSign::NEGATIVE};

  std::vector<CNFLit> filler = createLiterals(4, 7);

  auto clause1 = createClause({~decisionLit, assertingLit, ~filler[0]});
  auto clause2 = createClause({~filler[1], ~assertingLit, prop1});
  auto clause3 = createClause({~filler[2], ~filler[3], ~assertingLit, prop2});
  auto conflictingClause = createClause({~prop1, ~prop2});

  TestAssignmentProvider assignments;
  TestReasonProvider reasons;

  for (auto lit : filler) {
    assignments.addAssignment(lit);
    assignments.setAssignmentDecisionLevel(lit.getVariable(), 1);
  }

  assignments.addAssignment(decisionLit);
  assignments.setAssignmentDecisionLevel(decisionLit.getVariable(), 2);
  assignments.addAssignment(assertingLit);
  assignments.setAssignmentDecisionLevel(assertingLit.getVariable(), 2);
  reasons.setAssignmentReason(assertingLit.getVariable(), *clause1);
  assignments.addAssignment(prop1);
  assignments.setAssignmentDecisionLevel(prop1.getVariable(), 2);
  reasons.setAssignmentReason(prop1.getVariable(), *clause2);
  assignments.addAssignment(prop2);
  assignments.setAssignmentDecisionLevel(prop2.getVariable(), 2);
  reasons.setAssignmentReason(prop2.getVariable(), *clause3);

  assignments.setCurrentDecisionLevel(2);

  CNFVar maxVar{7};
  FirstUIPLearning<TestAssignmentProvider, TestReasonProvider, Clause>
      underTest(maxVar, assignments, reasons);
  auto result = underTest.computeConflictClause(*conflictingClause);
  auto expectedClause =
      std::vector<CNFLit>{~filler[1], ~filler[2], ~filler[3], ~assertingLit};

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

  auto clause1 = createClause({~decisionLit, intermediateLit});
  auto clause2 = createClause({~intermediateLit, conflLit});
  auto conflictingClause =
      createClause({~decisionLit, ~intermediateLit, ~conflLit});

  TestAssignmentProvider assignments;
  TestReasonProvider reasons;

  assignments.addAssignment(decisionLit);
  assignments.setAssignmentDecisionLevel(decisionLit.getVariable(), 1);
  assignments.addAssignment(intermediateLit);
  assignments.setAssignmentDecisionLevel(intermediateLit.getVariable(), 1);
  reasons.setAssignmentReason(intermediateLit.getVariable(), *clause1);
  assignments.addAssignment(conflLit);
  assignments.setAssignmentDecisionLevel(conflLit.getVariable(), 1);
  reasons.setAssignmentReason(conflLit.getVariable(), *clause2);
  assignments.setCurrentDecisionLevel(1);

  CNFVar maxVar{2};
  FirstUIPLearning<TestAssignmentProvider, TestReasonProvider, Clause>
      underTest(maxVar, assignments, reasons);

  if (!simulateOOM) {
    auto result = underTest.computeConflictClause(*conflictingClause);
    // Check that the asserting literal is the first one
    ASSERT_EQ(result.size(), 1ull);
    EXPECT_EQ(result[0], ~decisionLit);
  } else {
    try {
      underTest.computeConflictClause(*conflictingClause);
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
  TestReasonProvider reasons;

  auto waerden1 = createHeapClause(3);
  (*waerden1)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
  (*waerden1)[1] = CNFLit{CNFVar{9}, CNFSign::POSITIVE};
  (*waerden1)[2] = CNFLit{CNFVar{6}, CNFSign::POSITIVE};

  auto waerden2 = createHeapClause(3);
  (*waerden2)[0] = CNFLit{CNFVar{5}, CNFSign::POSITIVE};
  (*waerden2)[1] = CNFLit{CNFVar{4}, CNFSign::POSITIVE};
  (*waerden2)[2] = CNFLit{CNFVar{6}, CNFSign::POSITIVE};

  auto waerden3 = createHeapClause(3);
  (*waerden3)[0] = CNFLit{CNFVar{8}, CNFSign::POSITIVE};
  (*waerden3)[1] = CNFLit{CNFVar{4}, CNFSign::POSITIVE};
  (*waerden3)[2] = CNFLit{CNFVar{6}, CNFSign::POSITIVE};

  auto waerden4 = createHeapClause(3);
  (*waerden4)[0] = CNFLit{CNFVar{2}, CNFSign::POSITIVE};
  (*waerden4)[1] = CNFLit{CNFVar{4}, CNFSign::POSITIVE};
  (*waerden4)[2] = CNFLit{CNFVar{6}, CNFSign::POSITIVE};

  auto waerden5 = createHeapClause(3);
  (*waerden5)[0] = CNFLit{CNFVar{7}, CNFSign::NEGATIVE};
  (*waerden5)[1] = CNFLit{CNFVar{5}, CNFSign::NEGATIVE};
  (*waerden5)[2] = CNFLit{CNFVar{3}, CNFSign::NEGATIVE};

  auto waerden6 = createHeapClause(3);
  (*waerden6)[0] = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
  (*waerden6)[1] = CNFLit{CNFVar{5}, CNFSign::NEGATIVE};
  (*waerden6)[2] = CNFLit{CNFVar{8}, CNFSign::NEGATIVE};

  assignments.addAssignment(CNFLit{CNFVar{6}, CNFSign::NEGATIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{6}, 1);

  assignments.addAssignment(CNFLit{CNFVar{9}, CNFSign::NEGATIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{9}, 2);

  assignments.addAssignment(CNFLit{CNFVar{3}, CNFSign::POSITIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{3}, 2);
  reasons.setAssignmentReason(CNFVar{3}, *waerden1);

  assignments.addAssignment(CNFLit{CNFVar{4}, CNFSign::NEGATIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{4}, 3);

  assignments.addAssignment(CNFLit{CNFVar{5}, CNFSign::POSITIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{5}, 3);
  reasons.setAssignmentReason(CNFVar{5}, *waerden2);

  assignments.addAssignment(CNFLit{CNFVar{8}, CNFSign::POSITIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{8}, 3);
  reasons.setAssignmentReason(CNFVar{8}, *waerden3);

  assignments.addAssignment(CNFLit{CNFVar{2}, CNFSign::POSITIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{2}, 3);
  reasons.setAssignmentReason(CNFVar{2}, *waerden4);

  assignments.addAssignment(CNFLit{CNFVar{7}, CNFSign::NEGATIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{7}, 3);
  reasons.setAssignmentReason(CNFVar{7}, *waerden5);

  assignments.setCurrentDecisionLevel(3);

  CNFVar maxVar{16};
  FirstUIPLearning<TestAssignmentProvider, TestReasonProvider, Clause>
      underTest(maxVar, assignments, reasons);

  auto conflictClause = underTest.computeConflictClause(*waerden6);
  auto expectedClause =
      std::vector<CNFLit>{CNFLit(CNFVar(4), CNFSign::POSITIVE),
                          CNFLit(CNFVar(6), CNFSign::POSITIVE)};
  ASSERT_EQ(conflictClause.size(), 2ull);
  EXPECT_EQ(conflictClause[0], CNFLit(CNFVar(4), CNFSign::POSITIVE));
  EXPECT_EQ(conflictClause, expectedClause);

  underTest.test_assertClassInvariantsSatisfied();
}
}
