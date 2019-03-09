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

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include <cstdint>
#include <vector>

namespace jamsat {

struct TestClause {
public:
    using size_type = std::size_t;

    enum class Flag { SCHEDULED_FOR_DELETION };

    static auto constructIn(void* targetMemory, size_type clauseSize) -> TestClause*;
    static auto getAllocationSize(size_type clauseSize) -> std::size_t;
    auto size() const noexcept -> std::size_t;
    auto initialSize() const noexcept -> std::size_t;

    void setFlag(Flag f) noexcept;
    auto getFlag(Flag f) const noexcept -> bool;

    void setDestroyedFlag(char* flag) noexcept;

    auto operator==(TestClause const& rhs) const noexcept -> bool;
    auto operator!=(TestClause const& rhs) const noexcept -> bool;

    ~TestClause();

private:
    TestClause(size_type clauseSize);

    std::uint64_t m_dummy;
    size_type m_size;
    char* m_destroyedFlag;
    bool m_isScheduledForDeletion;
};

TestClause::TestClause(size_type clauseSize)
  : m_dummy(0), m_size(clauseSize), m_destroyedFlag(nullptr), m_isScheduledForDeletion(false) {
    (void)m_dummy;
}

TestClause::~TestClause() {
    if (m_destroyedFlag) {
        *m_destroyedFlag = 1;
    }
}

auto TestClause::constructIn(void* targetMemory, size_type clauseSize) -> TestClause* {
    return new (targetMemory) TestClause(clauseSize);
}

auto TestClause::getAllocationSize(size_type clauseSize) -> std::size_t {
    return sizeof(TestClause) + 4 * clauseSize;
}

auto TestClause::size() const noexcept -> std::size_t {
    return m_size;
}

auto TestClause::initialSize() const noexcept -> std::size_t {
    return m_size;
}

void TestClause::setDestroyedFlag(char* flag) noexcept {
    m_destroyedFlag = flag;
}

void TestClause::setFlag(Flag f) noexcept {
    m_isScheduledForDeletion = (f == Flag::SCHEDULED_FOR_DELETION);
}

auto TestClause::getFlag(Flag f) const noexcept -> bool {
    if (f == Flag::SCHEDULED_FOR_DELETION) {
        return m_isScheduledForDeletion;
    }
    return false;
}

auto TestClause::operator==(TestClause const& rhs) const noexcept -> bool {
    if (this == &rhs) {
        return true;
    }
    return this->m_destroyedFlag == rhs.m_destroyedFlag && this->m_dummy == rhs.m_dummy &&
           this->m_isScheduledForDeletion == rhs.m_isScheduledForDeletion &&
           this->m_size == rhs.m_size;
}

auto TestClause::operator!=(TestClause const& rhs) const noexcept -> bool {
    return !(*this == rhs);
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

TEST(UnitClauseDB, IterableClauseDB_allocationFailsForOversizedClause) {
    std::size_t const regionSize = 128;
    Region<TestClause> underTest{regionSize};
    TestClause* c1 = underTest.allocate(128);
    EXPECT_EQ(c1, nullptr);
}

TEST(UnitClauseDB, IterableClauseDB_furtherAllocationInRegionPossibleAfterFailure) {
    std::size_t const regionSize = 192;
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
    ASSERT_TRUE(cloningResult);
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
    ASSERT_TRUE(cloningResult);
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
    EXPECT_EQ(underTest.begin(), underTest.end());
}

TEST(UnitClauseDB, IterableClauseDB_firstClauseCanBeRetrievedFromRegionViaIteration) {
    std::size_t const regionSize = 1024;
    Region<TestClause> underTest{regionSize};

    TestClause* clause1 = underTest.allocate(10);
    underTest.allocate(5);
    auto regionIter = underTest.begin();

    ASSERT_NE(regionIter, underTest.end());
    EXPECT_EQ(&(*regionIter), clause1);
}

TEST(UnitClauseDB, IterableClauseDB_regionIteratorReachesEnd) {
    std::size_t const regionSize = 1024;
    Region<TestClause> underTest{regionSize};

    underTest.allocate(10);
    auto regionIter = underTest.begin();
    ASSERT_NE(regionIter++, underTest.end());
    EXPECT_EQ(regionIter, underTest.end());
    EXPECT_EQ(++underTest.begin(), underTest.end());
}

TEST(UnitClauseDB, IterableClauseDB_regionIsIterable) {
    std::size_t const regionSize = 2048;
    Region<TestClause> underTest{regionSize};

    std::vector<TestClause*> clauses;
    for (int i = 0; i < 16; ++i) {
        clauses.push_back(underTest.allocate(10 + i));
    }

    std::vector<TestClause*> iterationResult;
    for (TestClause& currentClause : underTest) {
        iterationResult.push_back(&currentClause);
    }

    EXPECT_EQ(clauses, iterationResult);
}

TEST(UnitClauseDB, IterableClauseDB_regionIsEmptyAfterClear) {
    std::size_t const regionSize = 128;
    Region<TestClause> underTest{regionSize};

    underTest.allocate(11);
    underTest.allocate(5);
    EXPECT_GT(underTest.getUsedSize(), 0ull);

    underTest.clear();
    EXPECT_EQ(underTest.getUsedSize(), 0ull);
    EXPECT_EQ(underTest.getFreeSize(), regionSize);
}

TEST(UnitClauseDB, IterableClauseDB_regionCanBeReusedAfterClear) {
    std::size_t const regionSize = 128;
    Region<TestClause> underTest{regionSize};

    underTest.allocate(20);
    EXPECT_GT(underTest.getUsedSize(), 0ull);

    underTest.clear();
    EXPECT_EQ(underTest.getUsedSize(), 0ull);

    underTest.allocate(20);
    EXPECT_GT(underTest.getUsedSize(), 0ull);
}

TEST(UnitClauseDB, IterableClauseDB_clausesAreDestroyedDuringRegionClear) {
    std::size_t const regionSize = 512;
    Region<TestClause> underTest{regionSize};
    std::vector<char> destroyedFlags{0, 0, 0, 0};

    for (int i = 0, end = destroyedFlags.size(); i < end; ++i) {
        TestClause* clause = underTest.allocate(i + 2);
        clause->setDestroyedFlag(&(destroyedFlags[i]));
    }

    underTest.clear();

    for (int i = 0, end = destroyedFlags.size(); i < end; ++i) {
        EXPECT_GT(destroyedFlags[i], 0) << "Destructor of clause " << i << " not called";
    }
}

TEST(UnitClauseDB, IterableClauseDB_allocateClauseInSingleRegion) {
    std::size_t const regionSize = 1024;
    IterableClauseDB<TestClause> underTest{regionSize};
    auto clause = underTest.createClause(10);

    ASSERT_TRUE(clause);
    EXPECT_EQ((*clause)->size(), 10ull);
}

TEST(UnitClauseDB, IterableClauseDB_allocateClauseLargerThanRegionSizeFails) {
    std::size_t const regionSize = 1024;
    IterableClauseDB<TestClause> underTest{regionSize};
    auto clause = underTest.createClause(1025);

    ASSERT_FALSE(clause);
}

TEST(UnitClauseDB, IterableClauseDB_allocateClauseAfterFaultSucceeds) {
    std::size_t const regionSize = 1024;
    IterableClauseDB<TestClause> underTest{regionSize};
    auto clauseA = underTest.createClause(1025);
    EXPECT_FALSE(clauseA);
    auto clauseB = underTest.createClause(13);
    ASSERT_TRUE(clauseB);
    EXPECT_EQ((*clauseB)->size(), 13ull);
}

TEST(UnitClauseDB, IterableClauseDB_emptyDBHasEmptyClauseRange) {
    std::size_t const regionSize = 1024;
    IterableClauseDB<TestClause> underTest{regionSize};
    auto clauseRange = underTest.getClauses();
    EXPECT_EQ(clauseRange.begin(), clauseRange.end());
}

namespace {
template <typename RefRng, typename PtrRng>
auto refRangeIsEqualToPtrRange(RefRng refRange, PtrRng ptrRange) noexcept -> bool {
    auto ptrAdder = [](typename boost::range_value<RefRng>::type& t) { return &t; };
    return boost::equal(ptrRange, refRange | boost::adaptors::transformed(ptrAdder));
}
}

TEST(UnitClauseDB, IterableClauseDB_clauseDBWithSingleClauseHasSingleClauseRange) {
    std::size_t const regionSize = 1024;
    IterableClauseDB<TestClause> underTest{regionSize};

    auto clause1 = underTest.createClause(5);
    ASSERT_TRUE(clause1);
    std::vector<TestClause*> expectedClauses{*clause1};

    EXPECT_TRUE(refRangeIsEqualToPtrRange(underTest.getClauses(), expectedClauses));
}

TEST(UnitClauseDB, IterableClauseDB_clauseDBWithMultipleClausesHasMatchingClauseRange) {
    std::size_t const regionSize = 2048;
    IterableClauseDB<TestClause> underTest{regionSize};

    std::vector<TestClause*> expectedClauses;
    for (int i = 0; i < 10; ++i) {
        auto clause = underTest.createClause(5);
        ASSERT_TRUE(clause);
        expectedClauses.push_back(*clause);
    }

    EXPECT_TRUE(refRangeIsEqualToPtrRange(underTest.getClauses(), expectedClauses));
}

TEST(UnitClauseDB, IterableClauseDB_clauseDBWithMultipleRegionsHasMatchingClauseRange) {
    std::size_t const regionSize = 128;
    IterableClauseDB<TestClause> underTest{regionSize};

    std::vector<TestClause*> expectedClauses;
    for (int i = 0; i < 128; ++i) {
        auto clause = underTest.createClause(i % 20);
        ASSERT_TRUE(clause);
        expectedClauses.push_back(*clause);
    }

    EXPECT_TRUE(refRangeIsEqualToPtrRange(underTest.getClauses(), expectedClauses));
}

namespace {
template <typename RefRng, typename PtrRng>
auto refRangeIsEqualToDerefPtrRange(RefRng refRange, PtrRng ptrRange) noexcept -> bool {
    auto ptrAdder = [](typename boost::range_value<RefRng>::type& t) { return &t; };
    return boost::equal(
        ptrRange,
        refRange | boost::adaptors::transformed(ptrAdder),
        [](typename std::remove_reference_t<typename boost::range_value<RefRng>::type>* t1,
           typename boost::range_value<PtrRng>::type t2) { return *t1 == *t2; });
}
}

TEST(UnitClauseDB, IterableClauseDB_compressEmptyClauseDB) {
    std::size_t const regionSize = 128;
    IterableClauseDB<TestClause> underTest{regionSize};
    underTest.compress();

    auto clauses = underTest.getClauses();
    EXPECT_EQ(clauses.begin(), clauses.end());
}

TEST(UnitClauseDB, IterableClauseDB_compressSingleElementClauseDBWithoutDelete) {
    std::size_t const regionSize = 1024;
    IterableClauseDB<TestClause> underTest{regionSize};

    auto clause1 = underTest.createClause(5);
    ASSERT_TRUE(clause1);
    std::vector<TestClause*> expectedClauses{*clause1};


    underTest.compress();

    EXPECT_TRUE(refRangeIsEqualToDerefPtrRange(underTest.getClauses(), expectedClauses));
}

TEST(UnitClauseDB, IterableClauseDB_compressSingleElementClauseDBWithDelete) {
    std::size_t const regionSize = 1024;
    IterableClauseDB<TestClause> underTest{regionSize};

    auto clause1 = underTest.createClause(5);
    ASSERT_TRUE(clause1);
    std::vector<TestClause*> expectedClauses{*clause1};

    (*clause1)->setFlag(TestClause::Flag::SCHEDULED_FOR_DELETION);
    underTest.compress();

    auto clauses = underTest.getClauses();
    EXPECT_EQ(clauses.begin(), clauses.end());
}

TEST(UnitClauseDB, IterableClauseDB_compressMultiRegionClauseDBWithDelete) {
    std::size_t const regionSize = 1024;
    IterableClauseDB<TestClause> underTest{regionSize};

    std::vector<int> clauseIDs{};
    for (int i = 0; i < 256; ++i) {
        auto clause = underTest.createClause(i % 20);
        ASSERT_TRUE(clause);
        if (i % 13 == 0 || i % 4 == 0) {
            (*clause)->setFlag(TestClause::Flag::SCHEDULED_FOR_DELETION);
        } else {
            clauseIDs.push_back((*clause)->size());
        }

        if (i % 61 == 0) {
            underTest.compress();
        }
    }

    underTest.compress();

    EXPECT_TRUE(boost::equal(clauseIDs,
                             underTest.getClauses() | boost::adaptors::transformed(
                                                          [](TestClause& t) { return t.size(); })));
}
}
