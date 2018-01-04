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

#include <libjamsat/utils/FlatteningIterator.h>

#include <vector>

namespace jamsat {
using NestedConstIntVecIterator = FlatteningIterator<std::vector<std::vector<int>>::const_iterator>;

namespace {
void test_FlatteningIterator_beginIsEnd(const std::vector<std::vector<int>> testData) {
    NestedConstIntVecIterator begin{testData.begin(), testData.end()};
    NestedConstIntVecIterator end{testData.end(), testData.end()};
    EXPECT_EQ(begin, end);
}
}
TEST(UnitUtils, FlatteningIterator_endIteratorsMatchOnEmptySeq) {
    test_FlatteningIterator_beginIsEnd({});
}

TEST(UnitUtils, FlatteningIterator_endIteratorsMatchOnSeqContainingOneEmptySeq) {
    test_FlatteningIterator_beginIsEnd({{}});
}

TEST(UnitUtils, FlatteningIterator_endIteratorsMatchOnSeqContainingMultipleEmptySeq) {
    test_FlatteningIterator_beginIsEnd({{}, {}, {}});
}

namespace {
void test_FlatteningIterator_flattenSeqTest(const std::vector<std::vector<int>> testData) {
    std::vector<int> expected;
    for (auto &vec : testData) {
        for (auto &i : vec) {
            expected.push_back(i);
        }
    }

    NestedConstIntVecIterator it{testData.begin(), testData.end()};
    NestedConstIntVecIterator end{testData.end(), testData.end()};

    std::vector<int> iterSeq;
    for (; it != end; ++it) {
        iterSeq.push_back(*it);
    }

    EXPECT_EQ(iterSeq, expected);
}
}

TEST(UnitUtils, FlatteningIterator_iterationOnSeqContainingSingleElementSeq) {
    test_FlatteningIterator_flattenSeqTest({{1}});
}

TEST(UnitUtils, FlatteningIterator_iterationOnSeqContainingMultiElementSeqs) {
    test_FlatteningIterator_flattenSeqTest({{1, 2}, {3}});
}

TEST(UnitUtils, FlatteningIterator_iterationOnSeqContainingMultiElementSeqsAndEmptyBack) {
    test_FlatteningIterator_flattenSeqTest({{5}, {1, 2}, {3}, {}});
}

TEST(UnitUtils, FlatteningIterator_iterationOnSeqContainingMultiElementSeqsMultiEmptyAtBack) {
    test_FlatteningIterator_flattenSeqTest({{5}, {}, {1, 2}, {3}, {}, {}});
}

TEST(UnitUtils, FlatteningIterator_iterationOnSeqContainingMultiElementSeqsMultiEmptyAtFront) {
    test_FlatteningIterator_flattenSeqTest({{}, {}, {5}, {1, 2}, {3}});
}

TEST(UnitUtils, FlatteningIterator_isMultipassIteratorForVectorOfVectors) {
    std::vector<std::vector<int>> testData{{}, {1}, {2, 3}};
    NestedConstIntVecIterator begin{testData.begin(), testData.end()};
    NestedConstIntVecIterator end{testData.end(), testData.end()};

    auto beginCopy = begin;
    begin++;
    EXPECT_EQ(*begin, 2);
    EXPECT_EQ(*beginCopy, 1);
    beginCopy++;
    EXPECT_EQ(*beginCopy, 2);

    EXPECT_EQ(begin, beginCopy);

    begin++;
    EXPECT_EQ(*begin, 3);
    beginCopy++;
    EXPECT_EQ(*beginCopy, 3);

    EXPECT_EQ(begin, beginCopy);

    begin++;
    beginCopy++;

    EXPECT_EQ(begin, end);
    EXPECT_EQ(beginCopy, end);
}

TEST(UnitUtils, FlatteningIterator_isSwappable) {
    std::vector<std::vector<int>> testData{{}, {1}, {2, 3}};
    NestedConstIntVecIterator x{testData.begin(), testData.end()};
    NestedConstIntVecIterator y{testData.begin(), testData.end()};

    ++x;
    swap(x, y);
    EXPECT_EQ(*x, 1);
    EXPECT_EQ(*y, 2);
}

TEST(UnitUtils, FlatteningIterator_isAccessibleViaPointOperator) {
    struct Z {
        int x;
        int y;
    };
    std::vector<std::vector<Z>> testData{{Z{1, 2}}};
    FlatteningIterator<std::vector<std::vector<Z>>::const_iterator> begin{testData.begin(),
                                                                          testData.end()};
    EXPECT_EQ(begin->x, (*begin).x);
}

TEST(UnitUtils, FlatteningIterator_isEqualToSelf) {
    std::vector<std::vector<int>> testData{{}, {1}, {2, 3}};
    NestedConstIntVecIterator begin{testData.begin(), testData.end()};
    ASSERT_EQ(begin, begin);
}

TEST(UnitUtils, FlatteningIterator_defaultConstructedNCIIsPastTheEnd) {
    std::vector<std::vector<int>> testData{{}, {1}, {2, 3}};
    NestedConstIntVecIterator begin{testData.begin(), testData.end()};
    NestedConstIntVecIterator explicitEnd{testData.end(), testData.end()};
    NestedConstIntVecIterator end;

    ASSERT_EQ(explicitEnd, end);

    ASSERT_NE(begin, end);
    ++begin;
    ASSERT_NE(begin, end);
    ++begin;
    ASSERT_NE(begin, end);
    ++begin;
    ASSERT_EQ(begin, end);
}
}
