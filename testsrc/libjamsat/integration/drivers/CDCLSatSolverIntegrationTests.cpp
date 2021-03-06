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

#include <libjamsat/drivers/CDCLSatSolver.h>
#include <toolbox/cnfgenerators/Rule110.h>
#include <toolbox/testutils/Minisat.h>


namespace jamsat {

TEST(DriversIntegration, CDCLSatSolver_problemWithEmptyClauseIsUnsatisfiable)
{
  std::unique_ptr<CDCLSatSolver> underTest = createCDCLSatSolver();
  underTest->addClause({});
  EXPECT_EQ(underTest->solve({})->isProblemSatisfiable(), TBools::FALSE);
}

TEST(DriversIntegration, CDCLSatSolver_problemWithNoClausesIsTriviallySatisfiable)
{
  std::unique_ptr<CDCLSatSolver> underTest = createCDCLSatSolver();
  EXPECT_EQ(underTest->solve({})->isProblemSatisfiable(), TBools::TRUE);
}

TEST(DriversIntegration, CDCLSatSolver_problemConsistingOfUnitClauseIsSatisfiable)
{
  std::unique_ptr<CDCLSatSolver> underTest = createCDCLSatSolver();
  underTest->addClause({1_Lit});
  EXPECT_EQ(underTest->solve({})->isProblemSatisfiable(), TBools::TRUE);
}

TEST(DriversIntegration, CDCLSatSolver_problemWithConflictingUnitClausesIsUnsatisfiable)
{
  std::unique_ptr<CDCLSatSolver> underTest = createCDCLSatSolver();
  underTest->addClause({1_Lit});
  underTest->addClause({~1_Lit});
  EXPECT_EQ(underTest->solve({})->isProblemSatisfiable(), TBools::FALSE);
}

TEST(DriversIntegration, CDCLSatSolver_assumptionsWithVarsOutsideOfProblemCanBeAdded)
{
  std::unique_ptr<CDCLSatSolver> underTest = createCDCLSatSolver();
  underTest->addClause({1_Lit});
  std::unique_ptr<SolvingResult> result = underTest->solve(std::vector<CNFLit>{100_Lit});
  ASSERT_TRUE(isTrue(result->isProblemSatisfiable()));
  Model const& model = *(result->getModel());
  EXPECT_EQ(model.getAssignment(100_Var), TBools::TRUE);
}

TEST(DriversIntegration, CDCLSatSolver_rule110_reachable)
{
  Rule110PredecessorStateProblem problem{"xx1xx", "x1xxx", 7};
  auto rule110Encoding = problem.getCNFEncoding();

  std::unique_ptr<CDCLSatSolver> underTest = createCDCLSatSolver();

  for (auto& clause : rule110Encoding.cnfProblem.getClauses()) {
    underTest->addClause(clause);
  }

  ASSERT_EQ(isSatisfiableViaMinisat(rule110Encoding.cnfProblem), TBools::TRUE)
      << "Bad test case: the problem is expected to be satisfiable";
  auto solvingResult = underTest->solve({});
  ASSERT_EQ(solvingResult->isProblemSatisfiable(), TBools::TRUE);
  // EXPECT_TRUE(isTrue(solvingResult.model->check(rule110Encoding.cnfProblem)));
}

TEST(DriversIntegration, CDCLSatSolver_rule110_unreachable)
{
  Rule110PredecessorStateProblem problem{"1x1x1", "01010", 7};
  auto rule110Encoding = problem.getCNFEncoding();

  std::unique_ptr<CDCLSatSolver> underTest = createCDCLSatSolver();

  for (auto& clause : rule110Encoding.cnfProblem.getClauses()) {
    underTest->addClause(clause);
  }
  ASSERT_EQ(isSatisfiableViaMinisat(rule110Encoding.cnfProblem), TBools::FALSE)
      << "Bad test case: the problem is expected not to be satisfiable";
  EXPECT_EQ(underTest->solve({})->isProblemSatisfiable(), TBools::FALSE);
}

TEST(DriversIntegration, CDCLSatSolver_rule110_incremental)
{
  Rule110PredecessorStateProblem problem{"xxxxxxxx", "11010111", 6};
  auto rule110Encoding = problem.getCNFEncoding();

  std::unique_ptr<CDCLSatSolver> underTest = createCDCLSatSolver();
  underTest->addProblem(rule110Encoding.cnfProblem);

  auto& inputs = rule110Encoding.freeInputs;
  ASSERT_EQ(inputs.size(), 8ULL);

  // Should be satistiable with input "xxxxxxx1":
  auto result = underTest->solve({inputs[7]});
  EXPECT_EQ(result->isProblemSatisfiable(), TBools::TRUE);

  // Should not be satisfiable with input "1x1x1x11":
  result = underTest->solve({inputs[0], inputs[2], inputs[4], inputs[6], inputs[7]});
  EXPECT_EQ(result->isProblemSatisfiable(), TBools::FALSE);

  // Should be satistiable with input "xxxxxxxx":
  result = underTest->solve({});
  EXPECT_EQ(result->isProblemSatisfiable(), TBools::TRUE);

  // Should be satistiable with input "00000001":
  result = underTest->solve({~inputs[0],
                             ~inputs[1],
                             ~inputs[2],
                             ~inputs[3],
                             ~inputs[4],
                             ~inputs[5],
                             ~inputs[6],
                             inputs[7]});
  EXPECT_EQ(result->isProblemSatisfiable(), TBools::TRUE);
}
}
