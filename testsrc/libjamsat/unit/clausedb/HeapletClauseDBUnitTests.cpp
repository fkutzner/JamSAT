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

#include <cstdint>

#include <libjamsat/clausedb/HeapletClauseDB.h>

namespace jamsat {
TEST(UnitClauseDB, HeapletIsUninitializedBeforeInitialization) {
    clausedb_detail::Heaplet underTest{512};
    EXPECT_FALSE(underTest.isInitialized());
    EXPECT_EQ(underTest.getFreeSize(), 0ull);
}

struct alignas(32) LargeAlignedTestStruct {
    int x;
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
}
