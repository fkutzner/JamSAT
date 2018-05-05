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

#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/Watcher.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>
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
 * \tparam AssignmentProvider   A type satisfying the \ref AssignmentProvider concept.
 * \tparam ClauseT              A type satisfying the \ref SimpleClause concept.
 *
 * \concept{ReasonProvider}
 */
template <class AssignmentProvider, class ClauseT>
class Propagation {
private:
    using WatcherType = detail_propagation::Watcher<ClauseT>;
    using WatchersType = detail_propagation::Watchers<ClauseT>;
    using WatchersRangeType = typename WatchersType::AllWatchersRange;
    using ClauseRangeType = decltype(boost::adaptors::transform(
        std::declval<WatchersRangeType>(),
        std::declval<std::function<const ClauseT *(const WatcherType &)>>()));

public:
    using ClauseType = ClauseT;
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
    Propagation(CNFVar maxVar, AssignmentProvider &assignmentProvider);

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
    ClauseT *registerClause(ClauseT &clause);


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
    void registerEquivalentSubstitutingClause(ClauseT &clause);

    enum class ClearMode { NO_KEEP_REASONS, KEEP_REASONS };

    /**
     * \brief Unregisters all clauses from the propagation system.
     *
     * \param mode     Iff set to NO_KEEP_REASONS, the reason clause assignments are cleared
     *                 as well.
     */
    void clear(ClearMode mode) noexcept;

    /**
     * \brief Removes clauses marked as "to be deleted" from the propagation system.
     *
     * Clauses are considered marked "to be deleted" iff they have been passed to
     * markToBeDeleted().
     */
    void eraseClausesToBeDeleted();

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
     */
    ClauseT *propagateUntilFixpoint(CNFLit toPropagate);

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
     */
    ClauseT *propagate(CNFLit toPropagate, size_t &amountOfNewFacts);

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
    const ClauseT *getAssignmentReason(CNFVar variable) const noexcept;

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
    ClauseRange getClausesInPropagationOrder() const noexcept;

    /**
     * \brief Returns a map representing the binary clauses registered with the
     * propagation system.
     *
     * \return Let M be the value returned by this function. For each literal L with a
     * variable no greater than the current maximum variable, M[L] returns a
     * range containing exactly the literals L' such binary clause (L L') or
     * (L' L) has been registered with the propagation system.
     */
    BinariesMap getBinariesMap() const noexcept;


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
    bool isAssignmentReason(const ClauseT &clause, const DecisionLevelProvider &dlProvider) const
        noexcept;

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
    void updateAssignmentReason(const ClauseT &oldClause, const ClauseT &newClause) noexcept;

private:
    ClauseT *propagateBinaries(CNFLit toPropagate, size_t &amountOfNewFacts);
    ClauseT *registerClause(ClauseT &clause, bool autoPropagate);

    AssignmentProvider &m_assignmentProvider;
    BoundedMap<CNFVar, const ClauseT *> m_reasons;
    detail_propagation::Watchers<ClauseT> m_binaryWatchers;

    /**
     * \internal
     *
     * Invariants for m_watchers: for each registered clause C,
     *  - \p m_watchers contains exactly two different watchers pointing to C.
     *  - the lists \p m_watchers.getWatchers(C[0]) and \p
     * m_watchers.getWatchers(C[1]) each contain a watcher pointing to C.
     */
    detail_propagation::Watchers<ClauseT> m_watchers;
};

/********** Implementation ****************************** */

template <class AssignmentProvider, class ClauseT>
Propagation<AssignmentProvider, ClauseT>::Propagation(CNFVar maxVar,
                                                      AssignmentProvider &assignmentProvider)
  : m_assignmentProvider(assignmentProvider)
  , m_reasons(maxVar)
  , m_binaryWatchers(maxVar)
  , m_watchers(maxVar) {
    JAM_ASSERT(isRegular(maxVar), "Argument maxVar must be a regular variable.");
}

template <class AssignmentProvider, class ClauseT>
ClauseT *Propagation<AssignmentProvider, ClauseT>::registerClause(ClauseT &clause,
                                                                  bool autoPropagate) {
    JAM_ASSERT(clause.size() >= 2ull, "Illegally small clause argument");
    JAM_LOG_PROPAGATION(info, "Registering clause " << &clause << " ("
                                                    << toString(clause.begin(), clause.end())
                                                    << ") for propagation.");
    detail_propagation::Watcher<ClauseT> watcher1{clause, clause[0], 1};
    detail_propagation::Watcher<ClauseT> watcher2{clause, clause[1], 0};

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
        m_assignmentProvider.addAssignment(clause[0]);
        auto confl = propagateUntilFixpoint(clause[0]);
        // Fix the reason since this was not a decision:
        m_reasons[clause[0].getVariable()] = &clause;
        return confl;
    }
    return nullptr;
}

