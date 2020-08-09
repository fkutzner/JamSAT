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

namespace jamsat {
TEST(UnitCNFProblem, CNFVarHasStrictWeakOrdering)
{
  CNFVar varA{5};
  CNFVar varB{6};
  CNFVar varC{5};

  EXPECT_TRUE(varA == varC);
  EXPECT_FALSE(varA < varC);
  EXPECT_FALSE(varA > varC);

  EXPECT_TRUE(varA != varB);
  EXPECT_TRUE(varA < varB);
  EXPECT_TRUE(varB > varA);

  EXPECT_TRUE(varA == varA);
}

TEST(UnitCNFProblem, undefinedCNFVarGreaterThanAllOtherVars)
{
  CNFVar max{CNFVar::getMaxRawValue()};
  EXPECT_TRUE(max < CNFVar::getUndefinedVariable());
}

TEST(UnitCNFProblem, nextCNFVarOfSmallVarIsDefined)
{
  CNFVar var{10};
  CNFVar next = nextCNFVar(var);
  EXPECT_EQ(next.getRawValue(), CNFVar::RawVariable{11});
}

TEST(UnitCNFProblem, nextCNFVarOfMaxVarIsUndefined)
{
  CNFVar var{CNFVar::getMaxRawValue()};
  CNFVar next = nextCNFVar(var);
  EXPECT_EQ(next, CNFVar::getUndefinedVariable());
}

TEST(UnitCNFProblem, undefinedVariableIsNotRegular)
{
  EXPECT_FALSE(isRegular(CNFVar::getUndefinedVariable()));
}

TEST(UnitCNFProblem, variableWithinRegularRangeIsRegular)
{
  EXPECT_TRUE(isRegular(CNFVar{CNFVar::getMaxRawValue()}));
  EXPECT_TRUE(isRegular(CNFVar{0}));
}

TEST(UnitCNFProblem, invertSign)
{
  CNFSign positiveSign = CNFSign::POSITIVE;
  EXPECT_EQ(invert(positiveSign), CNFSign::NEGATIVE);
}

TEST(UnitCNFProblem, CNFLitHasStrictWeakOrdering)
{
  CNFLit litA{CNFVar{5}, CNFSign::NEGATIVE};
  CNFLit litB{CNFVar{6}, CNFSign::POSITIVE};
  CNFLit litC{CNFVar{5}, CNFSign::NEGATIVE};
  CNFLit litD{CNFVar{5}, CNFSign::POSITIVE};

  EXPECT_TRUE(litA == litC);
  EXPECT_FALSE(litA < litC);
  EXPECT_FALSE(litA > litC);

  EXPECT_TRUE(litA != litB);
  EXPECT_TRUE(litA < litB);
  EXPECT_TRUE(litB > litA);

  EXPECT_TRUE(litA == litA);
  EXPECT_TRUE(litA < litD);
}

TEST(UnitCNFProblem, undefinedCNFLitGreaterThanAllOtherLits)
{
  CNFLit max{CNFVar{CNFVar::getMaxRawValue()}, CNFSign::POSITIVE};
  EXPECT_TRUE(max < CNFLit::getUndefinedLiteral());
}

TEST(UnitCNFProblem, negateLiteral)
{
  CNFVar variable{5};
  CNFLit underTest{variable, CNFSign::POSITIVE};
  CNFLit negated = ~underTest;

  EXPECT_EQ(negated.getSign(), CNFSign::NEGATIVE);
  EXPECT_EQ(negated.getVariable(), variable);
}

TEST(UnitCNFProblem, literalEquivalency)
{
  CNFLit underTest{CNFVar{4}, CNFSign::POSITIVE};
  CNFLit inequalToUnderTestByVar{CNFVar{5}, CNFSign::POSITIVE};
  CNFLit inequalToUnderTestBySign{CNFVar{5}, CNFSign::POSITIVE};
  CNFLit copyOfUnderTest = underTest;

  EXPECT_NE(underTest, inequalToUnderTestByVar);
  EXPECT_NE(underTest, inequalToUnderTestBySign);
  EXPECT_EQ(underTest, copyOfUnderTest);
}

TEST(UnitCNFProblem, printVariable)
{
  CNFVar underTest{5};
  std::stringstream target;
  target << underTest;

  std::string printedVariable;
  target >> printedVariable;
  EXPECT_EQ(printedVariable, "6");
}

TEST(UnitCNFProblem, printNegativeLiteral)
{
  CNFLit underTest{CNFVar{5}, CNFSign::NEGATIVE};
  std::stringstream target;
  target << underTest;

  std::string printedVariable;
  target >> printedVariable;
  EXPECT_EQ(printedVariable, "-6");
}

TEST(UnitCNFProblem, printPositiveLiteral)
{
  CNFLit underTest{CNFVar{5}, CNFSign::POSITIVE};
  std::stringstream target;
  target << underTest;

  std::string printedVariable;
  target >> printedVariable;
  EXPECT_EQ(printedVariable, "6");
}

TEST(UnitCNFProblem, variableOfUndefinedLiteralIsUndefiend)
{
  CNFVar undefinedLiteralVar = CNFLit::getUndefinedLiteral().getVariable();
  EXPECT_EQ(undefinedLiteralVar, CNFVar::getUndefinedVariable());
}

TEST(UnitCNFProblem, maxLitForVarIsGreaterThanNegate)
{
  CNFVar testInput{4};
  CNFLit maxLit = getMaxLit(testInput);
  EXPECT_GT(maxLit, ~maxLit);
}

#if !defined(NDEBUG)
TEST(UnitCNFProblem, cannotNegateUndefinedLiteral)
{
  CNFLit underTest = CNFLit::getUndefinedLiteral();
  ASSERT_DEATH(~underTest, ".*");
}
#endif
}
