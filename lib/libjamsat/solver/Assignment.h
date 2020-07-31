/* Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)

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

#include <boost/range.hpp>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/Watcher.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/BoundedStack.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {

/**
 * \ingroup JamSAT_Solver
 * 
 * \brief Class representing a variable assignment.
 */
class Assignment final {
public:
    using const_iterator = BoundedStack<CNFLit>::const_iterator;

    /** Assignment level index */
    using level = std::uint32_t;

    /** Range type for assignments, expressed as literals */
    using AssignmentRange = boost::iterator_range<const_iterator>;

    using size_type = BoundedStack<CNFLit>::size_type;

    using Reason = Clause;
    using DecisionLevel = level;

    /**
     * \brief Constructs an assignment object.
     * 
     * \param max_var The maximum variable occurring in the problem instance.
     * 
     * \throw std::bad_alloc on memory allocation failure.
     */
    explicit Assignment(CNFVar max_var);

    /**
     * \brief Increases the maximum variable occurring in the problem instance.
     * 
     * \param var    A variable; must not be smaller than the previous maximum variable.
     * 
     * \throw std::bad_alloc on memory allocation failure.
     */
    void increaseMaxVar(CNFVar var);

    /** Unit propagation mode */
    enum class up_mode { exclude_lemmas, include_lemmas };

    /**
     * \brief Adds the given literal to the current variable assignment along with all
     *   immediately consequential assignments, via unit propagation.
     * 
     * The variable `v` of \p l is assigned `true` iff the sign of \p l is positive.
     * 
     * \param l                 The literal to be added. \p l must be unassigned.
     * \param up_mode           If exclude_lemmas, then only clauses not marked
     *                          as redundant are considered when computing consequences.
     *                          Otherwise, all clauses are considered.
     * 
     * \returns If the operation results in a conflicting assignment, a conflicting clause
     *   is returned, ie. a clause that is falsified under the new assignment. Otherwise,
     *   `nullptr` is returned.
     * 
     * \throw std::bad_alloc on memory allocation failure.
     */
    auto append(CNFLit l, up_mode mode = up_mode::include_lemmas) -> Clause*;

    /**
     * \brief Returns the truth value of the given literal under the current assignment.
     */
    auto getAssignment(CNFLit lit) const noexcept -> TBool;

    /**
     * \brief Returns the truth value of the given variable under the current assignment.
     */
    auto getAssignment(CNFVar var) const noexcept -> TBool;

    /**
     * \brief Returns the most recently assigned truth value of the given variable.
     */
    auto getPhase(CNFVar var) const noexcept -> TBool;

    /**
     * \brief Returns `true` iff all variables have an assignment.
     */
    auto isComplete() const noexcept -> bool;

    /**
     * \brief Returns the number of current variable assignments.
     */
    auto getNumAssignments() const noexcept -> size_type;

    /**
     * \brief Registers a clause (without assignments) for participating in consequence
     *   computation.
     * 
     * \param clause    A clause that is not yet participating in consequence computation.
     *   \p clause must reference a valid object until `clear()` is called or the assignment
     *   object is destroyed. If the clause is modified (except by this object),
     *   `registerClauseModification()` must be called accordingly. No literal in \p clause
     *   must have an assignment yet.
     * 
     * \throw std::bad_alloc on memory allocation failure.
     */
    void registerClause(Clause& clause);

    /**
     * \brief Registers a clause currently forcing an assignment for participating in
     *   consequence computation.
     * 
     * The forced assignment and all its consequences are added to the assignment.
     * 
     * \param clause   A clause that is not yet participating in consequence computation.
     *   \p clause must reference a valid object until `clear()` is called or the assignment
     *   object is destroyed. If the clause is modified (except by this object),
     *   `registerClauseModification()` must be called accordingly. All literals except
     *   the first one must have a `false` assignment; the first literal of the clause must
     *   be unassigned.
     * 
     * \param asserting_lit The first literal of \p{clause}.
     * 
     * \returns If any consequence causes the assignment to become inconsistent, a clause
     *   which is unsatisfied under the current assignment is returned. Otherwise, nullptr
     *   is returned.
     * 
     * \throw std::bad_alloc on memory allocation failure.
     */
    auto registerLemma(Clause& clause) -> Clause*;

    /**
     * \brief Registers a clause modification.
     * 
     * \param clause   The modified clause.
     */
    void registerClauseModification(Clause& clause) noexcept;

    /**
     * \brief Unregisters all clauses from participating in consequence computation.
     */
    void clearClauses() noexcept;


    /**
     * \brief Returns the clause having forced the assignment of the given variable.
     * 
     * \returns the clause having forced the assignment of the given variable, if any; otherwise,
     *   `nullptr` is returned.
     */
    auto getReason(CNFVar var) const noexcept -> Clause const*;


    /**
     * \brief Returns the clause having forced the assignment of the given variable.
     * 
     * \returns the clause having forced the assignment of the given variable, if any; otherwise,
     *   `nullptr` is returned.
     */
    auto getReason(CNFVar var) noexcept -> Clause*;


    /**
     * \brief Returns `true` iff the given clause is the reason for any variable assignment.
     * 
     * \param clause    A clause registered with this assignment object. The clause must have at
     *                  least 2 literals.
     */
    auto isReason(Clause& clause) noexcept -> bool;

    /**
     * \brief Determines whether the given variable's assignment was forced by propagation.
     */
    auto isForced(CNFVar var) const noexcept;

    /**
     * \brief Increases the level of the assignment.
     */
    void newLevel() noexcept;

    /**
     * \brief Gets the current level.
     */
    auto getCurrentLevel() const noexcept;

    /**
     * \brief Gets the level on which var has been assigned.
     * 
     * \param var   A variable currently having an assignment.
     */
    auto getLevel(CNFVar var) const noexcept -> level;

