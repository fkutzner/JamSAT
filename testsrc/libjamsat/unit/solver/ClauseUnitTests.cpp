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
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/Clause.h>

namespace jamsat {
TEST(UnitSolver, allocateClauseOnHeap) {
  auto allocatedClause = createHeapClause(11);
  ASSERT_NE(allocatedClause.get(), nullptr);
  EXPECT_EQ(allocatedClause->getSize(), 11ull);
}

TEST(UnitSolver, freshHeapClauseContainsUndefinedLiterals) {
  auto underTest = createHeapClause(11);
  ASSERT_NE(underTest.get(), nullptr);
  for (Clause::size_type i = 0; i < underTest->getSize(); ++i) {
    EXPECT_EQ((*underTest)[i], CNFLit::undefinedLiteral);
  }
}

TEST(UnitSolver, heapClauseIsWritable) {
  auto underTest = createHeapClause(11);
  ASSERT_NE(underTest.get(), nullptr);
  CNFLit testLiteral{CNFVar{3}, CNFSign::NEGATIVE};
  (*underTest)[3] = testLiteral;
  EXPECT_EQ((*underTest)[3], testLiteral);
}

TEST(UnitSolver, iterateOverEmptyClause) {
  auto underTest = createHeapClause(0);
  ASSERT_NE(underTest.get(), nullptr);
  bool iterated = false;
  for (auto &lit : *underTest) {
    (void)lit;
    iterated = true;
  }
  ASSERT_FALSE(iterated);
}

TEST(UnitSolver, iterateOverClause) {
  auto underTest = createHeapClause(11);
  ASSERT_NE(underTest.get(), nullptr);
  CNFLit testLiteral1{CNFVar{1}, CNFSign::NEGATIVE};
  CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};
  (*underTest)[3] = testLiteral1;
  (*underTest)[4] = testLiteral2;

  int i = 0;
  for (auto &lit : *underTest) {
    if (i == 3) {
      EXPECT_EQ(lit, testLiteral1);
    } else if (i == 4) {
      EXPECT_EQ(lit, testLiteral2);
    }
  }
}

TEST(UnitSolver, shrinkClause) {
  auto underTest = createHeapClause(11);
  ASSERT_NE(underTest.get(), nullptr);
  ASSERT_EQ(underTest->end() - underTest->begin(), 11);
  ASSERT_EQ(underTest->getSize(), 11ull);

  underTest->shrink(5);
  EXPECT_EQ(underTest->end() - underTest->begin(), 5);
  EXPECT_EQ(underTest->getSize(), 5ull);
}
}
