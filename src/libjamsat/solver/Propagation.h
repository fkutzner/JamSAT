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

#include "Clause.h"
#include "Watcher.h"
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/ArrayMap.h>
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
   * This method may only be called if one of the following conditions is
   * satisfied:
   *
   * - No literals occurring in \p clause have an assignment.
   * - All literals occurring in \p clause except for the first one are assigned
   * to FALSE.
   *
   * If the second condition holds and the first literal in \p clause has no
   * assignment, the value of the first literal gets propagated until fixpoint.
   *
   * \param clause    The clause to be registered, which must exist until this
   * is destroyed or it is deregistered from this.
   * \returns         A conflicting clause if adding \clause caused a
   * propagation and a conflict occured; nullptr otherwise.
   */
  Clause *registerClause(Clause &clause);

  /**
   * \brief Unregisters a clause in the propagation system.
   *
   * \param clause    A clause that had previously been registered. After this
   * method returns, the clause may be destroyed.
   */
  void unregisterClause(Clause &clause);

  /**
   * \brief Propagates the given fact wrt. the clauses registered in the
   * propagation object, further propagating forced assignments until the
   * variable assignment  reaches a fixpoint.
   *
   * As soon as a new fact has been deduced, the assignment provider's
   * addLiteral(l) method is called with l encoding the new fact. If the
   * propagation leads to a conflict, a pointer to the clause falsified under
   * the current assignment is returned.
   *
   * \param toPropagate       The fact to propagate, encoded as a literal.
   */
  Clause *propagateUntilFixpoint(CNFLit toPropagate);

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
  ArrayMap<CNFVar, const Clause *> m_reasons;

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
    : m_assignmentProvider(assignmentProvider), m_reasons(maxVar),
      m_watchers(maxVar) {}

template <class AssignmentProvider>
Clause *Propagation<AssignmentProvider>::registerClause(Clause &clause) {
  JAM_ASSERT(clause.getSize() >= 2ull, "Illegally small clause argument");
  detail_propagation::Watcher watcher1{clause, clause[0], 1};
  detail_propagation::Watcher watcher2{clause, clause[1], 0};
  m_watchers.addWatcher(clause[0], watcher2);
  m_watchers.addWatcher(clause[1], watcher1);

  TBool secondLiteralAssignment = m_assignmentProvider.getAssignment(clause[1]);
  // By method contract, if secondLiteralAssignment != INDETERMINATE, we need
  // to propagate the first literal.
  if (secondLiteralAssignment != TBool::INDETERMINATE) {
    m_assignmentProvider.addLiteral(clause[0]);
    auto confl = propagateUntilFixpoint(clause[0]);
    // Fix the reason since this was not a decision:
    m_reasons[clause[0].getVariable()] = &clause;
    return confl;
  }
  return nullptr;
}

template <class AssignmentProvider>
const Clause *
Propagation<AssignmentProvider>::getAssignmentReason(CNFVar variable) const
    noexcept {
  JAM_ASSERT(variable.getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Variable out of bounds");
  return m_reasons[variable];
}

template <class AssignmentProvider>
bool Propagation<AssignmentProvider>::hasForcedAssignment(CNFVar variable) const
    noexcept {
  JAM_ASSERT(variable.getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Variable out of bounds");
  return m_reasons[variable] != nullptr;
}

template <class AssignmentProvider>
Clause *
Propagation<AssignmentProvider>::propagateUntilFixpoint(CNFLit toPropagate) {
  JAM_ASSERT(toPropagate.getVariable().getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Literal variable out of bounds");
  m_reasons[toPropagate.getVariable()] = nullptr;
  auto trailEndIndex = m_assignmentProvider.getNumberOfAssignments();

  // Using the space on the trail beyond its current last literal as a
  // propagation queue.
  auto propagationQueue = m_assignmentProvider.getAssignments(trailEndIndex);

  size_t amountOfNewFacts = 0;
  Clause *conflictingClause = propagate(toPropagate, amountOfNewFacts);
  if (conflictingClause) {
    return conflictingClause;
  }

  // Propagate all forced assignments. New assignments are added to the
  // assignment provider by propagate, and therefore are also added to the
  // propagation queue.
  auto pqBegin = propagationQueue.begin();
  auto pqEnd = propagationQueue.end() + amountOfNewFacts;
  while (pqBegin != pqEnd) {
    size_t localNewFacts = 0;
    conflictingClause = propagate(*pqBegin, localNewFacts);
    pqEnd += localNewFacts;
    if (conflictingClause) {
      return conflictingClause;
    }
    ++pqBegin;
  }

  // No more forced assignments can be propagated => fixpoint reached.
  return nullptr;
}

template <class AssignmentProvider>
Clause *Propagation<AssignmentProvider>::propagate(CNFLit toPropagate,
                                                   size_t &amountOfNewFacts) {
  JAM_ASSERT(toPropagate.getVariable().getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Literal variable out of bounds");

  m_reasons[toPropagate.getVariable()] = nullptr;
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
        // The FALSE literal is moved into the unwatched of the clause here,
        // such that an INDETERMINATE or TRUE literal gets watched.
        //
        // If otherLit is INDETERMINATE, this clause does not force anything,
        // and we can skip propagation.
        //
        // Since FALSE literals are moved into the non-watched part of the
        // clause as much as possible, otherLit can only be FALSE due to
        // a forced assignment which has not been propagated yet (but will
        // still be propagated in the future, causing a possible conflict
        // or propagation to be detected).
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
        m_reasons[otherWatchedLit.getVariable()] = &clause;
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
