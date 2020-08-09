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

/**
 * \file FirstUIPLearning.h
 * \brief A lemma derivation ("clause learning") implementation using First-UIP learning.
 *
 * Caution: lemma derivation is a performance-critical part of CDCL search.
 */

#pragma once

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/concepts/ClauseTraits.h>
#include <libjamsat/concepts/SolverTypeTraits.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/FaultInjector.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/Truth.h>

#if defined(JAM_ENABLE_CA_LOGGING)
#define JAM_LOG_CA(x, y) JAM_LOG(x, "resolu", y)
#else
#define JAM_LOG_CA(x, y)
#endif

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
 * after a conflict occurred. The conflict clause can then be used as a lemma.
 *
 * This implementation closely follows Donald Knuth's prosaic description
 * of first-UIP clause learning. See "The Art of Computer Programming", chapter 7.2.2.2.
 *
 * \tparam DLProvider           A type that is a model of the DecisionLevelProvider concept.
 * \tparam ReasonProvider       A type that is a model of the ReasonProvider concept, with the
 *                              reason type satisfying the LiteralContainer concept.
 *
 */
template <class DLProvider, class ReasonProvider>
class FirstUIPLearning {

public:
  /// The clause type. This type equals the reason provider's clause type.
  using Clause = typename ReasonProvider::Reason;

  static_assert(isReason_provider<ReasonProvider, Clause>::value,
                "Template argument ReasonProvider must satisfy the ReasonProvider concept, but"
                " does not");
  static_assert(is_literal_container<Clause>::value,
                "The ReasonProvider's reason type must satisfy LiteralContainer, but does not");
  static_assert(is_decision_level_provider<DLProvider>::value,
                "Template argument DLProvider must satisfy the DecisionLevelProvider concept,"
                " but does not");


  /**
   * \brief Constructs a new FirstUIPLearning instance.
   *
   * \param maxVar      The maximum variable occuring in the problem to be
   *                    solved. \p maxVar must be a regular variable.
   * \param dlProvider  The decision level providing object. Needs to live as
   *                    long as the constructed object.
   * \param reasonProvider  The assignment reason providing object. Needs to
   *                        live as long as the constructed object.
   */
  FirstUIPLearning(CNFVar maxVar,
                   const DLProvider& dlProvider,
                   ReasonProvider const& reasonProvider);

  /**
   * \brief Given a conflicting clause, computes a conflict clause.
   *
   * \param[in] conflictingClause  The conflicting clause, ie. a clause being
   *                               falsified through propagation under the current assignment.
   * \param[out] result            The conflict clause determined via resolutions
   *                               of the conflicting clause with reason clauses. The asserting
   *                               literal is placed first in the result.
   */
  void computeConflictClause(Clause& conflictingClause, std::vector<CNFLit>& result) const;


  /**
   * \brief Set the callback for variables seen during conflict resolution.
   *
   * The provided callback is called for all variables in the conflicting clause
   * as well as for all variables in all clauses with which the conflicting clause
   * is resolved. Per call to computeConflictClause(), the callback is invoked at
   * most once per variable.
   *
   * \param callback  The callback function to be installed (see above).
   */
  void setOnSeenVariableCallback(std::function<void(CNFVar)> const& callback) noexcept;

  /**
   * \brief Increases the maximum variable occuring in the problem to be solved.
   *
   * \param newMaxVar The new maximum variable. \p newMaxVar must not be smaller than the
   *                  previous maximum variable, and must be a regular variable.
   */
  void increaseMaxVarTo(CNFVar newMaxVar);

  /**
   * \brief Asserts that the class invariants are satisfied.
   */
  void test_assertClassInvariantsSatisfied() const noexcept;

private:
  /**
   * \brief Initializes the conflicting clause.
   *
   * The conflicting clause \p result is initialized by inserting all literals
   * of \p conflictingClause occuring on another decision level than the current
   * one. Furthermore, if the asserting literal is found during this call, it is
   * prepended to \p result. (This can happen if current decision level's
   * decision literal occurs in the conflicting clause.) Otherwise, the
   * undefined literal is prepended to \p result. All literals contained in \p
   * conflictingClause are stamped.
   *
   * When this method is invoked, class invariant A needs to hold. When this
   * method returns, \p{ m_stamps[v] = 1 } iff a literal occurs in \p result
   * with variable \p v or \p v is the variable of a literal at which resolution
   * needs to be performed.
   *
   * \param[in]  conflictingClause   A clause containing more than 1 literal of
   *                                 the current decision level.
   * \param[out] result              An empty vector, in which the initial part
   *                                 of the conflicting clause is stored.
   *
   * \returns The amount of literals on the current decision level found in
   *          \p conflictingClause.
   */
  auto initializeResult(Clause const& conflictingClause, std::vector<CNFLit>& result) const -> int;

