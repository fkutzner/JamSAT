/* Copyright (c) 2019 Felix Kutzner (github.com/fkutzner)

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

#include "IpasirSolver.h"

#include <libjamsat/JamSatIpasir.h>

#include <cassert>

namespace jamsat {
IpasirSolver::IpasirSolver() {}
IpasirSolver::~IpasirSolver() {}

namespace {
class IpasirAPIWrapper : public IpasirSolver {
public:
    IpasirAPIWrapper();
    virtual ~IpasirAPIWrapper();

    void addClause(std::vector<int> const& literals) noexcept override;
    auto solve(std::vector<int> const& assumedFacts) noexcept -> Result override;
    auto getValue(int literal) noexcept -> Value override;
    auto isFailed(int literal) noexcept -> bool override;
    void setTerminateFn(void* state, int (*terminate)(void* state)) noexcept override;
    void setLearnFn(void* state,
                    int max_length,
                    void (*learn)(void* state, int* clause)) noexcept override;

    IpasirAPIWrapper(IpasirAPIWrapper const& rhs) = delete;
    IpasirAPIWrapper(IpasirAPIWrapper&& rhs) = delete;
    auto operator=(IpasirAPIWrapper const& rhs) = delete;
    auto operator=(IpasirAPIWrapper&& rhs) = delete;

private:
    void* m_solver;
};

IpasirAPIWrapper::IpasirAPIWrapper() : m_solver{nullptr} {
    m_solver = ipasir_init();
}

IpasirAPIWrapper::~IpasirAPIWrapper() {
    if (m_solver) {
        ipasir_release(m_solver);
        m_solver = nullptr;
    }
}

void IpasirAPIWrapper::addClause(std::vector<int> const& literals) noexcept {
    for (int lit : literals) {
        ipasir_add(m_solver, lit);
    }
    ipasir_add(m_solver, 0);
}

auto IpasirAPIWrapper::solve(std::vector<int> const& assumedFacts) noexcept -> Result {
    for (int assumption : assumedFacts) {
        ipasir_assume(m_solver, assumption);
    }

    int result = ipasir_solve(m_solver);
    assert(result == 0 || result == 10 || result == 20);
    switch (result) {
    case 0:
        return Result::INDETERMINATE;
    case 10:
        return Result::SATISFIABLE;
    case 20:
        return Result::UNSATISFIABLE;
    default:
        return Result::INDETERMINATE;
    }
}

auto IpasirAPIWrapper::getValue(int literal) noexcept -> Value {
    int result = ipasir_val(m_solver, literal);
    assert(result == 0 || result == literal || result == -literal);
    if (result == 0) {
        return Value::DONTCARE;
    }
    return result == literal ? Value::TRUE : Value::FALSE;
}

auto IpasirAPIWrapper::isFailed(int literal) noexcept -> bool {
    return ipasir_failed(m_solver, literal);
}

void IpasirAPIWrapper::setTerminateFn(void* state, int (*terminate)(void* state)) noexcept {
    ipasir_set_terminate(m_solver, state, terminate);
}

void IpasirAPIWrapper::setLearnFn(void* state,
                                  int max_length,
                                  void (*learn)(void* state, int* clause)) noexcept {
    ipasir_set_learn(m_solver, state, max_length, learn);
}
}

auto createIpasirSolver() -> std::unique_ptr<IpasirSolver> {
    return std::make_unique<IpasirAPIWrapper>();
}
}
