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

#include <unordered_map>
#include <unordered_set>

#include "TestAssignmentProvider.h"
#include <libjamsat/solver/Clause.h>
#include <libjamsat/solver/FirstUIPLearning.h>

namespace jamsat {
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

TEST(UnitSolver, firstUIPLearningWithEmptyClauseProducesEmptyClause) {
  TestAssignmentProvider assignments;
  TestReasonProvider reasons;

  CNFVar maxVar{16};
  auto emptyClause = createHeapClause(0);
  FirstUIPLearning<TestAssignmentProvider, TestReasonProvider> underTest(
      maxVar, assignments, reasons);

  auto conflictClause = underTest.computeConflictClause(*emptyClause);
  EXPECT_TRUE(conflictClause.empty());
}

namespace {
std::unique_ptr<Clause> createClause(const std::vector<CNFLit> &literals) {
  auto result = createHeapClause(literals.size());
  for (Clause::size_type i = 0; i < literals.size(); ++i) {
    (*result)[i] = literals[i];
  }
  return result;
}

std::vector<CNFLit> createLiterals(int min, int max) {
  std::vector<CNFLit> result;

  for (int i = min; i <= max; ++i) {
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

TEST(UnitSolver,
     firstUIPLearningWithSinglePropagationOnCurrentLevelIs1ResolutionStep) {
  TestAssignmentProvider assignments;
  TestReasonProvider reasons;

  auto lit = createLiterals(0, 16);

  CNFVar maxVar{16};
  FirstUIPLearning<TestAssignmentProvider, TestReasonProvider> underTest(
      maxVar, assignments, reasons);

  auto unrelatedClause = createClause({lit[0], lit[1], ~lit[4]});
  auto distantReasonClause = createClause({lit[6], ~lit[1], lit[3], lit[7]});

  auto conflictingClause = createClause({lit[10], lit[15], lit[16]});
  auto reasonClause = createClause({lit[10], ~lit[1], ~lit[15]});

  // Situation: ~lit[10] has been decided on level 3,
  // ~lit[15] was a subsequent propagation,
  // lit[16] has been assigned on level 2, all other on level 1.

  for (auto literal : lit) {
    assignments.setDecisionLevel(literal.getVariable(), 1);
  }
  assignments.setDecisionLevel(lit[10].getVariable(), 3);
  assignments.setDecisionLevel(lit[15].getVariable(), 3);
  assignments.setDecisionLevel(lit[16].getVariable(), 2);
  reasons.setAssignmentReason(CNFVar{15}, *reasonClause);
  reasons.setAssignmentReason(CNFVar{1}, *distantReasonClause);
  reasons.setAssignmentReason(CNFVar{6}, *unrelatedClause);

  auto conflictClause = underTest.computeConflictClause(*conflictingClause);
  EXPECT_EQ(conflictClause.size(), 3ull);
  EXPECT_TRUE(equalLits(conflictClause, {lit[10], lit[16], ~lit[1]}));
}
}
