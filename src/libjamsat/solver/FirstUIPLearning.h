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

#include <stdexcept>
#include <vector>

#include "Clause.h"

#include <boost/log/trivial.hpp>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
/**
 * \ingroup JamSAT_Solver
 *
 * \class jamsat::FirstUIPLearning
 *
 * \brief A CDCL first-UIP-base clause learning implementation.
 *
 * Usage example: Use FirstUIPLearning with a Trail implementation as a
 * DLProvider and Propagation as a ReasonProvider to compute conflict clauses
 * after a conflict occurred.
 *
 * \tparam DLProvider   A class type having the method T <tt>
 * getAssignmentDecisionLevel(CNFVar variable) const noexcept </tt> (with T
 * being an integral type) which returns the current decision level of \p
 * variable, and T <tt> getCurrentDecisionLevel() const noexcept </tt> returning
 * the current decision level.
 * \tparam ReasonProvider A class type having the method <tt> const Clause*
 * getAssignmentReason(CNFVar variable) const noexcept </tt> returning the
 * reason for the assignment of \p variable.
 */
template <class DLProvider, class ReasonProvider> class FirstUIPLearning {
public:
  /**
   * \brief Constructs a new FirstUIPLearning instance.
   *
   * \param maxVar    The maximum variable occuring in the problem to be solved.
   * \param dlProvider  The decision level providing object. Needs to live as
   * long as the constructed object.
   * \param reasonProvider The assignment reason providing object. Needs to live
   * as long as the constructed object.
   */
  FirstUIPLearning(CNFVar maxVar, const DLProvider &dlProvider,
                   const ReasonProvider &reasonProvider);

  /**
   * \brief Given a conflicting clause, computes a conflict clause.
   *
   * \param conflictingClause  The conflicting clause, ie. a clause being
   * falsified through propagation under the current assignment.
   * \returns The conflict clause determined via resolutions of the conflicting
   * clause with reason clauses.
   */
  std::vector<CNFLit> computeConflictClause(Clause &conflictingClause) const;

private:
  std::vector<CNFLit>
  computeUnoptimizedConflictClause(Clause &conflictingClause) const;
  int initializeResult(const Clause &conflictingClause,
                       std::vector<CNFLit> &result) const;
  void addResolvent(const Clause &reason, CNFLit assertingLit,
                    std::vector<CNFLit> &result, int &unresolvedCount) const;

  const DLProvider &m_dlProvider;
  const ReasonProvider &m_reasonProvider;
  const CNFVar m_maxVar;

  // Class invariant A: m_stamps[x] = 0 for all keys x
  mutable BoundedMap<CNFLit, char> m_stamps;
};

template <class DLProvider, class ReasonProvider>
FirstUIPLearning<DLProvider, ReasonProvider>::FirstUIPLearning(
    CNFVar maxVar, const DLProvider &dlProvider,
    const ReasonProvider &reasonProvider)
    : m_dlProvider(dlProvider), m_reasonProvider(reasonProvider),
      m_maxVar(maxVar), m_stamps(CNFLit{maxVar, CNFSign::POSITIVE}) {}

namespace {
bool isAllZero(const BoundedMap<CNFLit, char> &stamps, CNFVar maxVar) noexcept {
  bool result = true;
  for (CNFVar::RawVariable v = 0; v <= maxVar.getRawValue(); ++v) {
    result &= (stamps[CNFLit{CNFVar{v}, CNFSign::POSITIVE}] == 0);
    result &= (stamps[CNFLit{CNFVar{v}, CNFSign::NEGATIVE}] == 0);
  }
  return result;
}
}

