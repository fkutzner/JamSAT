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

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <ostream>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>
#include <boost/range.hpp>
#include <boost/range/join.hpp>

#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/proof/DRUPCertificate.h>
#include <libjamsat/proof/Model.h>

#include <libjamsat/branching/VSIDSBranchingHeuristic.h>
#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/HeapletClauseDB.h>
#include <libjamsat/solver/ClauseDBReductionPolicies.h>
#include <libjamsat/solver/ClauseMinimization.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/RestartPolicies.h>
#include <libjamsat/solver/Trail.h>
#include <libjamsat/utils/RangeUtils.h>
#include <libjamsat/utils/StampMap.h>

#if defined(JAM_ENABLE_LOGGING) && defined(JAM_ENABLE_SOLVER_LOGGING)
#include <boost/log/trivial.hpp>
#define JAM_LOG_SOLVER(x, y) BOOST_LOG_TRIVIAL(x) << "[cdcldr] " << y
#else
#define JAM_LOG_SOLVER(x, y)
#endif

namespace jamsat {

struct CDCLSatSolverDefaultTypes {
    using Clause = jamsat::Clause;
    using ClauseDB = jamsat::HeapletClauseDB<CDCLSatSolverDefaultTypes::Clause>;
    using Trail = jamsat::Trail;
    using Propagation =
        jamsat::Propagation<CDCLSatSolverDefaultTypes::Trail, CDCLSatSolverDefaultTypes::Clause>;
    using ConflictAnalyzer = FirstUIPLearning<CDCLSatSolverDefaultTypes::Trail, Propagation,
                                              CDCLSatSolverDefaultTypes::Clause>;
    using BranchingHeuristic = VSIDSBranchingHeuristic<CDCLSatSolverDefaultTypes::Trail>;
    using RestartPolicy = GlucoseRestartPolicy;
    using ClauseDBReductionPolicy =
        GlucoseClauseDBReductionPolicy<jamsat::Clause, std::vector<jamsat::Clause *>, LBD>;
};

/**
 * \ingroup JamSAT_Solver
 *
 * \brief CDCL-based SAT solver
 *
 * \tparam ST   TODO: document SAT solver subsystem type concept
 */
template <typename ST = CDCLSatSolverDefaultTypes>
class CDCLSatSolver {
public:
    struct SolvingResult {
        TBool isSatisfiable;
    };

    /**
     * \brief The configuration structure for CDCLSatSolver.
     */
    struct Configuration {
        /**
         * Optional pointer to the ostream where the solver shall emit the
         * certificate of unsatisfiability. The solver emits such a certificate
         * iff this option is set.
         */
        boost::optional<std::ostream *> certificateStream;

        /**
         * The maximum amount of memory which can be allocated for clauses.
         */
        uint64_t clauseMemoryLimit;
    };

    /**
     * \brief Constructs a CDCLSatSolver instance.
     *
     * \param config    The configuration for the constructed instance.
     */
    CDCLSatSolver(Configuration config);

    /**
     * \brief Adds the clauses of the given CNF problem instance to be solved to the
     *        solver.
     *
     * \param problem   The CNF problem instance to be added.
     *
     * \throws std::bad_alloc   The clause database does not have enough memory to
     *                          hold \p problem
     */
    void addProblem(const CNFProblem &problem);

    /**
     * \brief Adds a clause of the CNF problem instance to be solved to the solver.
     *
     * \param clause    A CNF clause.
     *
     * \throws std::bad_alloc   The clause database does not have enough memory to
     *                          hold \p clause
     */
    void addClause(const CNFClause &clause);

    /**
     * \brief Determines whether the CNF problem specified via the methods
     *        `addProblem()` rsp. `addClause()` is satisfiable.
     *
     * Beginning with the second call to `solve()`, no certificate of
     * unsatisfiability is emitted.
     *
     * \param assumptions   A collection of literals which the solver shall
     *                      assume to have the value "true". If \p assumptions
     *                      is not empty, no certificate of unsatisfiability is
     *                      emitted.
     *
     * \return If the memory limit has been exceeded or `stop()` has been called
     *         during the execution of `solve()`, `TBool::INDETERMINATE` is
     *         returned. Otherwise, TBool::TRUE rsp. TBool::FALSE is returned if
     *         the CNF problem instance is satsifiable rsp. unsatisfiable with
     *         respect to the setting of \p assumptions .
     */
    SolvingResult solve(const std::vector<CNFLit> &assumptions) noexcept;

