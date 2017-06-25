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

#include <vector>

#include "Clause.h"
#include "Watcher.h"
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
/**
 * \ingroup JamSAT_Solver
 *
 * \class jamsat::Propagation
 *
 * \brief A CDCL fact propagation implementation.
 *
 * Usage example: Use Propagation with a Trail implementation as an assignment
 * provider to compute all forced assignments after a CDCL branching decision.
 *
 * \tparam AssignmentProvider   A class type T having the method TBool
 * T::getAssignment(CNFLit x) which returns the current variable assignment of
 * x and a method T::addLiteral(CNFLit x) registering the assignment x within
 * the assignment provider.
 */
template <class AssignmentProvider> class Propagation {
public:
  /**
   * \brief Constructs a new Propagation instance.
   *
   * \param maxVar              The largest variable occuring in the clauses on
   * which propagation is performed.
   * \param assignmentProvider  The assignment provider configured such that
   * assignments up to and including maxVar can be kept track of.
   */
  Propagation(CNFVar maxVar, AssignmentProvider &assignmentProvider);

  /**
   * \brief Registers a clause in the propagation system.
   *
   * This method may only be called when no variables have an assignment.
   *
   * \param clause    The clause to be registered, which must exist until this
   * is destroyed or it is deregistered from this.
   */
  void registerClause(Clause &clause);

  /**
   * \brief Unregisters a clause in the propagation system.
   *
   * \param clause    A clause that had previously been registered. After this
   * method returns, the clause may be destroyed.
   */
  void unregisterClause(Clause &clause);

  /**
   * \brief Propagates the given fact wrt. the clauses registered in the
   * propagation object,
   * further propagating forced assignments until the variable assignment
   * reaches a fixpoint.
   *
   * As soon as a new fact has been deduced, the assignment provider's
   * addLiteral(l) method is called with l encoding the new fact. If the
   * propagation leads to a conflict, a pointer to the clause falsified under
   * the current assignment is returned.
   *
   * \param toPropagate       The fact to propagate, encoded as a literal.
   * \param amountOfNewFacts  (out-parameter) The amount of facts added due to
   * propagation is stored in the variable referenced by \p amountOfNewFacts.
   * \returns \p nullptr if a fixpoint has been reached without any clause being
   * falsified; otherwise, the pointer to a clause falsified under the current
   * assignment is returned.
   */
  Clause *propagateUntilFixpoint(CNFLit toPropagate, size_t &amountOfNewFacts);

  /**
   * \brief Propagates the given fact wrt. the clauses registered in the
   * propagation object.
   *
   * As soon as a new fact has been deduced, the assignment provider's
   * addLiteral(l) method is called with l encoding the new fact. If the
   * propagation leads to a conflict, a pointer to the clause falsified under
   * the current assignment is returned.
   *
   * \param toPropagate       The fact to propagate, encoded as a literal.
   * \param amountOfNewFacts  (out-parameter) The amount of facts added due to
   * propagation is stored in the variable referenced by \p amountOfNewFacts.
   * \returns \p nullptr if the fact has been propagated without any clause
   * being falsified; otherwise, the pointer to a clause falsified under the
   * current assignment is returned.
   */
  Clause *propagate(CNFLit toPropagate, size_t &amountOfNewFacts);

  /**
   * \brief Determines whether the given variable as a forced assignment.
   *
   * \param variable  The target variable. Must not be greater than \p maxVar
   * passed to the constructor. \p variable must be a variable with a
   * determinate truth value.
   */
  bool hasForcedAssignment(CNFVar variable) const noexcept;

  /**
   * \brief Gets the pointer to the clause which forced the assignment of the
   * given variable.
   *
   * \param variable  The target variable. Must not be greater than \p maxVar
   * passed to the constructor. \p variable must be a variable with a
   * determinate truth value.
   * \returns    The clause which forced the assignment of the given variable.
   * If the assignment was not forced due to propagation, \p nullptr is returned
   * instead.
   */
  const Clause *getAssignmentReason(CNFVar variable) const noexcept;

private:
  AssignmentProvider &m_assignmentProvider;
  std::vector<const Clause *> m_reasons;

  /**
   * \internal
   *
   * Invariants for m_watchers: for each registered clause C,
   *  - \p m_watchers contains exactly two different watchers pointing to C.
   *  - the lists \p m_watchers.getWatchers(C[0]) and \p
   * m_watchers.getWatchers(C[1]) each contain a watcher pointing to C.
   */
  detail_propagation::Watchers m_watchers;
};

/********** Implementation ****************************** */

template <class AssignmentProvider>
Propagation<AssignmentProvider>::Propagation(
    CNFVar maxVar, AssignmentProvider &assignmentProvider)
    : m_assignmentProvider(assignmentProvider), m_reasons({}),
      m_watchers(maxVar) {
  m_reasons.resize(maxVar.getRawValue() + 1);
}

