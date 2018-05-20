/* Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)

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

#pragma once

#include <cstdint>
#include <vector>

#include <boost/range.hpp>

#include "libjamsat/utils/BoundedMap.h"
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/BoundedStack.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {

/**
 * \defgroup JamSAT_Solver  JamSAT core solver classes
 * This module contains the core classes of the JamSAT solver.
 */

/**
 * \ingroup JamSAT_Solver
 *
 * \class Trail
 *
 * \brief The solver's trail data structure.
 *
 * The trail is used to keep track of the assignment sequence, which is partitioned into
 * individually accessible decision levels. The assignment sequence is kept as a sequence of
 * literals.
 *
 * \concept{AssignmentProvider, DecisionLevelProvider}
 */
template <typename ClauseT>
class Trail {
private:
    BoundedStack<CNFLit> m_trail;
    std::vector<uint32_t> m_trailLimits;
    BoundedMap<CNFVar, TBool> m_assignments;
    BoundedMap<CNFVar, TBool> m_phases;
    uint32_t m_currentDecisionLevel;

    struct ReasonAndAssignmentLevel {
        ClauseT const *m_reason;
        uint32_t m_assignmentLevel;
    };

    // Grouping reason clause pointer and assignment levels
    // for cache efficiency: they are used together on hot paths
    // (e.g. stored together during propgation, read together
    // during First-UIP learning)
    BoundedMap<CNFVar, ReasonAndAssignmentLevel> m_reasonsAndALs;

public:
    using size_type = BoundedStack<CNFLit>::size_type;
    using DecisionLevel = uint32_t;
    using const_iterator = BoundedStack<CNFLit>::const_iterator;

    /**
     * \brief StampMap key for Trail::DecisionLevel
     */
    struct DecisionLevelKey {
        using Type = DecisionLevel;

        static size_t getIndex(DecisionLevel variable) { return static_cast<size_t>(variable); }
    };

    /**
     * \brief Constructs a new trail.
     *
     * \param maxVar    The maximum variable which will occur on the trail. \p maxVar must be a
     *                  regular variable.
     */
    explicit Trail(CNFVar maxVar);

    /**
     * \brief Sets up a new decision level on the trail, beginning from the next
     * added literal.
     */
    void newDecisionLevel() noexcept;

    /**
     * \brief Gets the current decision level.
     *
     * \returns The current decision level.
     */
    auto getCurrentDecisionLevel() const noexcept -> DecisionLevel;

    /**
     * \brief Removes all literals from the trail which belong to a decision level
     * greater than or equal to the given one. After this operation, the trail's
     * decision level matches the given decision level.
     */
    void shrinkToDecisionLevel(DecisionLevel level) noexcept;

    /**
     * \brief Removes all literals from the trail which have been assigned on
     * decision levels greater than \p level and sets the current decision level
     * to \p level.
     *
     * Literals on decision level \p level are not removed from the trail.
     *
     * \param level The target decision level. \p level must be smaller than the
     * current decision level.
     */
    void revisitDecisionLevel(DecisionLevel level) noexcept;

    /**
     * \brief Adds a literal to the end of the trail. Note that this literal will
     * belong to the current decision level.
     */
    void addAssignment(CNFLit literal) noexcept;

    /**
     * \brief Adds an assignment to the end of the trail, represented as a literal.
     * The added literal will belong to the current decision level.
     *
     * \param literal   The literal to be added.
     * \param reason    The clause whose assignment has forced the addition of
     *                  \p literal to the trail.
     */
    void addAssignment(CNFLit literal, ClauseT const &reason) noexcept;

    /**
     * \brief Gets the number of current variable assignments.
     *
     * \returns the number of current variable assignments.
     */
    auto getNumberOfAssignments() const noexcept -> size_type;

    /**
     * \brief Determines whether all variables have been assigned.
     *
     * \returns true iff all variables have an assignment different from INDETERMINATE.
     */
    bool isVariableAssignmentComplete() const noexcept;

    /**
     * \brief Gets the assignment for the given variable.
     *
     * \param variable  The target variable. Must not be greater than \p maxVar
     * passed to the constructor.
     * \returns The variable's current assignment. If the variable's assignment
     * has not been set yet, INDETERMINATE is returned.
     */
    auto getAssignment(CNFVar variable) const noexcept -> TBool;

    /**
     * \brief Gets the assignment for the given literal.
     *
     * \param literal  The target literal. Its variable must not be greater than
     * \p maxVar passed to the constructor.
     * \returns The literal's current assignment. If the literal's assignment
     * has not been set yet, INDETERMINATE is returned.
     */
    auto getAssignment(CNFLit literal) const noexcept -> TBool;

    /**
     * \brief Gets the decision level on which the given variable has been
     * assigned.
     *
     * \param variable  The target variable. Must not be greater than \p maxVar
     * passed to the constructor. \p variable must be a variable with a
     * determinate truth value.
     * \returns   The decsiion level where \p variable has been assigned.
     */
    auto getAssignmentDecisionLevel(CNFVar variable) const noexcept -> DecisionLevel;

