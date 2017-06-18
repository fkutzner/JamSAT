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

#pragma once

#include <queue>
#include <vector>

#include <boost/heap/priority_queue.hpp>

#include <libjamsat/branching/BranchingHeuristicBase.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
namespace detail {

class CNFVarActivityOrder {
public:
  CNFVarActivityOrder(std::vector<double> &activity) : m_activity(activity){};

  bool operator()(const CNFVar &lhs, const CNFVar &rhs) const {
    JAM_ASSERT(lhs.getRawValue() <
                   static_cast<CNFVar::RawVariableType>(m_activity.size()),
               "Index out of bounds");
    JAM_ASSERT(rhs.getRawValue() <
                   static_cast<CNFVar::RawVariableType>(m_activity.size()),
               "Index out of bounds");

    return m_activity[lhs.getRawValue()] <= m_activity[rhs.getRawValue()];
  }

private:
  const std::vector<double> &m_activity;
};
}

/**
 * \ingroup JamSAT_Branching
 *
 * \class jamsat::VSIDSBranchingHeuristic
 *
 * \brief A VSIDS Branching heuristic implementation.
 *
 * \tparam AssignmentProvider   A class type T having the method TBool
 * T::getAssignment(CNFLit x) which returns the current variable assignment of
 * x.
 */
template <class AssignmentProvider>
class VSIDSBranchingHeuristic : public BranchingHeuristicBase {
public:
  /**
   * \brief Constructs a new VSIDSBranchingHeuristic object.
   *
   * \param maxVar              The largest variable occurring in the SAT
   * problem instance to be solved.
   * \param assignmentProvider  A reference to an object using which the current
   * variable assignment can be obtained.
   */
  VSIDSBranchingHeuristic(CNFVar maxVar,
                          AssignmentProvider &assignmentProvider);

  /**
   * \brief Informs the branching heuristic that the given variable was
   * contained in a clause used to obtain a learned clause during conflict
   * resolution.
   *
   * \param variable  The variable as described above. \p variable must not be
   * larger than \p maxVar passed to this object's constructor.
   */
  void seenInConflict(CNFVar variable) noexcept;

  /**
   * \brief Obtains a branching literal if possible.
   *
   * The chosen variable \p v will not be used for branching again before
   * reset() or reset reset(\p v) has been called.
   *
   * \returns If a branching decision can be performed, this method returns a
   * literal \p L with variable \p v and sign \p s such that the solver can
   * assign \p v to the value corresponding to \p s as a branching decision.
   * Otherwise, CNFLit::undefinedLiteral is returned.
   */
  CNFLit pickBranchLiteral() noexcept;

  /**
   * \brief Resets the record of branching decisions.
   *
   * After calling this method, all variables which are marked as possible
   * decision variables and which are not assigned may be used for determining a
   * branching decision literal.
   */
  void reset() noexcept;

  /**
   * \brief Resets the record of branching decisions for the given variable.
   *
   * After calling this method, the given variable may be used in a branching
   * decision literal if it is marked as a possible decision variable and has no
   * assinment.
   *
   * \param variable    The variable to be reset.
   */
  void reset(CNFVar variable) noexcept;

private:
  std::vector<double> m_activity;
  detail::CNFVarActivityOrder m_activityOrder;

  using HeapCompareType = boost::heap::compare<detail::CNFVarActivityOrder>;
  boost::heap::priority_queue<CNFVar, HeapCompareType> m_variableOrder;
  const AssignmentProvider &m_assignmentProvider;

  static const double m_activityBump;
};

/********** Implementation ****************************** */

template <class AssignmentProvider>
VSIDSBranchingHeuristic<AssignmentProvider>::VSIDSBranchingHeuristic(
    CNFVar maxVar, AssignmentProvider &assignmentProvider)
    : BranchingHeuristicBase(maxVar), m_activity({}),
      m_activityOrder(m_activity), m_variableOrder(m_activityOrder),
      m_assignmentProvider(assignmentProvider) {
  m_activity.resize(maxVar.getRawValue() + 1);
  reset();
}

template <class AssignmentProvider>
CNFLit
VSIDSBranchingHeuristic<AssignmentProvider>::pickBranchLiteral() noexcept {

  CNFVar branchingVar = CNFVar::undefinedVariable;
  while (!m_variableOrder.empty()) {
    branchingVar = m_variableOrder.top();
    m_variableOrder.pop();

    if (m_assignmentProvider.getAssignment(branchingVar) ==
            TBool::INDETERMINATE &&
        isEligibleForDecisions(branchingVar)) {
      return CNFLit{branchingVar, CNFSign::POSITIVE};
    }
  };

  return CNFLit::undefinedLiteral;
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<AssignmentProvider>::seenInConflict(
    CNFVar variable) noexcept {
  m_activity[variable] += 1.0f;
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<AssignmentProvider>::reset() noexcept {
  m_variableOrder.clear();
  for (CNFVar::RawVariableType i = 0;
       i < static_cast<CNFVar::RawVariableType>(m_activity.size()); ++i) {
    m_variableOrder.emplace(CNFVar{i});
  }
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<AssignmentProvider>::reset(
    CNFVar variable) noexcept {
  m_variableOrder.emplace(CNFVar{variable});
}
}
