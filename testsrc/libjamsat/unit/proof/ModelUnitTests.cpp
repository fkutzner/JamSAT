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

#include <sstream>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/proof/Model.h>

namespace jamsat {
TEST(UnitProof, Model_valuesAreIndeterminateByDefault)
{
  auto underTest = createModel(CNFVar{7});
  for (CNFVar i = CNFVar{0}; i <= CNFVar{7}; i = nextCNFVar(i)) {
    EXPECT_EQ(underTest->getAssignment(i), TBools::INDETERMINATE)
        << "Variable " << i << " not assigned INDETERMINATE";
  }
}

TEST(UnitProof, Model_storesValues)
{
  auto underTest = createModel(CNFVar{7});
  underTest->setAssignment(CNFVar{4}, TBools::FALSE);
  underTest->setAssignment(CNFVar{5}, TBools::TRUE);
  EXPECT_EQ(underTest->getAssignment(CNFVar{4}), TBools::FALSE);
  EXPECT_EQ(underTest->getAssignment(CNFVar{5}), TBools::TRUE);
}

TEST(UnitProof, Model_valuesCanBeOverridden)
{
  auto underTest = createModel(CNFVar{7});
  underTest->setAssignment(CNFVar{4}, TBools::FALSE);
  underTest->setAssignment(CNFVar{4}, TBools::TRUE);
  EXPECT_EQ(underTest->getAssignment(CNFVar{4}), TBools::TRUE);
}

TEST(UnitProof, Model_variablesHigherThanMaxAreIndeterminate)
{
  auto underTest = createModel(CNFVar{7});
  EXPECT_EQ(underTest->getAssignment(CNFVar{14}), TBools::INDETERMINATE);
}

TEST(UnitProof, Model_sizeIsAutomaticallyIncreased)
{
  auto underTest = createModel(CNFVar{7});
  underTest->setAssignment(CNFVar{14}, TBools::TRUE);
  EXPECT_EQ(underTest->getAssignment(CNFVar{14}), TBools::TRUE);
}

TEST(UnitProof, Model_checkForEmptyProblemSucceeds)
{
  auto underTest = createModel(CNFVar{10});
  CNFProblem empty;
  ASSERT_EQ(underTest->check(empty), TBools::TRUE);
}

namespace {
CNFProblem createModelTestCNFProblem()
{
  std::stringstream conduit;
  conduit << "p cnf 100 4" << std::endl;
  conduit << "5 1 -3 -4 0" << std::endl;
  conduit << "1 -4 2 100 0" << std::endl;
  conduit << "4 0" << std::endl;
  conduit << "-1 0" << std::endl;
  CNFProblem testData;
  conduit >> testData;
  return testData;
}
}

TEST(UnitProof, Model_checkForSatisfyingAssignmentSucceeds)
{
  CNFProblem testData = createModelTestCNFProblem();
  auto underTest = createModel(CNFVar{10});
  underTest->setAssignment(CNFVar{0}, TBools::FALSE);
  underTest->setAssignment(CNFVar{1}, TBools::TRUE);
  underTest->setAssignment(CNFVar{2}, TBools::FALSE);
  underTest->setAssignment(CNFVar{3}, TBools::TRUE);
  underTest->setAssignment(CNFVar{4}, TBools::INDETERMINATE);

  ASSERT_EQ(underTest->check(testData), TBools::TRUE);
}

TEST(UnitProof, Model_checkForCompletelyIndeterminateAssignmentFails)
{
  CNFProblem testData = createModelTestCNFProblem();
  auto underTest = createModel(CNFVar{10});
  underTest->setAssignment(CNFVar{0}, TBools::INDETERMINATE);
  underTest->setAssignment(CNFVar{1}, TBools::INDETERMINATE);
  underTest->setAssignment(CNFVar{2}, TBools::INDETERMINATE);
  underTest->setAssignment(CNFVar{3}, TBools::INDETERMINATE);
  underTest->setAssignment(CNFVar{4}, TBools::INDETERMINATE);

  ASSERT_EQ(underTest->check(testData), TBools::FALSE);
}

TEST(UnitProof, Model_checkForNonsatisfyingAssignmentFails)
{
  CNFProblem testData = createModelTestCNFProblem();
  auto underTest = createModel(CNFVar{10});
  underTest->setAssignment(CNFVar{0}, TBools::FALSE);
  underTest->setAssignment(CNFVar{1}, TBools::FALSE);
  underTest->setAssignment(CNFVar{2}, TBools::FALSE);
  underTest->setAssignment(CNFVar{3}, TBools::TRUE);
  underTest->setAssignment(CNFVar{4}, TBools::TRUE);

  ASSERT_EQ(underTest->check(testData), TBools::FALSE);
}
}