    /**
     * \brief Gets the assignments of the requested decision level, expressed as
     * literals.
     *
     * \param level   The requested decision level.
     * \returns       an iterator range whose begin points to the first literal of
     * the decision level \level (if any) and whose end points to the first
     * literal beyond the last literal of that decision level. The begin iterator
     * remains valid until shrinkToDecisionLevel(x) is called with x < \p level.
     * The end iterator remains valid until shrinkToDecisionLevel(x) is called
     * with x < \p level and may be incremented once per subsequent call to
     * addAssignment(...) if \p level is the current decision level. Both iterators
     * are invalidated by calls to increaseMaxVarTo().
     */
    auto getDecisionLevelAssignments(DecisionLevel level) const noexcept
        -> boost::iterator_range<const_iterator>;

    /**
     * \brief Gets the variable assignments expressed as literals, beginning with
     * the assignment at the given index.
     *
     * \param index   The index of the first assignment to be included in the
     * result. \p index must not be greater than the current value of \p
     * getNumberOfAssignments() .
     * \returns       an iterator range whose begin points to the literal at index
     * \p index. Let \p level be the current decision level. The begin iterator
     * remains valid until shrinkToDecisionLevel(x) is called with x < \p level.
     * The end iterator remains valid until shrinkToDecisionLevel(x) is called
     * with x < \p level and may be incremented once per subsequent call to
     * addAssignment(...) if \p level is the current decision level. Both iterators
     * are invalidated by calls to increaseMaxVarTo().
     */
    auto getAssignments(size_type beginIndex) -> boost::iterator_range<const_iterator>;

    /**
     * \brief Gets the value of the given variable's last assignment.
     *
     * \param variable The target variable. Must not be greater than \p maxVar
     * passed to the constructor. \p variable must be a variable with a
     * determinate truth value.
     * \returns the value of the last assignment of \p variable . If \p variable
     * has not been assigned yet, the result is TBools::FALSE.
     */
    auto getPhase(CNFVar variable) const noexcept -> TBool;

    /**
     * \brief Increases the maximum variable which may occur on the trail.
     *
     * Any new variables will initially not have an assignment. Calling this method invalidates
     * all iterators referring to the trail object.
     *
     * \param newMaxVar     the new maximum variable. Must not be smaller than the previous maximum
     *                      variable, and must be a regular variable.
     */
    void increaseMaxVarTo(CNFVar newMaxVar);

    /**
     * \brief Returns the assignment reason clause for \p variable.
     *
     * \param variable      A variable with an assignment.
     * \returns             The assignment reason clause of \p variable. If \p variable has
     *                      no assignment reason clause, `nullptr` is returned.
     */
    auto getAssignmentReason(CNFVar variable) const noexcept -> ClauseT const *;

    /**
     * \brief Sets the assignment reason clause for \p variable.
     *
     * \param variable      A variable with an assignment.
     * \param reason        The reason clause for the assignment of \p variable.
     */
    void setAssignmentReason(CNFVar variable, ClauseT const *reason) noexcept;
};

/********** Implementation ****************************** */

template <typename ClauseT>
Trail<ClauseT>::Trail(CNFVar maxVar)
  : m_trail(maxVar.getRawValue() + 1)
  , m_trailLimits({0})
  , m_assignments(maxVar, TBools::INDETERMINATE)
  , m_phases(maxVar, TBools::FALSE)
  , m_currentDecisionLevel(0)
  , m_reasonsAndALs(maxVar) {
    JAM_ASSERT(isRegular(maxVar), "Argument maxVar must be a regular variable.");
}

template <typename ClauseT>
void Trail<ClauseT>::newDecisionLevel() noexcept {
    m_trailLimits.push_back(m_trail.size());
    ++m_currentDecisionLevel;
}

template <typename ClauseT>
auto Trail<ClauseT>::getCurrentDecisionLevel() const noexcept -> DecisionLevel {
    return m_currentDecisionLevel;
}

template <typename ClauseT>
void Trail<ClauseT>::shrinkToDecisionLevel(Trail::DecisionLevel level) noexcept {
    JAM_ASSERT(level < m_trailLimits.size(),
               "Cannot shrink to a decision level higher than the current one");
    for (auto i = m_trail.begin() + m_trailLimits[level]; i != m_trail.end(); ++i) {
        m_phases[i->getVariable()] = m_assignments[(*i).getVariable()];
        m_assignments[i->getVariable()] = TBools::INDETERMINATE;
    }

    m_trail.pop_to(m_trailLimits[level]);
    m_trailLimits.resize(level + 1);
    m_currentDecisionLevel = level;
}

template <typename ClauseT>
void Trail<ClauseT>::revisitDecisionLevel(Trail::DecisionLevel level) noexcept {
    JAM_ASSERT(
        level < m_trailLimits.size() - 1,
        "Cannot revisit current decision level or a decision level higher than the current one");

    for (auto i = m_trail.begin() + m_trailLimits[level + 1]; i != m_trail.end(); ++i) {
        m_phases[i->getVariable()] = m_assignments[(*i).getVariable()];
        m_assignments[i->getVariable()] = TBools::INDETERMINATE;
    }

    m_trail.pop_to(m_trailLimits[level + 1]);
    m_trailLimits.resize(level + 1);
    m_currentDecisionLevel = level;
}

