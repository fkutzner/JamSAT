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
#include <libjamsat/simplification/ClauseMinimization.h>
#include <libjamsat/simplification/LightweightSimplifier.h>
#include <libjamsat/simplification/UnaryOptimizations.h>
#include <libjamsat/solver/AssignmentAnalysis.h>
#include <libjamsat/solver/ClauseDBReduction.h>
#include <libjamsat/solver/ClauseDBReductionPolicies.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/RestartPolicies.h>
#include <libjamsat/solver/Statistics.h>
#include <libjamsat/solver/Trail.h>
#include <libjamsat/utils/Casts.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/OccurrenceMap.h>
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
    using Trail = jamsat::Trail<CDCLSatSolverDefaultTypes::Clause>;
    using Propagation = jamsat::Propagation<Trail>;
    using ConflictAnalyzer = FirstUIPLearning<Trail, Propagation>;
    using BranchingHeuristic = VSIDSBranchingHeuristic<Trail>;
    using RestartPolicy = GlucoseRestartPolicy;
    using ClauseDBReductionPolicy =
        GlucoseClauseDBReductionPolicy<jamsat::Clause, std::vector<jamsat::Clause*>, LBD>;
    using LightweightSimplifier =
        jamsat::LightweightSimplifier<Propagation, Trail, ConflictAnalyzer>;
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
        boost::optional<std::ostream*> certificateStream;

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
    void addProblem(const CNFProblem& problem);

    /**
     * \brief Adds a clause of the CNF problem instance to be solved to the solver.
     *
     * \param clause    A CNF clause.
     *
     * \throws std::bad_alloc   The clause database does not have enough memory to
     *                          hold \p clause
     */
    void addClause(const CNFClause& clause);

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
    SolvingResult solve(const std::vector<CNFLit>& assumptions);

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
    SolvingResult createSolvingResult(TBool result, std::vector<CNFLit> const& failedAssumptions);

    enum UnitClausePropagationResult { CONSISTENT, CONFLICTING };


    UnitClausePropagationResult propagateOnSystemLevels(std::vector<CNFLit> const& toPropagate,
                                                        std::vector<CNFLit>* failedAssumptions);
    UnitClausePropagationResult propagateUnitClauses(std::vector<CNFLit>& units);
    UnitClausePropagationResult propagateAssumptions(std::vector<CNFLit> const& assumptions,
                                                     std::vector<CNFLit>& failedAssumptions);

    TBool solveUntilRestart(const std::vector<CNFLit>& assumptions,
                            std::vector<CNFLit>& failedAssumptions);

    struct ConflictHandlingResult {
        bool learntUnitClause;
        typename ST::Trail::DecisionLevel backtrackLevel;
    };

    ConflictHandlingResult deriveLemma(typename ST::Clause& conflicting,
                                       typename ST::Clause** newLemmaOut);

    void optimizeLemma(std::vector<CNFLit>& lemma);

    void prepareBacktrack(typename ST::Trail::DecisionLevel level);
    void backtrackToLevel(typename ST::Trail::DecisionLevel level);
    void backtrackAll();

    typename ST::Trail m_trail;
    typename ST::Propagation m_propagation;
    typename ST::BranchingHeuristic m_branchingHeuristic;
    typename ST::ConflictAnalyzer m_conflictAnalyzer;
    typename ST::ClauseDB m_clauseDB;
    typename ST::RestartPolicy m_restartPolicy;
    typename ST::LightweightSimplifier m_simplifier;

    std::atomic<bool> m_stopRequested;
    CNFVar m_maxVar;

    std::vector<CNFLit> m_lemmaBuffer;

    std::vector<CNFLit> m_unitClauses;
    std::vector<typename ST::Clause*> m_problemClauses;
    typename std::vector<typename ST::Clause*>::size_type m_newProblemClausesBeginIdx;
    std::vector<typename ST::Clause*> m_lemmas;
    uint64_t m_amntBinariesLearnt;

    typename ST::ClauseDBReductionPolicy m_clauseDBReductionPolicy;

    StampMap<uint16_t, CNFVar::Index, CNFLit::Index, typename ST::Trail::DecisionLevelKey> m_stamps;
    Statistics<> m_statistics;

    bool m_detectedUNSAT;

    uint64_t m_conflictsUntilSimplification;
    uint64_t m_conflictsUntilFLE;
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
  , m_simplifier(CNFVar{0}, m_propagation, m_trail)
  , m_stopRequested(false)
  , m_maxVar(0)
  , m_lemmaBuffer()
  , m_unitClauses()
  , m_problemClauses()
  , m_newProblemClausesBeginIdx(0)
  , m_lemmas()
  , m_amntBinariesLearnt(0)
  , m_clauseDBReductionPolicy(1300, m_lemmas)
  , m_stamps(getMaxLit(CNFVar{0}).getRawValue())
  , m_statistics()
  , m_detectedUNSAT(false)
  , m_conflictsUntilSimplification(0)
  , m_conflictsUntilFLE(180000) {
    m_conflictAnalyzer.setOnSeenVariableCallback(
        [this](CNFVar var) { m_branchingHeuristic.seenInConflict(var); });
}

