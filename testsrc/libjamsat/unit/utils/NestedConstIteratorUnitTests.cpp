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

#include <libjamsat/utils/NestedConstIterator.h>

#include <vector>

namespace jamsat {
using NestedConstIntVecIterator = NestedConstIterator<std::vector<std::vector<int>>>;

namespace {
void test_NestedConstIterator_beginIsEnd(const std::vector<std::vector<int>> testData) {
    NestedConstIntVecIterator begin{testData.begin(), testData.end()};
    NestedConstIntVecIterator end{testData.end(), testData.end()};
    EXPECT_EQ(begin, end);
}
}
TEST(UnitUtils, NestedConstIterator_endIteratorsMatchOnEmptySeq) {
    test_NestedConstIterator_beginIsEnd({});
}

TEST(UnitUtils, NestedConstIterator_endIteratorsMatchOnSeqContainingOneEmptySeq) {
    test_NestedConstIterator_beginIsEnd({{}});
}

TEST(UnitUtils, NestedConstIterator_endIteratorsMatchOnSeqContainingMultipleEmptySeq) {
    test_NestedConstIterator_beginIsEnd({{}, {}, {}});
}

namespace {
void test_NestedConstIterator_flattenSeqTest(const std::vector<std::vector<int>> testData) {
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

TEST(UnitUtils, NestedConstIterator_iterationOnSeqContainingSingleElementSeq) {
    test_NestedConstIterator_flattenSeqTest({{1}});
}

TEST(UnitUtils, NestedConstIterator_iterationOnSeqContainingMultiElementSeqs) {
    test_NestedConstIterator_flattenSeqTest({{1, 2}, {3}});
}

TEST(UnitUtils, NestedConstIterator_iterationOnSeqContainingMultiElementSeqsAndEmptyBack) {
    test_NestedConstIterator_flattenSeqTest({{5}, {1, 2}, {3}, {}});
}

TEST(UnitUtils, NestedConstIterator_iterationOnSeqContainingMultiElementSeqsMultiEmptyAtBack) {
    test_NestedConstIterator_flattenSeqTest({{5}, {}, {1, 2}, {3}, {}, {}});
}

TEST(UnitUtils, NestedConstIterator_iterationOnSeqContainingMultiElementSeqsMultiEmptyAtFront) {
    test_NestedConstIterator_flattenSeqTest({{}, {}, {5}, {1, 2}, {3}});
}

TEST(UnitUtils, NestedConstIterator_isMultipassIteratorForVectorOfVectors) {
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

TEST(UnitUtils, NestedConstIterator_isSwappable) {
    std::vector<std::vector<int>> testData{{}, {1}, {2, 3}};
    NestedConstIntVecIterator x{testData.begin(), testData.end()};
    NestedConstIntVecIterator y{testData.begin(), testData.end()};

    ++x;
    swap(x, y);
    EXPECT_EQ(*x, 1);
    EXPECT_EQ(*y, 2);
}

TEST(UnitUtils, NestedConstIterator_isAccessibleViaPointOperator) {
    struct Z {
        int x;
        int y;
    };
    std::vector<std::vector<Z>> testData{{Z{1, 2}}};
    NestedConstIterator<std::vector<std::vector<Z>>> begin{testData.begin(), testData.end()};
    EXPECT_EQ(begin->x, (*begin).x);
}
}
