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
#include <stdexcept>

#include <libjamsat/utils/FaultInjector.h>

namespace jamsat {
TEST(UnitUtils, noTestFaultThrowsAreInjectedByDefault) {
  throwOnInjectedTestFault<std::logic_error>("logic errors",
                                             "exception_what_msg");
}

TEST(UnitUtils, enabledTestFaultThrowsAreExecuted) {
  FaultInjectorResetRAII faultInjectorResetter;
  auto &faultInjector = FaultInjector::getInstance();
  faultInjector.enableFaults("logic errors");
  EXPECT_TRUE(faultInjector.isFaultEnabled("logic errors"));
  try {
    throwOnInjectedTestFault<std::logic_error>("logic errors",
                                               "exception_what_msg");
    FAIL() << "Expected exception to be thrown";
  } catch (std::logic_error &exception) {
    EXPECT_EQ(exception.what(), std::string("exception_what_msg"));
  } catch (...) {
    FAIL() << "Wrong exception type";
  }
}

TEST(UnitUtils, faultInjectorResetRAIIRestoresEnabledFaults) {
  FaultInjectorResetRAII faultInjectorResetter;

  auto &faultInjector = FaultInjector::getInstance();
  faultInjector.enableFaults("fooFaults");

  {
    FaultInjectorResetRAII resetRAII;
    faultInjector.reset();
  }

  ASSERT_NE(faultInjector.begin(), faultInjector.end());
  auto faultInjectorIter = faultInjector.begin();
  EXPECT_EQ(++faultInjectorIter, faultInjector.end());
  EXPECT_EQ(*(faultInjector.begin()), std::string{"fooFaults"});
}
}
