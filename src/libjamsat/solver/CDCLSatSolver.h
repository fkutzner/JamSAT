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

#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/proof/DRUPCertificate.h>
#include <libjamsat/proof/Model.h>

#include <libjamsat/branching/VSIDSBranchingHeuristic.h>
#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/HeapletClauseDB.h>
#include <libjamsat/solver/ClauseMinimization.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/solver/Propagation.h>
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

    void backtrackToLevel(typename ST::Trail::DecisionLevel level);


    typename ST::Trail m_trail;
    typename ST::Propagation m_propagation;
    typename ST::BranchingHeuristic m_branchingHeuristic;
    typename ST::ConflictAnalyzer m_conflictAnalyzer;
    typename ST::ClauseDB m_clauseDB;

    std::atomic<bool> m_stopRequested;
    std::atomic<bool> m_isSolving;
    CNFVar m_maxVar;

    std::vector<CNFLit> m_unitClauses;
    std::vector<typename ST::Clause *> m_problemClauses;
    std::vector<typename ST::Clause *> m_learntClauses;
    std::unordered_map<CNFLit, std::vector<CNFLit>> m_binaryClauses;

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
  , m_stopRequested(false)
  , m_isSolving(false)
  , m_maxVar(0)
  , m_unitClauses()
  , m_problemClauses()
  , m_learntClauses()
  , m_binaryClauses()
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

    CNFVar oldMaxVar = m_maxVar;
    for (auto lit : compressedClause) {
        m_maxVar = std::max(m_maxVar, lit.getVariable());
    }

    if (compressedClause.size() == 2) {
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
        auto assignment = m_trail.getAssignment(unit);
        if (assignment != TBool::INDETERMINATE &&
            toTBool(unit.getSign() == CNFSign::POSITIVE) != assignment) {
            JAM_LOG_SOLVER(info, "Detected conflict at unit clause " << unit);
            return UnitClausePropagationResult::CONFLICTING;
        }

        if (assignment == TBool::INDETERMINATE) {
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
    backtrackToLevel(0);
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


        if (conflictingClause != nullptr) {
            typename ST::Clause *learntClause;
            auto conflictHandlingResult = deriveClause(*conflictingClause, &learntClause);
            JAM_LOG_SOLVER(info, "Backtracking to decision level "
                                     << conflictHandlingResult.backtrackLevel);
            backtrackToLevel(conflictHandlingResult.backtrackLevel);
            if (conflictHandlingResult.learntUnitClause) {
                // Perform a restart to check for unsatisfiability during unit-clause
                // propagation
                return TBool::INDETERMINATE;
            }

            m_propagation.registerClause(*learntClause);

            --conflictsUntilMaintenance;
            if (conflictsUntilMaintenance == 0) {
                // TODO: do post-learning stuff (clausedb cleaning, adjustment of heuristics,
                // inprocessing, ...)

                if (m_stopRequested.load()) {
                    return TBool::INDETERMINATE;
                }

                conflictsUntilMaintenance = 5000;
            }
        }
    }

    return TBool::TRUE;
}

template <typename ST>
void CDCLSatSolver<ST>::optimizeLearntClause(std::vector<CNFLit> &learntClause) {
    eraseRedundantLiterals(learntClause, m_propagation, m_trail, m_stamps);
    if (learntClause.size() < 30 /* TODO: make constant configurable */) {
        LBD lbd = getLBD(learntClause, m_trail, m_stamps);
        if (lbd <= 6 /* TODO: make constant configurable */) {
            resolveWithBinaries(learntClause, m_binaryClauses, learntClause[0], m_stamps);
        }
    }
}

template <typename ST>
typename CDCLSatSolver<ST>::ConflictHandlingResult
CDCLSatSolver<ST>::deriveClause(typename ST::Clause &conflicting, typename ST::Clause **learntOut) {
    /* TODO: bad_alloc handling... */

    typename ST::Trail::DecisionLevel backtrackLevel = 0;

    auto learnt = m_conflictAnalyzer.computeConflictClause(conflicting);
    optimizeLearntClause(learnt);

    JAM_LOG_SOLVER(info, "Learnt clause: (" << toString(learnt.begin(), learnt.end()) << ")");
    if (learnt.size() == 1) {
        m_unitClauses.push_back(learnt[0]);
    } else {
        auto &learntClause = m_clauseDB.allocate(learnt.size());
        std::copy(learnt.begin(), learnt.end(), learntClause.begin());
        *learntOut = &learntClause;

        if (learnt.size() == 2) {
            m_binaryClauses[learnt[0]].push_back(learnt[1]);
            m_binaryClauses[learnt[1]].push_back(learnt[0]);
        } else {
            m_learntClauses.push_back(&learntClause);
        }

        for (auto lit = learntClause.begin() + 1; lit != learntClause.end(); ++lit) {
            backtrackLevel =
                std::max(backtrackLevel, m_trail.getAssignmentDecisionLevel(lit->getVariable()));
        }
    }

    return ConflictHandlingResult{learnt.size() == 1, backtrackLevel};
}

template <typename ST>
void CDCLSatSolver<ST>::backtrackToLevel(typename ST::Trail::DecisionLevel level) {
    for (auto currentDL = m_trail.getCurrentDecisionLevel(); currentDL >= level; --currentDL) {
        for (auto lit : m_trail.getDecisionLevelAssignments(currentDL)) {
            m_branchingHeuristic.reset(lit.getVariable());
        }
        if (currentDL == 0) {
            break;
        }
    }
    m_trail.shrinkToDecisionLevel(level);
}

template <typename ST>
typename CDCLSatSolver<ST>::SolvingResult
CDCLSatSolver<ST>::solve(const std::vector<CNFLit> &assumptions) noexcept {
    if (m_detectedUNSAT) {
        return createSolvingResult(TBool::FALSE);
    }

    if (m_problemClauses.empty()) {
        return createSolvingResult(TBool::TRUE);
    }

    m_stopRequested.store(false);
    m_isSolving.store(true);

    m_trail.increaseMaxVarTo(m_maxVar);
    m_propagation.increaseMaxVarTo(m_maxVar);
    m_branchingHeuristic.increaseMaxVarTo(m_maxVar);
    m_conflictAnalyzer.increaseMaxVarTo(m_maxVar);
    m_stamps.increaseSizeTo(getMaxLit(m_maxVar).getRawValue());

    for (auto clause : m_problemClauses) {
        m_propagation.registerClause(*clause);
    }

    for (CNFVar i{0}; i <= m_maxVar; i = nextCNFVar(i)) {
        // Note: no real support for assumptions yet...
        m_branchingHeuristic.setEligibleForDecisions(i, true);
    }

    TBool intermediateResult = TBool::INDETERMINATE;
    while (intermediateResult == TBool::INDETERMINATE && !m_stopRequested.load()) {
        intermediateResult = solveUntilRestart(assumptions);
    }

    if (intermediateResult == TBool::FALSE) {
        m_detectedUNSAT = true;
    }

    m_isSolving.store(false);
    m_stopRequested.store(false);
    SolvingResult result = createSolvingResult(intermediateResult);
    backtrackToLevel(0);
    return result;
}
}
