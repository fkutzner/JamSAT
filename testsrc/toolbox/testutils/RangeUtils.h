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

#pragma once

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace jamsat {
/**
 * \ingroup JamSAT_TestInfrastructure
 *
 * \brief Checks whether a range contains the expected values.
 *
 * The order of the elements in the range is not checked. This function checks (using Google
 * Tests's EXPECT_EQ macro) that all elements in \p r are contained in \p expected and that
 * the amount of elements in \p r is equal to the amount of elements in \p .
 *
 * \param r         A range.
 * \param expected  A container containing the elements expected to occur in \p r.
 *
 * \tparam Range    A range type.
 * \tparam Expected A container type. The elements storable in objects of this type must be
 *                  comparable to the objects obtainable from \p Range objects.
 */
template <typename Range, typename Expected>
void expectRangeContainsValues(const Range &r, const Expected &expected) {
    uint64_t count = 0;
    for (auto &elem : r) {
        EXPECT_NE(std::find(expected.begin(), expected.end(), elem), expected.end())
            << "Element " << elem << " not expected in result range";
        ++count;
    }
    EXPECT_EQ(count, expected.size()) << "Result range larger than expected";
}
}
