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

#include <libjamsat/utils/BoundedStack.h>

namespace jamsat {
TEST(UnitUtils, elementsAreSortedByInsertionOrderInBoundedStack) {
    BoundedStack<int> underTest{42};
    underTest.push_back(10);
    underTest.push_back(20);

    ASSERT_EQ(underTest.back(), 20);
    underTest.pop();

    underTest.push_back(30);

    ASSERT_EQ(underTest.back(), 30);
    underTest.pop();
    EXPECT_EQ(underTest.back(), 10);
    underTest.pop();
    EXPECT_TRUE(underTest.empty());
}

TEST(UnitUtils, multiPopResizesStackCorrectly) {
    BoundedStack<int> underTest{42};
    EXPECT_TRUE(underTest.empty());
    underTest.push_back(10);
    underTest.push_back(20);
    underTest.push_back(30);
    underTest.push_back(40);
    EXPECT_EQ(underTest.size(), 4ull);
    underTest.pop_to(2);
    EXPECT_EQ(underTest.size(), 2ull);
    EXPECT_EQ(underTest.back(), 20);
}

TEST(UnitUtils, stackIteratorsRemainValidAfterPush) {
    BoundedStack<int> underTest{1024};
    EXPECT_TRUE(underTest.empty());
    underTest.push_back(10);
    underTest.push_back(20);
    underTest.push_back(30);
    underTest.push_back(40);

    auto begin = underTest.begin();
    auto end = underTest.end();

    for (int i = 50; i < 1000; ++i) {
        underTest.push_back(i);
    }

    EXPECT_EQ(underTest.begin(), begin);

    underTest.pop_to(4);

    EXPECT_EQ(underTest.begin(), begin);
    EXPECT_EQ(underTest.end(), end);
}

TEST(UnitUtils, boundedStackIsIterableInInsertionOrder) {
    BoundedStack<int> underTest{42};
    EXPECT_TRUE(underTest.empty());
    underTest.push_back(10);
    underTest.push_back(20);
    underTest.push_back(30);

    auto begin = underTest.begin();
    ASSERT_TRUE(begin + 3 == underTest.end());
    EXPECT_EQ(*begin, 10);
    EXPECT_EQ(*(begin + 1), 20);
    EXPECT_EQ(*(begin + 2), 30);
}

TEST(UnitUtils, boundedStackIsConstIterableInInsertionOrder) {
    BoundedStack<int> underTest{42};
    EXPECT_TRUE(underTest.empty());
    underTest.push_back(10);
    underTest.push_back(20);
    underTest.push_back(30);

    const BoundedStack<int> &constUnderTest = underTest;

    auto begin = constUnderTest.begin();
    ASSERT_TRUE(begin + 3 == constUnderTest.end());
    EXPECT_EQ(*begin, 10);
    EXPECT_EQ(*(begin + 1), 20);
    EXPECT_EQ(*(begin + 2), 30);
}

TEST(UnitUtils, boundedStackIsConstructibleWithMaxSizeZero) {
    BoundedStack<int> underTest{0};
    EXPECT_TRUE(underTest.empty());
    EXPECT_EQ(underTest.size(), 0ull);
}

TEST(UnitUtils, boundedStackSizeIsIncreasable) {
    BoundedStack<int> underTest{1};
    underTest.push_back(10);
    underTest.increaseMaxSizeBy(2);
    underTest.push_back(11);
    underTest.push_back(12);

    EXPECT_EQ(underTest.back(), 12);
    underTest.pop();
    EXPECT_EQ(underTest.back(), 11);
    underTest.pop();
    EXPECT_EQ(underTest.back(), 10);
    underTest.pop();
    EXPECT_TRUE(underTest.empty());
}
}
