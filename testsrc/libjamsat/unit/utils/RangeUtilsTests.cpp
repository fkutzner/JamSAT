/* Copyright (c) 2018,2020 Felix Kutzner (github.com/fkutzner)

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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libjamsat/utils/RangeUtils.h>

#include <algorithm>
#include <vector>

using ::testing::ContainerEq;
using ::testing::Eq;
using ::testing::IsEmpty;

namespace jamsat {
TEST(UnitUtils, withoutRedundanciesComputesEmptyVectorForEmptyInput)
{
  std::vector<int> empty;
  auto reduced = withoutRedundancies(empty.begin(), empty.end());
  EXPECT_TRUE(reduced.empty());
}

TEST(UnitUtils, withoutRedundanciesRetainsNonredundantItems)
{
  std::vector<float> input{1.0f, 2.0f, -1.0f};
  auto reduced = withoutRedundancies(input.begin(), input.end());
  ASSERT_EQ(reduced.size(), input.size());
  EXPECT_TRUE(std::is_permutation(reduced.begin(), reduced.end(), input.begin()));
}

TEST(UnitUtils, withoutRedundanciesOmitsRedundantItems)
{
  std::vector<float> input{1.0f, 2.0f, -1.0f, 2.0f, 1.0f};
  auto reduced = withoutRedundancies(input.begin(), input.end());
  ASSERT_EQ(reduced.size(), 3ULL);

  std::vector<float> expected{1.0f, 2.0f, -1.0f};
  EXPECT_TRUE(std::is_permutation(reduced.begin(), reduced.end(), expected.begin()));
}

TEST(UnitUtils, swapWithLastElement_WhenVecIsEmpty_NothingIsMoved)
{
  std::vector<int> empty;
  std::size_t result = swapWithLastElement(empty, 1);
  EXPECT_THAT(result, Eq(0));
  EXPECT_THAT(empty, IsEmpty());
}

TEST(UnitUtils, swapWithLastElement_WhenVecDoesNotContainElement_NothingIsMoved)
{
  std::vector<int> testInput = {3, 4, 5};
  std::vector<int> originalTestInput = testInput;

  std::size_t result = swapWithLastElement(testInput, 1);
  EXPECT_THAT(result, Eq(0));
  EXPECT_THAT(testInput, ContainerEq(originalTestInput));
}

TEST(UnitUtils, swapWithLastElement_WhenVecContainsElementOnce_ItIsMovedToEnd)
{
  std::vector<int> testInput{3, 1, 5};
  std::vector<int> expectedResult{3, 5, 1};

  std::size_t result = swapWithLastElement(testInput, 1);
  EXPECT_THAT(result, Eq(1));
  EXPECT_THAT(testInput, ContainerEq(expectedResult));
}

TEST(UnitUtils, swapWithLastElement_WhenVecContainsElementMoreThanOnce_ThenOnlyFirstOneIsMoved)
{
  std::vector<int> testInput{3, 1, 5, 1, 20};
  std::vector<int> expectedResult{3, 20, 5, 1, 1};

  std::size_t result = swapWithLastElement(testInput, 1);
  EXPECT_THAT(result, Eq(1));
  EXPECT_THAT(testInput, ContainerEq(expectedResult));
}
}
