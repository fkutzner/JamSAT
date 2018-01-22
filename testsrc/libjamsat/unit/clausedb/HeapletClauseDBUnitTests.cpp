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

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <unordered_set>

#include <boost/optional.hpp>

#define JAM_EXPOSE_INTERNAL_TESTING_INTERFACES
#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/HeapletClauseDB.h>

namespace jamsat {
TEST(UnitClauseDB, HeapletIsUninitializedBeforeInitialization) {
    clausedb_detail::Heaplet underTest{512};
    EXPECT_FALSE(underTest.isInitialized());
    EXPECT_EQ(underTest.getFreeSize(), 0ull);
}

struct alignas(32) LargeAlignedTestStruct {
    int x;

    static LargeAlignedTestStruct *constructIn(void *memory) noexcept {
        return new (memory) LargeAlignedTestStruct{};
    }
};

TEST(UnitClauseDB, HeapletIsInitializedAfterCallingInitialize) {
    clausedb_detail::Heaplet underTest{512};
    underTest.initialize();
    EXPECT_TRUE(underTest.isInitialized());
}

TEST(UnitClauseDB, HeapletIsEmptyAfterCallingInitialize) {
    clausedb_detail::Heaplet underTest{512};
    underTest.initialize();
    EXPECT_EQ(underTest.getFreeSize(), 512ull);
}

TEST(UnitClauseDB, HeapletAllocationsAreAlignedCorrectly) {
    clausedb_detail::Heaplet underTest{512};
    underTest.initialize();
    auto allocated = underTest.allocate<LargeAlignedTestStruct>(sizeof(LargeAlignedTestStruct));
    ASSERT_NE(allocated, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(allocated) % 32, 0ull);

    auto allocated2 = underTest.allocate<LargeAlignedTestStruct>(sizeof(LargeAlignedTestStruct));
    ASSERT_NE(allocated2, nullptr);
    ASSERT_NE(allocated, allocated2);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(allocated2) % 32, 0ull);
}

TEST(UnitClauseDB, HeapletAllocationsDecreaseFreeSize) {
    clausedb_detail::Heaplet underTest{512};
    underTest.initialize();
    auto allocated = underTest.allocate<LargeAlignedTestStruct>(sizeof(LargeAlignedTestStruct));
    ASSERT_NE(allocated, nullptr);
    EXPECT_LE(underTest.getFreeSize(), 512ull - 32ull);
    auto newSize = underTest.getFreeSize();

    auto allocated2 = underTest.allocate<LargeAlignedTestStruct>(sizeof(LargeAlignedTestStruct));
    ASSERT_NE(allocated2, nullptr);
    ASSERT_NE(allocated, allocated2);
    EXPECT_LE(underTest.getFreeSize(), newSize - 32ull);
}

struct alignas(8) SmallAlignedTestStruct {
    int x;

    static SmallAlignedTestStruct *constructIn(void *memory) noexcept {
        return new (memory) SmallAlignedTestStruct{};
    }
};

TEST(UnitClauseDB, HeapletReturnsNullptrWhenOutOutOfSpace) {
    clausedb_detail::Heaplet underTest{23};
    underTest.initialize();
    auto allocated = underTest.allocate<SmallAlignedTestStruct>(sizeof(SmallAlignedTestStruct));
    ASSERT_NE(allocated, nullptr);
    allocated = underTest.allocate<SmallAlignedTestStruct>(sizeof(SmallAlignedTestStruct));
    ASSERT_NE(allocated, nullptr);
    allocated = underTest.allocate<SmallAlignedTestStruct>(sizeof(SmallAlignedTestStruct));
    EXPECT_EQ(allocated, nullptr);
}

TEST(UnitClauseDB, HeapletIsEmptyAfterAllocationsAndClear) {
    clausedb_detail::Heaplet underTest{23};
    underTest.initialize();
    underTest.allocate<SmallAlignedTestStruct>(sizeof(SmallAlignedTestStruct));
    underTest.clear();
    EXPECT_EQ(underTest.getFreeSize(), 23ull);
}

TEST(UnitClauseDB, HeapletIsUninitializedAfterMoveAssignment) {
    clausedb_detail::Heaplet underTest{23};
    underTest.initialize();
    ASSERT_TRUE(underTest.isInitialized());

    clausedb_detail::Heaplet moveTarget{1};
    moveTarget = std::move(underTest);
    EXPECT_TRUE(moveTarget.isInitialized());
    EXPECT_EQ(moveTarget.getFreeSize(), 23ull);
    EXPECT_FALSE(underTest.isInitialized());
}

