/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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
 * \file SSRWithHyperBinaryResolution.h
 * \brief Self-subsuming resolution and subsumption checks using virtual binary clauses
 */

#pragma once

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/simplification/SimplificationStats.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/Printers.h>
#include <libjamsat/utils/Truth.h>

#include <boost/variant.hpp>

#include <stdexcept>

#if defined(JAM_ENABLE_INFLIGHTSIMP_LOGGING)
#define JAM_LOG_SSRWITHHBR(x, y) JAM_LOG(x, "ssrhbr", y)
#else
#define JAM_LOG_SSRWITHHBR(x, y)
#endif

namespace jamsat {

/**
 *
 * \brief An exception indicating that a provided literal is a failed
 *        literal.
 *
 * This exception is thrown by ssrWithHyperBinaryResolution() when the pivot
 * literal is detected to be a failed literal.
 *
 * \ingroup JamSAT_Simplification_SSRWithHBR
 *
 * \tparam ClauseType       The type of the conflicting clause
 */
template <typename ClauseType>
class FailedLiteralException : public std::exception {
public:
    FailedLiteralException(ClauseType* conflictingClause, size_t decisionLevelToRevisit);

    virtual ~FailedLiteralException();

    /**
     * \brief Returns the conflicting clause.
     * \returns The conflicting clause.
     */
    ClauseType* getConflictingClause() const noexcept;

    /**
     * \brief Returns the decision level to revisit after having finished
     *        exception handling.
     * \returns The decision level to revisit after having finished
     *        exception handling.
     */
    size_t getDecisionLevelToRevisit() const noexcept;

private:
    ClauseType* m_conflictingClause;
    size_t m_decisionLevelToRevisit;
};

namespace simp_ssrhbr_detail {
template <typename OccurrenceMapT,
          typename ModFnT,
          typename PropagationT,
          typename AssignmentProviderT,
          typename StampMapT>
struct SSRWithHBRParams {
    static_assert(
        std::is_same<typename PropagationT::Clause, typename AssignmentProviderT::Clause>::value,
        "Propagation and AssignmentProvider must have the same Clause type");

    using Clause = typename PropagationT::Clause;
    using OccurrenceMap = OccurrenceMapT;
    using ModFn = ModFnT;
    using Propagation = PropagationT;
    using AssignmentProvider = AssignmentProviderT;
    using StampMap = StampMapT;

