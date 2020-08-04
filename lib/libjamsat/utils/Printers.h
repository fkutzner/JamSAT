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
 * \file utils/Printers.h
 * \brief Debug output, formatters etc.
 */

#pragma once

#include <sstream>
#include <string>

namespace jamsat {

/**
 * \brief Prints a sequence of objects to an std::string.
 *
 * \ingroup JamSAT_Utils
 *
 * \param begin       The sequence's begin iterator.
 * \param end         The sequence's end iterator.
 * \tparam InputIt    A type satisfying the InputIterator concept. For an object `i` of type
 *                    `CNFLitIt` and an `std::ostream` `o`, `o << *i` must be a valid expression.
 */
template <typename CNFLitIt>
std::string toString(CNFLitIt begin, CNFLitIt end);

/**
 * \brief Helper function calling to_string defined in the JamSAT
 *   namespace or, if no such function is defined for `T`, forwards
 *   the call to `std::to_string()`.
 * 
 * \ingroup JamSAT_Utils
 * 
 * \param item      The item to convert to std::string.
 */
template <typename T>
auto to_string(T&& item) -> std::string;

/********** Implementation ****************************** */

template <typename CNFLitIt>
std::string toString(CNFLitIt begin, CNFLitIt end) {
    std::stringstream output;

    if (begin == end) {
        return "";
    }
    output << *begin;
    ++begin;

    while (begin != end) {
        output << " " << *begin;
        ++begin;
    }

    return output.str();
}

namespace jamsat_printers_detail {
using std::to_string;

template <typename T>
auto toString(T&& item) -> std::string {
    return to_string(std::forward<T>(item));
}
}

template <typename T>
auto to_string(T&& item) -> std::string {
    return jamsat_printers_detail::toString(std::forward<T>(item));
}
}
