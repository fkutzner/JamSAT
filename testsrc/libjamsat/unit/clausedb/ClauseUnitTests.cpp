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
#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>

namespace jamsat {

TEST(UnitClauseDB, allocateClauseOnHeap) {
    auto allocatedClause = createHeapClause(11);
    ASSERT_NE(allocatedClause.get(), nullptr);
    EXPECT_EQ(allocatedClause->size(), 11ull);
}

TEST(UnitClauseDB, nonemptyHeapClausesHaveSufficientMemory) {
    auto allocatedClause = createHeapClause(11);
    ASSERT_NE(allocatedClause.get(), nullptr);

    auto addrJustBeyondClause = reinterpret_cast<uintptr_t>(allocatedClause->end());
    auto addrBeginClause = reinterpret_cast<uintptr_t>(allocatedClause.get());

    auto computedSize = Clause::getAllocationSize(11);
    EXPECT_EQ(computedSize, addrJustBeyondClause - addrBeginClause);
}

TEST(UnitClauseDB, singleLitHeapClausesHaveSufficientMemory) {
    auto allocatedClause = createHeapClause(1);
    ASSERT_NE(allocatedClause.get(), nullptr);

    auto addrJustBeyondClause = reinterpret_cast<uintptr_t>(allocatedClause->end());
    auto addrBeginClause = reinterpret_cast<uintptr_t>(allocatedClause.get());

    auto computedSize = Clause::getAllocationSize(1);
    EXPECT_EQ(computedSize, addrJustBeyondClause - addrBeginClause);
}

TEST(UnitClauseDB, emptyHeapClausesHaveSufficientMemory) {
    auto allocatedClause = createHeapClause(0);
    ASSERT_NE(allocatedClause.get(), nullptr);

    auto addrJustBeyondClause = reinterpret_cast<uintptr_t>(allocatedClause->end());
    auto addrBeginClause = reinterpret_cast<uintptr_t>(allocatedClause.get());

    // The struct is trailed by a CNFLit whose memory does not get accessed in
    // size-0 clauses.
    EXPECT_LE(sizeof(Clause), addrJustBeyondClause - addrBeginClause + sizeof(CNFLit));
}

TEST(UnitClauseDB, freshHeapClauseContainsUndefinedLiterals) {
    auto underTest = createHeapClause(11);
    ASSERT_NE(underTest.get(), nullptr);
    for (Clause::size_type i = 0; i < underTest->size(); ++i) {
        EXPECT_EQ((*underTest)[i], CNFLit::getUndefinedLiteral());
    }
}

TEST(UnitClauseDB, heapClauseIsWritable) {
    auto underTest = createHeapClause(11);
    ASSERT_NE(underTest.get(), nullptr);
    CNFLit testLiteral{CNFVar{3}, CNFSign::NEGATIVE};
    (*underTest)[3] = testLiteral;
    EXPECT_EQ((*underTest)[3], testLiteral);
}

TEST(UnitClauseDB, iterateOverEmptyClause) {
    auto underTest = createHeapClause(0);
    ASSERT_NE(underTest.get(), nullptr);
    bool iterated = false;
    for (auto &lit : *underTest) {
        (void)lit;
        iterated = true;
    }
    ASSERT_FALSE(iterated);
}

namespace {
void test_iterateOverClause_setup(Clause &underTest) {
    CNFLit testLiteral1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};
    underTest[0] = CNFLit::getUndefinedLiteral();
    underTest[3] = testLiteral1;
    underTest[4] = testLiteral2;
}

template <typename C>
void test_iterateOverClause_check(C &underTest) {
    CNFLit testLiteral1{CNFVar{1}, CNFSign::NEGATIVE};
    CNFLit testLiteral2{CNFVar{2}, CNFSign::NEGATIVE};

    EXPECT_EQ(underTest[0], CNFLit::getUndefinedLiteral());

    int i = 0;
    for (auto &lit : underTest) {
        if (i == 3) {
            EXPECT_EQ(lit, testLiteral1);
        } else if (i == 4) {
            EXPECT_EQ(lit, testLiteral2);
        }
        ++i;
    }
}
}

TEST(UnitClauseDB, iterateOverClause) {
    auto underTest = createHeapClause(11);
    ASSERT_NE(underTest.get(), nullptr);
    test_iterateOverClause_setup(*underTest);
    test_iterateOverClause_check(*underTest);
}