    /**
     * \brief Asynchronously instructs the solver to stop solving.
     *
     * This method may be called while `solve()` is being executed. When `solve()`
     * is being executed and this method is called, the solver will stop execution
     * in a timely manner. Calling this method while `solve()` is not being executed
     * has no effect.
     */
    void stop() noexcept;

private:
    SolvingResult createSolvingResult(TBool result);

    enum UnitClausePropagationResult { CONSISTENT, CONFLICTING };
    UnitClausePropagationResult propagateUnitClauses(const std::vector<CNFLit> &units);

    TBool solveUntilRestart(const std::vector<CNFLit> &assumptions);

    struct ConflictHandlingResult {
        bool learntUnitClause;
        typename ST::Trail::DecisionLevel backtrackLevel;
    };

    ConflictHandlingResult deriveClause(typename ST::Clause &conflicting,
                                        typename ST::Clause **learntOut);

    void optimizeLearntClause(std::vector<CNFLit> &learntClause);

    void prepareBacktrack(typename ST::Trail::DecisionLevel level);
    void backtrackToLevel(typename ST::Trail::DecisionLevel level);
    void backtrackAll();

    void reduceClauseDB();


    typename ST::Trail m_trail;
    typename ST::Propagation m_propagation;
    typename ST::BranchingHeuristic m_branchingHeuristic;
    typename ST::ConflictAnalyzer m_conflictAnalyzer;
    typename ST::ClauseDB m_clauseDB;
    typename ST::RestartPolicy m_restartPolicy;

    std::atomic<bool> m_stopRequested;
    std::atomic<bool> m_isSolving;
    CNFVar m_maxVar;

    std::vector<CNFLit> m_unitClauses;
    std::vector<typename ST::Clause *> m_problemClauses;
    typename std::vector<typename ST::Clause *>::size_type m_newProblemClausesBeginIdx;
    std::vector<typename ST::Clause *> m_learntClauses;
    std::unordered_map<CNFLit, std::vector<CNFLit>> m_binaryClauses;
    uint64_t m_amntBinariesLearnt;

    typename ST::ClauseDBReductionPolicy m_clauseDBReductionPolicy;

    StampMap<uint16_t, CNFVarKey, CNFLitKey, typename ST::Trail::DecisionLevelKey> m_stamps;

