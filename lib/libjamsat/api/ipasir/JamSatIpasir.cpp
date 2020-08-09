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

/**
 * \file JamSatIpasir.cpp
 * \brief IPASIR API implementation
 */

#include <libjamsat/JamSatIpasir.h>
#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/drivers/CDCLSatSolver.h>
#include <libjamsat/utils/Assert.h>

#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <iostream>

// Note: the try/catch blocks contained in this file are mostly defensive,
// preventing the solver to crash the client. Eventually, exceptions should
// not escape the solver, but in the short term, they do - plus, the API
// implementation might suffer std::bad_alloc exceptions as well.

namespace jamsat {
namespace {

/**
 * \ingroup JamSAT_API_IPASIR
 *
 * \brief Converts an IPASIR literal to a CNFLit
 *
 * \param ipasirLit     An IPASIR literal
 * \returns             The corresponding CNF literal
 */
CNFLit ipasirLitToCNFLit(int ipasirLit) noexcept
{
  CNFSign sign = (ipasirLit > 0 ? CNFSign::POSITIVE : CNFSign::NEGATIVE);
  CNFVar var{static_cast<CNFVar::RawVariable>(std::abs(ipasirLit))};
  return CNFLit{var, sign};
}

/**
 * \ingroup JamSAT_API_IPASIR
 *
 * \brief IPASIR API SAT solver context
 *
 * This class is responsible for maintaining a SAT solver created via the IPASIR API and
 * handling IPASIR calls. To facilitate this, the public methods of IPASIRContext closely
 * match the IPASIR API functions.
 */
class IPASIRContext {
public:
  /**
   * \brief Context structure for the kill-thread
   *
   * The kill-thread is responsible for asynchronously stopping the solver when demanded
   * by the IPASIR client.
   */
  struct IPASIRKillThreadContext {
    /** Lock for this data structure */
    std::mutex m_lock;

    /**
     * A pointer to the solver, if the context has a solver. If no solver exists, this
     * field is nullptr.
     */
    CDCLSatSolver* m_solver;

    /**
     * A function returning `true` when the solver should be stopped. This function is
     * polled by the kill-thread.
     */
    std::function<bool()> m_userKillCallback;

    /**
     * A flag that is set to `true` as long as the thread's parent IPASIRContext exists,
     * and is set by the parent IPASIRContext to `false` upon its destruction, indicating
     * that the kill-thread should be terminated.
     */
    bool m_parentIpasirContextExists;
  };


  IPASIRContext()
  {
    m_solver.reset(nullptr);
    m_clauseAddBuffer = CNFClause{};
    m_assumptionBuffer = std::vector<CNFLit>{};
    m_result.reset(nullptr);
    m_killThreadContext = nullptr;
    m_failed = false;
  }

  void add(int lit_or_zero)
  {
    if (m_failed) {
      return;
    }

    try {
      ensureSolverExists();
      if (lit_or_zero != 0) {
        m_clauseAddBuffer.push_back(ipasirLitToCNFLit(lit_or_zero));
      }
      else {
        m_solver->addClause(m_clauseAddBuffer);
        m_clauseAddBuffer.clear();
      }
    }
    catch (...) {
      // defensively catching all exceptions
      m_failed = true;
    }
  }

  void assume(int lit) noexcept
  {
    if (m_failed) {
      return;
    }

    try {
      ensureSolverExists();
      m_assumptionBuffer.push_back(ipasirLitToCNFLit(lit));
    }
    catch (...) {
      // defensively catching all exceptions
      m_failed = true;
    }
  }

  int solve() noexcept
  {
    constexpr static int indeterminateResult = 0;
    constexpr static int satisfiableResult = 10;
    constexpr static int unsatisfiableResult = 20;

    if (m_failed) {
      return indeterminateResult;
    }

    try {
      ensureSolverExists();
      m_result = m_solver->solve(m_assumptionBuffer);
      m_assumptionBuffer.clear();
      m_failedAssumptions.clear();

      if (isTrue(m_result->isProblemSatisfiable())) {
        return satisfiableResult;
      }
      if (isFalse(m_result->isProblemSatisfiable())) {
        // Eagerly copying the failed assumptions because
        // exceptions can't be handled in the failed() method
        if (!m_result->getFailedAssumptions().empty()) {
          m_failedAssumptions.insert(m_result->getFailedAssumptions().begin(),
                                     m_result->getFailedAssumptions().end());
        }
        return unsatisfiableResult;
      }
      return indeterminateResult;
    }
    catch (...) {
      // defensively catching all exceptions
      m_failed = true;
      return indeterminateResult;
    }
  }

