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
#include <vector>

#include <libjamsat/utils/BoundedMap.h>
#include <toolbox/testutils/RangeUtils.h>

namespace jamsat {
namespace {
class IntIndex {
public:
  using Type = int;
  static std::size_t getIndex(int i) { return static_cast<std::size_t>(i); }
};
}

TEST(UnitUtils, boundedMapSizeIsIndependentOfInsertions)
{
  BoundedMap<int, double, IntIndex> underTest{99};
  EXPECT_EQ(underTest.size(), 100ull);
  underTest[1] = 2.0f;
  EXPECT_EQ(underTest.size(), 100ull);
}

TEST(UnitUtils, boundedMapStoresValues)
{
  BoundedMap<int, double, IntIndex> underTest{10};
  underTest[1] = 2.0f;
  underTest[1] = 23.0f;
  underTest[2] = 223.0f;
  EXPECT_EQ(underTest[1], 23.0f);
  EXPECT_EQ(underTest[2], 223.0f);
}

TEST(UnitUtils, boundedMapReturnsDefaultValueForUnusedKeys)
{
  BoundedMap<int, double, IntIndex> underTest{10};
  EXPECT_EQ(underTest[1], 0.0f);
  EXPECT_EQ(underTest[2], 0.0f);
}

TEST(UnitUtils, boundedMapSizeIsIncreasable)
{
  BoundedMap<int, double, IntIndex> underTest{10};
  underTest[10] = 1.0f;
  underTest.increaseSizeTo(13);
  underTest[13] = 2.0f;
  EXPECT_EQ(underTest[10], 1.0f);
  EXPECT_EQ(underTest[13], 2.0f);
}

TEST(UnitUtils, boundedMapInitializesStorageWithDefaultValues)
{
  BoundedMap<int, double, IntIndex> underTest{5, 2.0f};
  EXPECT_EQ(underTest[4], 2.0f);
  underTest.increaseSizeTo(20);
  EXPECT_EQ(underTest[19], 2.0f);
}

TEST(UnitUtils, boundedMapValueRangeContainsDefaultValuesAfterConstruction)
{
  BoundedMap<int, double, IntIndex> underTest{4, 2.0f};
  expectRangeContainsValues(underTest.values(), std::vector<double>{2.0f, 2.0f, 2.0f, 2.0f, 2.0f});
}

TEST(UnitUtils, boundedMapValueRangeContainsExactlyTheValues)
{
  BoundedMap<int, double, IntIndex> underTest{2, 2.0f};
  underTest[0] = 3.0f;
  underTest[1] = 1.0f;
  underTest[2] = 2.0f;
  std::vector<double> expected{1.0f, 2.0f, 3.0f};
  expectRangeContainsValues(underTest.values(), expected);
  expectRangeContainsValues(const_cast<const decltype(underTest)*>(&underTest)->values(), expected);
}
}
