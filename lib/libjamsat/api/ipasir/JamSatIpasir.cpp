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

#include <libjamsat/JamSatIpasir.h>
#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/solver/CDCLSatSolver.h>
#include <libjamsat/utils/Assert.h>

#include <cmath>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

// Note: the try/catch blocks contained in this file are mostly defensive,
// preventing the solver to crash the client. Eventually, exceptions should
// not escape the solver, but in the short term, they do - plus, the API
// implementation might suffer std::bad_alloc exceptions as well.

namespace jamsat {
namespace {
CNFLit ipasirLitToCNFLit(int ipasirLit) noexcept {
    CNFSign sign = (ipasirLit > 0 ? CNFSign::POSITIVE : CNFSign::NEGATIVE);
    CNFVar var{static_cast<CNFVar::RawVariable>(std::abs(ipasirLit))};
    return CNFLit{var, sign};
}

class IPASIRContext {
public:
    struct IPASIRKillThreadContext {
        std::mutex m_lock;
        CDCLSatSolver* m_solver;
        std::function<bool()> m_userKillCallback;
        bool m_parentIpasirContextExists;
    };


    IPASIRContext() {
        // TODO: add a configuration function for this
        // TODO: remove the bound in the default case?
        uint64_t defaultMemLimit = 1024ULL * 1024ULL * 8192ULL;
        if (defaultMemLimit > std::numeric_limits<uintptr_t>::max()) {
            defaultMemLimit = std::numeric_limits<uintptr_t>::max();
        }

        m_solver.reset(nullptr);
        m_clauseAddBuffer = CNFClause{};
        m_assumptionBuffer = std::vector<CNFLit>{};
        m_result.reset(nullptr);
        m_killThreadContext = nullptr;
        m_failed = false;
    }

    // TODO: Add a reconfigure method to the solver
    void ensureSolverExists() {
        if (!m_solver) {
            m_solver = createCDCLSatSolver();
            if (m_killThreadContext != nullptr) {
                std::lock_guard<std::mutex> lock(m_killThreadContext->m_lock);
                m_killThreadContext->m_solver = m_solver.get();
            }
        }
    }

    void add(int lit_or_zero) {
        if (m_failed) {
            return;
        }

        try {
            ensureSolverExists();
            if (lit_or_zero != 0) {
                m_clauseAddBuffer.push_back(ipasirLitToCNFLit(lit_or_zero));
            } else {
                m_solver->addClause(m_clauseAddBuffer);
                m_clauseAddBuffer.clear();
            }
        } catch (std::exception&) {
            m_failed = true;
        }
    }

    void assume(int lit) noexcept {
        if (m_failed) {
            return;
        }

        try {
            ensureSolverExists();
            m_assumptionBuffer.push_back(ipasirLitToCNFLit(lit));
        } catch (std::exception&) {
            m_failed = true;
        }
    }

    int solve() noexcept {
        if (m_failed) {
            return 0;
        }

        try {
            ensureSolverExists();
            m_result = m_solver->solve(m_assumptionBuffer);
            m_assumptionBuffer.clear();
            m_failedAssumptions.clear();

            if (isTrue(m_result->isProblemSatisfiable())) {
                return 10;
            }
            if (isFalse(m_result->isProblemSatisfiable())) {
                // Eagerly copying the failed assumptions because
                // exceptions can't be handled in the failed() method
                if (!m_result->getFailedAssumptions().empty()) {
                    m_failedAssumptions.insert(m_result->getFailedAssumptions().begin(),
                                               m_result->getFailedAssumptions().end());
                }
                return 20;
            }
            return 0;
        } catch (std::exception&) {
            m_failed = true;
            return 0;
        }
    }