    bool m_detectedUNSAT;
};

/********** Implementation ****************************** */

template <typename ST>
CDCLSatSolver<ST>::CDCLSatSolver(Configuration config)
  : m_trail(CNFVar{0})
  , m_propagation(CNFVar{0}, m_trail)
  , m_branchingHeuristic(CNFVar{0}, m_trail)
  , m_conflictAnalyzer(CNFVar{0}, m_trail, m_propagation)
  , m_clauseDB(config.clauseMemoryLimit / 128, config.clauseMemoryLimit)
  , m_restartPolicy(typename ST::RestartPolicy::Options{})
  , m_stopRequested(false)
  , m_isSolving(false)
  , m_maxVar(0)
  , m_unitClauses()
  , m_problemClauses()
  , m_newProblemClausesBeginIdx(0)
  , m_learntClauses()
  , m_binaryClauses()
  , m_amntBinariesLearnt(0)
  , m_clauseDBReductionPolicy(1300, m_learntClauses)
  , m_stamps(getMaxLit(CNFVar{0}).getRawValue())
  , m_detectedUNSAT(false) {}

template <typename ST>
void CDCLSatSolver<ST>::stop() noexcept {
    if (m_isSolving.load() == true) {
        // No race condition can happen here due to the order in which solve() sets
        // m_isSolving and resets m_stopRequested
        m_stopRequested.store(true);
    }
}

template <typename ST>
void CDCLSatSolver<ST>::addClause(const CNFClause &clause) {
    std::vector<CNFLit> compressedClause = withoutRedundancies(clause.begin(), clause.end());
    JAM_LOG_SOLVER(info, "Adding clause (" << toString(clause.begin(), clause.end()) << ")");

    for (auto lit : compressedClause) {
        m_maxVar = std::max(m_maxVar, lit.getVariable());
    }

    if (compressedClause.size() == 2) {
        if (compressedClause[0] == ~compressedClause[1]) {
            // The clause is satisfied under any complete assignment and can be ignored.
            // This check is not a performance optimization: some subsystems expect that no
            // clause (l -l) exist for any literal l.
            return;
        }
        m_binaryClauses[compressedClause[0]].push_back(compressedClause[1]);
        m_binaryClauses[compressedClause[1]].push_back(compressedClause[0]);
    }

    if (compressedClause.empty()) {
        m_detectedUNSAT = true;
    } else if (compressedClause.size() == 1) {
        m_unitClauses.push_back(compressedClause[0]);
    } else {
        auto &internalClause = m_clauseDB.allocate(compressedClause.size());
        std::copy(compressedClause.begin(), compressedClause.end(), internalClause.begin());
        m_problemClauses.push_back(&internalClause);
    }
}

template <typename ST>
void CDCLSatSolver<ST>::addProblem(const CNFProblem &problem) {
    for (auto &clause : problem.getClauses()) {
        addClause(clause);
    }
}

template <typename ST>
typename CDCLSatSolver<ST>::SolvingResult CDCLSatSolver<ST>::createSolvingResult(TBool result) {
    return SolvingResult{result};
}

template <typename ST>
typename CDCLSatSolver<ST>::UnitClausePropagationResult
CDCLSatSolver<ST>::propagateUnitClauses(const std::vector<CNFLit> &units) {
    JAM_LOG_SOLVER(info, "Propagating unit clauses...");
    for (auto unit : units) {
        auto assignment = m_trail.getAssignment(unit.getVariable());
        if (isDeterminate(assignment) &&
            toTBool(unit.getSign() == CNFSign::POSITIVE) != assignment) {
            JAM_LOG_SOLVER(info, "Detected conflict at unit clause " << unit);
            return UnitClausePropagationResult::CONFLICTING;
        }

        if (!isDeterminate(assignment)) {
            m_trail.addAssignment(unit);
        } else {
            JAM_ASSERT(toTBool(unit.getSign() == CNFSign::POSITIVE) == assignment,
                       "Illegal unit clause conflict");
        }

        if (m_propagation.propagateUntilFixpoint(unit) != nullptr) {
            JAM_LOG_SOLVER(info, "Detected conflict at unit clause " << unit);
            return UnitClausePropagationResult::CONFLICTING;
        }

        m_branchingHeuristic.setEligibleForDecisions(unit.getVariable(), false);
    }
    return UnitClausePropagationResult::CONSISTENT;
}

template <typename ST>
TBool CDCLSatSolver<ST>::solveUntilRestart(const std::vector<CNFLit> &assumptions) {
    JAM_LOG_SOLVER(info, "Restarting the solver, backtracking to decision level 0.");
    backtrackAll();
    if (propagateUnitClauses(m_unitClauses) != UnitClausePropagationResult::CONSISTENT ||
        propagateUnitClauses(assumptions) != UnitClausePropagationResult::CONSISTENT) {
        return TBool::FALSE;
    }

    int conflictsUntilMaintenance = 5000;

    while (!m_trail.isVariableAssignmentComplete()) {
        m_trail.newDecisionLevel();
        auto decision = m_branchingHeuristic.pickBranchLiteral();
        JAM_LOG_SOLVER(info, "Picked decision literal " << decision << ", now at decision level "
                                                        << m_trail.getCurrentDecisionLevel());
        JAM_ASSERT(decision != CNFLit::getUndefinedLiteral(),
                   "The branching heuristic is not expected to return an undefined literal");
        m_trail.addAssignment(decision);

        auto conflictingClause = m_propagation.propagateUntilFixpoint(decision);
        while (conflictingClause != nullptr) {
            JAM_LOG_SOLVER(info, "Last propagation resulted in a conflict");
            typename ST::Clause *learntClause;
            auto conflictHandlingResult = deriveClause(*conflictingClause, &learntClause);
            JAM_LOG_SOLVER(info, "Backtracking to decision level "
                                     << conflictHandlingResult.backtrackLevel);

            m_clauseDBReductionPolicy.registerConflict();

            if (conflictHandlingResult.learntUnitClause) {
                // Perform a restart to check for unsatisfiability during unit-clause
                // propagation
                return TBool::INDETERMINATE;
            }

            LBD learntClauseLBD = (*learntClause).template getLBD<LBD>();
            m_restartPolicy.registerConflict({learntClauseLBD});
            backtrackToLevel(conflictHandlingResult.backtrackLevel);

            conflictingClause = m_propagation.registerClause(*learntClause);
            --conflictsUntilMaintenance;
        }

        if (conflictsUntilMaintenance <= 0) {
            // TODO: do post-learning stuff (clausedb cleaning, adjustment of heuristics,
            // inprocessing, ...)

            if (m_stopRequested.load()) {
                return TBool::INDETERMINATE;
            }

            conflictsUntilMaintenance = 5000;
        }

        if (m_restartPolicy.shouldRestart()) {
            JAM_LOG_SOLVER(info, "Performing restart");
            m_restartPolicy.registerRestart();
            return TBool::INDETERMINATE;
        }

        if (m_clauseDBReductionPolicy.shouldReduceDB()) {
            JAM_LOG_SOLVER(info, "Reducing the clause database...");
            reduceClauseDB();
        }
    }

    return TBool::TRUE;
}

template <typename ST>
void CDCLSatSolver<ST>::optimizeLearntClause(std::vector<CNFLit> &learntClause) {
    eraseRedundantLiterals(learntClause, m_propagation, m_trail, m_stamps);
    JAM_LOG_SOLVER(info, "  After redundant literal removal: ("
                             << toString(learntClause.begin(), learntClause.end()) << ")");
    if (learntClause.size() < 30 /* TODO: make constant configurable */) {
        LBD lbd = getLBD(learntClause, m_trail, m_stamps);
        if (lbd <= 6 /* TODO: make constant configurable */) {
            resolveWithBinaries(learntClause, m_binaryClauses, learntClause[0], m_stamps);
            JAM_LOG_SOLVER(info, "  After resolution with binary clauses: ("
                                     << toString(learntClause.begin(), learntClause.end()) << ")");
        }
    }
}

template <typename ST>
typename CDCLSatSolver<ST>::ConflictHandlingResult
CDCLSatSolver<ST>::deriveClause(typename ST::Clause &conflicting, typename ST::Clause **learntOut) {
    /* TODO: bad_alloc handling... */

    typename ST::Trail::DecisionLevel backtrackLevel = 0;

    auto learnt = m_conflictAnalyzer.computeConflictClause(conflicting);

    JAM_LOG_SOLVER(info, "Learnt clause: (" << toString(learnt.begin(), learnt.end()) << ")");
    optimizeLearntClause(learnt);
    JAM_LOG_SOLVER(info,
                   "Optimized learnt clause: (" << toString(learnt.begin(), learnt.end()) << ")");

    JAM_ASSERT(learnt.size() > 0, "The empty clause is not expected to be directly derivable");

    if (learnt.size() == 1) {
        m_unitClauses.push_back(learnt[0]);
    } else if (learnt.size() > 1) {
        auto &learntClause = m_clauseDB.allocate(learnt.size());
        std::copy(learnt.begin(), learnt.end(), learntClause.begin());
        learntClause.setLBD(getLBD(learntClause, m_trail, m_stamps));

        *learntOut = &learntClause;

        if (learnt.size() == 2) {
            m_binaryClauses[learnt[0]].push_back(learnt[1]);
            m_binaryClauses[learnt[1]].push_back(learnt[0]);
            ++m_amntBinariesLearnt;
            // No need to store pointers to binaries, since they are never relocated by
            // the clause allocator.
        } else {
            m_learntClauses.push_back(&learntClause);
        }

        // Place a non-asserting literal with the highest decision level second in
        // the clause to make sure that any new assignments get propagated correctly,
        // as the first two literals will be watched initially.
        // This way, the two watched literals are guaranteed to lose their assignments
        // when the solver backtracks from the current decision level.
        // Otherwise, the following might happen: suppose that the third literal L3 of
        // a learnt 3-literal clause is on decision level D3, and the second literal L2
        // is on level D2, with D3 > D2. The first literal has been forced to TRUE on
        // level D3+1.
        // When backtracking to D2, the assignment of L2 remains, so the second
        // watcher watches an already-assigned literal. If ~L3 is propagated again
        // now, the propagation system would fail to notice that the clause forces an
        // assignment.
        auto litWithMaxDecisionLevel = learntClause.begin() + 1;
        for (auto lit = learntClause.begin() + 1; lit != learntClause.end(); ++lit) {
            auto currentBacktrackLevel =
                std::max(backtrackLevel, m_trail.getAssignmentDecisionLevel(lit->getVariable()));
            if (currentBacktrackLevel > backtrackLevel) {
                litWithMaxDecisionLevel = lit;
                backtrackLevel = currentBacktrackLevel;
            }
        }
        std::swap(*litWithMaxDecisionLevel, learntClause[1]);
    }

    return ConflictHandlingResult{learnt.size() == 1, backtrackLevel};
}

template <typename ST>
void CDCLSatSolver<ST>::prepareBacktrack(typename ST::Trail::DecisionLevel level) {
    for (auto currentDL = m_trail.getCurrentDecisionLevel(); currentDL >= level; --currentDL) {
        for (auto lit : m_trail.getDecisionLevelAssignments(currentDL)) {
            m_branchingHeuristic.reset(lit.getVariable());
        }
        if (currentDL == 0) {
            break;
        }
    }
}

template <typename ST>
void CDCLSatSolver<ST>::backtrackToLevel(typename ST::Trail::DecisionLevel level) {
    JAM_ASSERT(level < m_trail.getCurrentDecisionLevel(), "Cannot backtrack to current level");
    prepareBacktrack(level + 1);
    m_trail.revisitDecisionLevel(level);
}

template <typename ST>
void CDCLSatSolver<ST>::backtrackAll() {
    prepareBacktrack(0);
    m_trail.shrinkToDecisionLevel(0);
}

template <typename ST>
typename CDCLSatSolver<ST>::SolvingResult
CDCLSatSolver<ST>::solve(const std::vector<CNFLit> &assumptions) noexcept {
    if (m_detectedUNSAT) {
        return createSolvingResult(TBool::FALSE);
    }

    if (m_problemClauses.empty() && m_unitClauses.empty()) {
        return createSolvingResult(TBool::TRUE);
    }

    m_stopRequested.store(false);
    m_isSolving.store(true);

    m_trail.increaseMaxVarTo(m_maxVar);
    m_propagation.increaseMaxVarTo(m_maxVar);
    m_branchingHeuristic.increaseMaxVarTo(m_maxVar);
    m_conflictAnalyzer.increaseMaxVarTo(m_maxVar);
    m_stamps.increaseSizeTo(getMaxLit(m_maxVar).getRawValue());

    for (auto newClausesIt = m_problemClauses.begin() + m_newProblemClausesBeginIdx;
         newClausesIt != m_problemClauses.end(); ++newClausesIt) {
        m_propagation.registerClause(**newClausesIt);
    }
    m_newProblemClausesBeginIdx = m_problemClauses.size();

    for (CNFVar i{0}; i <= m_maxVar; i = nextCNFVar(i)) {
        m_branchingHeuristic.setEligibleForDecisions(i, true);
    }
    for (CNFLit assumption : assumptions) {
        m_branchingHeuristic.setEligibleForDecisions(assumption.getVariable(), false);
    }

    TBool intermediateResult = TBool::INDETERMINATE;
    while (!isDeterminate(intermediateResult) && !m_stopRequested.load()) {
        intermediateResult = solveUntilRestart(assumptions);
    }

    m_isSolving.store(false);
    m_stopRequested.store(false);
    SolvingResult result = createSolvingResult(intermediateResult);
    backtrackAll();
    return result;
}

template <typename ST>
void CDCLSatSolver<ST>::reduceClauseDB() {
    auto amountKnownGood = m_problemClauses.size() + m_amntBinariesLearnt;
    auto toDeleteBegin = m_clauseDBReductionPolicy.getClausesMarkedForDeletion(amountKnownGood);

    if (toDeleteBegin == m_learntClauses.end()) {
        return;
    }

    std::vector<typename ST::Clause *> clausesAfterRelocation;
    m_clauseDB.retain(
        boost::range::join(
            boost::make_iterator_range(m_problemClauses.begin(), m_problemClauses.end()),
            boost::make_iterator_range(m_learntClauses.begin(), toDeleteBegin)),
        [this](typename ST::Clause const &clause) {
            return m_propagation.isAssignmentReason(clause, this->m_trail);
        },
        [this](typename ST::Clause const &reason, typename ST::Clause const &relocatedTo) {
            m_propagation.updateAssignmentReason(reason, relocatedTo);
        },
        boost::optional<decltype(std::back_inserter(clausesAfterRelocation))>{
            std::back_inserter(clausesAfterRelocation)});

    // Re-register relocated clauses:
    m_problemClauses.clear();
    m_learntClauses.clear();
    // The reasons have already been updated to point at the relocated clauses, so keep them:
    m_propagation.clear(ST::Propagation::ClearMode::KEEP_REASONS);
    for (auto clausePtr : clausesAfterRelocation) {
        if (clausePtr->template getLBD<LBD>() != 0) {
            m_learntClauses.push_back(clausePtr);
        } else {
            m_problemClauses.push_back(clausePtr);
        }

        // The clause has been removed during clear(), so it's okay to re-register
        // it in the exact same state:
        m_propagation.registerEquivalentSubstitutingClause(*clausePtr);
    }
}
}
