/* Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)

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

/**
 * \file utils/Casts.h
 * \brief Checked integral-type casts
 */

#pragma once

#include <type_traits>

#include <libjamsat/utils/Assert.h>

#if defined(JAM_ASSERT_ENABLED)
#include <cstdint>
#include <limits>
#endif

namespace jamsat {
/**
 * \ingroup JamSAT_Utils
 *
 * \brief A static_cast with bounds checking when assertions are enabled.
 *
 * \param value       The value to be casted to \p ToType.
 *
 * \tparam ToType     The target integral type.
 * \tparam FromType   Any type that is integral or implicitly convertible to an integral type.
 */
template <typename ToType, typename FromType>
auto static_checked_cast(FromType value) ->
    typename std::enable_if<std::is_integral<ToType>::value, ToType>::type {
#if defined(JAM_ASSERT_ENABLED)
    if (value >= 0) {
        uintmax_t i = static_cast<uintmax_t>(value);
        uintmax_t max = static_cast<uintmax_t>(std::numeric_limits<ToType>::max());
        JAM_ASSERT(i <= max, "Loss of precision caused loss of data");
    } else {
        intmax_t i = static_cast<intmax_t>(value);
        intmax_t min = static_cast<intmax_t>(std::numeric_limits<ToType>::min());
        JAM_ASSERT(std::numeric_limits<ToType>::min() < 0,
                   "Cast of a negative integer to an unsigned type failed");
        JAM_ASSERT(i >= min, "Loss of precision caused loss of data");
    }
#endif
    return static_cast<ToType>(value);
}
}
