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

#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/join.hpp>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/Watcher.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/Printers.h>
#include <libjamsat/utils/Truth.h>

#if defined(JAM_ENABLE_PROPAGATION_LOGGING)
#define JAM_LOG_PROPAGATION(x, y) JAM_LOG(x, "propgt", y)
#else
#define JAM_LOG_PROPAGATION(x, y)
#endif

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
 * \tparam AssignmentProviderT   A type that is a model of the \ref AssignmentProvider concept.
 *
 * \concept{ReasonProvider}
 */
template <class AssignmentProviderT>
class Propagation {
public:
    /// The clause type. This type equals the assignment provider's clause type.
    using Clause = typename AssignmentProviderT::Clause;

    /// The AssignmentProvider type.
    using AssignmentProvider = AssignmentProviderT;

private:
    using WatcherType = detail_propagation::Watcher<Clause>;
    using WatchersType = detail_propagation::Watchers<Clause>;
    using WatchersRangeType = typename WatchersType::AllWatchersRange;

    using PartialClauseRangeType = decltype(boost::adaptors::transform(
        std::declval<WatchersRangeType>(),
        std::declval<std::function<const Clause *(const WatcherType &)>>()));

    using ClauseRangeType = decltype(boost::range::join(std::declval<PartialClauseRangeType>(),
                                                        std::declval<PartialClauseRangeType>()));

public:
    using ClauseRange = ClauseRangeType;
    using BinariesMap = typename WatchersType::BlockerMapT;

    /**
     * \brief Constructs a new Propagation instance.
     *
     * \param maxVar              The largest variable occuring in the clauses on
     * which propagation is performed. \p maxVar must be a regular variable.
     * \param assignmentProvider  The assignment provider configured such that
     * assignments up to and including maxVar can be kept track of.
     */
    Propagation(CNFVar maxVar, AssignmentProviderT &assignmentProvider);

    /**
     * \brief Registers a clause in the propagation system.
     *
     * This method may only be called if one of the following conditions is satisfied:
     *
     * (1) No literals occurring in \p clause have an assignment.
     * (2) All literals occurring in \p clause except for the first one are assigned
     * to FALSE.
     *
     * If the condition (2) holds and the first literal in \p clause has no
     * assignment, the value of the first literal gets propagated until fixpoint.
     *
     * \param clause        The clause to be registered, which must exist until this
     * is destroyed or it is deregistered from this.
     * \returns         A conflicting clause if adding \clause caused a
     * propagation and a conflict occured; nullptr otherwise.
     */
    auto registerClause(Clause &clause) -> Clause *;


    /**
     * \brief Re-registers a clause with the propagation system.
     *
     * This method may only be called if the following conditions are met:
     *
     * (1) `clear()` has been invoked
     * (2) Neither `propagate()` nor `propagateToFixpoint()` has been invoked since the
     *     last invocation of `clear()`
     * (3) During the last call to `clear()`, a clause `C` has been removed from
     *     the propagation system with `C` containing exactly the literals contained
     *     in `clause`, in the same order.
     *
     * No assignments are propagated.
     *
     * Typical usage: re-register clauses after relocation during clause database cleanup.
     *
     * \param clause    The clause to be inserted.
     */
    void registerEquivalentSubstitutingClause(Clause &clause);

    /**
     * \brief Unregisters all clauses from the propagation system.
     */
    void clear() noexcept;

    enum PropagationMode : uint8_t { EXCLUDE_REDUNDANT_CLAUSES, INCLUDE_REDUNDANT_CLAUSES };

    /**
     * \brief Propagates the given fact wrt. the clauses registered in the
     * propagation object, further propagating forced assignments until the
     * variable assignment  reaches a fixpoint.
     *
     * As soon as a new fact has been deduced, the assignment provider's
     * addAssignment(l) method is called with l encoding the new fact. If the
     * propagation leads to a conflict, a pointer to the clause falsified under
     * the current assignment is returned.
     *
     * \param toPropagate       The fact to propagate, encoded as a literal.
     * \param propagationMode   Iff `INCLUDE_REDUNDANT_CLAUSES`, clauses having
     *                          the REDUNDANT set are propagated.
     */
    auto propagateUntilFixpoint(CNFLit toPropagate,
                                PropagationMode mode = PropagationMode::INCLUDE_REDUNDANT_CLAUSES)
        -> Clause *;