TEST(UnitClauseDB, iterateOverConstantClause) {
    auto underTest = createHeapClause(8);
    test_iterateOverClause_setup(*underTest);
    const Clause &underTestConst = *underTest;
    test_iterateOverClause_check(underTestConst);
}

TEST(UnitClauseDB, shrinkClause) {
    auto underTest = createHeapClause(11);
    ASSERT_NE(underTest.get(), nullptr);
    ASSERT_EQ(underTest->end() - underTest->begin(), 11);
    ASSERT_EQ(underTest->size(), 11ull);

    underTest->resize(5);
    EXPECT_EQ(underTest->end() - underTest->begin(), 5);
    EXPECT_EQ(underTest->size(), 5ull);
}

TEST(UnitClauseDB, assignClause) {
    auto assignee = createHeapClause(3);
    auto source = createHeapClause(3);

    source->setLBD<int>(10);
    (*source)[0] = CNFLit{CNFVar{100}, CNFSign::POSITIVE};
    (*source)[1] = CNFLit{CNFVar{10}, CNFSign::NEGATIVE};
    (*source)[2] = CNFLit{CNFVar{1000}, CNFSign::POSITIVE};

    *assignee = *source;

    EXPECT_EQ(assignee->getLBD<int>(), 10);
    EXPECT_TRUE(std::equal(assignee->begin(), assignee->end(), source->begin(), source->end()));
}

TEST(UnitClauseDB, tooLargeLBDValueIsCapped) {
    uint64_t largeLBDValue = std::numeric_limits<Clause::lbd_type>::max();
    ++largeLBDValue;

    auto underTest = createHeapClause(1);
    underTest->setLBD(largeLBDValue);

    EXPECT_EQ(underTest->getLBD<uint64_t>(), std::numeric_limits<Clause::lbd_type>::max());
}

TEST(UnitClauseDB, notTooLargeLBDValueIsStored) {
    uint64_t smallLBDValue = 10;

    auto underTest = createHeapClause(1);
    underTest->setLBD(smallLBDValue);

    EXPECT_EQ(underTest->getLBD<uint64_t>(), smallLBDValue);
}

TEST(UnitClauseDB, clauseIsEqualToSelf) {
    auto underTest = createHeapClause(2);
    (*underTest)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*underTest)[1] = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
    EXPECT_TRUE(*underTest == *underTest);
    EXPECT_FALSE(*underTest != *underTest);
}

TEST(UnitClauseDB, clauseIsEqualToEqualClause) {
    auto underTest = createHeapClause(2);
    auto otherClause = createHeapClause(2);

    underTest->setLBD(1);
    otherClause->setLBD(1);
    (*underTest)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*underTest)[1] = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
    (*otherClause)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*otherClause)[1] = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};

    EXPECT_TRUE(*underTest == *otherClause);
    EXPECT_FALSE(*underTest != *otherClause);
}

TEST(UnitClauseDB, clauseIsNotEqualToClauseOfDifferentSize) {
    auto underTest = createHeapClause(2);
    auto otherClause = createHeapClause(1);

    underTest->setLBD(1);
    otherClause->setLBD(1);
    (*underTest)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*underTest)[1] = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
    (*otherClause)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};

    EXPECT_FALSE(*underTest == *otherClause);
    EXPECT_TRUE(*underTest != *otherClause);
}

TEST(UnitClauseDB, clauseIsNotEqualToClauseOfDifferentLBD) {
    auto underTest = createHeapClause(2);
    auto otherClause = createHeapClause(2);

    underTest->setLBD(1);
    otherClause->setLBD(3);
    (*underTest)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*underTest)[1] = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
    (*otherClause)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*otherClause)[1] = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};

    EXPECT_FALSE(*underTest == *otherClause);
    EXPECT_TRUE(*underTest != *otherClause);
}

TEST(UnitClauseDB, clauseIsNotEqualToClauseWithDifferentLiterals) {
    auto underTest = createHeapClause(2);
    auto otherClause = createHeapClause(2);

    underTest->setLBD(1);
    otherClause->setLBD(1);
    (*underTest)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*underTest)[1] = CNFLit{CNFVar{2}, CNFSign::NEGATIVE};
    (*otherClause)[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*otherClause)[1] = CNFLit{CNFVar{1}, CNFSign::POSITIVE};

    EXPECT_FALSE(*underTest == *otherClause);
    EXPECT_TRUE(*underTest != *otherClause);
}
}
