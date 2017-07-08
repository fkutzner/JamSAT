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

TEST(UnitSolver, firstUIPLearningKnuthTest1) {
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

  assignments.addLiteral(CNFLit{CNFVar{6}, CNFSign::NEGATIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{6}, 1);

  assignments.addLiteral(CNFLit{CNFVar{9}, CNFSign::NEGATIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{9}, 2);

  assignments.addLiteral(CNFLit{CNFVar{3}, CNFSign::POSITIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{3}, 2);
  reasons.setAssignmentReason(CNFVar{3}, *waerden1);

  assignments.addLiteral(CNFLit{CNFVar{4}, CNFSign::NEGATIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{4}, 3);

  assignments.addLiteral(CNFLit{CNFVar{5}, CNFSign::POSITIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{5}, 3);
  reasons.setAssignmentReason(CNFVar{5}, *waerden2);

  assignments.addLiteral(CNFLit{CNFVar{8}, CNFSign::POSITIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{8}, 3);
  reasons.setAssignmentReason(CNFVar{8}, *waerden3);

  assignments.addLiteral(CNFLit{CNFVar{2}, CNFSign::POSITIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{2}, 3);
  reasons.setAssignmentReason(CNFVar{2}, *waerden4);

  assignments.addLiteral(CNFLit{CNFVar{7}, CNFSign::NEGATIVE});
  assignments.setAssignmentDecisionLevel(CNFVar{7}, 3);
  reasons.setAssignmentReason(CNFVar{7}, *waerden5);

  assignments.setCurrentDecisionLevel(3);

  CNFVar maxVar{16};
  FirstUIPLearning<TestAssignmentProvider, TestReasonProvider> underTest(
      maxVar, assignments, reasons);

  auto conflictClause = underTest.computeConflictClause(*waerden6);
  auto expectedClause =
      std::vector<CNFLit>{CNFLit(CNFVar(4), CNFSign::POSITIVE),
                          CNFLit(CNFVar(6), CNFSign::POSITIVE)};
  EXPECT_EQ(conflictClause, expectedClause);
}
}
