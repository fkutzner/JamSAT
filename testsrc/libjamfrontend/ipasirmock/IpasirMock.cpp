/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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

#include "IpasirMock.h"

#include <libjamsat/JamSatIpasir.h>

#include <exception>
#include <iostream>
#include <vector>

const char* IPASIRTestMockSignature = "JamSAT IPASIR test mock";

namespace jamsat {
jamsat::IpasirMockContext* currentIPASIRMockContext = nullptr;

auto getCurrentIPASIRMockContext() noexcept -> IpasirMockContext& {
    return *reinterpret_cast<jamsat::IpasirMockContext*>(currentIPASIRMockContext);
}
}

extern "C" {
auto ipasir_signature() -> const char* {
    return IPASIRTestMockSignature;
}

auto ipasir_init() -> void* {
    if (jamsat::currentIPASIRMockContext != nullptr) {
        std::cerr << "Detected forbidden concurrent usage of the IPASIR mock" << std::endl;
        std::terminate();
    }
    jamsat::currentIPASIRMockContext = new jamsat::IpasirMockContext{};
    return reinterpret_cast<void*>(jamsat::currentIPASIRMockContext);
}

void ipasir_release(void* solver) {
    if (jamsat::currentIPASIRMockContext != reinterpret_cast<jamsat::IpasirMockContext*>(solver)) {
        std::cerr << "Detected forbidden concurrent usage of the IPASIR mock" << std::endl;
        std::terminate();
    }
    jamsat::currentIPASIRMockContext = nullptr;
    delete reinterpret_cast<jamsat::IpasirMockContext*>(solver);
}

void ipasir_add(void* solver, int lit_or_zero) {
    jamsat::IpasirMockContext* context = reinterpret_cast<jamsat::IpasirMockContext*>(solver);
    context->m_literals.push_back(lit_or_zero);
}

void ipasir_assume(void* solver, int lit) {
    jamsat::IpasirMockContext* context = reinterpret_cast<jamsat::IpasirMockContext*>(solver);
    context->m_assumptions.push_back(lit);
}

auto ipasir_solve(void* solver) -> int {
    jamsat::IpasirMockContext* context = reinterpret_cast<jamsat::IpasirMockContext*>(solver);
    context->m_assumptionsAtLastSolveCall = context->m_assumptions;
    context->m_assumptions.clear();
    return context->m_cfgSolveResult;
}

auto ipasir_val(void* solver, int lit) -> int {
    jamsat::IpasirMockContext* context = reinterpret_cast<jamsat::IpasirMockContext*>(solver);
    return context->m_cfgLiteralVals[lit];
}

auto ipasir_failed(void* solver, int lit) -> int {
    jamsat::IpasirMockContext* context = reinterpret_cast<jamsat::IpasirMockContext*>(solver);
    return context->m_cfgLiteralFailures[lit];
}

void ipasir_set_terminate(void* solver, void* state, int (*terminate)(void* state)) {
    (void)solver;
    (void)state;
    (void)terminate;
}

void ipasir_set_learn(void* solver,
                      void* state,
                      int max_length,
                      void (*learn)(void* state, int* clause)) {
    (void)solver;
    (void)state;
    (void)max_length;
    (void)learn;
}

int jamsat_ipasir_set_logger(void*, void*, void (*)(void* state, const char* message)) {
    return 0;
}
}
