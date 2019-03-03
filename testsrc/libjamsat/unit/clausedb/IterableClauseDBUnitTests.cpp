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
    Region<TestClause> underTest{1024};
    TestClause* c = underTest.allocate(2);
    EXPECT_EQ(c->size(), 2ull);
}

TEST(UnitClauseDB, IterableClauseDB_sizesAreUpdatedAfterAllocationInRegion) {
    Region<TestClause> underTest{1024};
    ASSERT_EQ(underTest.getFreeSize(), 1024);
    ASSERT_EQ(underTest.getUsedSize(), 0);

    underTest.allocate(2);

    EXPECT_GE(underTest.getUsedSize(), TestClause::getAllocationSize(2));
    EXPECT_EQ(underTest.getUsedSize() + underTest.getFreeSize(), 1024ull);
}

TEST(UnitClauseDB, IterableClauseDB_allocationsInRegionDontOverlap) {
    Region<TestClause> underTest{1024};
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
    Region<TestClause> underTest{128};
    TestClause* c1 = underTest.allocate(10);
    EXPECT_NE(c1, nullptr);
    TestClause* c2 = underTest.allocate(64);
    EXPECT_EQ(c2, nullptr);
}

TEST(UnitClauseDB, IterableClauseDB_furtherAllocationInRegionPossibleAfterFailure) {
    Region<TestClause> underTest{128};
    TestClause* c1 = underTest.allocate(10);
    ASSERT_NE(c1, nullptr);
    EXPECT_EQ(c1->size(), 10);
    TestClause* c2 = underTest.allocate(64);
    EXPECT_EQ(c2, nullptr);
    TestClause* c3 = underTest.allocate(11);
    ASSERT_NE(c3, nullptr);
    EXPECT_EQ(c3->size(), 11);
}

}