  /**
   * \brief Completes result to be ( \p result joined with \p work ) resolved
   * with \p reason at \p resolveAtLit, omitting the literals of the current
   * decision level in the new \p result. `work` is the set of literals whose
   * variable has been assigned on the current decision level and which have been
   * encountered so far during the resolution process.
   *
   * When this method is invoked, it must hold that \p{ m_stamps[v] = 1 } iff \p
   * v is the variable of a literal occuring in \p result or \p v is the
   * variable of a literal at which resolution still remains to be performed.
   * This also holds when this method returns.
   *
   * \param[in] reason              The reason clause with which resolution
   * should be performed.
   * \param[in] resolveAtLit        The literal at whose variable which resolution should be
   *                                performed. This must be a literal contained in \p work.
   * \param[in,out] result          The result vector as described above
   *
   * \returns The amount of literals added to \p work
   */
  auto addResolvent(Clause const& reason, CNFLit resolveAtLit, std::vector<CNFLit>& result) const
      -> int;

  /**
   * \brief Iteratively resolves \p result with reason clauses of literals
   * occurring on the current decision level, aborting when having reached
   * the first unique implication point.
   *
   * \param[in, out] result          The vector in which the resolvent is stored.
   * \param[in] unresolvedCount      The amount of literals contained in \p result
   *                                 whose variable is on the current decision level.
   */
  void resolveUntilUIP(std::vector<CNFLit>& result, int unresolvedCount) const;

  /**
   * \brief Clears \p m_stamps for the variables of the given literals.
   *
   * \param lits    A vector of literals.
   */
  void clearStamps(std::vector<CNFLit> const& lits) const noexcept;

  const DLProvider& m_dlProvider;
  const ReasonProvider& m_reasonProvider;
  const CNFVar m_maxVar;

  // Temporary storage for stamps, since we can't afford to allocate
  // (and thus initialize) a vector of m_maxVar variables each time
  // a conflict clause is computed. This member variable is governed
  // by class invariant A.
  //
  // A variable v can be "stamped" for two reasons:
  //  - if v has been assigned on the current decision level: when traversing down the trail,
  //    resolution with v's reason clause needs to be performed.
  //  - if v has not been assigned on the current decision level: v occurs in the result.
  //
  // Note that only one variable assigned on the current decision level can actually occur
  // in the result: the variable of the asserting literal (UIP).
  //
  // Thus, m_stamps is used both for keeping track of remaining resolution work, and
  // for quickly deciding whether a variable already occurs in the result. These two concerns
  // are handled by the same data structure for memory efficiency.
  mutable BoundedMap<CNFVar, char> m_stamps;

  // Callback called once for every literal seen during conflict analysis
  std::function<void(CNFVar)> m_onSeenVariableCallback;

  // Class invariant A: m_stamps[x] = 0 for all keys x
};

/********** Implementation ****************************** */

template <class DLProvider, class ReasonProvider>
FirstUIPLearning<DLProvider, ReasonProvider>::FirstUIPLearning(CNFVar maxVar,
                                                               const DLProvider& dlProvider,
                                                               const ReasonProvider& reasonProvider)
  : m_dlProvider(dlProvider), m_reasonProvider(reasonProvider), m_maxVar(maxVar), m_stamps(maxVar)
{
  JAM_ASSERT(isRegular(maxVar), "Argument maxVar must be a regular variable.");
}

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<DLProvider, ReasonProvider>::increaseMaxVarTo(CNFVar newMaxVar)
{
  JAM_ASSERT(isRegular(newMaxVar), "Argument newMaxVar must be a regular variable.");
  CNFVar firstNewVar = CNFVar{static_cast<CNFVar::RawVariable>(m_stamps.size())};
  m_stamps.increaseSizeTo(newMaxVar);

  for (CNFVar i = firstNewVar; i <= newMaxVar; i = nextCNFVar(i)) {
    m_stamps[i] = 0;
  }
}

#if defined(JAM_ASSERT_ENABLED)
namespace detail_solver {
inline bool isAllZero(const BoundedMap<CNFVar, char>& stamps, CNFVar maxVar) noexcept
{
  bool result = true;
  for (CNFVar::RawVariable v = 0; v <= maxVar.getRawValue(); ++v) {
    result &= (stamps[CNFVar{v}] == 0);
  }
  return result;
}
}
#endif