    OccurrenceMap* const occMap;
    ModFn const* const notifyModificationAhead;
    Propagation* const propagation;
    AssignmentProvider* const assignments;
    StampMap* const tempStamps;
};
}

/**
 * \brief Creates a parameter-struct for ssrWithHyperBinaryResolution()
 *
 * \ingroup JamSAT_Simplification_SSRWithHBR
 *
 * \tparam OccurrenceMap            An OccurrenceMap specialization for `Propagation::Clause`
 * \tparam ModFn                    A type such that for an object `o` of type `ModFn` and
 *                                  an object `c` of type `Clause`, `o(&c)` is a valid
 *                                  expression.
 * \tparam Propagation              A type that is a model of the Propagation concept, with
 *                                  `Propagation::Clause` being the same type as
 *                                  `OccurrenceMap::Container` and `AssignmentProvider::Clause`.
 * \tparam AssignmentProvider       A type that is a model of the AssignmentProvider concept,
 *                                  with `AssignmentProvider::Clause` being the same type
 *                                  as `Propagation::Clause`.
 * \tparam StampMap                 A StampMap type for CNFLit
 *
 * \param occMap                    An OccurrenceMap containing the non-unary problem clauses
 *                                  to be optimized. Clauses occurring in \p occMap may be
 *                                  shortened or scheduled for deletion.
 * \param notifyModificationAhead   A callable object to which a pointer to each clause to be
 *                                  modified get passed before the modification.
 * \param propagation               A propagation object.
 * \param assignments               An assignment provider in sync with \p propagation.
 * \param tempStamps                A StampMap capable of stamping all CNFLit objects occurring
 *                                  in any clause contained in \p occMap, and during propagation
 *                                  of \p resolveAt with \p propagation.
 */
template <typename OccurrenceMapT,
          typename ModFnT,
          typename PropagationT,
          typename AssignmentProviderT,
          typename StampMapT>
auto createSSRWithHBRParams(OccurrenceMapT& occMap,
                            ModFnT const& notifyModificationAhead,
                            PropagationT& propagation,
                            AssignmentProviderT& assignmentProvider,
                            StampMapT& tempStamps) -> simp_ssrhbr_detail::
    SSRWithHBRParams<OccurrenceMapT, ModFnT, PropagationT, AssignmentProviderT, StampMapT>;

/**
 * \brief Performs self-subsuming resolution and strengthening with hyper-binary
 *        resolution.
 *
 * \ingroup JamSAT_Simplification_SSRWithHBR
 *
 * Precondition: All assignments forced by unary clauses (wrt. \p params.propagation)
 * have been propagated to fixpoint.
 *
 * Computes the set `A` of assignments implied by an assignment represented
 * by `resolveAt` and for each clause containing `resolveAt`, applies the following:
 *
 * - (a) if the intersection of `A` and `C` is not empty, `C` is scheduled for deletion
 *   since it is redundant.
 * - (b) for each `c in C`: if `~c in A`, `c` is removed from `C`.
 *
 * When this function returns, \p params.assignments contains exactly the assignments it
 * contians at the corresponding call to this function.
 *
 * \tparam SSRWithHBRParamsT        Any return type of createSSRWithHBRParams()
 *
 * \param params                    Parameter object created using createSSRWithHBRParams().
 * \param resolveAt                 The pivot literal.
 *
 *
 * \throws FailedLiteralException<Propagation::Clause>   If `~resolveAt` is a failed literal,
 *                                  a \p FailedLiteralException
 *                                  is thrown. The \p propagation object is left in the conflicting
 *                                  state. The conflicting clause can be obtained from the exception
 *                                  object.
 *
 * \returns                         A statistics object indicating how many clauses have been
 *                                  scheduled for deletion and how many literals have been
 *                                  removed from clauses via strengthening.
 *
 *
 */
template <typename SSRWithHBRParamsT>
auto ssrWithHyperBinaryResolution(SSRWithHBRParamsT& params, CNFLit resolveAt)
    -> SimplificationStats;

/********** Implementation ****************************** */

namespace simp_ssrhbr_detail {
enum ClauseOptimizationResult { UNCHANGED, STRENGTHENED, SCHEDULED_FOR_DELETION };

// Removes stamped literals from a clause and marks the clause as scheduled for deletion
// if it contains some literal L such that ~L is stamped. To prevent the clause to shrink
// to unary size, the size is not reduced further than to 2. This is relevant for a
// special case: when all literals in the clause except for resolveAt are implied by
// ~resolveAt, and this didn't cause a conflict during propagation since clause is
// redundant and thus excluded from this propagation, the clause is actually a conflicting
// clause. However, this seems to be quite a rare case, and to keep the code simple, the
// binary form of the clause is kept instead - the corresponding fact will then soon be
// learnt via CDCL.
template <typename SSRWithHBRParamsT, typename Clause>
auto ssrWithHBRMinimizeOrDelete(SSRWithHBRParamsT& params,
                                Clause& clause,
                                typename SSRWithHBRParamsT::StampMap::Stamp stamp,
                                SimplificationStats& simpStats) -> ClauseOptimizationResult;
}

template <typename OccurrenceMapT,
          typename ModFnT,
          typename PropagationT,
          typename AssignmentProviderT,
          typename StampMapT>
auto createSSRWithHBRParams(OccurrenceMapT& occMap,
                            ModFnT const& notifyModificationAhead,
                            PropagationT& propagation,
                            AssignmentProviderT& assignmentProvider,
                            StampMapT& tempStamps) -> simp_ssrhbr_detail::
    SSRWithHBRParams<OccurrenceMapT, ModFnT, PropagationT, AssignmentProviderT, StampMapT> {
    return {&occMap, &notifyModificationAhead, &propagation, &assignmentProvider, &tempStamps};
}

template <typename SSRWithHBRParamsT>
auto ssrWithHyperBinaryResolution(SSRWithHBRParamsT& params, CNFLit resolveAt)
    -> SimplificationStats {

    using Clause = typename SSRWithHBRParamsT::Propagation::Clause;

    SimplificationStats result;

    auto& occMap = *(params.occMap);
    auto& propagation = *(params.propagation);
    auto& assignments = *(params.assignments);
    auto& tempStamps = *(params.tempStamps);

    if (assignments.getAssignment(resolveAt) != TBools::INDETERMINATE) {
        // The assignment of resolveAt is already forced by a unary clause
        return result;
    }

    auto backtrackLevel = assignments.getCurrentLevel();
    assignments.newLevel();
    assignments.append(~resolveAt);
    auto confl = propagation.propagateUntilFixpoint(
        ~resolveAt, SSRWithHBRParamsT::Propagation::PropagationMode::EXCLUDE_REDUNDANT_CLAUSES);
    if (confl) {
        // Deliberately not backtracking before throwing the exception:
        // The current assignment & reason clauses might be needed by
        // the exception handler to derive failed literals.
        throw FailedLiteralException<Clause>(confl, backtrackLevel);
    }
    OnExitScope backtrackOnExit{
        [&assignments, backtrackLevel]() { assignments.undoToLevel(backtrackLevel); }};

    auto currentDL = assignments.getLevelAssignments(assignments.getCurrentLevel());
    if (currentDL.size() == 1) {
        return result; // the assignment didn't force any other assignments
    }

    auto stampingContext = tempStamps.createContext();
    auto stamp = stampingContext.getStamp();
    for (auto lit = currentDL.begin() + 1; lit != currentDL.end(); ++lit) {
        tempStamps.setStamped(*lit, stamp, true);
    }

    for (auto clause : occMap[resolveAt]) {
        // Skip this clause if it has been scheduled for deletion or if
        // changing it would not be sound.
        if (clause->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION) ||
            propagation.isAssignmentReason(*clause, assignments) ||
            !clause->mightContain(resolveAt) ||
            std::find(clause->begin(), clause->end(), resolveAt) == clause->end()) {
            continue;
        }

        simp_ssrhbr_detail::ClauseOptimizationResult optResult;
        optResult = simp_ssrhbr_detail::ssrWithHBRMinimizeOrDelete(params, *clause, stamp, result);

        JAM_ASSERT(clause->size() >= 2, "Not expecting to find new unaries during SSR with HBR");
        if (optResult != simp_ssrhbr_detail::ClauseOptimizationResult::UNCHANGED) {
            bool deleted =
                (optResult == simp_ssrhbr_detail::ClauseOptimizationResult::SCHEDULED_FOR_DELETION);
            (void)deleted; // suppress warning when logging is disabled
            JAM_LOG_SSRWITHHBR(info,
                               "Modified clause " << std::addressof(*clause) << " (now: "
                                                  << toString(clause->begin(), clause->end())
                                                  << (deleted ? ", deleted)" : ")"));
        }
    }
    return result;
}

