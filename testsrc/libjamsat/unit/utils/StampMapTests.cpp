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

#include <complex>
#include <cstdint>

#include <libjamsat/utils/StampMap.h>

namespace jamsat {

struct IntStampKey {
    using Type = int;
    static size_t getIndex(int x) { return (x >= 0) ? 2 * x : -2 * x + 1; }
};

struct ComplexStampKey {
    using Type = std::complex<char>;
    static size_t getIndex(const Type &x) { return 2 * x.real() + 2 * x.imag() + 1; }
};

TEST(UnitUtils, StampMap_singleKeyTypeReadWrite) {
    StampMap<uint64_t, IntStampKey> underTest{10};
    auto stampContext = underTest.createContext();
    auto stamp = stampContext.getStamp();

    EXPECT_FALSE(underTest.isStamped(3, stamp));
    EXPECT_FALSE(underTest.isStamped(4, stamp));
    underTest.setStamped(3, stamp, true);
    EXPECT_TRUE(underTest.isStamped(3, stamp));
    EXPECT_FALSE(underTest.isStamped(4, stamp));
    underTest.setStamped(3, stamp, false);
    EXPECT_FALSE(underTest.isStamped(3, stamp));
    EXPECT_FALSE(underTest.isStamped(4, stamp));
}

TEST(UnitUtils, StampMap_twoKeyTypesReadWrite) {
    StampMap<uint64_t, ComplexStampKey, IntStampKey> underTest{32};
    auto stampContext = underTest.createContext();
    auto stamp = stampContext.getStamp();

    ComplexStampKey::Type testValue1{3, 0};
    ComplexStampKey::Type testValue2{2, 5};

    EXPECT_FALSE(underTest.isStamped(3, stamp));
    EXPECT_FALSE(underTest.isStamped(testValue1, stamp));
    EXPECT_FALSE(underTest.isStamped(testValue2, stamp));

    underTest.setStamped(testValue1, stamp, true);
    EXPECT_TRUE(underTest.isStamped(testValue1, stamp));
    EXPECT_TRUE(underTest.isStamped(3, stamp)); // mapped to same internal key
    EXPECT_FALSE(underTest.isStamped(testValue2, stamp));
}

TEST(UnitUtils, StampMap_contextDestructionClearsStampMap) {
    StampMap<uint8_t, ComplexStampKey, IntStampKey> underTest{32};
    ComplexStampKey::Type testValue1{3, 0};

    {
        auto stampContext = underTest.createContext();
        auto stamp = stampContext.getStamp();
        underTest.setStamped(testValue1, stamp, true);
        EXPECT_TRUE(underTest.isStamped(testValue1, stamp));
        // stampContext should clean the stamps on destruction
    }

    {
        auto newContext = underTest.createContext();
        auto newStamp = newContext.getStamp();
        EXPECT_FALSE(underTest.isStamped(testValue1, newStamp));
    }
}

TEST(UnitUtils, StampMap_stampMapIsClearedOnInnerStampWraparound) {
    StampMap<uint8_t, ComplexStampKey, IntStampKey> underTest{32};
    ComplexStampKey::Type testValue1{3, 0};
    {
        auto stampContext = underTest.createContext();
        auto stamp = stampContext.getStamp();
        underTest.setStamped(testValue1, stamp, true);
    }

    // The inner stamp type is incremented for each new context, except
    // when its maximum is reached - then, all saved stamping information
    // needs to be cleared. underTest has a maximum inner stamp value of
    // 255.
    for (int i = 0; i < 384; ++i) {
        auto stampContext = underTest.createContext();
        auto stamp = stampContext.getStamp();
        EXPECT_FALSE(underTest.isStamped(testValue1, stamp));
    }
}
}
