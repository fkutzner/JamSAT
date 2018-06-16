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

#include <boost/range/algorithm/equal.hpp>
#include <gtest/gtest.h>
#include <vector>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>

#include "HeapClauseDB.h"

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;

namespace {
template <class ClauseDB>
void clauseDBTest_dbIsEmptyAfterInitialization() {
    ClauseDB underTest;
    EXPECT_EQ(underTest.size(), 0ull);
}

template <class ClauseDB>
void clauseDBTest_createSingletonClause() {
    ClauseDB underTest;
    const std::vector<CNFLit> input{1_Lit};
    auto& result = underTest.insertClause(input);

    EXPECT_EQ(underTest.size(), 1ull);
    ASSERT_EQ(result.size(), 1ull);
    EXPECT_TRUE(boost::equal(input, result));
}

template <class ClauseDB>
void clauseDBTest_createTernaryClause() {
    ClauseDB underTest;
    const std::vector<CNFLit> input{1_Lit, 10_Lit, 100_Lit};
    auto& result = underTest.insertClause(input);

    EXPECT_EQ(underTest.size(), 1ull);
    ASSERT_EQ(result.size(), 3ull);
    EXPECT_TRUE(boost::equal(input, result));
}

template <class ClauseDB>
void clauseDBTest_createUndestroyableSingletonClause() {
    ClauseDB underTest;
    const std::vector<CNFLit> input{1_Lit};
    auto& result = underTest.insertUndestroyableClause(input);

    EXPECT_EQ(underTest.size(), 1ull);
    ASSERT_EQ(result.size(), 1ull);
    EXPECT_TRUE(boost::equal(input, result));
}

template <class ClauseDB>
void clauseDBTest_createUndestroyableTernaryClause() {
    ClauseDB underTest;
    const std::vector<CNFLit> input{1_Lit, 10_Lit, 100_Lit};
    auto& result = underTest.insertUndestroyableClause(input);

    EXPECT_EQ(underTest.size(), 1ull);
    ASSERT_EQ(result.size(), 3ull);
    EXPECT_TRUE(boost::equal(input, result));
}

template <class ClauseDB>
void clauseDBTest_destroyedClausesAreMarkedDestroyed() {
    ClauseDB underTest;
    const std::vector<CNFLit> input{10_Lit};
    auto& clause = underTest.insertClause(input);

    ASSERT_FALSE(underTest.isDestroyed(clause));
    underTest.destroy(clause);
    EXPECT_TRUE(underTest.isDestroyed(clause));
}

template <class ClauseDB>
void clauseDBTest_destroyedClausesVanishInDBPurge() {
    ClauseDB underTest;
    const std::vector<CNFLit> destroyClause{10_Lit};
    const std::vector<CNFLit> keepClause{11_Lit, 12_Lit};

    auto& insertedDestroyClause = underTest.insertClause(destroyClause);
    auto& insertedKeepClause = underTest.insertClause(keepClause);
    EXPECT_EQ(underTest.size(), 2ull);

    ASSERT_FALSE(underTest.isDestroyed(insertedDestroyClause));
    underTest.destroy(insertedDestroyClause);
    ASSERT_TRUE(underTest.isDestroyed(insertedDestroyClause));
    underTest.purgeDestroyedClauses();
    EXPECT_EQ(underTest.size(), 1ull);

    EXPECT_FALSE(underTest.contains(insertedDestroyClause));
    EXPECT_TRUE(underTest.contains(insertedKeepClause));
}
}

TEST(UnitClauseDB, HeapClauseDB_dbIsEmptyAfterInitialization_TrivialClause) {
    clauseDBTest_dbIsEmptyAfterInitialization<HeapClauseDB<TrivialClause>>();
}

TEST(UnitClauseDB, HeapClauseDB_dbIsEmptyAfterInitialization_Clause) {
    clauseDBTest_dbIsEmptyAfterInitialization<HeapClauseDB<Clause>>();
}

TEST(UnitClauseDB, HeapClauseDB_createSingletonClause_TrivialClause) {
    clauseDBTest_createSingletonClause<HeapClauseDB<TrivialClause>>();
}

TEST(UnitClauseDB, HeapClauseDB_createSingletonClause_Clause) {
    clauseDBTest_createSingletonClause<HeapClauseDB<Clause>>();
}

TEST(UnitClauseDB, HeapClauseDB_createTernaryClause_TrivialClause) {
    clauseDBTest_createTernaryClause<HeapClauseDB<TrivialClause>>();
}

TEST(UnitClauseDB, HeapClauseDB_createTernaryClause_Clause) {
    clauseDBTest_createTernaryClause<HeapClauseDB<Clause>>();
}

TEST(UnitClauseDB, HeapClauseDB_createUndestroyableSingletonClause_TrivialClause) {
    clauseDBTest_createUndestroyableSingletonClause<HeapClauseDB<TrivialClause>>();
}

TEST(UnitClauseDB, HeapClauseDB_createUndestroyableSingletonClause_Clause) {
    clauseDBTest_createUndestroyableSingletonClause<HeapClauseDB<Clause>>();
}

TEST(UnitClauseDB, HeapClauseDB_createUndestroyableTernaryClause_TrivialClause) {
    clauseDBTest_createUndestroyableTernaryClause<HeapClauseDB<TrivialClause>>();
}

TEST(UnitClauseDB, HeapClauseDB_createUndestroyableTernaryClause_Clause) {
    clauseDBTest_createUndestroyableTernaryClause<HeapClauseDB<Clause>>();
}

TEST(UnitClauseDB, HeapClauseDB_destroyedClausesAreMarkedDestroyed_TrivialClause) {
    clauseDBTest_destroyedClausesAreMarkedDestroyed<HeapClauseDB<TrivialClause>>();
}

TEST(UnitClauseDB, HeapClauseDB_destroyedClausesAreMarkedDestroyed_Clause) {
    clauseDBTest_destroyedClausesAreMarkedDestroyed<HeapClauseDB<Clause>>();
}

TEST(UnitClauseDB, HeapClauseDB_destroyedClausesVanishInDBPurge_TrivialClause) {
    clauseDBTest_destroyedClausesVanishInDBPurge<HeapClauseDB<TrivialClause>>();
}

TEST(UnitClauseDB, HeapClauseDB_destroyedClausesVanishInDBPurge_Clause) {
    clauseDBTest_destroyedClausesVanishInDBPurge<HeapClauseDB<Clause>>();
}
}