template <class DLProvider, class ReasonProvider>
int FirstUIPLearning<DLProvider, ReasonProvider>::initializeResult(
    const Clause &conflictingClause, std::vector<CNFLit> &result) const {
  const auto currentLevel = m_dlProvider.getCurrentDecisionLevel();
  uint32_t unresolvedCount = 0;

  for (auto lit : conflictingClause) {
    auto dl = m_dlProvider.getAssignmentDecisionLevel(lit.getVariable());
    if (dl == currentLevel &&
        m_reasonProvider.getAssignmentReason(lit.getVariable()) != nullptr) {
      BOOST_LOG_TRIVIAL(trace) << "Adding work: " << lit;
      ++unresolvedCount;
    } else {
      BOOST_LOG_TRIVIAL(trace) << "Adding literal: " << lit;
      result.push_back(~lit);
    }
    if (m_reasonProvider.getAssignmentReason(lit.getVariable()) == nullptr) {
      m_stamps[lit] = 1;
    } else {
      m_stamps[~lit] = 1;
    }
  }

  JAM_ASSERT(unresolvedCount >= 2,
             "Conflicting clauses need to contain at least 2"
             " literals on the current decision level");

  return unresolvedCount;
}

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<DLProvider, ReasonProvider>::addResolvent(
    const Clause &reason, CNFLit assertingLit, std::vector<CNFLit> &result,
    int &unresolvedCount) const {
  const auto currentLevel = m_dlProvider.getCurrentDecisionLevel();

  for (auto reasonLit : reason) {
    BOOST_LOG_TRIVIAL(trace) << "Looking at: " << reasonLit;
    if (reasonLit != assertingLit && m_stamps[~reasonLit] == 0) {
      m_stamps[~reasonLit] = 1;
      if (m_dlProvider.getAssignmentDecisionLevel(reasonLit.getVariable()) ==
              currentLevel &&
          m_reasonProvider.getAssignmentReason(reasonLit.getVariable()) !=
              nullptr) {
        BOOST_LOG_TRIVIAL(trace) << "Adding work: " << reasonLit;
        ++unresolvedCount;
      } else {
        BOOST_LOG_TRIVIAL(trace) << "Adding literal: " << reasonLit;
        result.push_back(reasonLit);
      }
    }
  }
}

template <class DLProvider, class ReasonProvider>
std::vector<CNFLit>
FirstUIPLearning<DLProvider, ReasonProvider>::computeUnoptimizedConflictClause(
    Clause &conflictingClause) const {
  // See Donald Knuth's work on Satisfiability. This implementation deviates
  // from its prosaic description by Knuth in that it goes down the trail not
  // repeatedly, but only once, since all literals occuring in reason clauses
  // with a negative value must occur further down the trail (otherwise, the
  // propagation algorithm would need to have had knowledge of the future).

  JAM_ASSERT(isAllZero(m_stamps, m_maxVar), "Class invariant A violated");

  // TODO: Short description of First-UIP clause learning and Knuth reference.

  try {
    std::vector<CNFLit> result;
    // perform resolution

    int unresolvedCount = 0;

    // Stamp literals on the current decision level and mark them as resolution
    // "work". All others already belong to the result: resolution is not
    // performed at these literals, since none of their inverses can appear in
    // reason clauses for variables on the current decision level. They may
    // appear in those reason clauses with the same sign, though, which is why
    // we need to keep track of the literals already included in the result.
    unresolvedCount = initializeResult(conflictingClause, result);

    CNFLit assertingLit = CNFLit::undefinedLiteral;
    auto trailIterators = m_dlProvider.getAssignments(0);
    auto span = trailIterators.end() - trailIterators.begin();
    auto cursor = trailIterators.begin() + span - 1;
    const auto currentLevel = m_dlProvider.getCurrentDecisionLevel();

    while (unresolvedCount > 1) {
      assertingLit = *cursor;
      BOOST_LOG_TRIVIAL(trace) << "Current asserting literal: " << *cursor;
      if (m_stamps[assertingLit] == 0) {
        --cursor;
        continue;
      }

      if (m_dlProvider.getAssignmentDecisionLevel(assertingLit.getVariable()) ==
          currentLevel) {
        --unresolvedCount;
      }

      auto reason =
          m_reasonProvider.getAssignmentReason(assertingLit.getVariable());

      if (reason != nullptr) {
        addResolvent(*reason, assertingLit, result, unresolvedCount);
      }

      if (cursor == trailIterators.begin()) {
        JAM_ASSERT(unresolvedCount == 0,
                   "Encountered unresolved literal after traversing the trail");
        break;
      }
      --cursor;
    }

    return result;
  } catch (std::bad_alloc &oomException) {
    // clean up, throw on oomException
    for (CNFVar::RawVariable v = 0; v <= m_maxVar.getRawValue(); ++v) {
      m_stamps[CNFLit{CNFVar{v}, CNFSign::POSITIVE}] = 0;
      m_stamps[CNFLit{CNFVar{v}, CNFSign::NEGATIVE}] = 0;
    }
    throw oomException;
  }
}

template <class DLProvider, class ReasonProvider>
std::vector<CNFLit>
FirstUIPLearning<DLProvider, ReasonProvider>::computeConflictClause(
    Clause &conflictingClause) const {
  return computeUnoptimizedConflictClause(conflictingClause);
}
}