namespace simp_ssrhbr_detail {
template <typename SSRWithHBRParamsT, typename Clause>
auto ssrWithHBRMinimizeOrDelete(SSRWithHBRParamsT& params,
                                Clause& clause,
                                typename SSRWithHBRParamsT::StampMap::Stamp stamp,
                                SimplificationStats& simpStats) -> ClauseOptimizationResult {
    auto notifyModificationAheadFn = *(params.notifyModificationAhead);
    bool clauseModified = false;
    bool strengthened = false;
    for (auto litIt = clause.begin(); litIt != clause.end();) {
        if (params.tempStamps->isStamped(*litIt, stamp)) {
            // remove by subsumption: the clause contains some literal
            // b such that (resolveAt b) is a "virtual" binary
            if (!clauseModified) {
                notifyModificationAheadFn(&clause);
            }
            ++simpStats.amntClausesRemovedBySubsumption;
            clause.setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
            break;
        } else if (params.tempStamps->isStamped(~(*litIt), stamp)) {
            // strengthen the clause: the clause contains some
            // literal b such that (resolveAt ~b) is a "virtual" binary,
            // therefore b can be removed via resolution:

            if (clause.size() == 2) {
                JAM_ASSERT(clause.getFlag(Clause::Flag::REDUNDANT), "Illegal non-redundant clause");
                break;
            }

            if (!clauseModified) {
                notifyModificationAheadFn(&clause);
                clauseModified = true;
            }
            ++simpStats.amntLiteralsRemovedByStrengthening;
            litIt = clause.erase(litIt);
            strengthened = true;
        } else {
            ++litIt;
        }
    }

    if (clause.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION)) {
        return ClauseOptimizationResult::SCHEDULED_FOR_DELETION;
    } else if (strengthened) {
        clause.clauseUpdated();
        return ClauseOptimizationResult::STRENGTHENED;
    } else {
        return ClauseOptimizationResult::UNCHANGED;
    }
}
}

template <typename ClauseType>
FailedLiteralException<ClauseType>::FailedLiteralException(ClauseType* conflictingClause,
                                                           size_t decisionLevelToRevisit)
  : std::exception()
  , m_conflictingClause{conflictingClause}
  , m_decisionLevelToRevisit{decisionLevelToRevisit} {}

template <typename ClauseType>
FailedLiteralException<ClauseType>::~FailedLiteralException() {}

template <typename ClauseType>
ClauseType* FailedLiteralException<ClauseType>::getConflictingClause() const noexcept {
    return m_conflictingClause;
}

template <typename ClauseType>
size_t FailedLiteralException<ClauseType>::getDecisionLevelToRevisit() const noexcept {
    return m_decisionLevelToRevisit;
}
}