    int val(int lit) noexcept {
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

    int failed(int lit) noexcept {
        // TODO: may the find method throw an exception?
        auto found =
            (m_failedAssumptions.find(ipasirLitToCNFLit(lit)) != m_failedAssumptions.end());
        return found ? 1 : 0;
    }

    void setTerminate(void* state, int (*terminate)(void* state)) noexcept {
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
                std::thread killThread{
                    [](std::unique_ptr<IPASIRKillThreadContext> context) {
                        while (true) {
                            std::chrono::milliseconds period{100};
                            std::this_thread::sleep_for(
                                std::chrono::duration_cast<std::chrono::nanoseconds>(period));
                            std::lock_guard<std::mutex> lock(context->m_lock);
                            if (!context->m_parentIpasirContextExists) {
                                return;
                            }
                            if (context->m_solver == nullptr) {
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
        } catch (std::exception&) {
            m_failed = true;
        }
    }

    void setLearn(void* state, int max_length, void (*learn)(void* state, int* clause)) noexcept {
        JAM_ASSERT(false, "IPASIR set_learn() is not implemented yet");
        (void)state;
        (void)learn;
        (void)max_length;
    }

    ~IPASIRContext() {
        // Shut down the kill thread
        if (m_killThreadContext != nullptr) {
            std::lock_guard<std::mutex> lock(m_killThreadContext->m_lock);
            m_killThreadContext->m_parentIpasirContextExists = false;
            m_killThreadContext->m_solver = nullptr;
        }
    }

private:
    std::unique_ptr<CDCLSatSolver> m_solver;
    CNFClause m_clauseAddBuffer;
    std::vector<CNFLit> m_assumptionBuffer;
    std::unique_ptr<SolvingResult> m_result;
    std::unordered_set<CNFLit> m_failedAssumptions;

    // If the killThreadContext object exists, it is owned by the kill-thread
    IPASIRKillThreadContext* m_killThreadContext;

    bool m_failed;
};
}
}

extern "C" {
const char* ipasir_signature() {
    return JAMSAT_SIGNATURE;
}

void* ipasir_init() {
    try {
        return reinterpret_cast<void*>(new jamsat::IPASIRContext{});
    } catch (std::exception&) {
        return nullptr;
    }
}

void ipasir_release(void* solver) {
    if (solver == nullptr) {
        return;
    }
    delete (reinterpret_cast<jamsat::IPASIRContext*>(solver));
}

void ipasir_add(void* solver, int lit_or_zero) {
    if (solver == nullptr) {
        return;
    }
    reinterpret_cast<jamsat::IPASIRContext*>(solver)->add(lit_or_zero);
}

void ipasir_assume(void* solver, int lit) {
    if (solver == nullptr) {
        return;
    }
    reinterpret_cast<jamsat::IPASIRContext*>(solver)->assume(lit);
}

int ipasir_solve(void* solver) {
    if (solver == nullptr) {
        return 0;
    }
    return reinterpret_cast<jamsat::IPASIRContext*>(solver)->solve();
}

int ipasir_val(void* solver, int lit) {
    JAM_ASSERT(solver != nullptr, "The IPASIR solver is not in the SAT state");
    return reinterpret_cast<jamsat::IPASIRContext*>(solver)->val(lit);
}

int ipasir_failed(void* solver, int lit) {
    JAM_ASSERT(solver != nullptr, "The IPASIR solver is not in the UNSAT state");
    return reinterpret_cast<jamsat::IPASIRContext*>(solver)->failed(lit);
}

void ipasir_set_terminate(void* solver, void* state, int (*terminate)(void* state)) {
    if (solver == nullptr) {
        return;
    }
    reinterpret_cast<jamsat::IPASIRContext*>(solver)->setTerminate(state, terminate);
}

void ipasir_set_learn(void* solver,
                      void* state,
                      int max_length,
                      void (*learn)(void* state, int* clause)) {
    if (solver == nullptr) {
        return;
    }
    reinterpret_cast<jamsat::IPASIRContext*>(solver)->setLearn(state, max_length, learn);
}
}
