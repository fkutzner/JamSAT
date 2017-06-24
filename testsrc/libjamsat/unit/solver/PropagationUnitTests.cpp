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

#include <libjamsat/solver/Propagation.h>

namespace jamsat {
class DummyAssignmentProvider {
public:
  TBool getAssignment(CNFVar variable) const noexcept {
    auto possibleAssgn = m_assignments.find(variable);
    if (possibleAssgn != m_assignments.end()) {
      return possibleAssgn->second;
    }
    return TBool::INDETERMINATE;
  }

  TBool getAssignment(CNFLit literal) {
    auto varAssgn = getAssignment(literal.getVariable());
    if (literal.getSign() == CNFSign::POSITIVE ||
        varAssgn == TBool::INDETERMINATE) {
      return varAssgn;
    }
    return varAssgn == TBool::FALSE ? TBool::TRUE : TBool::FALSE;
  }

  void addLiteral(CNFLit literal) {
    m_assignments[literal.getVariable()] =
        (literal.getSign() == CNFSign::POSITIVE ? TBool::TRUE : TBool::FALSE);
  }

  void clearLiteral(CNFLit literal) {
    m_assignments.erase(literal.getVariable());
  }

private:
  std::unordered_map<CNFVar, TBool> m_assignments;
};

TEST(UnitSolver, propagateWithoutClausesIsNoop) {
  DummyAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<DummyAssignmentProvider> underTest(maxVar, assignments);

  size_t amntNewFacts = 0xFFFF;
  CNFLit propagatedLit = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
  auto conflictingClause = underTest.propagate(propagatedLit, amntNewFacts);

  EXPECT_EQ(amntNewFacts, 0ull);
  EXPECT_EQ(conflictingClause, nullptr);
  EXPECT_FALSE(underTest.hasForcedAssignment(propagatedLit.getVariable()));
}

TEST(UnitSolver, propagateToFixpointWithoutClausesIsNoop) {
  DummyAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<DummyAssignmentProvider> underTest(maxVar, assignments);

  size_t amntNewFacts = 0xFFFF;
  CNFLit propagatedLit = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
  auto conflictingClause =
      underTest.propagateUntilFixpoint(propagatedLit, amntNewFacts);

  EXPECT_EQ(amntNewFacts, 0ull);
  EXPECT_EQ(conflictingClause, nullptr);
  EXPECT_FALSE(underTest.hasForcedAssignment(propagatedLit.getVariable()));
}

TEST(UnitSolver, falsingSingleLiteralInBinaryClauseCausesPropagation) {
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binaryClause = createHeapClause(2);
  (*binaryClause)[0] = lit1;
  (*binaryClause)[1] = lit2;

  DummyAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<DummyAssignmentProvider> underTest(maxVar, assignments);
  underTest.registerClause(*binaryClause);

  assignments.addLiteral(~lit2);

  size_t amntNewFacts = 0xFFFF;
  auto conflictingClause = underTest.propagate(~lit2, amntNewFacts);
  EXPECT_EQ(conflictingClause, nullptr); // no conflict expected
  EXPECT_EQ(amntNewFacts, 1ull);
  EXPECT_EQ(assignments.getAssignment(CNFVar{1}), TBool::FALSE);
}

TEST(UnitSolver, propagateWithSingleTrueClauseCausesNoPropagation) {
  CNFLit lit1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit lit2{CNFVar{2}, CNFSign::NEGATIVE};
  auto binaryClause = createHeapClause(2);
  (*binaryClause)[0] = lit1;
  (*binaryClause)[1] = lit2;

  DummyAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<DummyAssignmentProvider> underTest(maxVar, assignments);
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

  DummyAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<DummyAssignmentProvider> underTest(maxVar, assignments);
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

  DummyAssignmentProvider assignments;
  CNFVar maxVar{4};
  Propagation<DummyAssignmentProvider> underTest(maxVar, assignments);
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

// TODO: test watcher restoration
}
