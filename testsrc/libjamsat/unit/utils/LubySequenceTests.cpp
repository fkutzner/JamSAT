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
#include <libjamsat/utils/LubySequence.h>

#include <vector>

namespace jamsat {
TEST(UnitUtils, LubySequence_firstElementsMatchLubySequence)
{
  // reference data taken from https://oeis.org/search?q=luby
  std::vector<LubySequence::Element> initialLubySegment = {1, 1, 2, 1, 1, 2, 4, 1, 1,  2, 1,
                                                           1, 2, 4, 8, 1, 1, 2, 1, 1,  2, 4,
                                                           1, 1, 2, 1, 1, 2, 4, 8, 16, 1};

  LubySequence underTest;
  std::vector<LubySequence::Element> result;
  for (decltype(initialLubySegment)::size_type i = 0; i < initialLubySegment.size(); ++i) {
    result.push_back(underTest.current());
    auto nextElement = underTest.next();
    ASSERT_EQ(nextElement, underTest.current());
  }

  EXPECT_EQ(result, initialLubySegment);
}
}
