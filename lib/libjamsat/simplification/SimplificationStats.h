/* Copyright (c) 2018, 2019 Felix Kutzner (github.com/fkutzner)

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

#include <cstdint>

namespace jamsat {
/**
 * \defgroup JamSAT_Simplification_Unary  Unary-based Optimizations
 * \ingroup JamSAT_Simplification
 *
 * This submodule contains functions for strengthening or deleting
 * clauses containing literals with a fixed value.
 */


/**
 * \brief Simplification statistics
 *
 * \ingroup JamSAT_Simplification
 */
struct SimplificationStats {
    uint32_t amntClausesRemovedBySubsumption = 0;
    uint32_t amntClausesStrengthened = 0;
    uint32_t amntLiteralsRemovedByStrengthening = 0;
    uint32_t amntUnariesLearnt = 0;
};

auto operator+(SimplificationStats const& lhs, SimplificationStats const& rhs) noexcept
    -> SimplificationStats;
auto operator+=(SimplificationStats& lhs, SimplificationStats const& rhs) noexcept
    -> SimplificationStats&;

/********** Implementation ****************************** */

inline auto operator+(SimplificationStats const& lhs, SimplificationStats const& rhs) noexcept
    -> SimplificationStats {
    SimplificationStats result;
    result.amntClausesRemovedBySubsumption =
        lhs.amntClausesRemovedBySubsumption + rhs.amntClausesRemovedBySubsumption;
    result.amntClausesStrengthened = lhs.amntClausesStrengthened + rhs.amntClausesStrengthened;
    result.amntLiteralsRemovedByStrengthening =
        lhs.amntLiteralsRemovedByStrengthening + rhs.amntLiteralsRemovedByStrengthening;
    result.amntUnariesLearnt = lhs.amntUnariesLearnt + rhs.amntUnariesLearnt;
    return result;
}

inline auto operator+=(SimplificationStats& lhs, SimplificationStats const& rhs) noexcept
    -> SimplificationStats& {
    lhs = lhs + rhs;
    return lhs;
}
}