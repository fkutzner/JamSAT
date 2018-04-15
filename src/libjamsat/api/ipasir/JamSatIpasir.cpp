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
        typename SolverType::Configuration config;
        config.clauseMemoryLimit = 10485760;
        m_solver = std::make_unique<SolverType>(config);
    }

    void add(int lit_or_zero) {
        if (lit_or_zero != 0) {
            m_clauseAddBuffer.push_back(ipasirLitToCNFLit(lit_or_zero));
        } else {
            m_solver->addClause(m_clauseAddBuffer);
            m_clauseAddBuffer.clear();
        }
    }

    void assume(int lit) { m_assumptionBuffer.push_back(ipasirLitToCNFLit(lit)); }

    int solve() {
        auto result = m_solver->solve(m_assumptionBuffer);
        m_assumptionBuffer.clear();

        if (result.isSatisfiable == TBool::TRUE) {
            return 10;
        }
        if (result.isSatisfiable == TBool::FALSE) {
            return 20;
        }
        return 0;
    }

    int val(int lit) {
        JAM_ASSERT(false, "IPASIR val() is not implemented yet");
        (void)lit;
        return 0;
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
    std::unique_ptr<CDCLSatSolver<>> m_solver;
    CNFClause m_clauseAddBuffer;
    std::vector<CNFLit> m_assumptionBuffer;
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