template <class DLProvider, class ReasonProvider>
auto FirstUIPLearning<DLProvider, ReasonProvider>::initializeResult(
    Clause const& conflictingClause, std::vector<CNFLit>& result) const -> int
{

  result.push_back(CNFLit::getUndefinedLiteral());

  // Mark the literals on the current decision levels as work, put
  // the rest into the result, stamp them all - this can be done
  // by resolving the conflicting clause with an empty clause and
  // adding an imaginary literal L rsp. ~L to the two clauses. The
  // imaginary literal is CNFLit::getUndefinedLiteral(), in this case.
  int unresolvedCount = addResolvent(conflictingClause, CNFLit::getUndefinedLiteral(), result);

  // m_stamps is in a dirty state now, simulating out of memory conditions
  // for testing purposes (if enabled via FaultInjector)

#ifdef JAM_ENABLE_TEST_FAULTS
  throwOnInjectedTestFault<std::bad_alloc>("FirstUIPLearning/low_memory");
#endif

  // If unresolvedCount == 1, the single literal on the current decision level
  // would have gotten a forced assignment on a lower decision level, which
  // is impossible. If unresolvedCount == 0, the clause has no literals
  // on the current decision level and could not have been part of the
  // conflict in the first place, either.
  JAM_ASSERT(unresolvedCount >= 2,
             "Implementation error: fewer than 2 literals on current lvl found"
             " during initialization.");

  return unresolvedCount;
}

