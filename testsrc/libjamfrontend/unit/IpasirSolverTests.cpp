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

#include <gtest/gtest.h>

#include <libjamfrontend/IpasirSolver.h>

#include <libjamfrontend/ipasirmock/IpasirMock.h>

// Test setup: this test is linked agains a mock IPASIR implementation exposing
// its state via an IpasirMockContext object.

TEST(UnitFrontendIpasirSolver, ipasirSolverAddsClausesWithZeroTermination) {
    std::unique_ptr<jamsat::IpasirSolver> underTest = jamsat::createIpasirSolver();
    jamsat::IpasirMockContext& mockCtx = jamsat::getCurrentIPASIRMockContext();

    underTest->addClause({2, 3, 5, 7});
    underTest->addClause({11});

    std::vector<int> expected = {2, 3, 5, 7, 0, 11, 0};
    EXPECT_EQ(mockCtx.m_literals, expected);
}

TEST(UnitFrontendIpasirSolver, ipasirSolverTranslatesLiteralValuesCorrectly) {
    std::unique_ptr<jamsat::IpasirSolver> underTest = jamsat::createIpasirSolver();
    jamsat::IpasirMockContext& mockCtx = jamsat::getCurrentIPASIRMockContext();

    mockCtx.m_cfgLiteralVals[2] = 2;
    mockCtx.m_cfgLiteralVals[3] = -3;
    mockCtx.m_cfgLiteralVals[4] = 0;

    EXPECT_EQ(underTest->getValue(2), jamsat::IpasirSolver::Value::TRUE);
    EXPECT_EQ(underTest->getValue(3), jamsat::IpasirSolver::Value::FALSE);
    EXPECT_EQ(underTest->getValue(4), jamsat::IpasirSolver::Value::DONTCARE);
}

TEST(UnitFrontendIpasirSolver, ipasirSolverTranslatesLiteralFailuresCorrectly) {
    std::unique_ptr<jamsat::IpasirSolver> underTest = jamsat::createIpasirSolver();
    jamsat::IpasirMockContext& mockCtx = jamsat::getCurrentIPASIRMockContext();

    mockCtx.m_cfgLiteralFailures[2] = 1;
    mockCtx.m_cfgLiteralFailures[3] = 0;

    EXPECT_EQ(underTest->isFailed(2), true);
    EXPECT_EQ(underTest->isFailed(3), false);
}

TEST(UnitFrontendIpasirSolver, ipasirSolverSetsAssumptionsOnSolve) {
    std::unique_ptr<jamsat::IpasirSolver> underTest = jamsat::createIpasirSolver();
    jamsat::IpasirMockContext& mockCtx = jamsat::getCurrentIPASIRMockContext();

    mockCtx.m_cfgSolveResult = 0;

    std::vector<int> assumedFacts = {10, 11, 12, 13};
    underTest->solve(assumedFacts);
    EXPECT_EQ(mockCtx.m_assumptionsAtLastSolveCall, assumedFacts);

    assumedFacts = {13, 14};
    underTest->solve(assumedFacts);
    EXPECT_EQ(mockCtx.m_assumptionsAtLastSolveCall, assumedFacts);

    assumedFacts = {};
    underTest->solve(assumedFacts);
    EXPECT_EQ(mockCtx.m_assumptionsAtLastSolveCall, assumedFacts);
}

TEST(UnitFrontendIpasirSolver, ipasirSolverTranslatesSolveResultCorrectly) {
    std::unique_ptr<jamsat::IpasirSolver> underTest = jamsat::createIpasirSolver();
    jamsat::IpasirMockContext& mockCtx = jamsat::getCurrentIPASIRMockContext();
    mockCtx.m_cfgSolveResult = 0;
    auto result = underTest->solve({});
    EXPECT_EQ(result, jamsat::IpasirSolver::Result::INDETERMINATE);

    mockCtx.m_cfgSolveResult = 10;
    result = underTest->solve({});
    EXPECT_EQ(result, jamsat::IpasirSolver::Result::SATISFIABLE);

    mockCtx.m_cfgSolveResult = 20;
    result = underTest->solve({});
    EXPECT_EQ(result, jamsat::IpasirSolver::Result::UNSATISFIABLE);
}