    /**
     * \brief Propagates the given fact wrt. the clauses registered in the
     * propagation object.
     *
     * As soon as a new fact has been deduced, the assignment provider's
     * addAssignment(l) method is called with l encoding the new fact. If the
     * propagation leads to a conflict, a pointer to the clause falsified under
     * the current assignment is returned.
     *
     * \param toPropagate       The fact to propagate, encoded as a literal.
     * \param amountOfNewFacts  (out-parameter) The amount of facts added due to
     * propagation is stored in the variable referenced by \p amountOfNewFacts.
     * \returns \p nullptr if the fact has been propagated without any clause
     * being falsified; otherwise, the pointer to a clause falsified under the
     * current assignment is returned.
     *
     * \tparam propagateRedundantClauses    Iff true, clauses with the REDUNDANT
     *                                      flag are propagated.
     */
    template <bool propagateRedundantClauses = true>
    auto propagate(CNFLit toPropagate, size_t &amountOfNewFacts) -> Clause *;

    /**
     * \brief Determines whether the given variable as a forced assignment.
     *
     * \param variable  The target variable. Must not be greater than \p maxVar
     * passed to the constructor. \p variable must be a variable with a
     * determinate truth value.
     */
    auto hasForcedAssignment(CNFVar variable) const noexcept -> bool;

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
    auto getAssignmentReason(CNFVar variable) const noexcept -> const Clause *;

    /**
     * \brief Increases the maximum variable which may occur during propagation.
     *
     * \param newMaxVar     The new maximum variable. Must not be smaller than the previous
     *                      maximum variable, and must be a regular variable.
     */
    void increaseMaxVarTo(CNFVar newMaxVar);


    /**
     * \brief Returns a range of clause pointers in order of propagation.
     *
     * For pointers c1, c2 in the returned range, it holds that during the next propagation of a
     * fact, if both *c1 and *c2 are accessed, *c1 is accessed before *c2.
     *
     * \returns A range of clause pointers as described above.
     */
    auto getClausesInPropagationOrder() const noexcept -> ClauseRange;

    /**
     * \brief Returns a map representing the binary clauses registered with the
     * propagation system.
     *
     * \return Let M be the value returned by this function. For each literal L with a
     * variable no greater than the current maximum variable, M[L] returns a
     * range containing exactly the literals L' such binary clause (L L') or
     * (L' L) has been registered with the propagation system.
     */
    auto getBinariesMap() const noexcept -> BinariesMap;


    /**
     * \brief Determines whether the given clause is an assignment reason clause.
     *
     * \param clause        A clause containing at least two literals. The literals in \p clause
     *                      must not contain variables greater than the Propagation object's
     *                      maximum variable.
     * \param dlProvider    The decision level provider.
     * \returns             True iff \p clause is an assignment reason clause.
     *
     * \tparam DecisionLevelProvider    A type satisfying the DecisionLevelProvider concept.
     */
    template <typename DecisionLevelProvider>
    auto isAssignmentReason(const Clause &clause, const DecisionLevelProvider &dlProvider) const
        noexcept -> bool;

    /**
     * \brief Updates the location of an assignment reason clause.
     *
     * This method replaces the assignment reason clause for the variable assignment forced by
     * \p oldClause by the clause \p newClause. \p newClause must be equal to \p oldClause
     * (including the ordering of the literals within the clauses). After completion of this
     * method, \p oldClause is no longer referenced by the propagation system.
     *
     * \param oldClause     An assignment reason clause.
     * \param newClause     The replacement for \p oldClause, with oldClause == newClause.
     */
    void updateAssignmentReason(const Clause &oldClause, const Clause &newClause) noexcept;

    /**
     * \brief Returns the amount of assignments which have been placed on
     * the trail during the last propagation to fixpoint, but have not been
     * propagated.
     *
     * \return The amount of assignments which have been placed on
     * the trail during the last propagation to fixpoint, but have not been
     * propagated.
     */
    auto getCurrentAmountOfUnpropagatedAssignments() const noexcept -> uint64_t;

    /**
     * \brief Notifies the propagation system that a clause will have been
     * modified before the next propagation.
     *
     * For a clause `C`, this method needs to be called when the set of literals
     * contained in `C` will change before the next propagation.
     *
     * This method must also be called if `C` is about to be deleted and shall
     * not be taken into account during the next propagation.
     *
     * @param clause  A clause that is currently registered with the propagation
     *                system and contains at least three literals. If the set of
     *                literals contained in `clause` will be different than
     *                at the last propagation rsp. registration of `clause`, this
     *                method must be called before the modification takes place.
     */
    void notifyClauseModificationAhead(Clause const &clause) noexcept;

private:
    Clause *propagateBinaries(CNFLit toPropagate, size_t &amountOfNewFacts);
    Clause *registerClause(Clause &clause, bool autoPropagate);
    void cleanupWatchers(CNFLit lit);