template <class AssignmentProvider, class ClauseT>
ClauseT *Propagation<AssignmentProvider, ClauseT>::registerClause(ClauseT &clause) {
    // Register with auto-propagation enabled:
    return registerClause(clause, true);
}

template <class AssignmentProvider, class ClauseT>
void Propagation<AssignmentProvider, ClauseT>::registerEquivalentSubstitutingClause(
    ClauseT &clause) {
    // Register with auto-propagation disabled
    registerClause(clause, false);
}

template <class AssignmentProvider, class ClauseT>
const ClauseT *Propagation<AssignmentProvider, ClauseT>::getAssignmentReason(CNFVar variable) const
    noexcept {
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_reasons.size()),
               "Variable out of bounds");
    return m_reasons[variable];
}

template <class AssignmentProvider, class ClauseT>
bool Propagation<AssignmentProvider, ClauseT>::hasForcedAssignment(CNFVar variable) const noexcept {
    JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariable>(m_reasons.size()),
               "Variable out of bounds");
    return m_reasons[variable] != nullptr;
}

template <class AssignmentProvider, class ClauseT>
ClauseT *Propagation<AssignmentProvider, ClauseT>::propagateUntilFixpoint(CNFLit toPropagate) {
    JAM_LOG_PROPAGATION(info, "Propagating assignment until fixpoint: " << toPropagate);
    JAM_ASSERT(toPropagate.getVariable().getRawValue() <
                   static_cast<CNFVar::RawVariable>(m_reasons.size()),
               "Literal variable out of bounds");
    m_reasons[toPropagate.getVariable()] = nullptr;
    auto trailEndIndex = m_assignmentProvider.getNumberOfAssignments();

    // Using the space on the trail beyond its current last literal as a
    // propagation queue.
    auto propagationQueue = m_assignmentProvider.getAssignments(trailEndIndex);

    size_t amountOfNewFacts = 0;
    ClauseT *conflictingClause = propagate(toPropagate, amountOfNewFacts);
    if (conflictingClause) {
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
        conflictingClause = propagate(*pqBegin, localNewFacts);
        pqEnd += localNewFacts;
        if (conflictingClause) {
            return conflictingClause;
        }
        ++pqBegin;
    }

    JAM_LOG_PROPAGATION(info, "  Done propagating to fixpoint.");
    // No more forced assignments can be propagated => fixpoint reached.
    return nullptr;
}

template <class AssignmentProvider, class ClauseT>
ClauseT *Propagation<AssignmentProvider, ClauseT>::propagateBinaries(CNFLit toPropagate,
                                                                     size_t &amountOfNewFacts) {
    CNFLit negatedToPropagate = ~toPropagate;
    auto watcherListTraversal = m_binaryWatchers.getWatchers(negatedToPropagate);
    while (!watcherListTraversal.hasFinishedTraversal()) {
        auto &currentWatcher = *watcherListTraversal;
        CNFLit secondLit = currentWatcher.getOtherWatchedLiteral();
        TBool assignment = m_assignmentProvider.getAssignment(secondLit);

        if (isFalse(assignment)) {
            // conflict case:
            return &currentWatcher.getClause();
        } else if (!isDeterminate(assignment)) {
            // propagation case:
            ++amountOfNewFacts;
            ClauseT *reason = &currentWatcher.getClause();
            m_reasons[secondLit.getVariable()] = reason;
            JAM_LOG_PROPAGATION(info,
                                "  Forced assignment: " << secondLit << " Reason: " << reason);
            m_assignmentProvider.addAssignment(secondLit);

            ++watcherListTraversal;
            continue;
        }

        ++watcherListTraversal;
    }
    watcherListTraversal.finishedTraversal();
    return nullptr;
}

