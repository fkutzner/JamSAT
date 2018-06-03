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

#include <libjamsat/utils/OverApproximatingSet.h>

#include <cstdint>

namespace jamsat {

struct TestUIntKey {
    using Type = uint32_t;

    constexpr static auto getIndex(uint32_t value) -> size_t { return value; }
};

TEST(UnitUtils, OverApproximatingSetIsEmptyAfterConstruction) {
    OverApproximatingSet<64, TestUIntKey> underTest;
    EXPECT_FALSE(underTest.mightContain(0UL));
    EXPECT_FALSE(underTest.mightContain(1UL));
    EXPECT_FALSE(underTest.mightContain(1048576UL));
}

TEST(UnitUtils, OverApproximatingSetDefinitelyContainsValueAfterInsert) {
    OverApproximatingSet<64, TestUIntKey> underTest;
    underTest.insert(0UL);
    EXPECT_TRUE(underTest.mightContain(0UL));

    ASSERT_FALSE(underTest.mightContain(1048577UL)) << "Bad test data";
    underTest.insert(1048577UL);
    EXPECT_TRUE(underTest.mightContain(1048577UL));
}

TEST(UnitUtils, OverApproximatingSetRecognizesRealSubset) {
    OverApproximatingSet<64, TestUIntKey> subset;
    subset.insert(1UL);
    subset.insert(128UL);
    subset.insert(255UL);

    OverApproximatingSet<64, TestUIntKey> superset{subset};
    superset.insert(3UL);

    EXPECT_TRUE(subset.mightBeSubsetOf(superset));
}

TEST(UnitUtils, OverApproximatingSetRecognizesDefinitelyNotSubset) {
    OverApproximatingSet<64, TestUIntKey> setA;
    setA.insert(1UL);
    setA.insert(128UL);
    setA.insert(255UL);

    OverApproximatingSet<64, TestUIntKey> setB{setA};
    setB.insert(3UL); // gets mapped to a bit that is not set in setA

    EXPECT_FALSE(setB.mightBeSubsetOf(setA));
}

TEST(UnitUtils, OverApproximatingSetIsEmptyAfterClear) {
    OverApproximatingSet<64, TestUIntKey> underTest;
    for (unsigned int i = 0; i < 64; ++i) {
        underTest.insert(i);
    }
    underTest.clear();
    for (unsigned int i = 0; i < 64; ++i) {
        EXPECT_FALSE(underTest.mightContain(i)) << "Set not empty; contains " << i;
    }
}
}