template <class AssignmentProvider>
void Propagation<AssignmentProvider>::registerClause(Clause &clause) {
  JAM_ASSERT(clause.getSize() >= 2ull, "Illegally small clause argument");
  detail_propagation::Watcher watcher1{clause, clause[0], 1};
  detail_propagation::Watcher watcher2{clause, clause[1], 0};
  m_watchers.addWatcher(clause[0], watcher2);
  m_watchers.addWatcher(clause[1], watcher1);
}

template <class AssignmentProvider>
const Clause *
Propagation<AssignmentProvider>::getAssignmentReason(CNFVar variable) const
    noexcept {
  JAM_ASSERT(variable.getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Variable out of bounds");
  return m_reasons[variable.getRawValue()];
}

template <class AssignmentProvider>
bool Propagation<AssignmentProvider>::hasForcedAssignment(CNFVar variable) const
    noexcept {
  JAM_ASSERT(variable.getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Variable out of bounds");
  return m_reasons[variable.getRawValue()] != nullptr;
}

template <class AssignmentProvider>
Clause *Propagation<AssignmentProvider>::propagateUntilFixpoint(
    CNFLit toPropagate, size_t &amountOfNewFacts) {
  JAM_ASSERT(toPropagate.getVariable().getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Literal variable out of bounds");
  m_reasons[toPropagate.getVariable().getRawValue()] = nullptr;
  amountOfNewFacts = 0;
  return nullptr;
}

template <class AssignmentProvider>
Clause *Propagation<AssignmentProvider>::propagate(CNFLit toPropagate,
                                                   size_t &amountOfNewFacts) {
  JAM_ASSERT(toPropagate.getVariable().getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Literal variable out of bounds");

  m_reasons[toPropagate.getVariable().getRawValue()] = nullptr;
  amountOfNewFacts = 0;
  CNFLit negatedToPropagate = ~toPropagate;

  // Traverse all watchers referencing clauses containing ~toPropagate to find
  // new forced assignments.
  auto watcherListTraversal = m_watchers.getWatchers(negatedToPropagate);
  while (!watcherListTraversal.hasFinishedTraversal()) {
    auto currentWatcher = *watcherListTraversal;
    CNFLit otherWatchedLit = currentWatcher.getOtherWatchedLiteral();
    TBool assignment = m_assignmentProvider.getAssignment(otherWatchedLit);

    if (assignment == TBool::TRUE) {
      // The clause is already satisfied and can be ignored for propagation.
      ++watcherListTraversal;
      continue;
    }

    auto &clause = currentWatcher.getClause();

    // otherWatchedLit might not actually be the other watched literal due to
    // the swap at (*), so restore it
    otherWatchedLit = clause[1 - currentWatcher.getIndex()];
    assignment = m_assignmentProvider.getAssignment(otherWatchedLit);
    if (assignment == TBool::TRUE) {
      // The clause is already satisfied and can be ignored for propagation.
      ++watcherListTraversal;
      continue;
    }

    // Invariant: both watchers pointing to the clause have an other watched
    // literal pointing either to clause[0] or clause[1], but not to the literal
    // which is their index in m_watchers.

    bool actionIsForced = true;
    for (Clause::size_type i = 2; i < clause.getSize(); ++i) {
      CNFLit currentLiteral = clause[i];
      if (m_assignmentProvider.getAssignment(currentLiteral) != TBool::FALSE) {
        // Swapping the FALSE-assigned literal with the non-false one to
        // re-establish the invariant that first two literals cannot be FALSE
        // unless all other literals are FALSE. If a watched literal becomes
        // FALSE in the future, it will either be swapped with a non-FALSE
        // literal beyond the second one, or cause a propagation/conflict to
        // happen. In the case of a propagation, the assignment to TRUE of the
        // remaining literal R will be removed in the same
        // backtracking operation as the FALSE assignment of ~toPropagate;
        // in case of a conflict, both watched
        // literals have been assigned on the current decision level, and both
        // their assignments are removed in the ensuing backtracking step.
        JAM_ASSERT(
            assignment == TBool::INDETERMINATE,
            "Invariant violated: other watched literal must be unassigned");
        std::swap(clause[currentWatcher.getIndex()],
                  clause[i]); // (*, see above)
        m_watchers.addWatcher(currentLiteral, currentWatcher);
        watcherListTraversal.removeCurrent();
        actionIsForced = false;
        break;
      }
    }

    if (actionIsForced) {
      // Invariant holding here: all literals in the clause beyond the second
      // literal have the value FALSE.
      if (assignment == TBool::FALSE) {
        // Conflict case: all literals are FALSE. Return the conflicting clause.
        watcherListTraversal.finishedTraversal();
        return &clause;
      } else {
        // Propagation case: otherWatchedLit is the only remaining unassigned
        // literal
        ++amountOfNewFacts;
        m_assignmentProvider.addLiteral(otherWatchedLit);
      }

      // Only advancing the traversal if an action is forced, since otherwise
      // the current watcher has been removed via removeCurrent() and
      // watcherListTraversal already points to the next watcher.
      ++watcherListTraversal;
    }
  }

  watcherListTraversal.finishedTraversal();
  return nullptr;
}
}
