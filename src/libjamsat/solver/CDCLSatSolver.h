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
#include <memory>
#include <ostream>
#include <vector>

#include <boost/optional.hpp>
#include <boost/range.hpp>


#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/proof/DRUPCertificate.h>
#include <libjamsat/proof/Model.h>

#include <libjamsat/branching/VSIDSBranchingHeuristic.h>
#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/HeapletClauseDB.h>
#include <libjamsat/proof/Model.h>
#include <libjamsat/solver/AssignmentAnalysis.h>
#include <libjamsat/solver/ClauseDBReduction.h>
#include <libjamsat/solver/ClauseDBReductionPolicies.h>
#include <libjamsat/solver/ClauseMinimization.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/RestartPolicies.h>
#include <libjamsat/solver/Statistics.h>
#include <libjamsat/solver/Trail.h>
#include <libjamsat/utils/Casts.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/RangeUtils.h>
#include <libjamsat/utils/StampMap.h>

#include <iostream>

#if defined(JAM_ENABLE_SOLVER_LOGGING)
#define JAM_LOG_SOLVER(x, y) JAM_LOG(x, "cdcldr", y)
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
        std::unique_ptr<Model> model;
        std::vector<CNFLit> failedAssumptions;
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
     *         during the execution of `solve()`, `TBools::INDETERMINATE` is
     *         returned. Otherwise, TBools::TRUE rsp. TBools::FALSE is returned if
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
    SolvingResult createSolvingResult(TBool result, std::vector<CNFLit> const &failedAssumptions);

    enum UnitClausePropagationResult { CONSISTENT, CONFLICTING };


    UnitClausePropagationResult propagateOnSystemLevels(std::vector<CNFLit> const &toPropagate,
                                                        std::vector<CNFLit> *failedAssumptions);
    UnitClausePropagationResult propagateUnitClauses(std::vector<CNFLit> const &units);
    UnitClausePropagationResult propagateAssumptions(std::vector<CNFLit> const &assumptions,
                                                     std::vector<CNFLit> &failedAssumptions);

    TBool solveUntilRestart(const std::vector<CNFLit> &assumptions,
                            std::vector<CNFLit> &failedAssumptions);

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

    typename ST::Trail m_trail;
    typename ST::Propagation m_propagation;
    typename ST::BranchingHeuristic m_branchingHeuristic;
    typename ST::ConflictAnalyzer m_conflictAnalyzer;
    typename ST::ClauseDB m_clauseDB;
    typename ST::RestartPolicy m_restartPolicy;

    std::atomic<bool> m_stopRequested;
    CNFVar m_maxVar;

    std::vector<CNFLit> m_lemmaBuffer;

    std::vector<CNFLit> m_unitClauses;
    std::vector<typename ST::Clause *> m_problemClauses;
    typename std::vector<typename ST::Clause *>::size_type m_newProblemClausesBeginIdx;
    std::vector<typename ST::Clause *> m_learntClauses;
    uint64_t m_amntBinariesLearnt;

    typename ST::ClauseDBReductionPolicy m_clauseDBReductionPolicy;

    StampMap<uint16_t, CNFVarKey, CNFLitKey, typename ST::Trail::DecisionLevelKey> m_stamps;
    Statistics<> m_statistics;

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
  , m_maxVar(0)
  , m_lemmaBuffer()
  , m_unitClauses()
  , m_problemClauses()
  , m_newProblemClausesBeginIdx(0)
  , m_learntClauses()
  , m_amntBinariesLearnt(0)
  , m_clauseDBReductionPolicy(1300, m_learntClauses)
  , m_stamps(getMaxLit(CNFVar{0}).getRawValue())
  , m_statistics()
  , m_detectedUNSAT(false) {
    m_conflictAnalyzer.setOnSeenVariableCallback(
        [this](CNFVar var) { m_branchingHeuristic.seenInConflict(var); });
}

template <typename ST>
void CDCLSatSolver<ST>::stop() noexcept {
    m_stopRequested.store(true);
}