template <class DLProvider, class ReasonProvider>
auto FirstUIPLearning<DLProvider, ReasonProvider>::addResolvent(Clause const& reason,
                                                                CNFLit resolveAtLit,
                                                                std::vector<CNFLit>& result) const
    -> int
{
  int unresolvedCount = 0;

  // Stamp literals on the current decision level and mark them as resolution
  // "work". All others already belong to the result: resolution is not
  // performed at these literals, since none of their inverses can appear in
  // reason clauses for variables on the current decision level. They may
  // appear in those reason clauses with the same sign, though, which is why
  // we need to keep track of the literals already included in the result.

  const auto currentLevel = m_dlProvider.getCurrentLevel();

  if (resolveAtLit != CNFLit::getUndefinedLiteral()) {
    m_stamps[resolveAtLit.getVariable()] = 0;
  }

  // Micro-optimization: no push_back to avoid branches in the next loop
  // `result` is a reused array, so in most cases this will won't actually
  // cause allocations
  auto effectiveResultSize = result.size();
  result.resize(effectiveResultSize + reason.size());

  for (auto reasonLit : reason) {
    if (resolveAtLit != reasonLit && m_stamps[reasonLit.getVariable()] == 0) {
      m_stamps[reasonLit.getVariable()] = 1;

      if (m_dlProvider.getLevel(reasonLit.getVariable()) == currentLevel) {
        ++unresolvedCount;
      }
      else {
        result[effectiveResultSize] = reasonLit;
        ++effectiveResultSize;
      }
    }
  }

  result.resize(effectiveResultSize);

#ifdef JAM_ENABLE_TEST_FAULTS
  // m_stamps may be in a dirty state, simulating out of memory conditions
  // for testing purposes (if enabled via FaultInjector)
  throwOnInjectedTestFault<std::bad_alloc>("FirstUIPLearning/low_memory");
#endif
  return unresolvedCount;
}

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<DLProvider, ReasonProvider>::resolveUntilUIP(std::vector<CNFLit>& result,
                                                                   int unresolvedCount) const
{

  // unresolvedCount counts how many literals L are left to resolve on the
  // current decision level. Until unresolvedCount is 1, the algorithm
  // picks such a literal L and resolves the current result with the reason
  // of L, if ~L is not a branching literal. (If the latter holds, L occurs
  // in the result and ~L occurs on the trail, making the resolution
  // possible.) When unresolvedCount == 1, the single remaining literal L
  // on the current decision level is the asserting literal.

  JAM_LOG_CA(info, "  Resolving until UIP. Literals to resolve: " << unresolvedCount);

  const auto currentLevel = m_dlProvider.getCurrentLevel();
  auto trailIterators = m_dlProvider.getLevelAssignments(currentLevel);
  auto span = std::distance(trailIterators.begin(), trailIterators.end());
  auto cursor = trailIterators.begin() + span - 1;

  // Going down the trail backwards once, resolving the result with reason
  // clauses of items marked as "work" (i.e. literals occuring in the result
  // which are on the current decision level).
  //
  // Note: for efficiency, literals on the current decision level are not
  // actually stored in the result (except for the asserting literal at the UIP),
  // since they would be removed by resolution anyway.
  //
  // This suffices for first-UIP
  // learning, since given a literal L at the i'th position of the trail whose
  // assignment has been forced by propagation, the reason clause of L can
  // only contain literals which occur on the trail at indices j <= i. Thus,
  // if the reason of L contains resolution work, it's guaranteed that the
  // algorithm will visit L later on.

  while (unresolvedCount > 1) {
    const CNFLit resolveAtLit = *cursor;
    const CNFVar resolveAtVar = resolveAtLit.getVariable();
    JAM_LOG_CA(info, "  Resolving at literal: " << resolveAtLit);

    if (m_stamps[resolveAtVar] != 0) {
      if (m_onSeenVariableCallback) {
        m_onSeenVariableCallback(resolveAtLit.getVariable());
      }

      JAM_ASSERT(m_dlProvider.getLevel(resolveAtVar) == currentLevel,
                 "Expected to traverse only literals on the current decision level");
      auto reason = m_reasonProvider.getReason(resolveAtVar);

      JAM_ASSERT(reason != nullptr, "Encountered the UIP too early");
      unresolvedCount += addResolvent(*reason, resolveAtLit, result);
      --unresolvedCount;
      JAM_LOG_CA(info,
                 "  Resolved with reason clause "
                     << &reason << ". Remaining literals to resolve: " << unresolvedCount);
    }

    JAM_ASSERT(cursor != trailIterators.begin(),
               "Reached the beginning of the current "
               "decision level without finding the UIP");
    --cursor;
  }

  // Collect the asserting literal (UIP). Since the unresolved-count is 1, there is exactly
  // one literal L reachable by decrementing the cursor such that the variable of L is marked
  // in m_stamps. This is the asserting literal: its assignment caused the assignment of all
  // the current-decision-level literals and which ultimately caused the conflict and that have
  // been eliminated from the result using resolution.
  while (m_stamps[cursor->getVariable()] == 0) {
    JAM_ASSERT(cursor != trailIterators.begin(), "No UIP found on current decision level");
    --cursor;
  }
  result[0] = ~(*cursor);
  m_stamps[cursor->getVariable()] = 0;


  JAM_ASSERT(unresolvedCount == 1,
             "Implementation error: didn't find exactly one asserting literal");
}

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<DLProvider, ReasonProvider>::clearStamps(
    std::vector<CNFLit> const& lits) const noexcept
{
  for (CNFLit w : lits) {
    m_stamps[w.getVariable()] = 0;
    if (m_onSeenVariableCallback) {
      m_onSeenVariableCallback(w.getVariable());
    }
  }
}

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<DLProvider, ReasonProvider>::computeConflictClause(
    Clause& conflictingClause, std::vector<CNFLit>& result) const
{
  JAM_LOG_CA(info, "Beginning conflict analysis.");
  JAM_ASSERT(detail_solver::isAllZero(m_stamps, m_maxVar), "Class inv. A violated");

  try {
    result.clear();

    int unresolvedCount = initializeResult(conflictingClause, result);
    resolveUntilUIP(result, unresolvedCount);

    JAM_ASSERT(result[0] != CNFLit::getUndefinedLiteral(), "Didn't find an asserting literal");

    // Class invariant A gets satisfied here; the literals at which resolution
    // has been performed have already been un-stamped in the addResolvent
    // method.
    clearStamps(result);

    JAM_ASSERT(detail_solver::isAllZero(m_stamps, m_maxVar), "Class invariant A violated");

    JAM_LOG_CA(info, "Finished conflict resolution.");
  }
  catch (std::bad_alloc&) {
    // Restore class invariant A before throwing on the exception.
    for (CNFVar::RawVariable v = 0; v <= m_maxVar.getRawValue(); ++v) {
      m_stamps[CNFVar{v}] = 0;
    }
    throw;
  }
}

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<DLProvider, ReasonProvider>::setOnSeenVariableCallback(
    std::function<void(CNFVar)> const& callback) noexcept
{
  m_onSeenVariableCallback = callback;
}

template <class DLProvider, class ReasonProvider>
void FirstUIPLearning<DLProvider, ReasonProvider>::test_assertClassInvariantsSatisfied()
    const noexcept
{
  JAM_ASSERT(detail_solver::isAllZero(m_stamps, m_maxVar), "Class invariant A violated");
}
}
