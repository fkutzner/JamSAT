/* Copyright (c) 2017, 2018, 2019 Felix Kutzner (github.com/fkutzner)

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
 * \file CDCLSatSolver.cpp
 * \brief Default CDCL search implementation
 */

#include <libjamsat/drivers/CDCLSatSolver.h>

#include <libjamsat/branching/VSIDSBranchingHeuristic.h>
#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/IterableClauseDB.h>
#include <libjamsat/proof/DRATCertificate.h>
#include <libjamsat/proof/Model.h>
#include <libjamsat/simplification/ClauseMinimization.h>
#include <libjamsat/simplification/ProblemOptimizer.h>
#include <libjamsat/simplification/optimizers/FactCleaner.h>
#include <libjamsat/solver/Assignment.h>
#include <libjamsat/solver/AssignmentAnalysis.h>
#include <libjamsat/solver/ClauseDBReductionPolicies.h>
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/solver/RestartPolicies.h>
#include <libjamsat/solver/Statistics.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/Printers.h>
#include <libjamsat/utils/RangeUtils.h>
#include <libjamsat/utils/StampMap.h>

#include <boost/range/adaptors.hpp>
#include <boost/variant.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>


#if defined(JAM_ENABLE_SOLVER_LOGGING)
#define JAM_LOG_SOLVER(x, y) JAM_LOG(x, "solver", y)
#else
#define JAM_LOG_SOLVER(x, y)
#endif

namespace jamsat {
SolvingResult::~SolvingResult() {}

CDCLSatSolver::~CDCLSatSolver() {}

namespace {

/**
 * \ingroup JamSAT_Drivers
 *
 * \brief Solving result implementation for CDCLSatSolverImpl
 */
class SolvingResultImpl : public SolvingResult {
public:
  SolvingResultImpl(TBool result,
                    std::unique_ptr<Model> model,
                    std::vector<CNFLit> const& failedAssumptions);
  auto isProblemSatisfiable() const noexcept -> TBool override;

  auto getModel() const noexcept -> std::optional<std::reference_wrapper<Model const>> override;
  auto getFailedAssumptions() const noexcept -> std::vector<CNFLit> const& override;

  virtual ~SolvingResultImpl();

private:
  TBool m_result;
  std::unique_ptr<Model> m_model;
  std::vector<CNFLit> m_failedAssumptions;
};

/**
 * \ingroup JamSAT_Drivers
 *
 * \brief CDCL-based SAT solver implementation
 */
class CDCLSatSolverImpl : public CDCLSatSolver {
public:
  struct Config {
    /** The maximum lemma size for post-learning lemma minimizaition using binary resolution */
    std::size_t lemmaSimplificationSizeBound = 30;

    /** The maximum lemma LBD for post-learning lemma minimizaition using binary resolution */
    jamsat::LBD lemmaSimplificationLBDBound = 6;

    /** The number of restarts between attempts to simplify the problem */
    uint64_t simplificationFrequency = 5000;

    /** The region allocator's region size */
    std::size_t clauseRegionSize = 1048576;

    /**
     * The growth rate of the number of conflicts the solver waits between clause DB
     * reductions
     */
    uint32_t clauseRemovalIntervalGrowthRate = 1300;

    /** The restart policy configuration */
    GlucoseRestartPolicy::Options restartPolicyOptions = GlucoseRestartPolicy::Options{};

    /**
     * The maximum amount of clauses for which LBD updates are performed during
     * backtracking
     */
    std::size_t maxLBDUpdatesOnBacktrack = 32;


    /** Iff `true`, the solver regularly prints statistics */
    bool printStatistics = true;
  };

  explicit CDCLSatSolverImpl(Config const& configuration);

  void addProblem(CNFProblem const& problem) override;
  void addClause(CNFClause const& clause) override;
  auto solve(std::vector<CNFLit> const& assumedFacts) -> std::unique_ptr<SolvingResult> override;
  void stop() noexcept override;
  void setLogger(LoggerFn loggerFunction) override;
  void setDRATCertificate(DRATCertificate& cert) noexcept override;

  virtual ~CDCLSatSolverImpl();

private:
  using ClauseT = Clause;

  /** Adjusts the sizes of all subsystems after new SAT variables have been detected */
  void resizeSubsystems();

  /**
   * Adjusts subsystems storing pointers to clauses. This method restores the solver's
   * consistency after a clause database compression has been performed.
   *
   * This method may only be called during restarts.
   */
  void synchronizeSubsystemsWithClauseDB();

