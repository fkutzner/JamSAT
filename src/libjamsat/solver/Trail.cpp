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

#include "Trail.h"
#include "libjamsat/utils/Assert.h"

namespace jamsat {
Trail::Trail(CNFVar maxVar)
    : m_trail({}), m_trailLimits({0}), m_assignments({}),
      m_assignmentLevel({}) {
  JAM_ASSERT(
      maxVar != CNFVar::undefinedVariable,
      "Trail cannot be instantiated with the undefined variable as maxVar");
  m_trail.reserve(maxVar.getRawValue() + 1);
  m_assignments.resize(maxVar.getRawValue() + 1, TBool::INDETERMINATE);
  m_assignmentLevel.resize(maxVar.getRawValue() + 1);
}

void Trail::newDecisionLevel() noexcept {
  m_trailLimits.push_back(m_trail.size());
}

Trail::DecisionLevel Trail::getCurrentDecisionLevel() const noexcept {
  return m_trailLimits.size() - 1;
}

void Trail::shrinkToDecisionLevel(Trail::DecisionLevel level) noexcept {
  JAM_ASSERT(level < m_trailLimits.size(),
             "Cannot shrink to a decision level higher than the current one");
  m_trail.resize(m_trailLimits[level]);
  m_trailLimits.resize(level + 1);
}

void Trail::addLiteral(CNFLit literal) noexcept { m_trail.push_back(literal); }

Trail::size_type Trail::getNumberOfAssignments() const noexcept {
  return m_trail.size();
}

std::pair<Trail::const_iterator, Trail::const_iterator>
Trail::getDecisionLevelLiterals(DecisionLevel level) const noexcept {
  if (level >= m_trailLimits.size()) {
    return std::pair<const_iterator, const_iterator>(m_trail.end(),
                                                     m_trail.end());
  }

  const_iterator begin = m_trail.begin() + m_trailLimits[level];
  if (level == m_trailLimits.size()) {
    return std::pair<const_iterator, const_iterator>(begin, m_trail.end());
  } else {
    const_iterator end = m_trail.begin() + m_trailLimits[level + 1];
    return std::pair<const_iterator, const_iterator>(begin, end);
  }
}

Trail::DecisionLevel Trail::getAssignmentDecisionLevel(CNFVar variable) const
    noexcept {
  JAM_ASSERT(variable.getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_assignments.size()),
             "Variable out of bounds");
  return m_assignmentLevel[variable.getRawValue()];
}

TBool Trail::getAssignment(CNFVar variable) const noexcept {
  JAM_ASSERT(variable.getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_assignments.size()),
             "Variable out of bounds");
  return m_assignments[variable.getRawValue()];
}
}
