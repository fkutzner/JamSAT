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

#include <libjamsat/solver/CDCLSatSolver.h>
#include <toolbox/cnfgenerators/Rule110.h>

namespace jamsat {

TEST(SolverIntegration, CDCLSatSolver_problemWithEmptyClauseIsUnsatisfiable) {
    CDCLSatSolver<>::Configuration testConfig;
    testConfig.clauseMemoryLimit = 1048576ULL;
    CDCLSatSolver<> underTest{testConfig};

    underTest.addClause({});
    EXPECT_EQ(underTest.solve({}).isSatisfiable, TBool::FALSE);
}

TEST(SolverIntegration, CDCLSatSolver_problemWithNoClausesIsTriviallySatisfiable) {
    CDCLSatSolver<>::Configuration testConfig;
    testConfig.clauseMemoryLimit = 1048576ULL;
    CDCLSatSolver<> underTest{testConfig};
    EXPECT_EQ(underTest.solve({}).isSatisfiable, TBool::TRUE);
}

TEST(SolverIntegration, CDCLSatSolver_problemConsistingOfUnitClauseIsSatisfiable) {
    CDCLSatSolver<>::Configuration testConfig;
    testConfig.clauseMemoryLimit = 1048576ULL;

    CDCLSatSolver<> underTest{testConfig};
    underTest.addClause({CNFLit{CNFVar{1}, CNFSign::POSITIVE}});
    EXPECT_EQ(underTest.solve({}).isSatisfiable, TBool::TRUE);
}

TEST(SolverIntegration, SimpleCDCL_rule110_reachable) {
    Rule110PredecessorStateProblem problem{"1xxx0", "0xx10", 5};
    auto cnfProblem = problem.getCNFEncoding();

    CDCLSatSolver<>::Configuration testConfig;
    CDCLSatSolver<> underTest{testConfig};

    testConfig.clauseMemoryLimit = 1048576ULL;
    for (auto &clause : cnfProblem.getClauses()) {
        underTest.addClause(clause);
    }
    EXPECT_EQ(underTest.solve({}).isSatisfiable, TBool::TRUE);
}

TEST(SolverIntegration, SimpleCDCL_rule110_unreachable) {
    Rule110PredecessorStateProblem problem{"1xxx0", "00010", 5};
    auto cnfProblem = problem.getCNFEncoding();

    CDCLSatSolver<>::Configuration testConfig;
    CDCLSatSolver<> underTest{testConfig};

    testConfig.clauseMemoryLimit = 1048576ULL;
    for (auto &clause : cnfProblem.getClauses()) {
        underTest.addClause(clause);
    }
    EXPECT_EQ(underTest.solve({}).isSatisfiable, TBool::FALSE);
}
}
