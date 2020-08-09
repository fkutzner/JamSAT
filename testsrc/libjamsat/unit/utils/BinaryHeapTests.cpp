/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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

#include <libjamsat/utils/BinaryHeap.h>

#include <algorithm>
#include <cstdint>


namespace jamsat {

class IntIndex {
public:
  using Type = int;
  static constexpr auto getIndex(int i) -> std::size_t
  {
    int z = 2 * i;
    if (i < 0) {
      return static_cast<std::size_t>(-z);
    }
    return static_cast<std::size_t>(z + 1);
  }
};

class TestIntComparator {
public:
  TestIntComparator(int max) { (void)max; }

  auto operator()(int lhs, int rhs) const noexcept -> bool { return lhs < rhs; }

  void increaseMaxSizeTo(int newMaxElement) noexcept { (void)newMaxElement; }
};

TEST(UnitUtils, EmptyBinaryMaxHeapIsMarkedAsEmpty)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  EXPECT_TRUE(underTest.empty());
  EXPECT_EQ(underTest.size(), 0ULL);
}

TEST(UnitUtils, EmptyBinaryMaxHeapContainsNoElements)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  for (int i = -10; i <= 10; ++i) {
    EXPECT_FALSE(underTest.contains(i)) << "Heap unexpectedly contains element " << i;
  }
}

TEST(UnitUtils, SingleElementMaxHeapContainsExactlyOneElement)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  underTest.insert(5);

  EXPECT_FALSE(underTest.empty());
  EXPECT_EQ(underTest.size(), 1ULL);

  EXPECT_TRUE(underTest.contains(5));
  for (int i = -10; i <= 10; ++i) {
    if (i != 5) {
      EXPECT_FALSE(underTest.contains(i)) << "Heap unexpectedly contains element " << i;
    }
  }
}

TEST(UnitUtils, MaxHeapDoubleInsertionsDoNotDuplicateElements)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  underTest.insert(5);
  underTest.insert(5);
  EXPECT_EQ(underTest.size(), 1ULL);
  EXPECT_EQ(underTest.removeMax(), 5);
  EXPECT_TRUE(underTest.empty());
}

TEST(UnitUtils, SingleElementMaxHeapIsEmptyAfterRemoval)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  underTest.insert(5);

  auto removed = underTest.removeMax();
  EXPECT_EQ(removed, 5);
  EXPECT_TRUE(underTest.empty());
}

TEST(UnitUtils, BinaryMaxHeapHasHeapPropertyAfterInsertion)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  std::vector<int> testSeq = std::vector<int>{3, 9, 1, -5, -10, -9, 10, 0, -1, 7};
  for (auto i : testSeq) {
    underTest.insert(i);
  }
  EXPECT_TRUE(underTest.test_satisfiesHeapProperty());
  EXPECT_EQ(underTest.size(), testSeq.size());
}

TEST(UnitUtils, BinaryMaxHeapCanBeFilledToMax)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  for (int i = -10; i <= 10; ++i) {
    underTest.insert(i);
  }
  EXPECT_EQ(underTest.size(), 21ULL);
}

TEST(UnitUtils, BinaryMaxHeapHasDescendingRemovalSequence)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  std::vector<int> testSeq = std::vector<int>{3, 9, 1, -5, -10, -9, 10, 0, -1, 7};
  for (auto i : testSeq) {
    underTest.insert(i);
  }

  std::sort(testSeq.rbegin(), testSeq.rend());

  for (size_t i = 0; i < testSeq.size(); ++i) {
    auto removed = underTest.removeMax();
    EXPECT_EQ(testSeq[i], removed) << "Differing elements at removal step " << i;
    EXPECT_TRUE(underTest.test_satisfiesHeapProperty())
        << "Heap property violated at removal step " << i;
  }

  EXPECT_TRUE(underTest.empty());
}

TEST(UnitUtils, BinaryMaxHeapElementInsertedAfterRemoveCanBeRetrieved)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{10};
  std::vector<int> testSeq = std::vector<int>{3, 9, 1, -5};
  for (auto i : testSeq) {
    underTest.insert(i);
  }

  EXPECT_EQ(underTest.removeMax(), 9);
  EXPECT_EQ(underTest.removeMax(), 3);
  underTest.insert(2);
  EXPECT_EQ(underTest.removeMax(), 2);
  underTest.insert(-3);
  EXPECT_EQ(underTest.removeMax(), 1);
  EXPECT_EQ(underTest.removeMax(), -3);
  EXPECT_EQ(underTest.removeMax(), -5);
}

TEST(UnitUtils, BinaryMaxHeapCanBeResized)
{
  BinaryMaxHeap<int, TestIntComparator, IntIndex> underTest{5};
  std::vector<int> testSeq = std::vector<int>{3, 2, 1, -5};
  for (auto i : testSeq) {
    underTest.insert(i);
  }

  underTest.increaseMaxSizeTo(8);
  underTest.insert(8);
  EXPECT_EQ(underTest.size(), testSeq.size() + 1);
  EXPECT_EQ(underTest.removeMax(), 8);
}
}