template <class AssignmentProvider, class ClauseT>
ClauseT *Propagation<AssignmentProvider, ClauseT>::propagate(CNFLit toPropagate,
                                                             size_t &amountOfNewFacts) {
    JAM_LOG_PROPAGATION(info, "  Propagating assignment: " << toPropagate);
    JAM_ASSERT(toPropagate.getVariable().getRawValue() <
                   static_cast<CNFVar::RawVariable>(m_reasons.size()),
               "Literal variable out of bounds");

    amountOfNewFacts = 0;

    if (ClauseT *conflict = propagateBinaries(toPropagate, amountOfNewFacts)) {
        return conflict;
    }

    CNFLit negatedToPropagate = ~toPropagate;

    // Traverse all watchers referencing clauses containing ~toPropagate to find
    // new forced assignments.
    auto watcherListTraversal = m_watchers.getWatchers(negatedToPropagate);
    while (!watcherListTraversal.hasFinishedTraversal()) {
        auto currentWatcher = *watcherListTraversal;
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

        bool actionIsForced = true;
        for (typename ClauseT::size_type i = 2; i < clause.size(); ++i) {
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
                actionIsForced = false;
                break;
            }
        }

        if (actionIsForced) {
            // Invariant holding here: all literals in the clause beyond the second
            // literal have the value FALSE.
            if (isFalse(assignment)) {
                // Conflict case: all literals are FALSE. Return the conflicting clause.
                watcherListTraversal.finishedTraversal();
                JAM_LOG_PROPAGATION(info, "  Current assignment is conflicting at clause "
                                              << &clause << ".");
                return &clause;
            } else {
                // Propagation case: otherWatchedLit is the only remaining unassigned
                // literal
                ++amountOfNewFacts;
                m_reasons[otherWatchedLit.getVariable()] = &clause;
                JAM_LOG_PROPAGATION(info, "  Forced assignment: " << otherWatchedLit
                                                                  << " Reason: " << &clause);
                m_assignmentProvider.addAssignment(otherWatchedLit);
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

template <class AssignmentProvider, class ClauseT>
void Propagation<AssignmentProvider, ClauseT>::clear(ClearMode mode) noexcept {
    m_watchers.clear();
    if (mode == ClearMode::NO_KEEP_REASONS) {
        for (auto &clausePointer : m_reasons.values()) {
            clausePointer = nullptr;
        }
    }
}

template <class AssignmentProvider, class ClauseT>
void Propagation<AssignmentProvider, ClauseT>::eraseClausesToBeDeleted() {
#if defined(JAM_ASSERT_ENABLED)
    for (CNFLit assignment : m_assignmentProvider.getAssignments(0)) {
        auto reason = m_reasons[assignment.getVariable()];
        JAM_ASSERT(reason == nullptr || !isMarkedToBeDeleted(*reason),
                   "Can't erase reason clauses");
    }
#endif
    m_watchers.eraseWatchersToBeDeleted();
}

template <class AssignmentProvider, class ClauseT>
void Propagation<AssignmentProvider, ClauseT>::increaseMaxVarTo(CNFVar newMaxVar) {
    // TODO: assert that the assignment provider can provide assignments for newMaxVar.
    JAM_ASSERT(newMaxVar >= CNFVar{static_cast<CNFVar::RawVariable>(m_reasons.size() - 1)},
               "Argument newMaxVar must not be smaller than the previous maximum variable.");
    JAM_ASSERT(isRegular(newMaxVar), "Argument newMaxVar must be a regular variable.");
    m_watchers.increaseMaxVarTo(newMaxVar);
    m_reasons.increaseSizeTo(newMaxVar);
    m_binaryWatchers.increaseMaxVarTo(newMaxVar);
}

template <class AssignmentProvider, class ClauseT>
typename Propagation<AssignmentProvider, ClauseT>::ClauseRange
Propagation<AssignmentProvider, ClauseT>::getClausesInPropagationOrder() const noexcept {
    std::function<const ClauseT *(const WatcherType &)> trans = [](const WatcherType &w) {
        return &w.getClause();
    };

    return boost::adaptors::transform(m_watchers.getWatchersInTraversalOrder(), trans);
}

template <class AssignmentProvider, class ClauseT>
template <typename DecisionLevelProvider>
bool Propagation<AssignmentProvider, ClauseT>::isAssignmentReason(
    const ClauseT &clause, const DecisionLevelProvider &dlProvider) const noexcept {
    JAM_ASSERT(clause.size() >= 2, "Argument clause must at have a size of 2");
    for (auto var : {clause[0].getVariable(), clause[1].getVariable()}) {
        if (m_reasons[var] != &clause) {
            continue;
        }

        // The pointers in m_reasons do not get cleared eagerly during backtracking
        auto decisionLevel = dlProvider.getAssignmentDecisionLevel(var);
        if (decisionLevel <= dlProvider.getCurrentDecisionLevel()) {
            return true;
        }
    }
    return false;
}

template <class AssignmentProvider, class ClauseT>
void Propagation<AssignmentProvider, ClauseT>::updateAssignmentReason(
    const ClauseT &oldClause, const ClauseT &newClause) noexcept {
    // JAM_EXPENSIVE_ASSERT(oldClause == newClause, "Arguments oldClause and newClause must be
    // equal");
    for (auto var : {newClause[0].getVariable(), newClause[1].getVariable()}) {
        if (m_reasons[var] == &oldClause) {
            m_reasons[var] = &newClause;
        }
    }
}

template <class AssignmentProvider, class ClauseT>
auto Propagation<AssignmentProvider, ClauseT>::getBinariesMap() const noexcept -> BinariesMap {
    return m_binaryWatchers.getBlockerMap();
};
}
