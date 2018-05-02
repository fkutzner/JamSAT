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
  : m_trail(maxVar.getRawValue() + 1)
  , m_trailLimits({0})
  , m_assignments(maxVar, TBools::INDETERMINATE)
  , m_assignmentLevel(maxVar)
  , m_phases(maxVar, TBools::FALSE) {
    JAM_ASSERT(isRegular(maxVar), "Argument maxVar must be a regular variable.");
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
    for (auto i = m_trail.begin() + m_trailLimits[level]; i != m_trail.end(); ++i) {
        m_assignments[(*i).getVariable()] = TBools::INDETERMINATE;
    }

    m_trail.pop_to(m_trailLimits[level]);
    m_trailLimits.resize(level + 1);
}

void Trail::revisitDecisionLevel(Trail::DecisionLevel level) noexcept {
    JAM_ASSERT(
        level < m_trailLimits.size() - 1,
        "Cannot revisit current decision level or a decision level higher than the current one");

    for (auto i = m_trail.begin() + m_trailLimits[level + 1]; i != m_trail.end(); ++i) {
        m_assignments[(*i).getVariable()] = TBools::INDETERMINATE;
    }

    m_trail.pop_to(m_trailLimits[level + 1]);
    m_trailLimits.resize(level + 1);
}

void Trail::addAssignment(CNFLit literal) noexcept {
    JAM_ASSERT(literal.getVariable().getRawValue() <
                   static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    JAM_ASSERT(!isDeterminate(getAssignment(literal.getVariable())),
               "Variable has already been assigned");

    m_trail.push_back(literal);

    TBool value = TBool::fromUnderlyingValue(static_cast<TBool::UnderlyingType>(literal.getSign()));
    m_assignments[literal.getVariable()] = value;
    m_assignmentLevel[literal.getVariable()] = getCurrentDecisionLevel();
    m_phases[literal.getVariable()] = value;
}

Trail::size_type Trail::getNumberOfAssignments() const noexcept {
    return m_trail.size();
}

bool Trail::isVariableAssignmentComplete() const noexcept {
    return m_trail.size() == m_assignments.size();
}

boost::iterator_range<Trail::const_iterator>
Trail::getDecisionLevelAssignments(DecisionLevel level) const noexcept {
    if (level >= m_trailLimits.size()) {
        return boost::make_iterator_range(m_trail.end(), m_trail.end());
    }

    auto begin = m_trail.begin() + m_trailLimits[level];
    if (level + 1 == m_trailLimits.size()) {
        return boost::make_iterator_range(begin, m_trail.end());
    }
    auto end = m_trail.begin() + m_trailLimits[level + 1];
    return boost::make_iterator_range(begin, end);
}

boost::iterator_range<Trail::const_iterator> Trail::getAssignments(size_type beginIndex) {
    JAM_ASSERT(beginIndex <= m_trail.size(), "beginIndex out of bounds");
    return boost::make_iterator_range(m_trail.begin() + beginIndex, m_trail.end());
}

Trail::DecisionLevel Trail::getAssignmentDecisionLevel(CNFVar variable) const noexcept {
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    return m_assignmentLevel[variable];
}

TBool Trail::getAssignment(CNFVar variable) const noexcept {
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    return m_assignments[variable];
}

TBool Trail::getAssignment(CNFLit literal) const noexcept {
    CNFVar variable = literal.getVariable();
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    TBool variableAssignment = getAssignment(variable);
    TBool::UnderlyingType sign = static_cast<TBool::UnderlyingType>(literal.getSign());
    // TODO: further optimize if neccessary: flip CNFSign constants to get rid of the subtraction
    return TBool::fromUnderlyingValue(variableAssignment.getUnderlyingValue() ^ (1 - sign));
}

TBool Trail::getPhase(CNFVar variable) const noexcept {
    return m_phases[variable];
}

void Trail::increaseMaxVarTo(CNFVar newMaxVar) {
#if defined(JAM_ASSERT_ENABLED)
    {
        auto oldMaxVarRaw = m_assignments.size() - 1;
        JAM_ASSERT(newMaxVar.getRawValue() >= oldMaxVarRaw,
                   "Argument newMaxVar must not be smaller than the previous maximum variable");
        JAM_ASSERT(isRegular(newMaxVar), "The new maximum variable must be a regular variable.");
    }
#endif

    auto amountNewVariables = newMaxVar.getRawValue() + 1 - m_assignments.size();
    if (amountNewVariables == 0) {
        return;
    }

    CNFVar firstNewVar = CNFVar{static_cast<CNFVar::RawVariable>(m_assignments.size())};
    m_trail.increaseMaxSizeBy(amountNewVariables);
    m_assignments.increaseSizeTo(newMaxVar);
    m_assignmentLevel.increaseSizeTo(newMaxVar);
    m_phases.increaseSizeTo(newMaxVar);

    for (CNFVar i = firstNewVar; i <= newMaxVar; i = nextCNFVar(i)) {
        m_assignments[i] = TBools::INDETERMINATE;
        m_assignmentLevel[i] = 0;
        m_phases[i] = TBools::FALSE;
    }
}
}