  /** Sets all variables except for assumed facts as eligible for being branched on */
  void initializeBranchingHeuristic(std::vector<CNFLit> const& assumedFacts);

  enum class SimplificationResult { NONE, DETECTED_UNSAT };

  /**
   * Performs simplification if suitable.
   *
   * This method may only be called during restarts.
   * 
   * \returns SimplificationResult::DETECTED_UNSAT if unsatisfiability has been
   *   determined during simplification, eg. by finding contradicting facts. Otherwise,
   *   NONE is returned.
   */
  auto trySimplify() -> SimplificationResult;

  /**
   * Heuristically deletes clauses from the clause database.
   *
   * This method may only be called during restarts.
   */
  void tryReduceClauseDB();

  /**
   * Returns all variables on decision levels `[target, currentDecisionLevel]` to
   * the branching heuristic.
   *
   * \param target    see above
   */
  void prepareBacktrack(Assignment::Level targetLevel);

  /**
   * Backtracks all decisions. After this, the solver is on decision level
   * 0, without variable assignments.
   */
  void backtrackAll();

  /**
   * Backtracks all decisions on decision levels higher than the target
   * level. After this, the solver is on decision level `targetLevel`,
   * with the assignments of `targetLevel` having been preserved.
   *
   * \param targetLevel targetLevel
   */
  void backtrackToLevel(Assignment::Level targetLevel);

  /**
   * Performs CDCL until a restart needs to be performed.
   *
   * This method may only be called during restarts, ie. when the solver is on
   * decision level 0 and no SAT variables have been assigned.
   *
   * \param[in]  assumedFacts         Facts that shall be assumed (all variables <= m_maxVar)
   * \param[out] failedAssumptions    If the result is UNSAT and the setting of
   *                                  assumedFacts has led to the UNSAT result,
   *                                  a subset Z of assumedFacts is stored in
   *                                  failedAssumptions such that there is an UNSAT
   *                                  core that is activated by literals in Z.
   *
   * \returns If this method returns TBool::TRUE, the problem instance is satisfiable
   * and the current variable assignment is a model for the SAT problem instance. If
   * TBool::FALSE is returned, the problem instance is not satisfiable.
   * If TBool::INDETERMINATE is returned, a restart must be performed and the
   * solver is on decision level 0, with all variable assignemnts undone.
   */
  auto solveUntilRestart(std::vector<CNFLit> const& assumedFacts,
                         std::vector<CNFLit>& failedAssumptions) -> TBool;

  enum class FactPropagationResult { CONSISTENT, INCONSISTENT };

  /**
   * Propagates the given facts.
   *
   * \param[in]     factsToPropagate   Facts to be propagated (all variables <= m_maxVar)
   * \param[in,out] failedAssumptions  (see solveUntilRestart)
   *
   * \returns INCONSISTENT if a conflict occured during propagation. Otherwise, CONSISTENT
   *          is returned.
   */
  auto propagateFactsOnSystemLevels(std::vector<CNFLit> const& factsToPropagate,
                                    std::vector<CNFLit>* failedAssumptions)
      -> FactPropagationResult;

  /**
   * Propagates the given "hard facts" (ie. unary clauses).
   *
   * \param[in]     factsToPropagate   Facts to be propagated (all variables <= m_maxVar)
   *
   * \returns INCONSISTENT if a conflict occured during propagation. Otherwise, CONSISTENT
   *          is returned.
   */
  auto propagateHardFacts(std::vector<CNFLit>& facts) -> FactPropagationResult;


  /**
   * Propagates the given assumed facts.
   *
   * \param[in]     factsToPropagate   Facts to be propagated (all variables <= m_maxVar)
   * \param[in,out] failedAssumptions  (see solveUntilRestart)
   *
   * \returns INCONSISTENT if a conflict occured during propagation. Otherwise, CONSISTENT
   *          is returned.
   */
  auto propagateAssumedFacts(std::vector<CNFLit> const& assumedFacts,
                             std::vector<CNFLit>& failedAssumptions) -> FactPropagationResult;

  /**
   * Creates a SolvingResult object describing the current solver state.
   *
   * \param failedAssumptions  (see solveUntilRestart)
   * \returns a SolvingResult object describing the current solver state.
   */
  auto createSolvingResult(TBool result, std::vector<CNFLit> const& failedAssumptions)
      -> std::unique_ptr<SolvingResult>;