template <typename ST>
void CDCLSatSolver<ST>::stop() noexcept {
    m_stopRequested.store(true);
}

template <typename ST>
void CDCLSatSolver<ST>::addClause(const CNFClause& clause) {
    if (clause.empty()) {
        m_detectedUNSAT = true;
        return;
    }

    std::vector<CNFLit> compressedClause = withoutRedundancies(clause.begin(), clause.end());
    JAM_LOG_SOLVER(info, "Adding clause (" << toString(clause.begin(), clause.end()) << ")");

    // The solver requires that no clauses exist containing l as well as ~l.
    // Check if the clause can be ignored. withoutRedundancies returns a sorted
    // clause:
    for (auto claIt = compressedClause.begin() + 1; claIt != compressedClause.end(); ++claIt) {
        if (*(claIt - 1) == ~(*claIt)) {
            return;
        }
    }

    for (auto lit : compressedClause) {
        m_maxVar = std::max(m_maxVar, lit.getVariable());
    }

    if (compressedClause.size() == 1) {
        m_unitClauses.push_back(compressedClause[0]);
    } else {
        auto& internalClause = m_clauseDB.allocate(
            static_checked_cast<typename ST::Clause::size_type>(compressedClause.size()));
        std::copy(compressedClause.begin(), compressedClause.end(), internalClause.begin());
        internalClause.clauseUpdated();
        m_problemClauses.push_back(&internalClause);
    }
}

template <typename ST>
void CDCLSatSolver<ST>::addProblem(const CNFProblem& problem) {
    for (auto& clause : problem.getClauses()) {
        addClause(clause);
    }
}

template <typename ST>
typename CDCLSatSolver<ST>::SolvingResult
CDCLSatSolver<ST>::createSolvingResult(TBool result, std::vector<CNFLit> const& failedAssumptions) {
    std::unique_ptr<Model> model{nullptr};

    if (isTrue(result)) {
        model = createModel(m_maxVar);
        for (CNFLit lit : m_trail.getAssignments(0)) {
            model->setAssignment(lit.getVariable(),
                                 lit.getSign() == CNFSign::POSITIVE ? TBools::TRUE : TBools::FALSE);
        }
    }
    return SolvingResult{result,
                         std::move(model),
                         isFalse(result) ? std::move(failedAssumptions) : std::vector<CNFLit>{}};
}

