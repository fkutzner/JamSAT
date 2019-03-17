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

/**
 * \file utils/TraitUtils.h
 * \brief Utilities for creating type traits
 */

#pragma once

#include <type_traits>

/** \file */

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Preprocessor macro that evaluates to a type if and only if a given expression has the
 *        specified type.
 *
 * \param expr  An expression.
 * \param ty    A type.
 *
 * If \p expr has type \p ty, `JAM_REQUIRE_EXPR(expr, ty)` has type `void`. Otherwise, the
 * expression `JAM_REQUIRE_EXPR(expr, ty)` is ill-formed. This macro is intended to be used in
 * SFINAE constructs, inducing a substitution failure if `expr` is ill-formed, `ty` is ill-formed
 * or `decltype(expr)` is not the same type as `ty`.
 */
#define JAM_REQUIRE_EXPR(expr, ty) std::enable_if_t<std::is_same<decltype(expr), ty>::value, void>

namespace jamsat {
/**
 * \ingroup JamSAT_Utils
 *
 * \brief Utility metafunction mapping sequences of any type to the type `void`
 *
 * See https://en.cppreference.com/w/cpp/types/void_t for details about this metafunction.
 * `j_void_t` should be replaced with `std::void_t` when JamSAT moves to C++17.
 */
template <class...>
using j_void_t = void;
}