    AssignmentProviderT &m_assignmentProvider;
    detail_propagation::Watchers<Clause> m_binaryWatchers;

    /**
     * \internal
     *
     * The number of the assignments which have been placed on the trail
     * during the last propagation to fixpoint, but have not been propagated.
     * Keeping track of this to enable more precise statistics. This value's
     * computation is cheap enough to perform regardless of whether the
     * amount of propagations is aggregated by the statistics system.
     */
    uint64_t m_unpropagatedStats;

    /**
     * \internal
     *
     * Invariants for m_watchers: for each registered clause C,
     *  - \p m_watchers contains exactly two different watchers pointing to C.
     *  - the lists \p m_watchers.getWatchers(C[0]) and \p
     * m_watchers.getWatchers(C[1]) each contain a watcher pointing to C.
     */
    detail_propagation::Watchers<Clause> m_watchers;

    /**
     * \internal
     *
     * A flag-map for CNFLit marking literals for which the corresponding
     * watch-lists must be updated, i.e remove clauses scheduled for
     * deletion, rewrite watchers out of sync with their clause (i.e. first
     * or second literal has been changed)
     */
    BoundedMap<CNFLit, char> m_watcherUpdateRequired;
};

/********** Implementation ****************************** */

template <class AssignmentProvider>
Propagation<AssignmentProvider>::Propagation(CNFVar maxVar, AssignmentProvider &assignmentProvider)
  : m_assignmentProvider(assignmentProvider)
  , m_binaryWatchers(maxVar)
  , m_unpropagatedStats(0ULL)
  , m_watchers(maxVar)
  , m_watcherUpdateRequired(getMaxLit(maxVar)) {
    JAM_ASSERT(isRegular(maxVar), "Argument maxVar must be a regular variable.");
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::registerClause(Clause &clause, bool autoPropagate)
    -> Clause * {
    JAM_ASSERT(clause.size() >= 2ull, "Illegally small clause argument");
    JAM_LOG_PROPAGATION(info, "Registering clause " << &clause << " ("
                                                    << toString(clause.begin(), clause.end())
                                                    << ") for propagation.");
    bool isRedundant = clause.getFlag(Clause::Flag::REDUNDANT);
    detail_propagation::Watcher<Clause> watcher1{clause, clause[0], 1, isRedundant};
    detail_propagation::Watcher<Clause> watcher2{clause, clause[1], 0, isRedundant};

    auto &targetWatchList = (clause.size() <= 2 ? m_binaryWatchers : m_watchers);
    targetWatchList.addWatcher(clause[0], watcher2);
    targetWatchList.addWatcher(clause[1], watcher1);

    if (!autoPropagate) {
        return nullptr;
    }

    TBool secondLiteralAssignment = m_assignmentProvider.getAssignment(clause[1]);
    // By method contract, if secondLiteralAssignment != INDETERMINATE, we need
    // to propagate the first literal.
    if (isDeterminate(secondLiteralAssignment)) {
        JAM_EXPENSIVE_ASSERT(
            std::all_of(
                clause.begin() + 1, clause.end(),
                [this](CNFLit l) { return isFalse(m_assignmentProvider.getAssignment(l)); }),
            "Added a clause requiring first-literal propagation which does not actually "
            "force the first literal");
        JAM_LOG_PROPAGATION(info, "Propagating first literal of registered clause.");
        // Fix the reason since this was not a decision:
        m_assignmentProvider.addAssignment(clause[0], clause);
        auto confl = propagateUntilFixpoint(clause[0]);
        return confl;
    }
    return nullptr;
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::registerClause(Clause &clause) -> Clause * {
    // Register with auto-propagation enabled:
    return registerClause(clause, true);
}

template <class AssignmentProvider>
void Propagation<AssignmentProvider>::registerEquivalentSubstitutingClause(Clause &clause) {
    // Register with auto-propagation disabled
    registerClause(clause, false);
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::getAssignmentReason(CNFVar variable) const noexcept
    -> const Clause * {
    return m_assignmentProvider.getAssignmentReason(variable);
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::hasForcedAssignment(CNFVar variable) const noexcept -> bool {
    return m_assignmentProvider.getAssignmentReason(variable) != nullptr;
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::propagateUntilFixpoint(CNFLit toPropagate,
                                                             PropagationMode mode) -> Clause * {
    JAM_LOG_PROPAGATION(info, "Propagating assignment until fixpoint: " << toPropagate);
    auto trailEndIndex = m_assignmentProvider.getNumberOfAssignments();

    // Using the space on the trail beyond its current last literal as a
    // propagation queue.
    auto propagationQueue = m_assignmentProvider.getAssignments(trailEndIndex);

    m_unpropagatedStats = 0ULL;
    size_t amountOfNewFacts = 0;
    Clause *conflictingClause = nullptr;
    if (mode == PropagationMode::INCLUDE_REDUNDANT_CLAUSES) {
        conflictingClause = propagate<true>(toPropagate, amountOfNewFacts);
    } else {
        conflictingClause = propagate<false>(toPropagate, amountOfNewFacts);
    }
    if (conflictingClause) {
        m_unpropagatedStats = amountOfNewFacts;
        return conflictingClause;
    }

    // Propagate all forced assignments. New assignments are added to the
    // assignment provider by propagate, and therefore are also added to the
    // propagation queue.
    auto pqBegin = propagationQueue.begin();
    auto pqEnd = propagationQueue.end() + amountOfNewFacts;
    while (pqBegin != pqEnd) {
        JAM_LOG_PROPAGATION(info, "  Propagating until fixpoint: " << amountOfNewFacts
                                                                   << " assignments pending");
        size_t localNewFacts = 0;
        if (mode == PropagationMode::INCLUDE_REDUNDANT_CLAUSES) {
            conflictingClause = propagate<true>(*pqBegin, localNewFacts);
        } else {
            conflictingClause = propagate<false>(*pqBegin, localNewFacts);
        }
        pqEnd += localNewFacts;
        if (conflictingClause) {
            m_unpropagatedStats = (propagationQueue.end() - propagationQueue.begin()) - 1;
            return conflictingClause;
        }
        ++pqBegin;
    }

    JAM_LOG_PROPAGATION(info, "  Done propagating to fixpoint.");
    // No more forced assignments can be propagated => fixpoint reached.
    return nullptr;
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::propagateBinaries(CNFLit toPropagate,
                                                        size_t &amountOfNewFacts) -> Clause * {
    CNFLit negatedToPropagate = ~toPropagate;
    auto watcherListTraversal = m_binaryWatchers.getWatchers(negatedToPropagate);
    while (!watcherListTraversal.hasFinishedTraversal()) {
        auto &currentWatcher = *watcherListTraversal;
        CNFLit secondLit = currentWatcher.getOtherWatchedLiteral();
        TBool assignment = m_assignmentProvider.getAssignment(secondLit);

        if (isFalse(assignment)) {
            // conflict case:
            JAM_LOG_PROPAGATION(info, "  Current assignment is conflicting at clause "
                                          << &currentWatcher.getClause() << ".");
            return &currentWatcher.getClause();
        } else if (!isDeterminate(assignment)) {
            // propagation case:
            ++amountOfNewFacts;
            Clause &reason = currentWatcher.getClause();
            JAM_LOG_PROPAGATION(info,
                                "  Forced assignment: " << secondLit << " Reason: " << &reason);
            m_assignmentProvider.addAssignment(secondLit, reason);

            ++watcherListTraversal;
            continue;
        }

        ++watcherListTraversal;
    }
    watcherListTraversal.finishedTraversal();
    return nullptr;
}

template <class AssignmentProvider>
template <bool propagateRedundantClauses>
auto Propagation<AssignmentProvider>::propagate(CNFLit toPropagate, size_t &amountOfNewFacts)
    -> Clause * {
    JAM_LOG_PROPAGATION(info, "  Propagating assignment: " << toPropagate);
    amountOfNewFacts = 0;

    if (m_watcherUpdateRequired[~toPropagate] != 0) {
        cleanupWatchers(~toPropagate);
    }

    if (Clause *conflict = propagateBinaries(toPropagate, amountOfNewFacts)) {
        return conflict;
    }

    CNFLit negatedToPropagate = ~toPropagate;

    // Traverse all watchers referencing clauses containing ~toPropagate to find
    // new forced assignments.
    auto watcherListTraversal = m_watchers.getWatchers(negatedToPropagate);
    while (!watcherListTraversal.hasFinishedTraversal()) {
        auto currentWatcher = *watcherListTraversal;

        if (!propagateRedundantClauses && currentWatcher.isClauseRedundant()) {
            ++watcherListTraversal;
            continue;
        }

        CNFLit otherWatchedLit = currentWatcher.getOtherWatchedLiteral();
        TBool assignment = m_assignmentProvider.getAssignment(otherWatchedLit);

        if (isTrue(assignment)) {
            // The clause is already satisfied and can be ignored for propagation.
            ++watcherListTraversal;
            continue;
        }

        auto &clause = currentWatcher.getClause();

        // otherWatchedLit might not actually be the other watched literal due to
        // the swap at (*), so restore it
        otherWatchedLit = clause[1 - currentWatcher.getIndex()];
        assignment = m_assignmentProvider.getAssignment(otherWatchedLit);
        if (isTrue(assignment)) {
            // The clause is already satisfied and can be ignored for propagation.
            ++watcherListTraversal;
            continue;
        }

        // Invariant: both watchers pointing to the clause have an other watched
        // literal pointing either to clause[0] or clause[1], but not to the literal
        // which is their index in m_watchers.

        for (typename Clause::size_type i = 2; i < clause.size(); ++i) {
            CNFLit currentLiteral = clause[i];
            if (!isFalse(m_assignmentProvider.getAssignment(currentLiteral))) {
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
                std::swap(clause[currentWatcher.getIndex()], clause[i]); // (*, see above)
                m_watchers.addWatcher(currentLiteral, currentWatcher);
                watcherListTraversal.removeCurrent();

                // No action is forced: skip to outer_continue to save some branches
                // Unfortunately, clang doesn't seem to jump there automatically if
                // some flag is set here and tested later :(
                goto outer_continue;
            }
        }

        // An action is forced: otherwise, the jump to outer_continue would have
        // been taken in the loop above

        // Invariant holding here: all literals in the clause beyond the second
        // literal have the value FALSE.
        if (isFalse(assignment)) {
            // Conflict case: all literals are FALSE. Return the conflicting clause.
            watcherListTraversal.finishedTraversal();
            JAM_LOG_PROPAGATION(info,
                                "  Current assignment is conflicting at clause " << &clause << ".");
            return &clause;
        } else {
            // Propagation case: otherWatchedLit is the only remaining unassigned
            // literal
            ++amountOfNewFacts;
            JAM_LOG_PROPAGATION(info, "  Forced assignment: " << otherWatchedLit
                                                              << " Reason: " << &clause);
            m_assignmentProvider.addAssignment(otherWatchedLit, clause);
        }

        // Only advancing the traversal if an action is forced, since otherwise
        // the current watcher has been removed via removeCurrent() and
        // watcherListTraversal already points to the next watcher.
        ++watcherListTraversal;

    outer_continue:
        noOp();
    }

    watcherListTraversal.finishedTraversal();
    return nullptr;
}

template <class AssignmentProvider>
void Propagation<AssignmentProvider>::clear() noexcept {
    m_watchers.clear();
    m_binaryWatchers.clear();
}

template <class AssignmentProvider>
void Propagation<AssignmentProvider>::increaseMaxVarTo(CNFVar newMaxVar) {
    JAM_ASSERT(isRegular(newMaxVar), "Argument newMaxVar must be a regular variable.");
    m_watchers.increaseMaxVarTo(newMaxVar);
    m_binaryWatchers.increaseMaxVarTo(newMaxVar);
    m_watcherUpdateRequired.increaseSizeTo(getMaxLit(newMaxVar));
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::getClausesInPropagationOrder() const noexcept -> ClauseRange {
    std::function<const Clause *(const WatcherType &)> trans = [](const WatcherType &w) {
        return &w.getClause();
    };

    const auto regularClauses =
        boost::adaptors::transform(m_watchers.getWatchersInTraversalOrder(), trans);
    const auto binaryClauses =
        boost::adaptors::transform(m_binaryWatchers.getWatchersInTraversalOrder(), trans);

    return boost::range::join(regularClauses, binaryClauses);
}

template <class AssignmentProvider>
template <typename DecisionLevelProvider>
auto Propagation<AssignmentProvider>::isAssignmentReason(
    const Clause &clause, const DecisionLevelProvider &dlProvider) const noexcept -> bool {
    JAM_ASSERT(clause.size() >= 2, "Argument clause must at have a size of 2");
    for (auto var : {clause[0].getVariable(), clause[1].getVariable()}) {
        if (m_assignmentProvider.getAssignmentReason(var) != &clause) {
            continue;
        }

        // The reason pointers do not neccessarily get cleared eagerly during backtracking
        auto decisionLevel = dlProvider.getAssignmentDecisionLevel(var);
        if (decisionLevel <= dlProvider.getCurrentDecisionLevel()) {
            return true;
        }
    }
    return false;
}

template <class AssignmentProvider>
void Propagation<AssignmentProvider>::updateAssignmentReason(const Clause &oldClause,
                                                             const Clause &newClause) noexcept {
    // JAM_EXPENSIVE_ASSERT(oldClause == newClause, "Arguments oldClause and newClause must be
    // equal");
    for (auto var : {newClause[0].getVariable(), newClause[1].getVariable()}) {
        if (m_assignmentProvider.getAssignmentReason(var) == &oldClause) {
            m_assignmentProvider.setAssignmentReason(var, &newClause);
        }
    }
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::getBinariesMap() const noexcept -> BinariesMap {
    return m_binaryWatchers.getBlockerMap();
}

template <class AssignmentProvider>
auto Propagation<AssignmentProvider>::getCurrentAmountOfUnpropagatedAssignments() const noexcept
    -> uint64_t {
    return m_unpropagatedStats;
}

template <class AssignmentProvider>
void Propagation<AssignmentProvider>::notifyClauseModificationAhead(Clause const &clause) noexcept {
    JAM_ASSERT(clause.size() >= 2, "Can't modify clauses with size <= 1");
    m_watcherUpdateRequired[clause[0]] = 1;
    m_watcherUpdateRequired[clause[1]] = 1;
}

template <class AssignmentProvider>
void Propagation<AssignmentProvider>::cleanupWatchers(CNFLit lit) {
    // This is not implemented as a detail of the watcher data structure
    // since watchers may be moved from the "regular" watchers to the binary
    // ones.
    // Since notifyClauseModificationAhead() may not be called for binary
    // clauses, it is sufficient to traverse the non-binary watchers.

    auto watcherListTraversal = m_watchers.getWatchers(lit);
    while (!watcherListTraversal.hasFinishedTraversal()) {
        WatcherType currentWatcher = *watcherListTraversal;
        Clause &clause = currentWatcher.getClause();

        if (clause.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION) == true) {
            watcherListTraversal.removeCurrent();
            continue;
        }
        JAM_ASSERT(clause.size() >= 2,
                   "Clauses shrinked to size 1 must be removed from propagation");

        if (clause.size() == 2) {
            // The clause has become a binary clause ~> move to binary watchers
            currentWatcher.setOtherWatchedLiteral(clause[1 - currentWatcher.getIndex()]);

            // When a clause becomes binary, it may also lose its redundancy status.
            // However, the redundancy is not relevant for binary clauses wrt. propagation,
            // so just clear the flag:
            currentWatcher.setClauseRedundant(false);

            m_binaryWatchers.addWatcher(clause[currentWatcher.getIndex()], currentWatcher);
            watcherListTraversal.removeCurrent();
        } else if (clause[currentWatcher.getIndex()] != lit) {
            // The clause has been modified externally and this watcher watches
            // the wrong literal ~> move the watcher
            currentWatcher.setOtherWatchedLiteral(clause[1 - currentWatcher.getIndex()]);

            // Optimizations (e.g. subsumption) may promote redundant clauses to
            // non-redundant clauses, so update the redundancy flag:
            currentWatcher.setClauseRedundant(clause.getFlag(Clause::Flag::REDUNDANT));

            m_watchers.addWatcher(clause[currentWatcher.getIndex()], currentWatcher);
            watcherListTraversal.removeCurrent();
        } else {
            ++watcherListTraversal;
        }
    }
    watcherListTraversal.finishedTraversal();

    auto binaryWatcherListTraversal = m_binaryWatchers.getWatchers(lit);
    while (!binaryWatcherListTraversal.hasFinishedTraversal()) {
        WatcherType currentWatcher = *binaryWatcherListTraversal;
        Clause &clause = currentWatcher.getClause();

        if (clause.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION) == true) {
            binaryWatcherListTraversal.removeCurrent();
        } else {
            JAM_ASSERT(clause.size() >= 2,
                       "Clauses shrinked to size 1 must be removed from propagation");
            ++binaryWatcherListTraversal;
        }
    }
    binaryWatcherListTraversal.finishedTraversal();
    m_watcherUpdateRequired[lit] = 0;
}
}