TEST(UnitClauseDB, HeapletIsUninitializedAfterMoveConstruction) {
    clausedb_detail::Heaplet underTest{23};
    underTest.initialize();
    ASSERT_TRUE(underTest.isInitialized());

    clausedb_detail::Heaplet moveTarget{std::move(underTest)};
    EXPECT_TRUE(moveTarget.isInitialized());
    EXPECT_EQ(moveTarget.getFreeSize(), 23ull);
    EXPECT_FALSE(underTest.isInitialized());
}

namespace {
class ClassWithNontrivialConstructor {
public:
    ClassWithNontrivialConstructor(int x, int y) noexcept : m_x(x), m_y(y) {}

    int getX() const noexcept { return m_x; }

    int getY() const noexcept { return m_y; }

    static ClassWithNontrivialConstructor *constructIn(void *memory, int x, int y) noexcept {
        return new (memory) ClassWithNontrivialConstructor{x, y};
    }

private:
    int m_x;
    int m_y;
};
}

TEST(UnitClauseDB, HeapletPassesConstructionArgumentsToFactoryFunction) {
    clausedb_detail::Heaplet underTest{128};
    underTest.initialize();
    auto r = underTest.allocate<ClassWithNontrivialConstructor>(
        sizeof(ClassWithNontrivialConstructor), 4, 5);
    EXPECT_EQ(r->getX(), 4);
    EXPECT_EQ(r->getY(), 5);
}

TEST(UnitClauseDB, HeapletClauseDBIsEmptyAfterCreation) {
    HeapletClauseDB<Clause> underTest{512ull, 2048ull, {}, {}};
    // Assert that space is allocated lazily:
    EXPECT_EQ(underTest.test_getAvailableSpaceInActiveHeaplets(), 0ull);
    EXPECT_EQ(underTest.test_getAvailableSpaceInBinaryHeaplets(), 0ull);
    EXPECT_EQ(underTest.test_getAvailableSpaceInFreeHeaplets(), 0ull);
}

TEST(UnitClauseDB, HeapletClauseDBFreeSpaceDecreasedNonBinaryClauseCreation) {
    HeapletClauseDB<Clause> underTest{512ull, 2048ull, {}, {}};
    HeapletClauseDB<Clause>::size_type maxFree = 512ull;

    const Clause::size_type clauseSize = 5;
    auto &clause = underTest.allocate(clauseSize);
    (void)clause;
    maxFree -= Clause::getAllocationSize(clauseSize);

    EXPECT_GT(underTest.test_getAvailableSpaceInActiveHeaplets(), 0ull);
    EXPECT_LE(underTest.test_getAvailableSpaceInActiveHeaplets(), maxFree);
    EXPECT_TRUE(underTest.test_isRegionInActiveHeaplet(reinterpret_cast<void *>(&clause),
                                                       Clause::getAllocationSize(clauseSize)));

    const Clause::size_type clauseSize2 = 7;
    auto &clause2 = underTest.allocate(clauseSize2);
    (void)clause2;
    maxFree -= Clause::getAllocationSize(clauseSize2);
    EXPECT_GT(underTest.test_getAvailableSpaceInActiveHeaplets(), 0ull);
    EXPECT_LE(underTest.test_getAvailableSpaceInActiveHeaplets(), maxFree);
    EXPECT_TRUE(underTest.test_isRegionInActiveHeaplet(reinterpret_cast<void *>(&clause2),
                                                       Clause::getAllocationSize(clauseSize2)));
}

TEST(UnitClauseDB, HeapletClauseDBFreeSpaceDecreasedBinaryClauseCreation) {
    HeapletClauseDB<Clause> underTest{512ull, 2048ull, {}, {}};
    HeapletClauseDB<Clause>::size_type maxFree = 512ull;

    const Clause::size_type clauseSize = 2;
    auto &clause = underTest.allocate(clauseSize);
    (void)clause;
    maxFree -= Clause::getAllocationSize(clauseSize);

    EXPECT_GT(underTest.test_getAvailableSpaceInBinaryHeaplets(), 0ull);
    EXPECT_LE(underTest.test_getAvailableSpaceInBinaryHeaplets(), maxFree);
    EXPECT_TRUE(underTest.test_isRegionInBinaryHeaplet(reinterpret_cast<void *>(&clause),
                                                       Clause::getAllocationSize(clauseSize)));

    const Clause::size_type clauseSize2 = 2;
    auto &clause2 = underTest.allocate(clauseSize2);
    (void)clause2;
    maxFree -= Clause::getAllocationSize(clauseSize2);
    EXPECT_GT(underTest.test_getAvailableSpaceInBinaryHeaplets(), 0ull);
    EXPECT_LE(underTest.test_getAvailableSpaceInBinaryHeaplets(), maxFree);
    EXPECT_TRUE(underTest.test_isRegionInBinaryHeaplet(reinterpret_cast<void *>(&clause2),
                                                       Clause::getAllocationSize(clauseSize2)));
}