template <typename ST>
void CDCLSatSolver<ST>::addClause(const CNFClause &clause) {
    std::vector<CNFLit> compressedClause = withoutRedundancies(clause.begin(), clause.end());
    JAM_LOG_SOLVER(info, "Adding clause (" << toString(clause.begin(), clause.end()) << ")");

    for (auto lit : compressedClause) {
        m_maxVar = std::max(m_maxVar, lit.getVariable());
    }

    if (compressedClause.size() == 2 && compressedClause[0] == ~compressedClause[1]) {
        // The clause is satisfied under any complete assignment and can be ignored.
        // This check is not a performance optimization: some subsystems expect that no
        // clause (l -l) exist for any literal l.
        return;
    }

    if (compressedClause.empty()) {
        m_detectedUNSAT = true;
    } else if (compressedClause.size() == 1) {
        m_unitClauses.push_back(compressedClause[0]);
    } else {
        auto &internalClause = m_clauseDB.allocate(
            static_checked_cast<typename ST::Clause::size_type>(compressedClause.size()));
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
typename CDCLSatSolver<ST>::SolvingResult
CDCLSatSolver<ST>::createSolvingResult(TBool result, std::vector<CNFLit> const &failedAssumptions) {
    std::unique_ptr<Model> model{nullptr};

    if (isTrue(result)) {
        model = createModel(m_maxVar);
        for (CNFLit lit : m_trail.getAssignments(0)) {
            model->setAssignment(lit.getVariable(),
                                 lit.getSign() == CNFSign::POSITIVE ? TBools::TRUE : TBools::FALSE);
        }
    }
    return SolvingResult{result, std::move(model),
                         isFalse(result) ? std::move(failedAssumptions) : std::vector<CNFLit>{}};
}

template <typename ST>
typename CDCLSatSolver<ST>::UnitClausePropagationResult
CDCLSatSolver<ST>::propagateOnSystemLevels(std::vector<CNFLit> const &toPropagate,
                                           std::vector<CNFLit> *failedAssumptions) {
    JAM_LOG_SOLVER(info, "Propagating system-level assignments on level "
                             << m_trail.getCurrentDecisionLevel());

    for (auto unit : toPropagate) {
        auto assignment = m_trail.getAssignment(unit.getVariable());
        if (isDeterminate(assignment) &&
            toTBool(unit.getSign() == CNFSign::POSITIVE) != assignment) {
            JAM_LOG_SOLVER(info, "Detected conflict at unit clause " << unit);
            if (failedAssumptions != nullptr) {
                *failedAssumptions = analyzeAssignment(m_propagation, m_trail, m_stamps, unit);
            }
            return UnitClausePropagationResult::CONFLICTING;
        }

        if (!isDeterminate(assignment)) {
            m_trail.addAssignment(unit);
        } else {
            JAM_ASSERT(toTBool(unit.getSign() == CNFSign::POSITIVE) == assignment,
                       "Illegal unit clause conflict");
        }

        auto amntAssignments = m_trail.getNumberOfAssignments();
        bool unitConflict = (m_propagation.propagateUntilFixpoint(unit) != nullptr);
        m_statistics.registerPropagations(
            m_trail.getNumberOfAssignments() -
            m_propagation.getCurrentAmountOfUnpropagatedAssignments() - amntAssignments);

        if (unitConflict) {
            JAM_LOG_SOLVER(info, "Detected conflict at unit clause " << unit);
            if (failedAssumptions != nullptr) {
                *failedAssumptions = analyzeAssignment(m_propagation, m_trail, m_stamps, unit);
            }
            return UnitClausePropagationResult::CONFLICTING;
        }

        m_branchingHeuristic.setEligibleForDecisions(unit.getVariable(), false);
    }
    return UnitClausePropagationResult::CONSISTENT;
}

template <typename ST>
typename CDCLSatSolver<ST>::UnitClausePropagationResult
CDCLSatSolver<ST>::propagateUnitClauses(const std::vector<CNFLit> &units) {
    return propagateOnSystemLevels(units, nullptr);
}

template <typename ST>
typename CDCLSatSolver<ST>::UnitClausePropagationResult
CDCLSatSolver<ST>::propagateAssumptions(std::vector<CNFLit> const &assumptions,
                                        std::vector<CNFLit> &failedAssumptions) {
    return propagateOnSystemLevels(assumptions, &failedAssumptions);
}

template <typename ST>
TBool CDCLSatSolver<ST>::solveUntilRestart(const std::vector<CNFLit> &assumptions,
                                           std::vector<CNFLit> &failedAssumptions) {
    m_statistics.registerRestart();
    JAM_LOG_SOLVER(info, "Restarting the solver, backtracking to decision level 0.");
    backtrackAll();
    if (propagateUnitClauses(m_unitClauses) != UnitClausePropagationResult::CONSISTENT) {
        return TBools::FALSE;
    }
    m_trail.newDecisionLevel();
    if (propagateAssumptions(assumptions, failedAssumptions) !=
        UnitClausePropagationResult::CONSISTENT) {
        // TODO: do final conflict analysis here
        return TBools::FALSE;
    }

    while (!m_trail.isVariableAssignmentComplete()) {
        loggingEpochElapsed();
        m_trail.newDecisionLevel();
        auto decision = m_branchingHeuristic.pickBranchLiteral();
        m_statistics.registerDecision();
        JAM_LOG_SOLVER(info, "Picked decision literal " << decision << ", now at decision level "
                                                        << m_trail.getCurrentDecisionLevel());
        JAM_ASSERT(decision != CNFLit::getUndefinedLiteral(),
                   "The branching heuristic is not expected to return an undefined literal");
        m_trail.addAssignment(decision);

        auto amntAssignments = m_trail.getNumberOfAssignments();
        auto conflictingClause = m_propagation.propagateUntilFixpoint(decision);
        m_statistics.registerPropagations(
            m_trail.getNumberOfAssignments() -
            m_propagation.getCurrentAmountOfUnpropagatedAssignments() - amntAssignments);

        while (conflictingClause != nullptr) {
            m_statistics.registerConflict();
            JAM_LOG_SOLVER(info, "Last propagation resulted in a conflict");
            m_branchingHeuristic.beginHandlingConflict();
            typename ST::Clause *learntClause;
            auto conflictHandlingResult = deriveClause(*conflictingClause, &learntClause);
            m_branchingHeuristic.endHandlingConflict();

            JAM_LOG_SOLVER(info, "Backtracking to decision level "
                                     << conflictHandlingResult.backtrackLevel);

            m_clauseDBReductionPolicy.registerConflict();

            if (conflictHandlingResult.learntUnitClause) {
                // Perform a restart to check for unsatisfiability during unit-clause
                // propagation, and to have the unit clause on level 0
                m_statistics.registerLemma(1);
                return TBools::INDETERMINATE;
            }

            LBD learntClauseLBD = (*learntClause).template getLBD<LBD>();
            m_restartPolicy.registerConflict({learntClauseLBD});

            backtrackToLevel(conflictHandlingResult.backtrackLevel);

            auto amntConflAssignments = m_trail.getNumberOfAssignments();
            conflictingClause = m_propagation.registerClause(*learntClause);
            m_statistics.registerPropagations(
                m_trail.getNumberOfAssignments() -
                m_propagation.getCurrentAmountOfUnpropagatedAssignments() -

                amntConflAssignments);
            m_statistics.registerLemma(learntClause->size());

            if (conflictHandlingResult.backtrackLevel == 1 && conflictingClause != nullptr) {
                // Propagating the unit clauses and the assumptions now forces an assignment
                // under which some clause is already "false". Under the current assumptions,
                // the problem is not satisfiable. Perform a final restart to do
                // conflict analysis:
                return TBools::INDETERMINATE;
            }

            if (m_statistics.getCurrentEra().m_conflictCount % 10000ULL == 0ULL) {
                std::cout << m_statistics;
                if (m_stopRequested.load()) {
                    return TBools::INDETERMINATE;
                }
            }
        }

        if (m_restartPolicy.shouldRestart()) {
            JAM_LOG_SOLVER(info, "Performing restart");
            m_restartPolicy.registerRestart();
            return TBools::INDETERMINATE;
        }

        if (m_clauseDBReductionPolicy.shouldReduceDB()) {
            JAM_LOG_SOLVER(info, "Reducing the clause database...");
            auto amountKnownGood = m_amntBinariesLearnt;
            auto toDeleteBegin =
                m_clauseDBReductionPolicy.getClausesMarkedForDeletion(amountKnownGood);
            reduceClauseDB(m_clauseDB, m_propagation, m_trail,
                           boost::make_iterator_range(toDeleteBegin, m_learntClauses.end()),
                           m_problemClauses, m_learntClauses);
        }
    }

    return TBools::TRUE;
}

template <typename ST>
void CDCLSatSolver<ST>::optimizeLearntClause(std::vector<CNFLit> &learntClause) {
    eraseRedundantLiterals(learntClause, m_propagation, m_trail, m_stamps);
    JAM_LOG_SOLVER(info, "  After redundant literal removal: ("
                             << toString(learntClause.begin(), learntClause.end()) << ")");
    if (learntClause.size() < 30 /* TODO: make constant configurable */) {
        LBD lbd = getLBD(learntClause, m_trail, m_stamps);
        if (lbd <= 6 /* TODO: make constant configurable */) {
            auto binariesMap = m_propagation.getBinariesMap();
            resolveWithBinaries(learntClause, binariesMap, learntClause[0], m_stamps);
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

    // the lemma buffer gets cleared before being used in computeConflictClause
    m_conflictAnalyzer.computeConflictClause(conflicting, m_lemmaBuffer);

    JAM_LOG_SOLVER(info, "Learnt clause: (" << toString(learnt.begin(), learnt.end()) << ")");
    optimizeLearntClause(m_lemmaBuffer);
    JAM_LOG_SOLVER(info,
                   "Optimized learnt clause: (" << toString(learnt.begin(), learnt.end()) << ")");

    JAM_ASSERT(m_lemmaBuffer.size() > 0,
               "The empty clause is not expected to be directly derivable");

    if (m_lemmaBuffer.size() == 1) {
        m_unitClauses.push_back(m_lemmaBuffer[0]);
    } else if (m_lemmaBuffer.size() > 1) {
        auto &learntClause = m_clauseDB.allocate(
            static_checked_cast<typename ST::Clause::size_type>(m_lemmaBuffer.size()));
        std::copy(m_lemmaBuffer.begin(), m_lemmaBuffer.end(), learntClause.begin());
        learntClause.setLBD(getLBD(learntClause, m_trail, m_stamps));

        *learntOut = &learntClause;

        if (m_lemmaBuffer.size() == 2) {
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

    return ConflictHandlingResult{m_lemmaBuffer.size() == 1, backtrackLevel};
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
    m_statistics.printStatisticsDescription(std::cout);
    m_statistics.registerSolvingStart();
    OnExitScope updateStatsOnExit{[this]() {
        m_statistics.registerSolvingStop();
        m_statistics.concludeEra();
    }};

    m_stopRequested.store(false);
    if (m_detectedUNSAT) {
        return createSolvingResult(TBools::FALSE, {});
    }

    m_trail.increaseMaxVarTo(m_maxVar);
    m_propagation.increaseMaxVarTo(m_maxVar);
    m_branchingHeuristic.increaseMaxVarTo(m_maxVar);
    m_conflictAnalyzer.increaseMaxVarTo(m_maxVar);
    m_stamps.increaseSizeTo(getMaxLit(m_maxVar).getRawValue());

    for (auto newClausesIt = m_problemClauses.begin() + m_newProblemClausesBeginIdx;
         newClausesIt != m_problemClauses.end(); ++newClausesIt) {
        m_propagation.registerClause(**newClausesIt);
    }

    for (CNFVar i{0}; i <= m_maxVar; i = nextCNFVar(i)) {
        m_branchingHeuristic.setEligibleForDecisions(i, true);
    }
    for (CNFLit assumption : assumptions) {
        m_branchingHeuristic.setEligibleForDecisions(assumption.getVariable(), false);
    }

    std::vector<CNFLit> failedAssumptions;
    TBool intermediateResult = TBools::INDETERMINATE;
    while (!isDeterminate(intermediateResult) && !m_stopRequested.load()) {
        intermediateResult = solveUntilRestart(assumptions, failedAssumptions);
    }

    // Updating m_newProblemClausesBeginIdx late: pointers to binary clauses
    // that were present at the beginning of this method's execution may have
    // been remvoed from m_problemClauses during clause DB reduction
    m_newProblemClausesBeginIdx = m_problemClauses.size();

    SolvingResult result = createSolvingResult(intermediateResult, std::move(failedAssumptions));
    backtrackAll();
    return result;
}
}