  int val(int lit) noexcept
  {
    // The client may call this function only in the SAT case
    // and no function called by val() may throw ~> ignore m_failed

    if (!isTrue(m_result->isProblemSatisfiable())) {
      return 0;
    }

    JAM_ASSERT(m_result->getModel(), "Obtained SAT result, but did not produce a model");
    CNFLit internalLit = ipasirLitToCNFLit(lit);

    auto& model = (m_result->getModel())->get();
    TBool varAssignment = model.getAssignment(internalLit.getVariable());

    if (!isDeterminate(varAssignment)) {
      // "unimportant" case
      return 0;
    }

    // Flip the assignment if neccessary:
    TBool::UnderlyingType sign = static_cast<TBool::UnderlyingType>(internalLit.getSign());
    TBool litAssignment =
        TBool::fromUnderlyingValue(varAssignment.getUnderlyingValue() ^ (1 - sign));
    return (isTrue(litAssignment) ? lit : -lit);
  }

  int failed(int lit) noexcept
  {
    // TODO: may the find method throw an exception?
    auto found = (m_failedAssumptions.find(ipasirLitToCNFLit(lit)) != m_failedAssumptions.end());
    return found ? 1 : 0;
  }

  void setTerminate(void* state, int (*terminate)(void* state)) noexcept
  {
    try {
      bool launchNewThread = false;
      if (m_killThreadContext == nullptr) {
        m_killThreadContext = new IPASIRKillThreadContext();
        m_killThreadContext->m_parentIpasirContextExists = true;
        m_killThreadContext->m_solver = m_solver.get();
        launchNewThread = true;
      }

      std::lock_guard<std::mutex> lock(m_killThreadContext->m_lock);
      m_killThreadContext->m_userKillCallback = [terminate, state]() {
        return terminate(state) != 0;
      };

      if (launchNewThread) {
        launchKillThread();
      }
    }
    catch (...) {
      // defensively catching all exceptions
      m_failed = true;
    }
  }

  void setLearn(void* state, int max_length, void (*learn)(void* state, int* clause)) noexcept
  {
    JAM_ASSERT(false, "IPASIR set_learn() is not implemented yet");
    (void)state;
    (void)learn;
    (void)max_length;
  }

  void setLogger(void* state, void (*logger)(void* state, const char*))
  {
    try {
      ensureSolverExists();
      m_solver->setLogger(
          [logger, state](std::string const& message) { logger(state, message.c_str()); });
    }
    catch (...) {
      // defensively catching all exceptions
      m_failed = true;
    }
  }

  ~IPASIRContext()
  {
    // Shut down the kill thread
    if (m_killThreadContext != nullptr) {
      std::lock_guard<std::mutex> lock(m_killThreadContext->m_lock);
      m_killThreadContext->m_parentIpasirContextExists = false;
      m_killThreadContext->m_solver = nullptr;
    }
  }

private:
  void ensureSolverExists()
  {
    if (!m_solver) {
      m_solver = createCDCLSatSolver();

      if (m_killThreadContext != nullptr) {
        std::lock_guard<std::mutex> lock(m_killThreadContext->m_lock);
        m_killThreadContext->m_solver = m_solver.get();
      }
    }
  }

  void launchKillThread()
  {
    std::thread killThread{[](std::unique_ptr<IPASIRKillThreadContext> context) {
                             while (true) {
                               std::chrono::milliseconds period{100};
                               std::this_thread::sleep_for(
                                   std::chrono::duration_cast<std::chrono::nanoseconds>(period));
                               std::lock_guard<std::mutex> lock(context->m_lock);
                               if (!context->m_parentIpasirContextExists) {
                                 return;
                               }
                               if (context->m_solver == nullptr) {
                                 // The solver has not been set up yet. Wait some more
                                 continue;
                               }
                               if (context->m_userKillCallback()) {
                                 context->m_solver->stop();
                               }
                             }
                           },
                           std::unique_ptr<IPASIRKillThreadContext>{m_killThreadContext}};

    killThread.detach();
  }