TEST(UnitClauseDB, HeapletClauseDBAllocatesClausesOfCorrectSize) {
    HeapletClauseDB<Clause> underTest{512ull, 2048ull, {}, {}};
    const Clause::size_type clauseSize = 5;
    auto &clause = underTest.allocate(clauseSize);
    EXPECT_EQ(clause.size(), clauseSize);
}

TEST(UnitClauseDB, HeapletClauseDBUsesFreshHeapletWhenFirstIsFull) {
    HeapletClauseDB<Clause> underTest{512ull, 2048ull, {}, {}};
    auto &clause1 = underTest.allocate(96);
    (void)clause1;
    auto free = underTest.test_getAvailableSpaceInActiveHeaplets();
    auto &clause2 = underTest.allocate(96);
    (void)clause2;
    auto laterFree = underTest.test_getAvailableSpaceInActiveHeaplets();
    EXPECT_GT(laterFree, free);
}

TEST(UnitClauseDB, HeapletClauseDBThrowsBadAllocWhenOutOfMemory) {
    HeapletClauseDB<Clause> underTest{512ull, 1536ull, {}, {}};
    // Should be able to allocate no more than 1024 byte in non-binary clauses
    // Allocating at least 384 bytes at a time:
    underTest.allocate(96);
    underTest.allocate(96);

    // Second heaplet is also full now, expect exception:
    EXPECT_THROW(underTest.allocate(96), std::bad_alloc);
}

TEST(UnitClauseDB, HeapletClauseDBThrowsBadAllocWhenClauseTooLarge) {
    HeapletClauseDB<Clause> underTest{512ull, 1536ull, {}, {}};
    // Allocatnig a clause bigger than 512 byte, should not fit in any heaplet:
    EXPECT_THROW(underTest.allocate(384), std::bad_alloc);
}

namespace {
std::function<bool(const Clause &)> createNoReasonClausePred() noexcept {
    return [](const Clause &) { return false; };
}
}

TEST(UnitClauseDB, HeapletClauseDBIsEmptyAfterRetainingNoClauses) {
    HeapletClauseDB<Clause> underTest{512ull, 3096ull, createNoReasonClausePred(), {}};
    auto &clause1 = underTest.allocate(96);
    auto &clause2 = underTest.allocate(96);
    std::vector<Clause *> empty;
    underTest.retain(empty, boost::optional<std::vector<Clause *>::iterator>{});

    // The two heaplets used by the clauses should show up as free memory now:
    EXPECT_EQ(underTest.test_getAvailableSpaceInFreeHeaplets(), 1024ull);

    // There should only be noninitialized active heaplets now:
    EXPECT_EQ(underTest.test_getAvailableSpaceInActiveHeaplets(), 0ull);

    // The clauses should not be contained in active heaplets:
    EXPECT_FALSE(underTest.test_isRegionInActiveHeaplet(&clause1, Clause::getAllocationSize(96)));
    EXPECT_FALSE(underTest.test_isRegionInActiveHeaplet(&clause2, Clause::getAllocationSize(96)));
}

TEST(UnitClauseDB, HeapletClauseDBCanAllocateAfterRetainingNoClauses) {
    HeapletClauseDB<Clause> underTest{512ull, 2048ull, createNoReasonClausePred(), {}};
    underTest.allocate(80);
    underTest.allocate(80);
    std::vector<Clause *> empty;
    underTest.retain(empty, boost::optional<std::vector<Clause *>::iterator>{});
    EXPECT_NO_THROW(underTest.allocate(96));
    EXPECT_NO_THROW(underTest.allocate(96));
    EXPECT_NO_THROW(underTest.allocate(96));
    EXPECT_THROW(underTest.allocate(96), std::bad_alloc);
}

TEST(UnitClauseDB, HeapletClauseDBThrowsBadAllocWhenRetainIsOutOfMemory) {
    HeapletClauseDB<Clause> underTest{512ull, 1024ull, createNoReasonClausePred(), {}};
    // This clause DB has no heaplets that might be used for retain()
    auto &clause = underTest.allocate(3);
    std::vector<Clause *> empty;
    ASSERT_THROW(underTest.retain(empty, boost::optional<std::vector<Clause *>::iterator>{}),
                 std::bad_alloc);
    // Check exception safety: the clause should remain intact
    EXPECT_TRUE(underTest.test_isRegionInActiveHeaplet(&clause, Clause::getAllocationSize(3)));
}