    /**
     * \brief Undos all variable assignments on levels higher than \p{level}.
     * 
     * After calling this method, the current level is \p{level}.
     */
    void undoToLevel(level level) noexcept;

    /**
     * \brief Undos all variable assignments.
     * 
     * After calling this method, the current level is 0.
     */
    void undoAll() noexcept;

    /**
     * \brief Gets the assignments of the requested level, expressed as literals.
     *
     * \param level   The requested level.
     * \returns       an iterator range whose begin points to the first literal of
     *   the level \p level (if any) and whose end points to the first
     *   literal beyond the last literal of that level. The iterators
     *   remain valid until the assignment is modified.
     */
    auto getLevelAssignments(level level) const noexcept -> AssignmentRange;

    /**
     * \brief Gets a range over the current variable assignment, expressed as literals.
     * 
     * \returns       Iterator range over the current variable assignment.
     */
    auto getAssignments() const noexcept -> AssignmentRange;


    using BinariesMap = typename detail_propagation::Watchers<Clause>::BlockerMapT;

    /**
     * \brief Returns a map representing the binary clauses registered with the
     * assignment object.
     *
     * \return Let M be the value returned by this function. For each literal L with a
     * variable no greater than the current maximum variable, M[L] returns a
     * range containing exactly the literals L' such binary clause (L L') or
     * (L' L) has been registered with the assignment object.
     */
    auto getBinariesMap() const noexcept -> BinariesMap;

    Assignment& operator=(Assignment const&) = delete;
    Assignment(Assignment const&) = delete;

    Assignment& operator=(Assignment&&) noexcept = default;
    Assignment(Assignment&&) noexcept = default;

    ~Assignment() = default;


    // Exposed for testing purposes, do not call in production client code
    template <up_mode mode = up_mode::include_lemmas>
    auto propagate(CNFLit toPropagate, size_t& amountOfNewFacts) -> Clause*;

    // Exposed for testing purposes, do not call in production client code
    void assign(CNFLit literal, Clause* reason);


    /**
     * \brief StampMap key for Trail::DecisionLevel
     */
    class DecisionLevelKey {
    public:
        using Type = level;
        static size_t getIndex(DecisionLevel variable) { return static_cast<size_t>(variable); }
    };


private:
    auto propagateUntilFixpoint(CNFLit toPropagate, up_mode mode) -> Clause*;
    auto propagateBinaries(CNFLit toPropagate, size_t& amountOfNewFacts) -> Clause*;
    void cleanupWatchers();
    auto isWatcherCleanupRequired() const noexcept -> bool;
    void cleanupWatchers(CNFLit lit);

    using level_limit = uint32_t;

    /** \internal Variable assignments, in order of assignment */
    BoundedStack<CNFLit> m_trail;

    /** \internal m_levelLimits[i] is the index in m_trail where level i begins */
    std::vector<level_limit> m_levelLimits;

    /** \internal Map of variable assignments */
    BoundedMap<CNFVar, TBool> m_assignments;

    /** \internal Map of variable phases; updated during undoToLevel */
    BoundedMap<CNFVar, TBool> m_phases;

    /** \internal The current assignment level */
    level m_currentLevel;

    /** \internal Variable data grouped for cache-efficiency */
    struct ReasonAndAssignmentLevel {
        Clause* m_reason;
        level m_level;
    };

    /** \internal Reason and assignment level for each assigned variable */
    BoundedMap<CNFVar, ReasonAndAssignmentLevel> m_reasonsAndALs;

    /**
     * \internal
     * 
     * Watchers for binary clauses. These are kept separately from watchers on
     * longer clauses to save clause accesses.
     */
    detail_propagation::Watchers<Clause> m_binaryWatchers;

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
    BoundedMap<CNFLit, char> m_litsRequiringWatcherUpdate;

    /**
     * \internal
     *
     * A collection of elements marked in m_litsRequiringWatcherUpdate, for fast
     * iteration.
     */
    std::vector<CNFLit> m_litsRequiringWatcherUpdateAsVec;
};


inline auto Assignment::getAssignment(CNFLit lit) const noexcept -> TBool {
    TBool var_assignment = getAssignment(lit.getVariable());
    TBool::UnderlyingType sign = static_cast<TBool::UnderlyingType>(lit.getSign());
    // TODO: further optimize if neccessary: flip CNFSign constants to get rid of the subtraction
    return TBool::fromUnderlyingValue(var_assignment.getUnderlyingValue() ^ (1 - sign));
}

inline auto Assignment::getAssignment(CNFVar var) const noexcept -> TBool {
    return m_assignments[var];
}

inline auto Assignment::getPhase(CNFVar var) const noexcept -> TBool {
    return m_phases[var];
}

inline auto Assignment::getCurrentLevel() const noexcept {
    return m_currentLevel;
}

inline auto Assignment::getLevel(CNFVar var) const noexcept -> level {
    return m_reasonsAndALs[var].m_level;
}

inline auto Assignment::getReason(CNFVar var) const noexcept -> Clause const* {
    return m_reasonsAndALs[var].m_reason;
}

inline auto Assignment::getReason(CNFVar var) noexcept -> Clause* {
    return m_reasonsAndALs[var].m_reason;
}

inline auto Assignment::isForced(CNFVar var) const noexcept {
    return m_reasonsAndALs[var].m_reason != nullptr;
}

inline auto Assignment::isComplete() const noexcept -> bool {
    return m_trail.size() == m_assignments.size();
}

inline auto Assignment::getNumAssignments() const noexcept -> size_type {
    return m_trail.size();
}

inline auto Assignment::getBinariesMap() const noexcept -> BinariesMap {
    return m_binaryWatchers.getBlockerMap();
}

}