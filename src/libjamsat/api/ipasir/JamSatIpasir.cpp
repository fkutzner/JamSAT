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

#include <libjamsat/api/ipasir/JamSatIpasir.h>
#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/solver/CDCLSatSolver.h>
#include <libjamsat/utils/Assert.h>

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace jamsat {
namespace {
CNFLit ipasirLitToCNFLit(int ipasirLit) noexcept {
    CNFSign sign = (ipasirLit > 0 ? CNFSign::POSITIVE : CNFSign::NEGATIVE);
    CNFVar var{static_cast<CNFVar::RawVariable>(std::abs(ipasirLit))};
    return CNFLit{var, sign};
}

class IPASIRContext {
public:
    using SolverType = CDCLSatSolver<>;

    IPASIRContext() {
        // TODO: add a configuration function for this
        // TODO: remove the bound in the default case?
        uint64_t defaultMemLimit = 1024ULL * 1024ULL * 8192ULL;
        if (defaultMemLimit > std::numeric_limits<uintptr_t>::max()) {
            defaultMemLimit = std::numeric_limits<uintptr_t>::max();
        }

        m_config.clauseMemoryLimit = defaultMemLimit;
        m_solver.reset(nullptr);
        m_clauseAddBuffer = CNFClause{};
        m_assumptionBuffer = std::vector<CNFLit>{};
        m_result = typename SolverType::SolvingResult{};
    }

    // TODO: Add a reconfigure method to the solver
    void ensureSolverExists() {
        if (!m_solver) {
            m_solver = std::make_unique<SolverType>(m_config);
        }
    }

    void add(int lit_or_zero) {
        ensureSolverExists();
        if (lit_or_zero != 0) {
            m_clauseAddBuffer.push_back(ipasirLitToCNFLit(lit_or_zero));
        } else {
            m_solver->addClause(m_clauseAddBuffer);
            m_clauseAddBuffer.clear();
        }
    }

    void assume(int lit) {
        ensureSolverExists();
        m_assumptionBuffer.push_back(ipasirLitToCNFLit(lit));
    }

    int solve() {
        ensureSolverExists();
        m_result = m_solver->solve(m_assumptionBuffer);
        m_assumptionBuffer.clear();

        if (isTrue(m_result.isSatisfiable)) {
            return 10;
        }
        if (isFalse(m_result.isSatisfiable)) {
            return 20;
        }
        return 0;
    }

    int val(int lit) {
        if (!isTrue(m_result.isSatisfiable)) {
            // The client may call this function only in the SAT case
            return 0;
        }

        JAM_ASSERT(m_result.model.get() != nullptr,
                   "Obtained SAT result, but did not produce a model");
        CNFLit internalLit = ipasirLitToCNFLit(lit);
        TBool varAssignment = m_result.model->getAssignment(internalLit.getVariable());

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

    int failed(int lit) {
        JAM_ASSERT(false, "IPASIR failed() is not implemented yet");
        (void)lit;
        return 0;
    }

    void setTerminate(void *state, int (*terminate)(void *state)) {
        JAM_ASSERT(false, "IPASIR set_terminate() is not implemented yet");
        (void)state;
        (void)terminate;
    }

    void setLearn(void *state, int max_length, void (*learn)(void *state, int *clause)) {
        JAM_ASSERT(false, "IPASIR set_learn() is not implemented yet");
        (void)state;
        (void)learn;
        (void)max_length;
    }

private:
    typename SolverType::Configuration m_config;
    std::unique_ptr<CDCLSatSolver<>> m_solver;
    CNFClause m_clauseAddBuffer;
    std::vector<CNFLit> m_assumptionBuffer;
    typename SolverType::SolvingResult m_result;
};
}
}

extern "C" {
const char *ipasir_signature() {
    return nullptr;
}

void *ipasir_init() {
    return reinterpret_cast<void *>(new jamsat::IPASIRContext{});
}

void ipasir_release(void *solver) {
    delete (reinterpret_cast<jamsat::IPASIRContext *>(solver));
}

void ipasir_add(void *solver, int lit_or_zero) {
    reinterpret_cast<jamsat::IPASIRContext *>(solver)->add(lit_or_zero);
}

void ipasir_assume(void *solver, int lit) {
    reinterpret_cast<jamsat::IPASIRContext *>(solver)->assume(lit);
}

int ipasir_solve(void *solver) {
    return reinterpret_cast<jamsat::IPASIRContext *>(solver)->solve();
}

int ipasir_val(void *solver, int lit) {
    return reinterpret_cast<jamsat::IPASIRContext *>(solver)->val(lit);
}

int ipasir_failed(void *solver, int lit) {
    return reinterpret_cast<jamsat::IPASIRContext *>(solver)->failed(lit);
}

void ipasir_set_terminate(void *solver, void *state, int (*terminate)(void *state)) {
    reinterpret_cast<jamsat::IPASIRContext *>(solver)->setTerminate(state, terminate);
}

void ipasir_set_learn(void *solver, void *state, int max_length,
                      void (*learn)(void *state, int *clause)) {
    reinterpret_cast<jamsat::IPASIRContext *>(solver)->setLearn(state, max_length, learn);
}
}
