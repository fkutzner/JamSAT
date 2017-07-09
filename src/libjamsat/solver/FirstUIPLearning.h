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
#include <libjamsat/utils/FaultInjector.h>
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

  void test_assertClassInvariantsSatisfied() const noexcept;

private:
  std::vector<CNFLit>
  computeUnoptimizedConflictClause(Clause &conflictingClause) const;
  int initializeResult(const Clause &conflictingClause,
                       std::vector<CNFLit> &result,
                       std::vector<CNFLit> &work) const;
  void addResolvent(const Clause &reason, CNFLit resolveAtLit,
                    std::vector<CNFLit> &result, int &unresolvedCount,
                    std::vector<CNFLit> &work) const;

  const DLProvider &m_dlProvider;
  const ReasonProvider &m_reasonProvider;
  const CNFVar m_maxVar;

  // Temporary storage for stamps, since we can't afford to allocate
  // (and thus initialize) a vector of m_maxVar variables each time
  // a conflict clause is computed. This member variable is governed
  // by class invariant A.
  mutable BoundedMap<CNFVar, char> m_stamps;

  // Class invariant A: m_stamps[x] = 0 for all keys x
};

template <class DLProvider, class ReasonProvider>
FirstUIPLearning<DLProvider, ReasonProvider>::FirstUIPLearning(
    CNFVar maxVar, const DLProvider &dlProvider,
    const ReasonProvider &reasonProvider)
    : m_dlProvider(dlProvider), m_reasonProvider(reasonProvider),
      m_maxVar(maxVar), m_stamps(maxVar) {}

namespace {
bool isAllZero(const BoundedMap<CNFVar, char> &stamps, CNFVar maxVar) noexcept {
  bool result = true;
  for (CNFVar::RawVariable v = 0; v <= maxVar.getRawValue(); ++v) {
    result &= (stamps[CNFVar{v}] == 0);
  }
  return result;
}
}

template <class DLProvider, class ReasonProvider>
int FirstUIPLearning<DLProvider, ReasonProvider>::initializeResult(
    const Clause &conflictingClause, std::vector<CNFLit> &result,
    std::vector<CNFLit> &work) const {
  int unresolvedCount = 0;

  // Mark the literals on the current decision levels as work, put
  // the rest into the result, stamp them all - this can be done
  // by resolving the conflicting clause with an empty clause and
  // adding an imaginary literal L rsp. ~L to the two clauses. The
  // imaginary literal is CNFLit::undefinedLiteral, in this case.
  addResolvent(conflictingClause, CNFLit::undefinedLiteral, result,
               unresolvedCount, work);

  // m_stamps is in a dirty state now, simulating out of memory conditions
  // for testing purposes (if enabled via FaultInjector)
  throwOnInjectedTestFault<std::bad_alloc>("FirstUIPLearning/low_memory");

  return unresolvedCount;
}

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<DLProvider, ReasonProvider>::addResolvent(
    const Clause &reason, CNFLit resolveAtLit, std::vector<CNFLit> &result,
    int &unresolvedCount, std::vector<CNFLit> &work) const {

  // Stamp literals on the current decision level and mark them as resolution
  // "work". All others already belong to the result: resolution is not
  // performed at these literals, since none of their inverses can appear in
  // reason clauses for variables on the current decision level. They may
  // appear in those reason clauses with the same sign, though, which is why
  // we need to keep track of the literals already included in the result.

  const auto currentLevel = m_dlProvider.getCurrentDecisionLevel();

  if (resolveAtLit != CNFLit::undefinedLiteral) {
    m_stamps[resolveAtLit.getVariable()] = 0;
  }

  for (auto reasonLit : reason) {
    BOOST_LOG_TRIVIAL(trace) << "Looking at: " << reasonLit;
    if (reasonLit != resolveAtLit && m_stamps[reasonLit.getVariable()] == 0) {
      m_stamps[reasonLit.getVariable()] = 1;
      if (m_dlProvider.getAssignmentDecisionLevel(reasonLit.getVariable()) ==
          currentLevel) {
        BOOST_LOG_TRIVIAL(trace) << "Adding work: " << reasonLit;
        ++unresolvedCount;
        work.push_back(reasonLit);
      } else {
        BOOST_LOG_TRIVIAL(trace) << "Adding literal: " << reasonLit;
        result.push_back(reasonLit);
      }
    }
  }

  // m_stamps may be in a dirty state, simulating out of memory conditions
  // for testing purposes (if enabled via FaultInjector)
  throwOnInjectedTestFault<std::bad_alloc>("FirstUIPLearning/low_memory");
}