template <typename ClauseT>
void Trail<ClauseT>::addAssignment(CNFLit literal) noexcept {
    JAM_ASSERT(literal.getVariable().getRawValue() <
                   static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    JAM_ASSERT(!isDeterminate(getAssignment(literal.getVariable())),
               "Variable has already been assigned");

    m_trail.push_back(literal);

    TBool value = TBool::fromUnderlyingValue(static_cast<TBool::UnderlyingType>(literal.getSign()));
    m_assignments[literal.getVariable()] = value;
    m_reasonsAndALs[literal.getVariable()].m_assignmentLevel = getCurrentDecisionLevel();
    m_reasonsAndALs[literal.getVariable()].m_reason = nullptr;
}

template <typename ClauseT>
void Trail<ClauseT>::addAssignment(CNFLit literal, ClauseT const &reason) noexcept {
    addAssignment(literal);
    m_reasonsAndALs[literal.getVariable()].m_reason = &reason;
}

template <typename ClauseT>
auto Trail<ClauseT>::getNumberOfAssignments() const noexcept -> size_type {
    return m_trail.size();
}

template <typename ClauseT>
auto Trail<ClauseT>::isVariableAssignmentComplete() const noexcept -> bool {
    return m_trail.size() == m_assignments.size();
}

template <typename ClauseT>
auto Trail<ClauseT>::getDecisionLevelAssignments(DecisionLevel level) const noexcept
    -> boost::iterator_range<Trail<ClauseT>::const_iterator> {
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

template <typename ClauseT>
auto Trail<ClauseT>::getAssignments(size_type beginIndex)
    -> boost::iterator_range<Trail<ClauseT>::const_iterator> {
    JAM_ASSERT(beginIndex <= m_trail.size(), "beginIndex out of bounds");
    return boost::make_iterator_range(m_trail.begin() + beginIndex, m_trail.end());
}

template <typename ClauseT>
auto Trail<ClauseT>::getAssignmentDecisionLevel(CNFVar variable) const noexcept -> DecisionLevel {
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    return m_reasonsAndALs[variable].m_assignmentLevel;
}

template <typename ClauseT>
auto Trail<ClauseT>::getAssignment(CNFVar variable) const noexcept -> TBool {
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    return m_assignments[variable];
}

template <typename ClauseT>
auto Trail<ClauseT>::getAssignment(CNFLit literal) const noexcept -> TBool {
    CNFVar variable = literal.getVariable();
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    TBool variableAssignment = getAssignment(variable);
    TBool::UnderlyingType sign = static_cast<TBool::UnderlyingType>(literal.getSign());
    // TODO: further optimize if neccessary: flip CNFSign constants to get rid of the subtraction
    return TBool::fromUnderlyingValue(variableAssignment.getUnderlyingValue() ^ (1 - sign));
}

template <typename ClauseT>
auto Trail<ClauseT>::getPhase(CNFVar variable) const noexcept -> TBool {
    return m_phases[variable];
}

template <typename ClauseT>
void Trail<ClauseT>::increaseMaxVarTo(CNFVar newMaxVar) {
#if defined(JAM_ASSERT_ENABLED)
    auto oldMaxVarRaw = m_assignments.size() - 1;
    JAM_ASSERT(newMaxVar.getRawValue() >= oldMaxVarRaw,
               "Argument newMaxVar must not be smaller than the previous maximum variable");
    JAM_ASSERT(isRegular(newMaxVar), "The new maximum variable must be a regular variable.");
#endif

    auto amountNewVariables = newMaxVar.getRawValue() + 1 - m_assignments.size();
    if (amountNewVariables == 0) {
        return;
    }

    CNFVar firstNewVar = CNFVar{static_cast<CNFVar::RawVariable>(m_assignments.size())};
    m_trail.increaseMaxSizeBy(amountNewVariables);
    m_assignments.increaseSizeTo(newMaxVar);
    m_reasonsAndALs.increaseSizeTo(newMaxVar);
    m_phases.increaseSizeTo(newMaxVar);

    for (CNFVar i = firstNewVar; i <= newMaxVar; i = nextCNFVar(i)) {
        m_assignments[i] = TBools::INDETERMINATE;
        m_reasonsAndALs[i].m_assignmentLevel = 0;
        m_reasonsAndALs[i].m_reason = nullptr;
        m_phases[i] = TBools::FALSE;
    }
}

template <typename ClauseT>
auto Trail<ClauseT>::getAssignmentReason(CNFVar variable) const noexcept -> ClauseT const * {
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    return m_reasonsAndALs[variable].m_reason;
}

template <typename ClauseT>
void Trail<ClauseT>::setAssignmentReason(CNFVar variable, ClauseT const *reason) noexcept {
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_assignments.size()),
               "Variable out of bounds");
    m_reasonsAndALs[variable].m_reason = reason;
}
}