  /**
   * The solver object. Solvers are created lazily by ensureSolverExists() to
   * allow pre-construction configuration setting later on.
   */
  std::unique_ptr<CDCLSatSolver> m_solver;

  /** A buffer for collecting clause literals added via ipasir_add() */
  CNFClause m_clauseAddBuffer;

  /** A buffer for collecting assumed facts added via ipasir_assume() */
  std::vector<CNFLit> m_assumptionBuffer;

  /** The result of the last ipasir_solve() invocation */
  std::unique_ptr<SolvingResult> m_result;

  /**
   * If the last invocation of ipasir_solve() has produced an UNSAT result and
   * assumed facts have been used by the solver to obtain the UNSAT result,
   * this vector contains a set of assumed facts that has been used by the solver
   * to obtain the UNSAT result.
   */
  std::unordered_set<CNFLit> m_failedAssumptions;

  /**
   * Context object for the kill-thread. A kill-thread is created when setTerminate()
   * is called. Iff no kill-thread exists, m_killThreadContext is `nullptr`.
   * If m_killThreadContext points to an object, that object is owned by the kill-thread.
   */
  IPASIRKillThreadContext* m_killThreadContext;

  /**
   * A flag indicating that an error has occurred from which the solver cannot recover.
   * If m_failed is set, the solver always produces INDETERMINATE results.
   */
  bool m_failed;
};
}
}

extern "C" {
const char* ipasir_signature()
{
  return JAMSAT_SIGNATURE;
}

void* ipasir_init()
{
  try {
    return reinterpret_cast<void*>(new jamsat::IPASIRContext{});
  }
  catch (...) {
    return nullptr;
  }
}

void ipasir_release(void* solver)
{
  if (solver == nullptr) {
    return;
  }
  delete (reinterpret_cast<jamsat::IPASIRContext*>(solver));
}

void ipasir_add(void* solver, int lit_or_zero)
{
  if (solver == nullptr) {
    return;
  }
  reinterpret_cast<jamsat::IPASIRContext*>(solver)->add(lit_or_zero);
}

void ipasir_assume(void* solver, int lit)
{
  if (solver == nullptr) {
    return;
  }
  reinterpret_cast<jamsat::IPASIRContext*>(solver)->assume(lit);
}

int ipasir_solve(void* solver)
{
  if (solver == nullptr) {
    return 0;
  }
  return reinterpret_cast<jamsat::IPASIRContext*>(solver)->solve();
}

int ipasir_val(void* solver, int lit)
{
  JAM_ASSERT(solver != nullptr, "The IPASIR solver is not in the SAT state");
  return reinterpret_cast<jamsat::IPASIRContext*>(solver)->val(lit);
}

int ipasir_failed(void* solver, int lit)
{
  JAM_ASSERT(solver != nullptr, "The IPASIR solver is not in the UNSAT state");
  return reinterpret_cast<jamsat::IPASIRContext*>(solver)->failed(lit);
}

void ipasir_set_terminate(void* solver, void* state, int (*terminate)(void* state))
{
  if (solver == nullptr) {
    return;
  }
  reinterpret_cast<jamsat::IPASIRContext*>(solver)->setTerminate(state, terminate);
}

void ipasir_set_learn(void* solver,
                      void* state,
                      int max_length,
                      void (*learn)(void* state, int* clause))
{
  if (solver == nullptr) {
    return;
  }
  reinterpret_cast<jamsat::IPASIRContext*>(solver)->setLearn(state, max_length, learn);
}

int jamsat_ipasir_set_logger(void* solver,
                             void* state,
                             void (*logger)(void* state, const char* message))
{
  if (solver == nullptr) {
    return -1;
  }
  reinterpret_cast<jamsat::IPASIRContext*>(solver)->setLogger(state, logger);
  return 0;
}
}