  struct LemmaDerivationResult {
    boost::variant<CNFLit, ClauseT*> clause;
    Assignment::Level backtrackLevel;
    bool allocationFailed;
  };

  /**
   * Simplifies the given lemma.
   *
   * \param[in,out] lemma     The lemma to be simplified.
   */
  void optimizeLemma(std::vector<CNFLit>& lemma);

  /**
   * Derives a lemma from the given conflicting clause.
   *
   * This method must be called before backtracking from the conflict.
   *
   * \param conflictingClause     A clause that is falsified by the current variable
   *                              assignment.
   *
   * \returns The lemma derived from the conflict as well as the decision level
   *          to which to backtrack.
   */
  auto deriveLemma(ClauseT& conflictingClause) -> LemmaDerivationResult;


  /**
   * Recomputes the LBD value of the reason clauses associated with the assignments
   * on the current decision level.
   *
   * Udating reason clauses on the current level is relatively cheap since those clauses
   * have likely been used recently and thus are likely present in the L2 cache.
   */
  void updateReasonClauseLBDsOnCurrentLevel();

  enum class ResolveDecisionResult { CONTINUE, RESTART };

  /**
   * Assigns and propagates the given branching literal.
   *
   * This method may only be called when a new decision level L has been set up and
   * no assignments exist on L. When this method returns, a new decision level can
   * be created except when a restart is requested.
   *
   * \returns CONTINUE if the solver can begin a new decision level, RESTART if the
   *          solver must perform a restart.
   */
  auto resolveDecision(CNFLit decision) -> ResolveDecisionResult;

  /**
   * Adds the given clause to the U/NSAT proof if a certificate object is present.
   * 
   * \param clause    A clause satisfying the asymmetric tautology property.
   */
  void addATClauseToProof(gsl::span<CNFLit const> clause);

  /**
   * Finalizes the UNSAT proof if a certificate object is present.
   * 
   * m_certificate is set to nullptr by this method, since no further clauses
   * can be added to the proof.
   */
  void finalizeProofOnUnsat();


  // Solver subsystems
  Assignment m_assignment;
  VSIDSBranchingHeuristic<Assignment> m_branchingHeuristic;
  FirstUIPLearning<Assignment, Assignment> m_conflictAnalyzer;

  std::unique_ptr<ProblemOptimizer> m_optimizer;

  // Clause storage
  IterableClauseDB<ClauseT> m_clauseDB;
  std::vector<CNFLit> m_facts;
  std::vector<ClauseT*> m_lemmas;

  // Policies
  GlucoseClauseDBReductionPolicy<ClauseT, std::vector<ClauseT*>, LBD> m_clauseDBReductionPolicy;
  GlucoseRestartPolicy m_restartPolicy;

  // Control
  CNFVar m_maxVar;
  bool m_detectedUNSAT;
  bool m_hadUnrecoverableError;
  uint64_t m_amntBinariesLearnt;
  Statistics<> m_statistics;
  std::atomic<bool> m_stopRequested;
  Config m_configuration;

  // Buffers
  std::vector<CNFLit> m_lemmaBuffer;
  StampMap<uint16_t, CNFVar::Index, CNFLit::Index, Assignment::LevelKey> m_stamps;

  LoggerFn m_loggerFn;

  DRATCertificate* m_certificate;

