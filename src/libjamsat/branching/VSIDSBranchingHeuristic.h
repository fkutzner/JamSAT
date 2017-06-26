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

#include <algorithm>

#include <boost/heap/d_ary_heap.hpp>

#include <libjamsat/branching/BranchingHeuristicBase.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/ArrayMap.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
namespace detail {

class CNFVarActivityOrder {
public:
  explicit CNFVarActivityOrder(ArrayMap<CNFVar, double> &activity)
      : m_activity(activity){};

  bool operator()(const CNFVar &lhs, const CNFVar &rhs) const {
    JAM_ASSERT(lhs.getRawValue() <
                   static_cast<CNFVar::RawVariable>(m_activity.size()),
               "Index out of bounds");
    JAM_ASSERT(rhs.getRawValue() <
                   static_cast<CNFVar::RawVariable>(m_activity.size()),
               "Index out of bounds");

    return m_activity[lhs] <= m_activity[rhs];
  }

private:
  const ArrayMap<CNFVar, double> &m_activity;
};
}

/**
 * \ingroup JamSAT_Branching
 *
 * \class jamsat::VSIDSBranchingHeuristic
 *
 * \brief A VSIDS Branching heuristic implementation.
 *
 * Usage example: Use VSIDSBranchingHeuristic in a CDCL SAT solver to decide
 * which literal to put on the solver's trail (which can be used as an
 * assignment provider) when currently no further facts can be propagated.
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
                          const AssignmentProvider &assignmentProvider);

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

  /**
   * \brief Informs the heuristic that the solver is about to begin processing a
   * conflict.
   */
  void beginHandlingConflict() noexcept;

  /**
   * \brief Informs the heuristic that the solver has just finished processing a
   * conflict.
   */
  void endHandlingConflict() noexcept;

  /**
   * \brief Sets the activity value delta added to a variable's activity when it
   * is bumped.
   *
   * \param delta The new activity delta.
   */
  void setActivityBumpDelta(double delta) noexcept;

  /**
   * \brief Gets the activity value delta added to a variable's activity when it
   * is bumped.
   *
   * \returns The specified activity delta.
   */
  double getActivityBumpDelta() const noexcept;

private:
  void scaleDownActivities();

  ArrayMap<CNFVar, double> m_activity;
  detail::CNFVarActivityOrder m_activityOrder;

  using HeapCompare = boost::heap::compare<detail::CNFVarActivityOrder>;
  using HeapArity = boost::heap::arity<2>;
  using HeapMutability = boost::heap::mutable_<true>;
  using VariableHeap =
      boost::heap::d_ary_heap<CNFVar, HeapCompare, HeapArity, HeapMutability>;
  VariableHeap m_variableOrder;
  ArrayMap<CNFVar, VariableHeap::handle_type> m_heapVariableHandles;

  const AssignmentProvider &m_assignmentProvider;

  double m_activityBumpDelta;
  double m_decayRate;
  double m_maxDecayRate;
  int m_numberOfConflicts;
};

/********** Implementation ****************************** */

template <class AssignmentProvider>
VSIDSBranchingHeuristic<AssignmentProvider>::VSIDSBranchingHeuristic(
    CNFVar maxVar, const AssignmentProvider &assignmentProvider)
    : BranchingHeuristicBase(maxVar), m_activity(maxVar),
      m_activityOrder(m_activity), m_variableOrder(m_activityOrder),
      m_heapVariableHandles(maxVar), m_assignmentProvider(assignmentProvider),
      m_activityBumpDelta(1.0f), m_decayRate(0.8f), m_maxDecayRate(0.95f),
      m_numberOfConflicts(0) {
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
  m_activity[variable] += m_activityBumpDelta;

  if (m_activity[variable] >= 1e100) {
    scaleDownActivities();
  }

  m_variableOrder.increase(m_heapVariableHandles[variable]);
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<AssignmentProvider>::scaleDownActivities() {
  for (typename decltype(m_activity)::size_type i = 0; i < m_activity.size();
       ++i) {
    auto rawVariable = static_cast<CNFVar::RawVariable>(i);
    auto &activity = m_activity[CNFVar{rawVariable}];
    activity = 1e-100 * activity;
  }
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<AssignmentProvider>::reset() noexcept {
  m_variableOrder.clear();
  for (CNFVar::RawVariable i = 0;
       i < static_cast<CNFVar::RawVariable>(m_activity.size()); ++i) {
    auto heapHandle = m_variableOrder.emplace(CNFVar{i});
    m_heapVariableHandles[CNFVar{i}] = heapHandle;
  }
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<AssignmentProvider>::reset(
    CNFVar variable) noexcept {
  auto heapHandle = m_variableOrder.emplace(variable);
  m_heapVariableHandles[variable] = heapHandle;
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<AssignmentProvider>::setActivityBumpDelta(
    double delta) noexcept {
  m_activityBumpDelta = delta;
}

template <class AssignmentProvider>
double VSIDSBranchingHeuristic<AssignmentProvider>::getActivityBumpDelta() const
    noexcept {
  return m_activityBumpDelta;
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<
    AssignmentProvider>::beginHandlingConflict() noexcept {
  ++m_numberOfConflicts;
  if (m_numberOfConflicts == 5000) {
    m_decayRate = std::min(m_decayRate + 0.1, m_maxDecayRate);
    m_numberOfConflicts = 0;
  }
}

template <class AssignmentProvider>
void VSIDSBranchingHeuristic<
    AssignmentProvider>::endHandlingConflict() noexcept {
  m_activityBumpDelta *= (1 / m_decayRate);
}
}
