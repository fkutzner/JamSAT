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
#include <iostream>

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Casts.h>

namespace jamsat {
namespace {
template <typename ToType, typename FromType>
void test_staticCheckedCastSucceeds(FromType from) {
#if defined(JAM_ASSERT_ENABLED)
    static_checked_cast<ToType>(from);
    // not expecting an exception to be thrown
#else
    (void)from;
    std::cerr << "Warning: static cast checking is disabled, not testing anything" << std::endl;
#endif
}

template <typename ToType, typename FromType>
void test_staticCheckedCastFails(FromType from) {
#if defined(JAM_ASSERT_ENABLED)
    EXPECT_DEATH(static_checked_cast<ToType>(from), ".*");
#else
    (void)from;
    std::cerr << "Warning: static cast checking is disabled, not testing anything" << std::endl;
#endif
}
}

TEST(UnitUtils, staticCheckedCastSucceedsForSameWidthUintToUint) {
    test_staticCheckedCastSucceeds<uint32_t>(uint32_t{4});
}

TEST(UnitUtils, staticCheckedCastSucceedsForValidSmallUintToLargeUint) {
    test_staticCheckedCastSucceeds<uint64_t>(uint32_t{4});
}

TEST(UnitUtils, staticCheckedCastSucceedsForValidLargeUintToSmallUint) {
    test_staticCheckedCastSucceeds<uint32_t>(uint64_t{4});
}

TEST(UnitUtils, staticCheckedCastSucceedsForValidLargeIntToSmallUint) {
    test_staticCheckedCastSucceeds<uint32_t>(int64_t{4});
}

TEST(UnitUtils, staticCheckedCastSucceedsForValidSmallIntToLargeUint) {
    test_staticCheckedCastSucceeds<uint64_t>(int32_t{4});
}

TEST(UnitUtils, staticCheckedCastSucceedsForValidSmallIntToSmallUint) {
    test_staticCheckedCastSucceeds<uint32_t>(int32_t{4});
}

TEST(UnitUtils, staticCheckedCastSucceedsForValidNegLargeIntToNegSmallInt) {
    test_staticCheckedCastSucceeds<int32_t>(int64_t{-4});
}

TEST(UnitUtils, staticCheckedCastSucceedsForValidNegSmallIntToNegLargeInt) {
    test_staticCheckedCastSucceeds<int64_t>(int32_t{-4});
}

TEST(UnitUtils, staticCheckedCastFailsForInvalidUintToUintConversion) {
    test_staticCheckedCastFails<uint16_t>(std::numeric_limits<uint32_t>::max());
}

TEST(UnitUtils, staticCheckedCastFailsForInvalidUintToSameWidthIntConversion) {
    test_staticCheckedCastFails<int32_t>(std::numeric_limits<uint32_t>::max());
}

TEST(UnitUtils, staticCheckedCastFailsForInvalidNegIntToUintConversion) {
    test_staticCheckedCastFails<uint32_t>(std::numeric_limits<int32_t>::min());
}

TEST(UnitUtils, staticCheckedCastFailsForInvalidNegIntToNegIntConversion) {
    test_staticCheckedCastFails<int32_t>(std::numeric_limits<int64_t>::min());
}

namespace {
class ImplicitlyConvertibleToInt {
public:
    ImplicitlyConvertibleToInt(int x) : m_x(x) {}

    operator int() { return m_x; }

private:
    int m_x;
};
}

TEST(UnitUtils, staticCheckedCastSucceedsForSameWidthImplicitConvIntToInt) {
    test_staticCheckedCastSucceeds<int>(ImplicitlyConvertibleToInt{3});
}
}
