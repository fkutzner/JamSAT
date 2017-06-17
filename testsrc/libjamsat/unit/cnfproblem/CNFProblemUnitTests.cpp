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
#include <libjamsat/cnfproblem/CNFProblem.h>

namespace jamsat {
TEST(UnitCNFProblem, emptyCNFProblemHasSize0) {
  CNFProblem underTest;
  ASSERT_EQ(underTest.getSize(), 0ull);
}

TEST(UnitCNFProblem, emptyCNFProblemIsMarkedEmpty) {
  CNFProblem underTest;
  ASSERT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, emptyCNFProblemMaxVarIsUndefined) {
  CNFProblem underTest;
  ASSERT_EQ(underTest.getMaxVar(), CNFVar::undefinedVariable);
}

TEST(UnitCNFProblem, emptyCNFProblemHasNoClauses) {
  CNFProblem underTest;
  ASSERT_TRUE(underTest.getClauses().empty());
}

TEST(UnitCNFProblem, addedClauseCanBeRetrieved) {
  std::vector<CNFLit> clause = {CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                                CNFLit{CNFVar{3}, CNFSign::NEGATIVE}};

  CNFProblem underTest;
  underTest.addClause(clause);
  ASSERT_EQ(underTest.getSize(), 1ull);
  ASSERT_EQ(underTest.getClauses()[0], clause);
}

TEST(UnitCNFProblem, cnfProblemWithTwoClausesReportsSize) {
  std::vector<CNFLit> clause1 = {CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                                 CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};

  std::vector<CNFLit> clause2 = {CNFLit{CNFVar{5}, CNFSign::NEGATIVE},
                                 CNFLit{CNFVar{6}, CNFSign::POSITIVE}};

  CNFProblem underTest;
  underTest.addClause(clause1);
  underTest.addClause(clause2);
  ASSERT_EQ(underTest.getSize(), 2ull);
  ASSERT_FALSE(underTest.isEmpty());
}

TEST(UnitCNFProblem, cnfProblemOrderIsPreserved) {
  std::vector<CNFLit> clause1 = {CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                                 CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};

  std::vector<CNFLit> clause2 = {CNFLit{CNFVar{5}, CNFSign::NEGATIVE},
                                 CNFLit{CNFVar{6}, CNFSign::POSITIVE}};

  CNFProblem underTest;
  underTest.addClause(clause1);
  underTest.addClause(clause2);
  ASSERT_EQ(underTest.getSize(), 2ull);
  EXPECT_EQ(underTest.getClauses()[0], clause1);
  EXPECT_EQ(underTest.getClauses()[1], clause2);
}

TEST(UnitCNFProblem, cnfProblemReportsMaximumVariable) {
  std::vector<CNFLit> clause1 = {CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                                 CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};

  std::vector<CNFLit> clause2 = {CNFLit{CNFVar{5}, CNFSign::NEGATIVE},
                                 CNFLit{CNFVar{6}, CNFSign::POSITIVE}};

  CNFProblem underTest;
  underTest.addClause(clause1);
  underTest.addClause(clause2);
  EXPECT_EQ(underTest.getMaxVar(), CNFVar{6});
}

TEST(UnitCNFProblem, parseEmptyDIMACSInputData) {
  std::string testData = " ";
  std::stringstream conduit{testData};
  CNFProblem underTest;
  conduit >> underTest;
  EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseCommentOnlyDIMACSInputData) {
  std::stringstream conduit;
  conduit << "c Foo" << std::endl;
  conduit << "c" << std::endl;

  CNFProblem underTest;
  conduit >> underTest;
  EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseSingleClauseDIMACSInputData) {
  std::stringstream conduit;
  conduit << "p cnf 5 1" << std::endl;
  conduit << "1 2 -3 4 -5 0" << std::endl;

  CNFProblem underTest;
  conduit >> underTest;
  ASSERT_EQ(underTest.getSize(), 1ull);

  CNFClause expected = {
      CNFLit{CNFVar{0}, CNFSign::POSITIVE},
      CNFLit{CNFVar{1}, CNFSign::POSITIVE},
      CNFLit{CNFVar{2}, CNFSign::NEGATIVE},
      CNFLit{CNFVar{3}, CNFSign::POSITIVE},
      CNFLit{CNFVar{4}, CNFSign::NEGATIVE},
  };

  EXPECT_EQ(underTest.getClauses()[0], expected);
  EXPECT_EQ(underTest.getMaxVar(), CNFVar{4});
}
}
