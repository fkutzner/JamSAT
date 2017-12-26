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

#include <libjamsat/utils/SimpleMovingAverage.h>

namespace jamsat {
TEST(UnitUtils, SimpleMovingAverage_avgWithHorizon0Is0) {
    SimpleMovingAverage<int, double> underTest{0};
    EXPECT_EQ(underTest.getAverage(), 0.0f);
    underTest.add(4);
    EXPECT_EQ(underTest.getAverage(), 0.0f);
}

TEST(UnitUtils, SimpleMovingAverage_avgWithHorizon1IsLastValue) {
    SimpleMovingAverage<int, int> underTest{1};
    EXPECT_EQ(underTest.getAverage(), 0);
    underTest.add(4);
    EXPECT_EQ(underTest.getAverage(), 4);
    underTest.add(5);
    EXPECT_EQ(underTest.getAverage(), 5);
}

TEST(UnitUtils, SimpleMovingAverage_avgWithHorizon2IsMeanOfLastTwo) {
    SimpleMovingAverage<int, int> underTest{2};
    EXPECT_EQ(underTest.getAverage(), 0);
    underTest.add(4);
    EXPECT_EQ(underTest.getAverage(), 4);
    underTest.add(8);
    EXPECT_EQ(underTest.getAverage(), 6);
    underTest.add(2);
    EXPECT_EQ(underTest.getAverage(), 5);
}

TEST(UnitUtils, SimpleMovingAverage_avgWithHorizon5IsMeanOfLastFive) {
    SimpleMovingAverage<int, int> underTest{5};
    EXPECT_EQ(underTest.getAverage(), 0);

    underTest.add(2);
    EXPECT_EQ(underTest.getAverage(), 2);
    underTest.add(4);
    EXPECT_EQ(underTest.getAverage(), 3);
    underTest.add(6);
    EXPECT_EQ(underTest.getAverage(), 4);
    underTest.add(8);
    EXPECT_EQ(underTest.getAverage(), 5);
    underTest.add(10);
    EXPECT_EQ(underTest.getAverage(), 6);

    underTest.add(22);
    EXPECT_EQ(underTest.getAverage(), 10);
    underTest.add(54);
    EXPECT_EQ(underTest.getAverage(), 20);
}

TEST(UnitUtils, SimpleMovingAverage_avgIs0AfterClear) {
    SimpleMovingAverage<int, int> underTest{2};
    underTest.add(4);
    ASSERT_EQ(underTest.getAverage(), 4);
    underTest.clear();
    EXPECT_EQ(underTest.getAverage(), 0);
}

TEST(UnitUtils, SimpleMovingAverage_bufferIsEmptyAfterClear) {
    SimpleMovingAverage<int, int> underTest{2};
    underTest.add(4);
    underTest.clear();
    underTest.add(7);
    EXPECT_EQ(underTest.getAverage(), 7);
    underTest.add(13);
    EXPECT_EQ(underTest.getAverage(), 10);
    underTest.add(17);
    EXPECT_EQ(underTest.getAverage(), 15);
}

TEST(UnitUtils, SimpleMovingAverage_indicatesFullnessIffFull) {
    SimpleMovingAverage<int, int> underTest{2};
    EXPECT_FALSE(underTest.isFull());
    underTest.add(0);
    EXPECT_FALSE(underTest.isFull());
    underTest.add(1);
    EXPECT_TRUE(underTest.isFull());
    underTest.add(2);
    EXPECT_TRUE(underTest.isFull());
    underTest.clear();
    EXPECT_FALSE(underTest.isFull());
}
}
