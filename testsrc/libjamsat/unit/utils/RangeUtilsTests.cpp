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
#include <libjamsat/utils/RangeUtils.h>

#include <algorithm>
#include <vector>

namespace jamsat {
TEST(UnitUtils, withoutRedundanciesComputesEmptyVectorForEmptyInput) {
    std::vector<int> empty;
    auto reduced = withoutRedundancies(empty.begin(), empty.end());
    EXPECT_TRUE(reduced.empty());
}

TEST(UnitUtils, withoutRedundanciesRetainsNonredundantItems) {
    std::vector<float> input{1.0f, 2.0f, -1.0f};
    auto reduced = withoutRedundancies(input.begin(), input.end());
    ASSERT_EQ(reduced.size(), input.size());
    EXPECT_TRUE(std::is_permutation(reduced.begin(), reduced.end(), input.begin()));
}

TEST(UnitUtils, withoutRedundanciesOmitsRedundantItems) {
    std::vector<float> input{1.0f, 2.0f, -1.0f, 2.0f, 1.0f};
    auto reduced = withoutRedundancies(input.begin(), input.end());
    ASSERT_EQ(reduced.size(), 3ULL);

    std::vector<float> expected{1.0f, 2.0f, -1.0f};
    EXPECT_TRUE(std::is_permutation(reduced.begin(), reduced.end(), expected.begin()));
}
}