  static constexpr uint32_t m_printStatsInterval = 16384;
  static constexpr uint32_t m_checkStopInterval = 8192;
};


SolvingResultImpl::SolvingResultImpl(TBool result,
                                     std::unique_ptr<Model> model,
                                     std::vector<CNFLit> const& failedAssumptions)
  : SolvingResult()
  , m_result{result}
  , m_model{std::move(model)}
  , m_failedAssumptions{failedAssumptions}
{
}

SolvingResultImpl::~SolvingResultImpl() {}

auto SolvingResultImpl::isProblemSatisfiable() const noexcept -> TBool
{
  return m_result;
}

auto SolvingResultImpl::getModel() const noexcept
    -> std::optional<std::reference_wrapper<Model const>>
{
  if (m_model) {
    return std::cref(*m_model);
  }
  else {
    return {};
  }
}

auto SolvingResultImpl::getFailedAssumptions() const noexcept -> std::vector<CNFLit> const&
{
  return m_failedAssumptions;
}

CDCLSatSolverImpl::CDCLSatSolverImpl(Config const& configuration)
  : CDCLSatSolver()
  , m_assignment{CNFVar{0}}
  , m_branchingHeuristic{CNFVar{0}, m_assignment}
  , m_conflictAnalyzer{CNFVar{0}, m_assignment, m_assignment}
  , m_optimizer{createFactCleaner()}
  , m_clauseDB{configuration.clauseRegionSize}
  , m_facts{}
  , m_lemmas{}
  , m_clauseDBReductionPolicy{configuration.clauseRemovalIntervalGrowthRate, m_lemmas}
  , m_restartPolicy{configuration.restartPolicyOptions}
  , m_maxVar{CNFVar{0}}
  , m_detectedUNSAT{false}
  , m_hadUnrecoverableError{false}
  , m_amntBinariesLearnt{0}
  , m_statistics{}
  , m_stopRequested{false}
  , m_configuration{configuration}
  , m_lemmaBuffer{}
  , m_stamps{getMaxLit(CNFVar{0}).getRawValue()}
  , m_loggerFn{}
  , m_certificate{nullptr}
{
  m_conflictAnalyzer.setOnSeenVariableCallback(
      [this](CNFVar var) { m_branchingHeuristic.seenInConflict(var); });
}

CDCLSatSolverImpl::~CDCLSatSolverImpl() {}

void CDCLSatSolverImpl::addProblem(CNFProblem const& problem)
{
  for (CNFClause const& clause : problem.getClauses()) {
    addClause(clause);
  }
}

auto compressClause(CNFClause const& clause) -> std::optional<std::vector<CNFLit>>
{
  if (clause.empty()) {
    return std::vector<CNFLit>{};
  }

  std::vector<CNFLit> compressedClause = withoutRedundancies(clause.begin(), clause.end());

  // The solver requires that no clauses exist containing l as well as ~l.
  // Check if the clause can be ignored. withoutRedundancies returns a sorted
  // clause:
  for (auto claIt = compressedClause.begin() + 1; claIt != compressedClause.end(); ++claIt) {
    if (*(claIt - 1) == ~(*claIt)) {
      return std::optional<std::vector<CNFLit>>{};
    }
  }

  return compressedClause;
}

void CDCLSatSolverImpl::addClause(CNFClause const& clause)
{
  if (clause.empty()) {
    m_detectedUNSAT = true;
    return;
  }

  std::optional<std::vector<CNFLit>> compressed = compressClause(clause);

  if (!compressed) {
    // the clause is always satisfied and has been optimized away
    return;
  }

  if (compressed->size() == 1) {
    m_facts.push_back(compressed->at(0));
  }
  else {
    ClauseT* dbClause = m_clauseDB.createClause(compressed->size());
    if (dbClause == nullptr) {
      throw std::bad_alloc{};
    }

    std::copy(compressed->begin(), compressed->end(), dbClause->begin());
    dbClause->clauseUpdated();
  }

  for (CNFLit lit : *compressed) {
    m_maxVar = std::max(m_maxVar, lit.getVariable());
  }
}


auto CDCLSatSolverImpl::solve(std::vector<CNFLit> const& assumedFacts)
    -> std::unique_ptr<SolvingResult>
{
  try {
    m_statistics.registerSolvingStart();
    m_stopRequested.store(false);

    if (m_configuration.printStatistics && m_loggerFn) {
      m_loggerFn(m_statistics.getStatisticsDescription());
    }

    if (m_hadUnrecoverableError) {
      m_statistics.registerSolvingStop();
      return std::make_unique<SolvingResultImpl>(
          TBools::INDETERMINATE, nullptr, std::vector<CNFLit>{});
    }
    if (m_detectedUNSAT) {
      finalizeProofOnUnsat();
      m_statistics.registerSolvingStop();
      return std::make_unique<SolvingResultImpl>(TBools::FALSE, nullptr, std::vector<CNFLit>{});
    }

    for (CNFLit lit : assumedFacts) {
      m_maxVar = std::max(m_maxVar, lit.getVariable());
    }

    m_facts = withoutRedundancies(m_facts.begin(), m_facts.end());
    resizeSubsystems();
    synchronizeSubsystemsWithClauseDB();
    initializeBranchingHeuristic(assumedFacts);

    TBool intermediateResult = TBools::INDETERMINATE;
    std::vector<CNFLit> failedAssumptions;
    while (!isDeterminate(intermediateResult) && !m_stopRequested.load()) {
      if (trySimplify() == SimplificationResult::DETECTED_UNSAT) {
        failedAssumptions.clear();
        intermediateResult = TBools::FALSE;
        break;
      }
      tryReduceClauseDB();
      m_statistics.registerRestart();
      intermediateResult = solveUntilRestart(assumedFacts, failedAssumptions);
    }

    if (intermediateResult == TBools::FALSE) {
      finalizeProofOnUnsat();
    }

    auto result = createSolvingResult(intermediateResult, failedAssumptions);
    backtrackAll();
    m_statistics.registerSolvingStop();
    return result;
  }
  catch (std::bad_alloc const&) {
    if (m_loggerFn) {
      m_loggerFn("Error: out of memory");
    }
    m_hadUnrecoverableError = true;
    throw;
  }
  catch (FileIOError const&) {
    if (m_loggerFn) {
      m_loggerFn("Error: disk I/O");
    }
    m_hadUnrecoverableError = true;
    throw;
  }
}


auto CDCLSatSolverImpl::createSolvingResult(TBool result,
                                            std::vector<CNFLit> const& failedAssumptions)
    -> std::unique_ptr<SolvingResult>
{
  std::unique_ptr<Model> model{nullptr};

  if (isTrue(result)) {
    model = createModel(m_maxVar);
    for (CNFLit lit : m_assignment.getAssignments()) {
      model->setAssignment(lit.getVariable(),
                           lit.getSign() == CNFSign::POSITIVE ? TBools::TRUE : TBools::FALSE);
    }
  }

  return std::make_unique<SolvingResultImpl>(result,
                                             std::move(model),
                                             isFalse(result) ? std::move(failedAssumptions)
                                                             : std::vector<CNFLit>{});
}

void CDCLSatSolverImpl::resizeSubsystems()
{
  m_assignment.increaseMaxVar(m_maxVar);
  m_branchingHeuristic.increaseMaxVarTo(m_maxVar);
  m_stamps.increaseSizeTo(getMaxLit(m_maxVar).getRawValue());
  m_conflictAnalyzer.increaseMaxVarTo(m_maxVar);
}


void CDCLSatSolverImpl::synchronizeSubsystemsWithClauseDB()
{
  JAM_ASSERT(m_assignment.getNumAssignments() == 0,
             "Illegally attempted to synchronize the clause database in-flight");

  m_assignment.clearClauses();
  m_lemmas.clear();
  for (auto& clause : m_clauseDB.getClauses()) {
    m_assignment.registerClause(clause);
    if (clause.getFlag(Clause::Flag::REDUNDANT)) {
      m_lemmas.push_back(&clause);
    }
  }
}


void CDCLSatSolverImpl::initializeBranchingHeuristic(std::vector<CNFLit> const& assumedFacts)
{
  for (CNFVar i{0}; i <= m_maxVar; i = nextCNFVar(i)) {
    m_branchingHeuristic.setEligibleForDecisions(i, true);
  }
  for (CNFLit assumption : assumedFacts) {
    m_branchingHeuristic.setEligibleForDecisions(assumption.getVariable(), false);
  }
}


auto CDCLSatSolverImpl::trySimplify() -> SimplificationResult
{
  JAM_ASSERT(m_assignment.getNumAssignments() == 0,
             "Illegally attempted to simplify the problem in-flight");

  if (m_optimizer->wantsExecution(m_statistics.getCurrentEra())) {
    JAM_LOG_SOLVER(info, "Beginning simplification");

    PolymorphicClauseDB pmrClauseDB{std::move(m_clauseDB)};
    SharedOptimizerState sharedOptState{std::move(m_facts),
                                        std::move(pmrClauseDB),
                                        std::move(m_assignment),
                                        m_certificate,
                                        m_maxVar};

    SharedOptimizerState result =
        m_optimizer->optimize(std::move(sharedOptState), m_statistics.getCurrentEra());
    std::tie(m_facts, pmrClauseDB, m_assignment) = result.release();

    m_statistics.registerOptimizationStatistics(result.getStats());

    m_clauseDB = pmrClauseDB.release<decltype(m_clauseDB)>();

    if (result.hasBreakingChange()) {
      m_maxVar = result.getMaxVar();
      resizeSubsystems();
      synchronizeSubsystemsWithClauseDB();
    }

    JAM_LOG_SOLVER(info, "Finished simplification");

    bool const unsat = result.hasDetectedUnsat();
    return unsat ? SimplificationResult::DETECTED_UNSAT : SimplificationResult::NONE;
  }

  return SimplificationResult::NONE;
}


void CDCLSatSolverImpl::tryReduceClauseDB()
{
  JAM_ASSERT(m_assignment.getNumAssignments() == 0,
             "Illegally attempted to reduce the clause database in-flight");
  if (!m_clauseDBReductionPolicy.shouldReduceDB()) {
    return;
  }

  JAM_LOG_SOLVER(info, "Starting clause database reduction");

  size_t knownGood = m_amntBinariesLearnt;
  auto beginDel = m_clauseDBReductionPolicy.getClausesMarkedForDeletion(knownGood);
  m_statistics.registerLemmaDeletion(std::distance(beginDel, m_lemmas.end()));
  for (auto delIter = beginDel, end = m_lemmas.end(); delIter != end; ++delIter) {
    (*delIter)->setFlag(ClauseT::Flag::SCHEDULED_FOR_DELETION);
  }

  m_clauseDB.compress();
  synchronizeSubsystemsWithClauseDB();

  JAM_LOG_SOLVER(info, "Finished clause database reduction");
}


void CDCLSatSolverImpl::backtrackAll()
{
  JAM_LOG_SOLVER(info, "Backtracking to level 0");
  prepareBacktrack(0);
  m_assignment.undoAll();
}

void CDCLSatSolverImpl::backtrackToLevel(Assignment::Level targetLevel)
{
  JAM_LOG_SOLVER(info, "Backtracking by revisiting decision level " << targetLevel);
  prepareBacktrack(targetLevel + 1);
  m_assignment.undoToLevel(targetLevel);
}

void CDCLSatSolverImpl::prepareBacktrack(Assignment::Level level)
{
  updateReasonClauseLBDsOnCurrentLevel();

  for (auto l = m_assignment.getCurrentLevel(); l >= level; --l) {
    for (auto lit : m_assignment.getLevelAssignments(l)) {
      m_branchingHeuristic.reset(lit.getVariable());
    }
    if (l == 0) {
      break;
    }
  }
}

void CDCLSatSolverImpl::updateReasonClauseLBDsOnCurrentLevel()
{
  if (m_configuration.maxLBDUpdatesOnBacktrack == 0) {
    return;
  }

  auto level = m_assignment.getCurrentLevel();

  std::size_t updated = 0;
  for (auto lit : boost::adaptors::reverse(m_assignment.getLevelAssignments(level))) {
    if (m_assignment.isForced(lit.getVariable())) {
      ClauseT* reason = m_assignment.getReason(lit.getVariable());
      LBD newLBD = getLBD(*reason, m_assignment, m_stamps);
      reason->setLBD(newLBD);
      ++updated;

      if (updated == m_configuration.maxLBDUpdatesOnBacktrack) {
        return;
      }
    }
  }
}

auto CDCLSatSolverImpl::solveUntilRestart(std::vector<CNFLit> const& assumedFacts,
                                          std::vector<CNFLit>& failedAssumptions) -> TBool
{
  JAM_ASSERT(m_assignment.getNumAssignments() == 0,
             "Illegally called solveUntilRestart() in-flight");
  JAM_LOG_SOLVER(info, "Restarting");

  if (propagateHardFacts(m_facts) == FactPropagationResult::INCONSISTENT) {
    return TBools::FALSE;
  }
  m_assignment.newLevel();
  if (propagateAssumedFacts(assumedFacts, failedAssumptions) ==
      FactPropagationResult::INCONSISTENT) {
    return TBools::FALSE;
  }

  while (!m_assignment.isComplete()) {
    m_assignment.newLevel();
    auto decision = m_branchingHeuristic.pickBranchLiteral();
    JAM_ASSERT(decision != CNFLit::getUndefinedLiteral(),
               "The branching heuristic is not expected to return an undefined literal");
    JAM_LOG_SOLVER(info,
                   "Beginning new decision level " << m_assignment.getCurrentLevel()
                                                   << " with branching decision " << decision);

    if (resolveDecision(decision) == ResolveDecisionResult::RESTART ||
        m_restartPolicy.shouldRestart()) {
      JAM_LOG_SOLVER(info, "Performing restart");
      backtrackAll();
      m_restartPolicy.registerRestart();
      return TBools::INDETERMINATE;
    }

    if (m_statistics.getCurrentEra().m_conflictCount % m_checkStopInterval == 0 &&
        m_stopRequested.load()) {
      return TBools::INDETERMINATE;
    }
  }

  // don't backtrack, so that the satisfying assignment can be read
  return TBools::TRUE;
}

auto CDCLSatSolverImpl::propagateHardFacts(std::vector<CNFLit>& facts) -> FactPropagationResult
{
  JAM_LOG_SOLVER(info,
                 "Propagating hard facts on decision level " << m_assignment.getCurrentLevel());
  auto amntUnits = facts.size();
  auto result = propagateFactsOnSystemLevels(facts, nullptr);
  if (result != FactPropagationResult::INCONSISTENT &&
      m_assignment.getNumAssignments() != amntUnits) {
    auto oldAmntUnits = m_facts.size();
    m_facts.clear();
    auto newUnits = m_assignment.getAssignments();
    for (size_t i = 0; i < (newUnits.size() - oldAmntUnits); ++i) {
      m_statistics.registerLemma(1);
    }
    std::copy(newUnits.begin(), newUnits.end(), std::back_inserter(m_facts));
  }
  return result;
}


auto CDCLSatSolverImpl::propagateAssumedFacts(std::vector<CNFLit> const& assumedFacts,
                                              std::vector<CNFLit>& failedAssumptions)
    -> FactPropagationResult
{
  JAM_LOG_SOLVER(info,
                 "Propagating assumed facts on decision level " << m_assignment.getCurrentLevel());
  return propagateFactsOnSystemLevels(assumedFacts, &failedAssumptions);
}


auto CDCLSatSolverImpl::propagateFactsOnSystemLevels(std::vector<CNFLit> const& factsToPropagate,
                                                     std::vector<CNFLit>* failedAssumptions)
    -> FactPropagationResult
{
  for (auto fact : factsToPropagate) {
    auto assignment = m_assignment.getAssignment(fact.getVariable());

    if (isDeterminate(assignment)) {
      if (toTBool(fact.getSign() == CNFSign::POSITIVE) != assignment) {
        JAM_LOG_SOLVER(info, "Detected conflict at fact " << fact);
        if (failedAssumptions != nullptr) {
          *failedAssumptions = analyzeAssignment(m_assignment, m_assignment, m_stamps, fact);
        }
        return FactPropagationResult::INCONSISTENT;
      }
      else {
        continue;
      }
    }

    bool unitConflict = (m_assignment.append(fact) != nullptr);

    if (unitConflict) {
      JAM_LOG_SOLVER(info, "Detected conflict at fact " << fact);
      if (failedAssumptions != nullptr) {
        *failedAssumptions = analyzeAssignment(m_assignment, m_assignment, m_stamps, fact);
      }
      return FactPropagationResult::INCONSISTENT;
    }

    m_branchingHeuristic.setEligibleForDecisions(fact.getVariable(), false);
  }
  return FactPropagationResult::CONSISTENT;
}

auto CDCLSatSolverImpl::resolveDecision(CNFLit decision) -> ResolveDecisionResult
{
  m_statistics.registerDecision();
  ClauseT* conflictingClause = m_assignment.append(decision);

  while (conflictingClause != nullptr) {
    loggingEpochElapsed();
    JAM_LOG_SOLVER(info, "Handling a conflict at clause " << conflictingClause);
    m_statistics.registerConflict();
    m_branchingHeuristic.beginHandlingConflict();
    LemmaDerivationResult result = deriveLemma(*conflictingClause);
    if (result.allocationFailed) {
      throw std::bad_alloc{};
    }
    m_branchingHeuristic.endHandlingConflict();

    m_clauseDBReductionPolicy.registerConflict();

    if (CNFLit* newFact = boost::get<CNFLit>(&result.clause)) {
      m_facts.push_back(*newFact);
      m_statistics.registerLemma(1);
      addATClauseToProof({newFact, 1});
      return ResolveDecisionResult::RESTART;
    }
    else {
      ClauseT* newLemmaClause = boost::get<ClauseT*>(result.clause);

      if (newLemmaClause->size() > 2ULL) {
        newLemmaClause->setFlag(Clause::Flag::REDUNDANT);
      }
      m_statistics.registerLemma(newLemmaClause->size());

      LBD newLemmaLBD = (*newLemmaClause).template getLBD<LBD>();
      m_restartPolicy.registerConflict({newLemmaLBD});

      addATClauseToProof(newLemmaClause->span());
      backtrackToLevel(result.backtrackLevel);
      conflictingClause = m_assignment.registerLemma(*newLemmaClause);

      if (result.backtrackLevel == 0 ||
          (result.backtrackLevel == 1 && conflictingClause != nullptr)) {
        // Propagating the unit clauses and the assumptions now forces an assignment
        // under which some clause is already "false". Under the current assumptions,
        // the problem is not satisfiable. Perform a final restart to do
        // conflict analysis:
        return ResolveDecisionResult::RESTART;
      }
    }

    if (m_configuration.printStatistics &&
        m_statistics.getCurrentEra().m_conflictCount % m_printStatsInterval == 0) {
      if (m_loggerFn) {
        m_loggerFn(to_string(m_statistics));
      }
    }
  }

  return ResolveDecisionResult::CONTINUE;
}

auto CDCLSatSolverImpl::deriveLemma(ClauseT& conflictingClause) -> LemmaDerivationResult
{
  m_conflictAnalyzer.computeConflictClause(conflictingClause, m_lemmaBuffer);
  JAM_LOG_SOLVER(info, "Derived lemma " << toString(m_lemmaBuffer.begin(), m_lemmaBuffer.end()));
  optimizeLemma(m_lemmaBuffer);

  if (m_lemmaBuffer.size() == 1) {
    return LemmaDerivationResult{m_lemmaBuffer[0], 0, false};
  }
  else {
    ClauseT* newLemma = m_clauseDB.createClause(m_lemmaBuffer.size());

    if (newLemma == nullptr) {
      return LemmaDerivationResult{CNFLit::getUndefinedLiteral(), 0, true};
    }

    std::copy(m_lemmaBuffer.begin(), m_lemmaBuffer.end(), newLemma->begin());
    newLemma->clauseUpdated();
    newLemma->setLBD(getLBD(*newLemma, m_assignment, m_stamps));

    if (newLemma->size() > 2) {
      m_lemmas.push_back(newLemma);
    }
    else {
      ++m_amntBinariesLearnt;
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
    auto litWithMaxDecisionLevel = newLemma->begin() + 1;
    Assignment::Level backtrackLevel = 0;
    auto const lemmaEnd = newLemma->end();
    for (auto lit = newLemma->begin() + 1; lit != lemmaEnd; ++lit) {
      auto currentBacktrackLevel =
          std::max(backtrackLevel, m_assignment.getLevel(lit->getVariable()));
      if (currentBacktrackLevel > backtrackLevel) {
        litWithMaxDecisionLevel = lit;
        backtrackLevel = currentBacktrackLevel;
      }
    }
    std::swap(*litWithMaxDecisionLevel, (*newLemma)[1]);
    return LemmaDerivationResult{newLemma, backtrackLevel, false};
  }
}


void CDCLSatSolverImpl::optimizeLemma(std::vector<CNFLit>& lemma)
{
  eraseRedundantLiterals(lemma, m_assignment, m_assignment, m_stamps);
  JAM_LOG_SOLVER(
      info, "  After redundant literal removal: (" << toString(lemma.begin(), lemma.end()) << ")");
  if (lemma.size() <= m_configuration.lemmaSimplificationSizeBound) {
    LBD lbd = getLBD(lemma, m_assignment, m_stamps);
    if (lbd <= m_configuration.lemmaSimplificationLBDBound) {
      auto binariesMap = m_assignment.getBinariesMap();
      resolveWithBinaries(lemma, binariesMap, lemma[0], m_stamps);
      JAM_LOG_SOLVER(info,
                     "  After resolution with binary clauses: ("
                         << toString(lemma.begin(), lemma.end()) << ")");
    }
  }
}


void CDCLSatSolverImpl::stop() noexcept
{
  m_stopRequested.store(true);
}

void CDCLSatSolverImpl::setLogger(LoggerFn logger)
{
  m_loggerFn = std::move(logger);
}

void CDCLSatSolverImpl::setDRATCertificate(DRATCertificate& cert) noexcept
{
  m_certificate = &cert;
}

void CDCLSatSolverImpl::addATClauseToProof(gsl::span<CNFLit const> clause)
{
  if (m_certificate != nullptr) {
    m_certificate->addATClause(clause);
  }
}

void CDCLSatSolverImpl::finalizeProofOnUnsat()
{
  if (m_certificate != nullptr) {
    std::array<CNFLit, 0> emptyClause;
    m_certificate->addATClause(emptyClause);
    m_certificate->flush();
    m_certificate = nullptr; // preventing proof writes in subsequent solve calls
  }
}

}

auto createCDCLSatSolver() -> std::unique_ptr<CDCLSatSolver>
{
  // Currently, the solver is always instantiated with its default configuration, since the
  // API doesn't allow configuration yet.
  return std::make_unique<CDCLSatSolverImpl>(CDCLSatSolverImpl::Config{});
}
}