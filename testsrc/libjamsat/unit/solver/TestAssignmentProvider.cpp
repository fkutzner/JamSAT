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

#include "TestAssignmentProvider.h"
#include <libjamsat/utils/Assert.h>

namespace jamsat {

TestAssignmentProvider::TestAssignmentProvider() { m_trail.reserve(1024); }

TBool TestAssignmentProvider::getAssignment(CNFVar variable) const noexcept {
  auto possibleAssgn = m_assignments.find(variable);
  if (possibleAssgn != m_assignments.end()) {
    return possibleAssgn->second;
  }
  return TBool::INDETERMINATE;
}

TBool TestAssignmentProvider::getAssignment(CNFLit literal) const noexcept {
  auto varAssgn = getAssignment(literal.getVariable());
  if (literal.getSign() == CNFSign::POSITIVE ||
      varAssgn == TBool::INDETERMINATE) {
    return varAssgn;
  }
  return varAssgn == TBool::FALSE ? TBool::TRUE : TBool::FALSE;
}

void TestAssignmentProvider::addLiteral(CNFLit literal) noexcept {
  JAM_ASSERT(literal.getVariable().getRawValue() < 1024,
             "literal variable too large for TestAssignmentProvider");
  m_assignments[literal.getVariable()] =
      (literal.getSign() == CNFSign::POSITIVE ? TBool::TRUE : TBool::FALSE);
  m_trail.push_back(literal);
}

void TestAssignmentProvider::clearLiteral(CNFLit literal) noexcept {
  if (m_assignments.find(literal.getVariable()) != m_assignments.end()) {
    m_assignments.erase(literal.getVariable());

    m_trail.erase(
        std::find_if(m_trail.begin(), m_trail.end(), [literal](CNFLit &l) {
          return l.getVariable() == literal.getVariable();
        }));
  }
}

size_t TestAssignmentProvider::getNumberOfAssignments() const noexcept {
  return m_trail.size();
}

boost::iterator_range<std::vector<CNFLit>::const_iterator>
TestAssignmentProvider::getAssignments(size_t index) const noexcept {
  return boost::make_iterator_range(m_trail.begin() + index, m_trail.end());
}

TestAssignmentProvider::DecisionLevel
TestAssignmentProvider::getDecisionLevel(CNFVar variable) const noexcept {
  auto result = m_decisionLevels.find(variable);
  if (result != m_decisionLevels.end()) {
    return result->second;
  }
  return 0;
}

void TestAssignmentProvider::setDecisionLevel(CNFVar variable,
                                              DecisionLevel level) noexcept {
  m_decisionLevels[variable] = level;
}

TestAssignmentProvider::DecisionLevel
TestAssignmentProvider::getCurrentDecisionLevel() const noexcept {
  return m_currentLevel;
}
void TestAssignmentProvider::setCurrentDecisionLevel(
    TestAssignmentProvider::DecisionLevel level) noexcept {
  m_currentLevel = level;
}
}