template <typename ST>
typename CDCLSatSolver<ST>::UnitClausePropagationResult
CDCLSatSolver<ST>::propagateOnSystemLevels(std::vector<CNFLit> const& toPropagate,
                                           std::vector<CNFLit>* failedAssumptions) {
    JAM_LOG_SOLVER(info,
                   "Propagating system-level assignments on level "
                       << m_trail.getCurrentDecisionLevel());

    for (auto unit : toPropagate) {
        auto assignment = m_trail.getAssignment(unit.getVariable());
        if (isDeterminate(assignment) &&
            toTBool(unit.getSign() == CNFSign::POSITIVE) != assignment) {
            JAM_LOG_SOLVER(info, "Detected conflict at unit clause " << unit);
            if (failedAssumptions != nullptr) {
                *failedAssumptions = analyzeAssignment(m_trail, m_trail, m_stamps, unit);
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
CDCLSatSolver<ST>::propagateUnitClauses(std::vector<CNFLit>& units) {
    auto amntUnits = units.size();
    auto result = propagateOnSystemLevels(units, nullptr);
    if (result != UnitClausePropagationResult::CONFLICTING &&
        m_trail.getNumberOfAssignments() != amntUnits) {
        auto oldAmntUnits = units.size();
        units.clear();
        auto newUnits = m_trail.getAssignments(0);
        for (size_t i = 0; i < (newUnits.size() - oldAmntUnits); ++i) {
            m_statistics.registerLemma(1);
        }
        std::copy(newUnits.begin(), newUnits.end(), std::back_inserter(units));
    }
    return result;
}

template <typename ST>
typename CDCLSatSolver<ST>::UnitClausePropagationResult
CDCLSatSolver<ST>::propagateAssumptions(std::vector<CNFLit> const& assumptions,
                                        std::vector<CNFLit>& failedAssumptions) {
    return propagateOnSystemLevels(assumptions, &failedAssumptions);
}

template <typename ST>
TBool CDCLSatSolver<ST>::solveUntilRestart(const std::vector<CNFLit>& assumptions,
                                           std::vector<CNFLit>& failedAssumptions) {
    m_statistics.registerRestart();
    JAM_LOG_SOLVER(info, "Restarting the solver, backtracking to decision level 0.");
    backtrackAll();

    if (propagateUnitClauses(m_unitClauses) != UnitClausePropagationResult::CONSISTENT) {
        return TBools::FALSE;
    }

    m_trail.newDecisionLevel();

    if (m_statistics.getCurrentEra().m_conflictCount >= m_conflictsUntilFLE) {
        JAM_LOG_SOLVER(info, "Performing unrestricted failed literal elimination.");
        auto simpResult = m_simplifier.eliminateFailedLiterals(m_unitClauses);
        m_statistics.registerSimplification(simpResult);
        m_conflictsUntilFLE += 180000;
        return TBools::INDETERMINATE;
    }

    if (m_statistics.getCurrentEra().m_conflictCount >= m_conflictsUntilSimplification) {
        JAM_LOG_SOLVER(info, "Performing simplification.");
        auto simpResult =
            m_simplifier.simplify(m_unitClauses, m_problemClauses, m_lemmas, m_stamps);
        m_statistics.registerSimplification(simpResult);
        m_conflictsUntilSimplification += 40000;
        return TBools::INDETERMINATE;
    }

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
        JAM_LOG_SOLVER(info,
                       "Picked decision literal " << decision << ", now at decision level "
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
            typename ST::Clause* newLemma;
            auto conflictHandlingResult = deriveLemma(*conflictingClause, &newLemma);
            m_branchingHeuristic.endHandlingConflict();

            JAM_LOG_SOLVER(
                info, "Backtracking to decision level " << conflictHandlingResult.backtrackLevel);

            m_clauseDBReductionPolicy.registerConflict();

            if (conflictHandlingResult.learntUnitClause) {
                // Perform a restart to check for unsatisfiability during unit-clause
                // propagation, and to have the unit clause on level 0
                m_statistics.registerLemma(1);
                return TBools::INDETERMINATE;
            }

            LBD newLemmaLBD = (*newLemma).template getLBD<LBD>();
            if (newLemma->size() > 2ULL) {
                newLemma->setFlag(Clause::Flag::REDUNDANT);
            }
            m_restartPolicy.registerConflict({newLemmaLBD});

            backtrackToLevel(conflictHandlingResult.backtrackLevel);

            auto amntConflAssignments = m_trail.getNumberOfAssignments();
            conflictingClause = m_propagation.registerClause(*newLemma);
            m_statistics.registerPropagations(
                m_trail.getNumberOfAssignments() -
                m_propagation.getCurrentAmountOfUnpropagatedAssignments() -

                amntConflAssignments);
            m_statistics.registerLemma(newLemma->size());

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
            auto oldLemmasSize = m_lemmas.size();
            reduceClauseDB(m_clauseDB,
                           m_propagation,
                           m_trail,
                           boost::make_iterator_range(toDeleteBegin, m_lemmas.end()),
                           m_problemClauses,
                           m_lemmas);
            m_statistics.registerLemmaDeletion(oldLemmasSize - m_lemmas.size());
        }
    }

    return TBools::TRUE;
}

template <typename ST>
void CDCLSatSolver<ST>::optimizeLemma(std::vector<CNFLit>& lemma) {
    eraseRedundantLiterals(lemma, m_propagation, m_trail, m_stamps);
    JAM_LOG_SOLVER(info,
                   "  After redundant literal removal: (" << toString(lemma.begin(), lemma.end())
                                                          << ")");
    if (lemma.size() < 30 /* TODO: make constant configurable */) {
        LBD lbd = getLBD(lemma, m_trail, m_stamps);
        if (lbd <= 6 /* TODO: make constant configurable */) {
            auto binariesMap = m_propagation.getBinariesMap();
            resolveWithBinaries(lemma, binariesMap, lemma[0], m_stamps);
            JAM_LOG_SOLVER(info,
                           "  After resolution with binary clauses: ("
                               << toString(lemma.begin(), lemma.end()) << ")");
        }
    }
}

template <typename ST>
typename CDCLSatSolver<ST>::ConflictHandlingResult
CDCLSatSolver<ST>::deriveLemma(typename ST::Clause& conflicting,
                               typename ST::Clause** newLemmaOut) {
    /* TODO: bad_alloc handling... */

    typename ST::Trail::DecisionLevel backtrackLevel = 0;

    // the lemma buffer gets cleared before being used in computeConflictClause
    m_conflictAnalyzer.computeConflictClause(conflicting, m_lemmaBuffer);

    JAM_LOG_SOLVER(info,
                   "New lemma: (" << toString(m_lemmaBuffer.begin(), m_lemmaBuffer.end()) << ")");
    optimizeLemma(m_lemmaBuffer);
    JAM_LOG_SOLVER(info,
                   "Optimized new lemma: (" << toString(m_lemmaBuffer.begin(), m_lemmaBuffer.end())
                                            << ")");

    JAM_ASSERT(m_lemmaBuffer.size() > 0,
               "The empty clause is not expected to be directly derivable");

    if (m_lemmaBuffer.size() == 1) {
        m_unitClauses.push_back(m_lemmaBuffer[0]);
    } else if (m_lemmaBuffer.size() > 1) {
        auto& newLemma = m_clauseDB.allocate(
            static_checked_cast<typename ST::Clause::size_type>(m_lemmaBuffer.size()));
        std::copy(m_lemmaBuffer.begin(), m_lemmaBuffer.end(), newLemma.begin());
        newLemma.clauseUpdated();
        newLemma.setLBD(getLBD(newLemma, m_trail, m_stamps));

        *newLemmaOut = &newLemma;

        if (m_lemmaBuffer.size() == 2) {
            ++m_amntBinariesLearnt;
            m_problemClauses.push_back(&newLemma);
        } else {
            m_lemmas.push_back(&newLemma);
        }

        // Place a non-asserting literal with the highest decision level second in
        // the clause to make sure that any new assignments get propagated correctly,
        // as the first two literals will be watched initially.
        // This way, the two watched literals are guaranteed to lose their assignments
        // when the solver backtracks from the current decision level.
        // Otherwise, the following might happen: suppose that the third literal L3 of
        // a 3-literal lemma is on decision level D3, and the second literal L2
        // is on level D2, with D3 > D2. The first literal has been forced to TRUE on
        // level D3+1.
        // When backtracking to D2, the assignment of L2 remains, so the second
        // watcher watches an already-assigned literal. If ~L3 is propagated again
        // now, the propagation system would fail to notice that the clause forces an
        // assignment.
        auto litWithMaxDecisionLevel = newLemma.begin() + 1;
        for (auto lit = newLemma.begin() + 1; lit != newLemma.end(); ++lit) {
            auto currentBacktrackLevel =
                std::max(backtrackLevel, m_trail.getAssignmentDecisionLevel(lit->getVariable()));
            if (currentBacktrackLevel > backtrackLevel) {
                litWithMaxDecisionLevel = lit;
                backtrackLevel = currentBacktrackLevel;
            }
        }
        std::swap(*litWithMaxDecisionLevel, newLemma[1]);
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
CDCLSatSolver<ST>::solve(const std::vector<CNFLit>& assumptions) {
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

    // Elements in m_unitClauses must be distinct, but clients might add
    // redundant unaries
    m_unitClauses = withoutRedundancies(m_unitClauses.begin(), m_unitClauses.end());

    m_trail.increaseMaxVarTo(m_maxVar);
    m_propagation.increaseMaxVarTo(m_maxVar);
    m_branchingHeuristic.increaseMaxVarTo(m_maxVar);
    m_conflictAnalyzer.increaseMaxVarTo(m_maxVar);
    m_stamps.increaseSizeTo(getMaxLit(m_maxVar).getRawValue());
    m_simplifier.increaseMaxVarTo(m_maxVar);

    for (auto newClausesIt = m_problemClauses.begin() + m_newProblemClausesBeginIdx;
         newClausesIt != m_problemClauses.end();
         ++newClausesIt) {
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