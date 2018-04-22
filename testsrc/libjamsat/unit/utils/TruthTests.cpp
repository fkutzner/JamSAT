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

#include <libjamsat/utils/Truth.h>

namespace jamsat {

TEST(UnitUtils, DefaultConstructedTBoolIsEqualToTBoolsFALSE) {
    EXPECT_EQ(TBool{}, TBools::FALSE);
}

TEST(UnitUtils, TBoolConstructedWithValue0IsEqualToTBoolsFALSE) {
    EXPECT_EQ(TBool::fromUnderlyingValue(0), TBools::FALSE);
}

TEST(UnitUtils, TBoolConstructedWithValue1IsEqualToTBoolsTRUE) {
    EXPECT_EQ(TBool::fromUnderlyingValue(1), TBools::TRUE);
}

TEST(UnitUtils, TBoolConstructedWithValueGreater1IsEqualToTBoolsINDETERMINATE) {
    EXPECT_EQ(TBool::fromUnderlyingValue(2), TBools::INDETERMINATE);
    EXPECT_EQ(TBool::fromUnderlyingValue(5), TBools::INDETERMINATE);
}

TEST(UnitUtils, TBoolConstantsAreUnequal) {
    EXPECT_FALSE(TBools::TRUE == TBools::FALSE);
    EXPECT_TRUE(TBools::TRUE != TBools::FALSE);
    EXPECT_FALSE(TBools::FALSE == TBools::TRUE);
    EXPECT_TRUE(TBools::FALSE != TBools::TRUE);

    EXPECT_FALSE(TBools::TRUE == TBools::INDETERMINATE);
    EXPECT_TRUE(TBools::TRUE != TBools::INDETERMINATE);
    EXPECT_FALSE(TBools::INDETERMINATE == TBools::TRUE);
    EXPECT_TRUE(TBools::INDETERMINATE != TBools::TRUE);

    EXPECT_FALSE(TBools::FALSE == TBools::INDETERMINATE);
    EXPECT_TRUE(TBools::FALSE != TBools::INDETERMINATE);
    EXPECT_FALSE(TBools::INDETERMINATE == TBools::FALSE);
    EXPECT_TRUE(TBools::INDETERMINATE != TBools::FALSE);
}

TEST(UnitUtils, TBoolIsEqualToSelf) {
    EXPECT_TRUE(TBools::TRUE == TBools::TRUE);
    EXPECT_FALSE(TBools::TRUE != TBools::TRUE);
}

TEST(UnitUtils, TBoolInversionHasFixpointOnIndeterminates) {
    EXPECT_EQ(negate(TBools::INDETERMINATE), TBools::INDETERMINATE);
}

TEST(UnitUtils, TBoolInversionIsDeterminateForDeterminateInputs) {
    EXPECT_EQ(negate(TBools::TRUE), TBools::FALSE);
    EXPECT_EQ(negate(TBools::FALSE), TBools::TRUE);
}

TEST(UnitUtils, TBoolANDIsAnalogousToMin) {
    EXPECT_EQ(TBools::TRUE * TBools::TRUE, TBools::TRUE);
    EXPECT_EQ(TBools::TRUE * TBools::INDETERMINATE, TBools::INDETERMINATE);
    EXPECT_EQ(TBools::FALSE * TBools::INDETERMINATE, TBools::FALSE);
    EXPECT_EQ(TBools::FALSE * TBools::FALSE, TBools::FALSE);
}

TEST(UnitUtils, TBoolANDIsCommutative) {
    EXPECT_EQ(TBools::TRUE * TBools::INDETERMINATE, TBools::INDETERMINATE * TBools::TRUE);
    EXPECT_EQ(TBools::TRUE * TBools::FALSE, TBools::FALSE * TBools::TRUE);
    EXPECT_EQ(TBools::FALSE * TBools::INDETERMINATE, TBools::INDETERMINATE * TBools::FALSE);
}

TEST(UnitUtils, TBoolORIsAnalogousToMax) {
    EXPECT_EQ(TBools::TRUE + TBools::TRUE, TBools::TRUE);
    EXPECT_EQ(TBools::TRUE + TBools::INDETERMINATE, TBools::TRUE);
    EXPECT_EQ(TBools::FALSE + TBools::INDETERMINATE, TBools::INDETERMINATE);
    EXPECT_EQ(TBools::FALSE + TBools::FALSE, TBools::FALSE);
}

TEST(UnitUtils, TBoolORIsCommutative) {
    EXPECT_EQ(TBools::TRUE + TBools::INDETERMINATE, TBools::INDETERMINATE + TBools::TRUE);
    EXPECT_EQ(TBools::TRUE + TBools::FALSE, TBools::FALSE + TBools::TRUE);
    EXPECT_EQ(TBools::FALSE + TBools::INDETERMINATE, TBools::INDETERMINATE + TBools::FALSE);
}

TEST(UnitUtils, TBoolIsConvertibleToRawBool) {
    EXPECT_EQ(toRawBool(TBools::TRUE), true);
    EXPECT_EQ(toRawBool(TBools::FALSE), false);
}

TEST(UnitUtils, RawBoolIsConvertibleToTBool) {
    EXPECT_EQ(toTBool(true), TBools::TRUE);
    EXPECT_EQ(toTBool(false), TBools::FALSE);
}

TEST(UnitUtils, TBoolCompoundAssignmentANDisAND) {
    TBool lhs = TBools::TRUE;
    lhs *= TBools::FALSE;
    EXPECT_EQ(lhs, TBools::FALSE);

    lhs = TBools::INDETERMINATE;
    lhs *= TBools::TRUE;
    EXPECT_EQ(lhs, TBools::INDETERMINATE);
}

TEST(UnitUtils, TBoolCompoundAssignmentORisOR) {
    TBool lhs = TBools::TRUE;
    lhs += TBools::FALSE;
    EXPECT_EQ(lhs, TBools::TRUE);

    lhs = TBools::INDETERMINATE;
    lhs += TBools::FALSE;
    EXPECT_EQ(lhs, TBools::INDETERMINATE);
}

TEST(UnitUtils, TBoolDeterminacyCheck) {
    EXPECT_TRUE(isDeterminate(TBools::TRUE));
    EXPECT_TRUE(isDeterminate(TBools::FALSE));
    EXPECT_FALSE(isDeterminate(TBools::INDETERMINATE));
}
}
