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

#include "OnlineDRATChecker.h"

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/solver/Assignment.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/Printers.h>
#include <libjamsat/utils/RangeUtils.h>

#include <gsl/span>

#include <algorithm>

#if defined(JAM_ENABLE_CERT_LOGGING)
#define JAM_LOG_CERT(x, y) JAM_LOG(x, "uscert", y)
#else
#define JAM_LOG_CERT(x, y)
#endif

namespace jamsat {
namespace {

class OnlineDRATCheckerImpl : public OnlineDRATChecker {
public:
  explicit OnlineDRATCheckerImpl(CNFProblem const& problem);
  void addRATClause(gsl::span<CNFLit const> clause, size_t pivotIdx) override;
  void addATClause(gsl::span<CNFLit const> clause) override;
  void deleteClause(gsl::span<CNFLit const> clause) override;
  void flush() override;

  auto hasValidatedUnsat() const noexcept -> bool override;
  auto hasDetectedInvalidLemma() const noexcept -> bool override;
  auto hasDetectedUnsupportedLemma() const noexcept -> bool override;
  auto getResultComments() const noexcept -> std::vector<std::string> const& override;

private:
  void addProblem(CNFProblem const& problem);
  void addClause(gsl::span<CNFLit const> clause);
  void addUnaryClause(gsl::span<CNFLit const> clause);
  void addNonUnaryClause(gsl::span<CNFLit const> clause);


  bool isATClause(gsl::span<CNFLit const> clause);
  bool isRATClause(gsl::span<CNFLit const> clause, std::size_t pivotIdx);

  void log(std::string const& message);

  enum class State {
    NORMAL,
    VALIDATED_UNSAT,
    FINALIZED_PROOF,
    DETECTED_INVALID_LEMMA,
    DETECTED_UNSUPPORTED_LEMMA
  };

  State m_currentState;

  std::vector<std::string> m_resultComments;

  CNFVar m_maxVar;

