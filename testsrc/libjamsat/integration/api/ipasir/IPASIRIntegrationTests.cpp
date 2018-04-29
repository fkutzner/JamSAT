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

#include <gtest/gtest.h>

#include <libjamsat/api/ipasir/JamSatIpasir.h>
#include <libjamsat/utils/ControlFlow.h>

TEST(IpasirIntegration, solveWithImmediateConflict) {
    void *solver = ipasir_init();
    auto destroyOnRelease = jamsat::OnExitScope([solver]() { ipasir_release(solver); });

    ipasir_add(solver, 1);
    ipasir_add(solver, 0);

    ipasir_add(solver, -1);
    ipasir_add(solver, 0);

    int result = ipasir_solve(solver);
    EXPECT_EQ(result, 20);
}

namespace {
// Adds the problem (1 2) (-2 3) (-1 -3)
void addMiniSatisfiableProblem(void *solver) {
    ipasir_add(solver, 1);
    ipasir_add(solver, 2);
    ipasir_add(solver, 0);

    ipasir_add(solver, -2);
    ipasir_add(solver, 3);
    ipasir_add(solver, 0);

    ipasir_add(solver, -1);
    ipasir_add(solver, 3);
    ipasir_add(solver, 0);
}
}

TEST(IpasirIntegration, solveMiniSatisfiableProblem) {
    void *solver = ipasir_init();
    auto destroyOnRelease = jamsat::OnExitScope([solver]() { ipasir_release(solver); });

    addMiniSatisfiableProblem(solver);

    int result = ipasir_solve(solver);
    EXPECT_EQ(result, 10);

    // The literal 3 must be assigned "true" in all satisfying assignments
    EXPECT_EQ(ipasir_val(solver, 3), 3);
    EXPECT_EQ(ipasir_val(solver, -3), 3);

    // One of the literals 1 and 2 must also be assigned "true"
    EXPECT_TRUE(ipasir_val(solver, 2) == 2 || ipasir_val(solver, 1) == 1);
}

TEST(IpasirIntegration, assumptionsAreClearedBetweenSolveCalls) {
    void *solver = ipasir_init();
    auto destroyOnRelease = jamsat::OnExitScope([solver]() { ipasir_release(solver); });

    addMiniSatisfiableProblem(solver);

    // Force a top-level conflict:
    ipasir_assume(solver, -1);
    ipasir_assume(solver, -3);
    ASSERT_EQ(ipasir_solve(solver), 20);

    // No assumptions for second call ~> should be satisfiable
    ASSERT_EQ(ipasir_solve(solver), 10);
}
