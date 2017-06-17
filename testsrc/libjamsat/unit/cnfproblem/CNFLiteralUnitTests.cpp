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
TEST(UnitCNFProblem, invertSign) {
  CNFSign positiveSign = CNFSign::POSITIVE;
  EXPECT_EQ(invert(positiveSign), CNFSign::NEGATIVE);
}

TEST(UnitCNFProblem, negateLiteral) {
  CNFVar variable{5};
  CNFLit underTest{variable, CNFSign::POSITIVE};
  CNFLit negated = ~underTest;

  EXPECT_EQ(negated.getSign(), CNFSign::NEGATIVE);
  EXPECT_EQ(negated.getVariable(), variable);
}

TEST(UnitCNFProblem, literalEquivalency) {
  CNFLit underTest{CNFVar{4}, CNFSign::POSITIVE};
  CNFLit inequalToUnderTestByVar{CNFVar{5}, CNFSign::POSITIVE};
  CNFLit inequalToUnderTestBySign{CNFVar{5}, CNFSign::POSITIVE};
  CNFLit copyOfUnderTest = underTest;

  EXPECT_NE(underTest, inequalToUnderTestByVar);
  EXPECT_NE(underTest, inequalToUnderTestBySign);
  EXPECT_EQ(underTest, copyOfUnderTest);
}

TEST(UnitCNFProblem, printVariable) {
  CNFVar underTest{5};
  std::stringstream target;
  target << underTest;

  std::string printedVariable;
  target >> printedVariable;
  EXPECT_EQ(printedVariable, "5");
}

TEST(UnitCNFProblem, printNegativeLiteral) {
  CNFLit underTest{CNFVar{5}, CNFSign::NEGATIVE};
  std::stringstream target;
  target << underTest;

  std::string printedVariable;
  target >> printedVariable;
  EXPECT_EQ(printedVariable, "-5");
}

TEST(UnitCNFProblem, printPositiveLiteral) {
  CNFLit underTest{CNFVar{5}, CNFSign::POSITIVE};
  std::stringstream target;
  target << underTest;

  std::string printedVariable;
  target >> printedVariable;
  EXPECT_EQ(printedVariable, "+5");
}

#if !defined(NDEBUG)
TEST(UnitCNFProblem, cannotNegateUndefinedLiteral) {
  CNFLit underTest = CNFLit::undefinedLiteral;
  ASSERT_DEATH(~underTest, ".*");
}
#endif
}
