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

#include <libjamsat/clausedb/IterableClauseDB.h>

#include <cstdint>
#include <iostream>

namespace jamsat {

struct TestClause {
public:
    using size_type = std::size_t;

    static auto constructIn(void* targetMemory, size_type clauseSize) -> TestClause*;
    static auto getAllocationSize(size_type clauseSize) -> std::size_t;
    auto size() const noexcept -> std::size_t;
    auto initialSize() const noexcept -> std::size_t;

private:
    TestClause(size_type clauseSize);

    std::uint64_t m_dummy;
    size_type m_size;
};

TestClause::TestClause(size_type clauseSize) : m_dummy(0), m_size(clauseSize) {
    (void)m_dummy;
}

auto TestClause::constructIn(void* targetMemory, size_type clauseSize) -> TestClause* {
    return new (targetMemory) TestClause(clauseSize);
}

auto TestClause::getAllocationSize(size_type clauseSize) -> std::size_t {
    return 16 + 4 * clauseSize;
}

auto TestClause::size() const noexcept -> std::size_t {
    return m_size;
}

auto TestClause::initialSize() const noexcept -> std::size_t {
    return m_size;
}


TEST(UnitClauseDB, IterableClauseDB_allocateClauseInRegion) {
    std::size_t const regionSize = 1024;
    Region<TestClause> underTest{regionSize};
    TestClause* c = underTest.allocate(2);
    EXPECT_EQ(c->size(), 2ull);
}

TEST(UnitClauseDB, IterableClauseDB_sizesAreUpdatedAfterAllocationInRegion) {
    std::size_t const regionSize = 1024;
    Region<TestClause> underTest{regionSize};
    ASSERT_EQ(underTest.getFreeSize(), 1024);
    ASSERT_EQ(underTest.getUsedSize(), 0);

    underTest.allocate(2);

    EXPECT_GE(underTest.getUsedSize(), TestClause::getAllocationSize(2));
    EXPECT_EQ(underTest.getUsedSize() + underTest.getFreeSize(), regionSize);
}

TEST(UnitClauseDB, IterableClauseDB_allocationsInRegionDontOverlap) {
    std::size_t const regionSize = 1024;
    Region<TestClause> underTest{regionSize};
    TestClause* c1 = underTest.allocate(2);
    TestClause* c2 = underTest.allocate(5);
    EXPECT_GE(underTest.getUsedSize(),
              TestClause::getAllocationSize(2) + TestClause::getAllocationSize(5));

    EXPECT_EQ(c1->size(), 2ull);
    EXPECT_EQ(c2->size(), 5ull);
    auto c1AsInt = reinterpret_cast<std::uintptr_t>(c1);
    auto c2AsInt = reinterpret_cast<std::uintptr_t>(c2);
    EXPECT_GE(c2AsInt, c1AsInt + TestClause::getAllocationSize(c1->size()));
}

TEST(UnitClauseDB, IterableClauseDB_allocationFailsForFullRegion) {
    std::size_t const regionSize = 128;
    Region<TestClause> underTest{regionSize};
    TestClause* c1 = underTest.allocate(10);
    EXPECT_NE(c1, nullptr);
    TestClause* c2 = underTest.allocate(64);
    EXPECT_EQ(c2, nullptr);
}

TEST(UnitClauseDB, IterableClauseDB_furtherAllocationInRegionPossibleAfterFailure) {
    std::size_t const regionSize = 128;
    Region<TestClause> underTest{regionSize};
    TestClause* c1 = underTest.allocate(10);
    ASSERT_NE(c1, nullptr);
    EXPECT_EQ(c1->size(), 10);
    TestClause* c2 = underTest.allocate(64);
    EXPECT_EQ(c2, nullptr);
    TestClause* c3 = underTest.allocate(11);
    ASSERT_NE(c3, nullptr);
    EXPECT_EQ(c3->size(), 11);
}

TEST(UnitClauseDB, IterableClauseDB_cloneEmptyRegionYieldsEmptyNewAllocator) {
    std::size_t const regionSize = 128;
    Region<TestClause> underTest{regionSize};

    auto cloningResult = underTest.clone();
    ASSERT_TRUE(cloningResult.has_value());
    Region<TestClause> clone = std::move(*cloningResult);

    EXPECT_EQ(underTest.getFreeSize(), regionSize);
    EXPECT_EQ(clone.getFreeSize(), regionSize);
}

TEST(UnitClauseDB, IterableClauseDB_allocationsInClonedRegionDoNotAffectOriginal) {
    std::size_t const regionSize = 128;
    Region<TestClause> underTest{regionSize};

    TestClause* orig1 = underTest.allocate(10);
    auto usedInOriginal = underTest.getUsedSize();

    auto cloningResult = underTest.clone();
    ASSERT_TRUE(cloningResult.has_value());
    Region<TestClause> clone = std::move(*cloningResult);

    TestClause* clone1 = clone.allocate(11);
    EXPECT_EQ(underTest.getUsedSize(), usedInOriginal);

    std::uintptr_t origRegionBegin = reinterpret_cast<std::uintptr_t>(orig1);
    std::uintptr_t clonedClauseLoc = reinterpret_cast<std::uintptr_t>(clone1);

    EXPECT_FALSE(clonedClauseLoc >= origRegionBegin &&
                 clonedClauseLoc < origRegionBegin + regionSize);
}

TEST(UnitClauseDB, IterableClauseDB_emptyRegionHasNoClauses) {
    std::size_t const regionSize = 1024;
    Region<TestClause> underTest{regionSize};

    auto result = underTest.getFirstClause();
    EXPECT_FALSE(result.has_value());
}

TEST(UnitClauseDB, IterableClauseDB_firstClauseCanBeRetrievedFromRegion) {
    std::size_t const regionSize = 1024;
    Region<TestClause> underTest{regionSize};

    TestClause* clause1 = underTest.allocate(10);
    underTest.allocate(5);
    auto result = underTest.getFirstClause();
    auto constResult = static_cast<Region<TestClause> const*>(&underTest)->getFirstClause();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, clause1);
    ASSERT_TRUE(constResult.has_value());
    EXPECT_EQ(*constResult, static_cast<TestClause const*>(clause1));
}

TEST(UnitClauseDB, IterableClauseDB_secondClauseCanBeRetrievedFromRegion) {
    std::size_t const regionSize = 1024;
    Region<TestClause> underTest{regionSize};

    TestClause* clause1 = underTest.allocate(10);
    TestClause* clause2 = underTest.allocate(5);

    auto result = underTest.getNextClause(clause1);
    auto constResult = static_cast<Region<TestClause> const*>(&underTest)->getNextClause(clause1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, clause2);
    ASSERT_TRUE(constResult.has_value());
    EXPECT_EQ(*constResult, static_cast<TestClause const*>(clause2));
}

TEST(UnitClauseDB, IterableClauseDB_regionIsIterable) {
    std::size_t const regionSize = 2048;
    Region<TestClause> underTest{regionSize};

    std::vector<TestClause*> clauses;
    for (int i = 0; i < 16; ++i) {
        clauses.push_back(underTest.allocate(10 + i));
    }

    std::vector<TestClause*> iterationResult;
    auto currentClause = underTest.getFirstClause();
    while (currentClause.has_value()) {
        iterationResult.push_back(*currentClause);
        currentClause = underTest.getNextClause(*currentClause);
    }

    EXPECT_EQ(clauses, iterationResult);
}


}