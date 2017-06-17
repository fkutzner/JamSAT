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
}
