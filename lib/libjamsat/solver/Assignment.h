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

#include <boost/range.hpp>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/BoundedStack.h>
#include <libjamsat/utils/Casts.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {

/**
 * \ingroup JamSAT_Solver
 * 
 * \brief Class representing a variable assignment.
 * 
 * This class is responsible for maintaining a consistent variable assignment.
 */
class assignment {
    using const_iterator = BoundedStack<CNFLit>::const_iterator;

    /** Assignment level index */
    using level = std::uint32_t;

    /** Range type for assignments, expressed as literals */
    using assignment_range = boost::iterator_range<const_iterator>;

    using size_type = BoundedStack<CNFLit>::size_type;

    /**
     * \brief Undos all assignments and removes all clauses.
     */
    void clear() noexcept;

    /**
     * \brief Increases the maximum variable occurring in the problem instance.
     * 
     * \pram var    A variable; must not be smaller than the previous maximum variable.
     */
    void inc_max_var(CNFVar var);

    /**
     * \brief Adds the given literal to the current variable assignment along with all
     *   consequential assignments.
     * 
     * The variable `v` of \p l is assigned `true` iff the sign of \p l is positive.
     * 
     * \param l     The literal to be added. \p l must be unassigned.
     * 
     * \returns If the operation results in a conflicting assignment, a conflicting clause
     *   is returned, ie. a clause that is falsified under the new assignment. Otherwise,
     *   `nullptr` is returned.
     * 
     * \throws std::bad_alloc on memory allocation failure.
     */
    auto append(CNFLit l) -> Clause*;

    /**
     * \brief Returns the truth value of the given literal under the current assignment.
     */
    auto get(CNFLit lit) const noexcept -> TBool;

    /**
     * \brief Returns the truth value of the given variable under the current assignment.
     */
    auto get(CNFVar var) const noexcept -> TBool;

    /**
     * \brief Returns the most recently assigned truth value of the given variable.
     */
    auto get_phase(CNFVar var) const noexcept -> TBool;

    /**
     * \brief Returns `true` iff all variables have an assignment.
     */
    auto is_complete() const noexcept -> bool;

    /**
     * \brief Registers a clause (without assignments) for participating in consequence
     *   computation.
     * 
     * \param clause    A clause that is not yet participating in consequence computation.
     *   \p clause must reference a valid object until `clear()` is called or the assignment
     *   object is destroyed. If the clause is modified (except by this object),
     *   `register_clause_modification()` must be called accordingly. No literal in \p clause
     *   must have an assignment yet.
     */
    void register_new_clause(Clause& clause);

    /**
     * \brief Registers a new clause for participating in consequence computation.
     * 
     * \param clause   A clause that is not yet participating in consequence computation.
     *   \p clause must reference a valid object until `clear()` is called or the assignment
     *   object is destroyed. If the clause is modified (except by this object),
     *   `register_clause_modification()` must be called accordingly. All literals except
     *   the first one must have a `false` assignment; the first literal of the clause must
     *   be unassigned.
     * 
     *  \param asserting_lit The first literal of \p{clause}.
     */
    void register_new_clause(Clause& clause, CNFLit asserting_lit);

    /**
     * \brief Registers a clause modification.
     * 
     * \param clause   The modified clause.
     */
    void register_clause_modification(Clause& clause);

    /**
     * \brief Returns the clause having forced the assignment of the given variable.
     * 
     * \returns the clause having forced the assignment of the given variable, if any; otherwise,
     *   `nullptr` is returned.
     */
    auto get_reason(CNFVar var) const noexcept -> Clause*;

    /**
     * \brief Determines whether the given variable's assignment was forced by propagation.
     */
    auto is_forced(CNFVar var) const noexcept;

    /**
     * \brief Increases the level of the assignment.
     */
    void new_level() noexcept;

    /**
     * \brief Gets the current level.
     */
    auto get_current_level() const noexcept;

    /**
     * \brief Gets the level on which var has been assigned.
     * 
     * \param var   A variable currently having an assignment.
     */
    auto get_level(CNFVar var) const noexcept -> level;

    /**
     * \brief Undos all variable assignments on levels higher than \p{level}.
     * 
     * After calling this method, the current level is \p{level}.
     */
    void undo_to_level(level level) noexcept;

    /**
     * \brief Gets the assignments of the requested level, expressed as literals.
     *
     * \param level   The requested level.
     * \returns       an iterator range whose begin points to the first literal of
     *   the level \p level (if any) and whose end points to the first
     *   literal beyond the last literal of that level. The iterators
     *   remain valid until the assignment is modified.
     */
    auto get_level_assignments(level level) const noexcept -> assignment_range;

private:
};

}