TEST(UnitClauseDB, HeapletClauseDBContainsCorrectClausesAfterRetain) {
    HeapletClauseDB<Clause> underTest{64ull, 8192ull, createNoReasonClausePred(), {}};
    std::vector<Clause *> clauses{&underTest.allocate(4), &underTest.allocate(3),
                                  &underTest.allocate(10), &underTest.allocate(4)};
    std::vector<CNFLit> retainedClauseALiterals{CNFLit{CNFVar{3}, CNFSign::POSITIVE},
                                                CNFLit{CNFVar{2}, CNFSign::NEGATIVE},
                                                CNFLit{CNFVar{1}, CNFSign::POSITIVE}};
    std::vector<CNFLit> retainedClauseBLiterals{
        CNFLit{CNFVar{10}, CNFSign::POSITIVE}, CNFLit{CNFVar{8}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{6}, CNFSign::NEGATIVE}, CNFLit{CNFVar{4}, CNFSign::POSITIVE}};

    std::vector<Clause *> retained{clauses[1], clauses[3]};
    std::copy(retainedClauseALiterals.begin(), retainedClauseALiterals.end(), retained[0]->begin());
    std::copy(retainedClauseBLiterals.begin(), retainedClauseBLiterals.end(), retained[1]->begin());

    std::vector<Clause *> relocated;
    using BackInserterType = decltype(std::back_inserter(relocated));
    underTest.retain(retained, boost::optional<BackInserterType>{std::back_inserter(relocated)});
    ASSERT_EQ(relocated.size(), 2ull);

    EXPECT_FALSE(underTest.test_isRegionInActiveHeaplet(clauses[0], Clause::getAllocationSize(4)));
    EXPECT_FALSE(underTest.test_isRegionInActiveHeaplet(clauses[1], Clause::getAllocationSize(3)));
    EXPECT_FALSE(underTest.test_isRegionInActiveHeaplet(clauses[2], Clause::getAllocationSize(4)));
    EXPECT_FALSE(underTest.test_isRegionInActiveHeaplet(clauses[3], Clause::getAllocationSize(10)));

    ASSERT_TRUE(underTest.test_isRegionInActiveHeaplet(relocated[0], Clause::getAllocationSize(3)));
    ASSERT_TRUE(underTest.test_isRegionInActiveHeaplet(relocated[1], Clause::getAllocationSize(4)));

    EXPECT_EQ(retained[0]->size(), 3ull);
    EXPECT_EQ(retained[1]->size(), 4ull);
    EXPECT_TRUE(
        std::equal(retained[0]->begin(), retained[0]->end(), retainedClauseALiterals.begin()));
    EXPECT_TRUE(
        std::equal(retained[1]->begin(), retained[1]->end(), retainedClauseBLiterals.begin()));
}

TEST(UnitClauseDB, HeapletClauseDBAnnouncesRewriteOfReasonClauses) {
    std::vector<const Clause *> reasons;
    std::vector<std::pair<const Clause *, const Clause *>> reasonRelocations;

    auto reasonPred = [&reasons](const Clause &c) {
        return std::find(reasons.begin(), reasons.end(), &c) != reasons.end();
    };
    auto reasonRelocationRecv = [&reasonRelocations](const Clause &oldR, const Clause &newR) {
        EXPECT_EQ(oldR[0], newR[0]);
        reasonRelocations.push_back({&oldR, &newR});
    };

    HeapletClauseDB<Clause> underTest{512ull, 2048ull, reasonPred, reasonRelocationRecv};

    std::vector<Clause *> clauses{&underTest.allocate(4), &underTest.allocate(3),
                                  &underTest.allocate(10), &underTest.allocate(4)};
    reasons.push_back(clauses[0]);
    reasons.push_back(clauses[1]);

    (*clauses[0])[0] = CNFLit{CNFVar{3}, CNFSign::POSITIVE};
    (*clauses[1])[0] = CNFLit{CNFVar{5}, CNFSign::POSITIVE};

    std::vector<Clause *> retained{clauses[0], clauses[1], clauses[3]};
    std::vector<Clause *> relocated;
    using BackInserterType = decltype(std::back_inserter(relocated));
    underTest.retain(retained, boost::optional<BackInserterType>{std::back_inserter(relocated)});

    ASSERT_EQ(reasonRelocations.size(), 2ull);
    ASSERT_EQ(relocated.size(), 3ull);

    EXPECT_EQ(reasons[0], reasonRelocations[0].first);
    EXPECT_EQ(relocated[0], reasonRelocations[0].second);
    EXPECT_EQ(reasons[1], reasonRelocations[1].first);
    EXPECT_EQ(relocated[1], reasonRelocations[1].second);
}
}
