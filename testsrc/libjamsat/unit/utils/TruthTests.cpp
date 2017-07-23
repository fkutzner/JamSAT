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

TEST(UnitUtils, TBoolInversionHasFixpointOnIndeterminates) {
  EXPECT_EQ(negate(TBool::INDETERMINATE), TBool::INDETERMINATE);
}

TEST(UnitUtils, TBoolInversionIsDeterminateForDeterminateInputs) {
  EXPECT_EQ(negate(TBool::TRUE), TBool::FALSE);
  EXPECT_EQ(negate(TBool::FALSE), TBool::TRUE);
}

TEST(UnitUtils, TBoolANDIsAnalogousToMin) {
  EXPECT_EQ(TBool::TRUE * TBool::TRUE, TBool::TRUE);
  EXPECT_EQ(TBool::TRUE * TBool::INDETERMINATE, TBool::INDETERMINATE);
  EXPECT_EQ(TBool::FALSE * TBool::INDETERMINATE, TBool::FALSE);
  EXPECT_EQ(TBool::FALSE * TBool::FALSE, TBool::FALSE);
}

TEST(UnitUtils, TBoolANDIsCommutative) {
  EXPECT_EQ(TBool::TRUE * TBool::INDETERMINATE,
            TBool::INDETERMINATE * TBool::TRUE);
  EXPECT_EQ(TBool::TRUE * TBool::FALSE, TBool::FALSE * TBool::TRUE);
  EXPECT_EQ(TBool::FALSE * TBool::INDETERMINATE,
            TBool::INDETERMINATE * TBool::FALSE);
}

TEST(UnitUtils, TBoolORIsAnalogousToMax) {
  EXPECT_EQ(TBool::TRUE + TBool::TRUE, TBool::TRUE);
  EXPECT_EQ(TBool::TRUE + TBool::INDETERMINATE, TBool::TRUE);
  EXPECT_EQ(TBool::FALSE + TBool::INDETERMINATE, TBool::INDETERMINATE);
  EXPECT_EQ(TBool::FALSE + TBool::FALSE, TBool::FALSE);
}

TEST(UnitUtils, TBoolORIsCommutative) {
  EXPECT_EQ(TBool::TRUE + TBool::INDETERMINATE,
            TBool::INDETERMINATE + TBool::TRUE);
  EXPECT_EQ(TBool::TRUE + TBool::FALSE, TBool::FALSE + TBool::TRUE);
  EXPECT_EQ(TBool::FALSE + TBool::INDETERMINATE,
            TBool::INDETERMINATE + TBool::FALSE);
}

TEST(UnitUtils, TBoolIsConvertibleToRawBool) {
  EXPECT_EQ(toRawBool(TBool::TRUE), true);
  EXPECT_EQ(toRawBool(TBool::FALSE), false);
}

TEST(UnitUtils, RawBoolIsConvertibleToTBool) {
  EXPECT_EQ(toTBool(true), TBool::TRUE);
  EXPECT_EQ(toTBool(false), TBool::FALSE);
}

TEST(UnitUtils, TBoolCompoundAssignmentANDisAND) {
  TBool lhs = TBool::TRUE;
  lhs *= TBool::FALSE;
  EXPECT_EQ(lhs, TBool::FALSE);

  lhs = TBool::INDETERMINATE;
  lhs *= TBool::TRUE;
  EXPECT_EQ(lhs, TBool::INDETERMINATE);
}

TEST(UnitUtils, TBoolCompoundAssignmentORisOR) {
  TBool lhs = TBool::TRUE;
  lhs += TBool::FALSE;
  EXPECT_EQ(lhs, TBool::TRUE);

  lhs = TBool::INDETERMINATE;
  lhs += TBool::FALSE;
  EXPECT_EQ(lhs, TBool::INDETERMINATE);
}

TEST(UnitUtils, TBoolDeterminacyCheck) {
  EXPECT_TRUE(isDeterminate(TBool::TRUE));
  EXPECT_TRUE(isDeterminate(TBool::FALSE));
  EXPECT_FALSE(isDeterminate(TBool::INDETERMINATE));
}
}