  std::vector<std::unique_ptr<Clause>> m_clauses;
  Assignment m_assignment;
};


OnlineDRATCheckerImpl::OnlineDRATCheckerImpl(CNFProblem const& problem)
  : m_currentState{State::NORMAL}
  , m_resultComments{}
  , m_maxVar{problem.getMaxVar()}
  , m_clauses{}
  , m_assignment{problem.getMaxVar()}
{
  addProblem(problem);
}

void OnlineDRATCheckerImpl::addProblem(CNFProblem const& problem)
{
  // Adding all non-unary clauses before unary clauses to be able to detect
  // unary-level conflicts eagerly (needed for correctness)
  std::vector<CNFLit> facts;

  for (CNFClause const& rawClause : problem.getClauses()) {
    std::vector<CNFLit> clause = withoutRedundancies(rawClause.begin(), rawClause.end());

    if (clause.empty()) {
      m_currentState = State::VALIDATED_UNSAT;
      break;
    }
    else if (clause.size() == 1) {
      facts.push_back(clause[0]);
    }
    else {
      auto redundantP = [](CNFLit lhs, CNFLit rhs) { return lhs == ~rhs; };
      if (auto taut = std::adjacent_find(clause.begin(), clause.end(), redundantP);
          taut == clause.end()) {
        addClause(clause);
      }
    }
  }

  for (CNFLit lit : facts) {
    addClause({&lit, 1});
  }
}

auto OnlineDRATCheckerImpl::hasValidatedUnsat() const noexcept -> bool
{
  return m_currentState == State::FINALIZED_PROOF;
}

auto OnlineDRATCheckerImpl::hasDetectedInvalidLemma() const noexcept -> bool
{
  return m_currentState == State::DETECTED_INVALID_LEMMA;
}

auto OnlineDRATCheckerImpl::hasDetectedUnsupportedLemma() const noexcept -> bool
{
  return m_currentState == State::DETECTED_UNSUPPORTED_LEMMA;
}

auto OnlineDRATCheckerImpl::getResultComments() const noexcept -> std::vector<std::string> const&
{
  return m_resultComments;
}

void OnlineDRATCheckerImpl::addClause(gsl::span<CNFLit const> clause)
{
  JAM_ASSERT(!clause.empty(), "Adding empty clauses is not allowed");
  if (clause.size() > 1) {
    addNonUnaryClause(clause);
  }
  else if (clause.size() == 1) {
    addUnaryClause(clause);
  }
}

void OnlineDRATCheckerImpl::addUnaryClause(gsl::span<CNFLit const> clause)
{
  JAM_ASSERT(clause.size() == 1, "clause must be unary");
  CNFLit newFact = clause[0];
  if (newFact.getVariable() > m_maxVar) {
    m_assignment.increaseMaxVar(newFact.getVariable());
  }
  JAM_ASSERT(m_assignment.getCurrentLevel() == 0, "Adding clauses is only allowed on level 0");

  if (TBool curAssign = m_assignment.getAssignment(newFact); isDeterminate(curAssign)) {
    if (isFalse(curAssign)) {
      JAM_LOG_CERT(info, "Validated unsat at lit " << newFact);
      m_currentState = State::VALIDATED_UNSAT;
    }
  }
  else {
    Clause* conflicting = m_assignment.append(newFact);
    if (conflicting != nullptr) {
      m_currentState = State::VALIDATED_UNSAT;
    }
  }
}

void OnlineDRATCheckerImpl::addNonUnaryClause(gsl::span<CNFLit const> clause)
{
  JAM_ASSERT(clause.size() > 1, "clause must not be empty or unary");
  m_clauses.emplace_back(createHeapClause(clause.size()));
  Clause& insertedClause = *(m_clauses.back());

  std::copy(clause.begin(), clause.end(), insertedClause.begin());
  std::sort(insertedClause.begin(), insertedClause.end());
  insertedClause.clauseUpdated();

  for (CNFLit lit : clause) {
    if (lit.getVariable() > m_maxVar) {
      m_maxVar = lit.getVariable();
      m_assignment.increaseMaxVar(m_maxVar);
    }
  }

  m_assignment.registerClause(insertedClause);
}

void OnlineDRATCheckerImpl::addRATClause(gsl::span<CNFLit const> clause, size_t pivotIdx)
{
  JAM_LOG_CERT(info,
               "Adding RAT clause: (" << toString(clause.begin(), clause.end()) << "), pivot "
                                      << pivotIdx);

  if (clause.empty()) {
    log("Empty clause passed to addRATClause");
    m_currentState = State::DETECTED_INVALID_LEMMA;
    return;
  }

  if (m_currentState != State::NORMAL) {
    if (m_currentState == State::FINALIZED_PROOF || m_currentState == State::VALIDATED_UNSAT) {
      log("After proof completion, this checker accepts only the empty clause, but got: " +
          toString(clause.begin(), clause.end()));
      m_currentState = State::DETECTED_UNSUPPORTED_LEMMA;
    }
    return;
  }

  bool const hasRATProperty = isRATClause(clause, pivotIdx);
  if (!hasRATProperty) {
    log("Failed to validate RAT property for lemma " + toString(clause.begin(), clause.end()));
    m_currentState = State::DETECTED_INVALID_LEMMA;
  }

  addClause(clause);
}

void OnlineDRATCheckerImpl::addATClause(gsl::span<CNFLit const> clause)
{
  JAM_LOG_CERT(info, "Adding AT clause: (" << toString(clause.begin(), clause.end()) << ")");

  if (m_currentState != State::NORMAL) {
    if (m_currentState == State::VALIDATED_UNSAT && clause.empty()) {
      m_currentState = State::FINALIZED_PROOF;
      return;
    }
    else if (m_currentState == State::FINALIZED_PROOF || m_currentState == State::VALIDATED_UNSAT) {
      log("After proof completion, this checker accepts only the empty clause, but got: " +
          toString(clause.begin(), clause.end()));
      m_currentState = State::DETECTED_UNSUPPORTED_LEMMA;
      return;
    }

    // Otherwise: already complained earlier
    return;
  }

  if (clause.empty()) {
    // unsatisfiability has not been validated yet
    log("Failed to validate AT property for the empty clause");
    m_currentState = State::DETECTED_INVALID_LEMMA;
    return;
  }

  bool const hasATProperty = isATClause(clause);
  if (!hasATProperty) {
    log("Failed to validate AT property for lemma " + toString(clause.begin(), clause.end()));
    m_currentState = State::DETECTED_INVALID_LEMMA;
  }
  addClause(clause);
}

void OnlineDRATCheckerImpl::deleteClause(gsl::span<CNFLit const> clause)
{
  JAM_LOG_CERT(info, "Deleting clause: (" << toString(clause.begin(), clause.end()) << ")");
  // TODO: find clause & mark it as removed
}

bool OnlineDRATCheckerImpl::isATClause(gsl::span<CNFLit const> clause)
{
  Assignment::Level currentLevel = m_assignment.getCurrentLevel();
  OnExitScope restoreLevel{[currentLevel, this]() { m_assignment.undoToLevel(currentLevel); }};
  m_assignment.newLevel();

  bool hasConflict = false;
  for (CNFLit assignLit : clause) {
    if (TBool curAssign = m_assignment.getAssignment(~assignLit); isDeterminate(curAssign)) {
      if (isFalse(curAssign)) {
        return true;
      }
      continue;
    }

    hasConflict = (m_assignment.append(~assignLit) != nullptr);
    if (hasConflict) {
      return true;
    }
  }
  return false;
}

bool OnlineDRATCheckerImpl::isRATClause(gsl::span<CNFLit const> clause, std::size_t)
{
  // If the clause is AT, then it is also RAT, and AT is way cheaper to check
  if (isATClause(clause)) {
    return true;
  }

  log("RAT clauses not supported yet");
  m_currentState = State::DETECTED_UNSUPPORTED_LEMMA;
  return false;
}

void OnlineDRATCheckerImpl::flush() {}

void OnlineDRATCheckerImpl::log(std::string const& message)
{
  JAM_LOG_CERT(info, message);
  m_resultComments.push_back(message);
}
}


std::unique_ptr<OnlineDRATChecker> createOnlineDRATChecker(CNFProblem const& problem)
{
  return std::make_unique<OnlineDRATCheckerImpl>(problem);
}
}