template <class DLProvider, class ReasonProvider>
std::vector<CNFLit>
FirstUIPLearning<DLProvider, ReasonProvider>::computeUnoptimizedConflictClause(
    Clause &conflictingClause) const {
  // This implementation closely follows Donald Knuth's prosaic description
  // of first-UIP clause learning. See TAOCP, chapter 7.2.2.2.

  JAM_ASSERT(isAllZero(m_stamps, m_maxVar), "Class invariant A violated");

  try {
    std::vector<CNFLit> result;

    // The asserting literal will be stored as the first literal in the result.
    // Until the asserting literal has been determined, it is left undefined.
    result.push_back(CNFLit::undefinedLiteral);

    // The literals which are on the current decision level and which have been
    // encountered during the resolution process are stored in this vector.
    std::vector<CNFLit> work;

    // unresolvedCount counts how many literals L are left to resolve on the
    // current decision level. Until unresolvedCount is 1, the algorithm
    // picks such a literal L and resolves the current result with the reason
    // of L, if ~L is not a branching literal. (If the latter holds, L occurs
    // in the result and ~L occurs on the trail, making the resolution
    // possible.) When unresolvedCount == 1, the single remaining literal L
    // on the current decision level is the asserting literal.
    int unresolvedCount = initializeResult(conflictingClause, result, work);

    // If unresolvedCount == 1, the single literal on the current decision level
    // would have gotten a forced assignment on a lower decision level, which
    // is impossible. If unresolvedCount == 0, the clause has no literals
    // on the current decision level and could not have been part of the
    // conflict
    // in the first place, either.
    JAM_ASSERT(
        unresolvedCount >= 2,
        "Implementation error: fewer than 2 literals on current lvl found"
        " during initialization.");

    const auto currentLevel = m_dlProvider.getCurrentDecisionLevel();

    auto trailIterators =
        m_dlProvider.getDecisionLevelAssignments(currentLevel);
    auto span = trailIterators.end() - trailIterators.begin();
    auto cursor = trailIterators.begin() + span - 1;

    // Going down the trail backwards once, resolving the result with reason
    // clauses of items marked as "work" (i.e. literals occuring in the result
    // which are on the current decision level). This suffices, since given a
    // literal L at the i'th position of the trail whose assignment has been
    // forced by propagation, the reason clause of L can only contain literals
    // which occur on the trail at indices j <= i. Thus, if the reason of L
    // contains resolution work, it's guaranteed that the algorithm will visit
    // L later on.
    while (unresolvedCount > 1) {
      const CNFLit resolveAtLit = *cursor;
      const CNFVar resolveAtVar = resolveAtLit.getVariable();
      BOOST_LOG_TRIVIAL(trace) << "Current resolution candidate: " << *cursor;

      if (m_stamps[resolveAtVar] != 0 &&
          m_dlProvider.getAssignmentDecisionLevel(resolveAtVar) ==
              currentLevel) {
        auto reason = m_reasonProvider.getAssignmentReason(resolveAtVar);

        if (reason != nullptr) {
          addResolvent(*reason, resolveAtLit, result, unresolvedCount, work);
          --unresolvedCount;
        } else {
          // resolveAtLit is on the current decision level and can't be
          // removed from the result via resolution, so it must serve
          // as the asserting literal.
          BOOST_LOG_TRIVIAL(trace) << "Found the asserting literal: "
                                   << resolveAtLit;
          result[0] = ~resolveAtLit;
          m_stamps[resolveAtVar] = 0;

          // Not decreasing the unresolvedCount here, since this asserting
          // literal cannot be resolved, and _all_ other literals on the current
          // decision level need to be resolved.
        }
      }

      if (cursor == trailIterators.begin()) {
        break;
      }
      --cursor;
    }

    BOOST_LOG_TRIVIAL(trace) << "unresolved count after resolution: "
                             << unresolvedCount;
    JAM_ASSERT(
        unresolvedCount == 1,
        "Implementation error: didn't find exactly one asserting literal");

    // If the first UIP is not a branching literal, result[0] is still not a
    // defined literal. The asserting literal is now the single unresolved work
    // item, i.e. the single literal on the current decision level which has
    // been encountered in the resolution process, but not processed yet.
    if (result[0] == CNFLit::undefinedLiteral) {
      for (CNFLit w : work) {
        if (m_stamps[w.getVariable()] == 1) {
          BOOST_LOG_TRIVIAL(trace) << "Processing work with stamp: " << w;
          result[0] = w;
          break;
        }
      }
    }

    JAM_ASSERT(result[0] != CNFLit::undefinedLiteral,
               "Didn't find an asserting literal");

    // Satisfying class invariant A; the literals at which resolution
    // has been performed have already been un-stamped in the addResolvent
    // method.
    for (auto l : result) {
      m_stamps[l.getVariable()] = 0;
    }

    JAM_ASSERT(isAllZero(m_stamps, m_maxVar), "Class invariant A violated");

    return result;
  } catch (std::bad_alloc &oomException) {
    // Restore class invariant A before throwing on the exception.
    for (CNFVar::RawVariable v = 0; v <= m_maxVar.getRawValue(); ++v) {
      m_stamps[CNFVar{v}] = 0;
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

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<
    DLProvider, ReasonProvider>::test_assertClassInvariantsSatisfied() const
    noexcept {
  JAM_ASSERT(isAllZero(m_stamps, m_maxVar), "Class invariant A violated");
}
}
