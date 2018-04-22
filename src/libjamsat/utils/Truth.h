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

#include <libjamsat/utils/Assert.h>

namespace jamsat {
/**
 * \ingroup JamSAT_Utils
 *
 * \enum jamsat::Bool
 * \brief A byte-based boolean datatype.
 */
enum class Bool : uint8_t {
    /// The "false" value.
    FALSE = 0,

    /// The "true" value.
    TRUE = 1
};

/**
 * \brief Converts a Bool value to a bool value.
 *
 * \param value The value to be converted.
 * \returns Bool::TRUE iff value == true, Bool::FALSE otherwise.
 */
inline bool toRawBool(Bool value) {
    return value == Bool::TRUE ? true : false;
}

/**
 * \brief Converts a bool value to a Bool value.
 *
 * \param value The value to be converted.
 * \returns true iff value == Bool::TRUE, false otherwise.
 */
inline Bool toBool(bool rawValue) {
    return rawValue ? Bool::TRUE : Bool::FALSE;
}

// TODO: refactor towards TBool class, representing INDETERMINATE both by
// values 3 and 4 for better trail/propagation performance
/**
 * \ingroup JamSAT_Utils
 *
 * \enum jamsat::TBool
 *
 * \brief Values of three-valued logic: TRUE, FALSE and INDETERMINATE.
 */
enum class TBool : uint8_t {
    /// The "false" value.
    FALSE = 0,

    /// The "true" value.
    TRUE = 1,

    /// The indeterminate value.
    INDETERMINATE = 2
};

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Converts a TBool value to a bool value.
 *
 * \param value   The value to be converted. \p value must not be
 * TBool::INDETERMINATE.
 * \returns       true iff \p value equals TBool::TRUE.
 */
bool toBool(TBool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Checks if the given TBool value is determinate.
 *
 * \param value   The TBool value to be checked.
 * \returns       true iff \p value == TBool::INDETERMINATE.
 */
constexpr bool isDeterminate(TBool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Checks if the given TBool value is true.
 *
 * \param value   The TBool value to be checked.
 * \returns       true iff \p value == TBool::TRUE.
 */
constexpr bool isTrue(TBool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Checks if the given TBool value is false.
 *
 * \param value   The TBool value to be checked.
 * \returns       true iff \p value == TBool::FALSE.
 */
constexpr bool isFalse(TBool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Converts a bool value to a TBool value.
 *
 * \param value   The bool value to be converted.
 * \returns       TBool::TRUE iff \p value == true; TBool::FALSE otherwise.
 */
TBool toTBool(bool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Negates the given TBool value.
 *
 * This negation operator is defined as in Kleene's strong logic of
 * indeterminacy.
 *
 * \param a   The value to be inverted.
 * \returns   TBool::TRUE iff \p a == TBool::FALSE; TBool::FALSE iff \p a ==
 * TBool::TRUE; TBool::INDETERMINATE otherwise.
 */
TBool negate(const TBool a);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief AND operator for TBool values.
 *
 * This AND operator is defined as in Kleene's strong logic of indeterminacy.
 *
 * Note: Since overloading operator&& and operator|| would cause behaviour which
 * would violate the principle of least surprise (non-bool return value, no
 * short circuiting), the operators for three-valued logic are implemented using
 * the common alternative notation ("*" for "and", "+" for "or")
 *
 * \param lhs   The left-hand-side argument.
 * \param rhs   The right-hand-side argument.
 * \returns     TBool::TRUE if both \p lhs and \p rhs are equal to TBool::TRUE;
 * TBool::FALSE if any of \p lhs and \p rhs are equal to TBool::FALSE;
 * TBool::INDETERMINATE otherwise.
 */
TBool operator*(const TBool lhs, const TBool rhs);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Compound assignment AND operator for TBool values.
 *
 * This operator assigns \p lhs * \p rhs to \p lhs.
 *
 * Note: Since overloading operator&& and operator|| would cause behaviour which
 * would violate the principle of least surprise (non-bool return value, no
 * short circuiting), the operators for three-valued logic are implemented using
 * the common alternative notation ("*" for "and", "+" for "or")
 *
 * \param lhs   The left-hand-side argument.
 * \param rhs   The right-hand-side argument.
 * \returns     A reference to \p lhs.
 */
TBool &operator*=(const TBool &lhs, const TBool &rhs);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief OR operator for TBool values.
 *
 * This OR operator is defined as in Kleene's strong logic of indeterminacy.
 *
 * Note: Since overloading operator&& and operator|| would cause behaviour which
 * would violate the principle of least surprise (non-bool return value, no
 * short circuiting), the operators for three-valued logic are implemented using
 * the common alternative notation ("*" for "and", "+" for "or")
 *
 * \param lhs   The left-hand-side argument.
 * \param rhs   The right-hand-side argument.
 * \returns     TBool::TRUE if any of \p lhs and \p rhs are equal to
 * TBool::TRUE; TBool::FALSE if both \p lhs and \p rhs are equal to
 * TBool::FALSE; TBool::INDETERMINATE otherwise.
 */
TBool operator+(const TBool lhs, const TBool rhs);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Compound assignment OR operator for TBool values.
 *
 * This operator assigns \p lhs + \p rhs to \p lhs.
 *
 * Note: Since overloading operator&& and operator|| would cause behaviour which
 * would violate the principle of least surprise (non-bool return value, no
 * short circuiting), the operators for three-valued logic are implemented using
 * the common alternative notation ("*" for "and", "+" for "or")
 *
 * \param lhs   The left-hand-side argument.
 * \param rhs   The right-hand-side argument.
 * \returns     A reference to \p lhs.
 */
TBool &operator+=(const TBool &lhs, const TBool &rhs);

/********** Implementation ****************************** */

inline bool toRawBool(TBool value) {
    JAM_ASSERT(value != TBool::INDETERMINATE, "Can't convert indeterminate TBool to bool");
    return value == TBool::TRUE;
}

constexpr bool isDeterminate(TBool value) {
    return value != TBool::INDETERMINATE;
}

constexpr bool isTrue(TBool value) {
    return value == TBool::TRUE;
}

constexpr bool isFalse(TBool value) {
    return value == TBool::FALSE;
}

inline TBool toTBool(bool value) {
    return value ? TBool::TRUE : TBool::FALSE;
}

inline TBool negate(const TBool a) {
    switch (a) {
    case TBool::INDETERMINATE:
        return TBool::INDETERMINATE;
    case TBool::TRUE:
        return TBool::FALSE;
    case TBool::FALSE:
        return TBool::TRUE;
    default:
        JAM_ASSERT(false, "Detected 4th TBool value when only 3 should exist");
    }
}

inline TBool operator*(const TBool lhs, const TBool rhs) {
    if (lhs == TBool::TRUE && rhs == TBool::TRUE) {
        return TBool::TRUE;
    }
    if (lhs == TBool::FALSE || rhs == TBool::FALSE) {
        return TBool::FALSE;
    }
    return TBool::INDETERMINATE;
}

inline TBool &operator*=(TBool &lhs, const TBool rhs) {
    lhs = lhs * rhs;
    return lhs;
}

inline TBool operator+(const TBool lhs, const TBool rhs) {
    if (lhs == TBool::TRUE || rhs == TBool::TRUE) {
        return TBool::TRUE;
    }
    if (lhs == TBool::FALSE && rhs == TBool::FALSE) {
        return TBool::FALSE;
    }
    return TBool::INDETERMINATE;
}

inline TBool &operator+=(TBool &lhs, const TBool rhs) {
    lhs = lhs + rhs;
    return lhs;
}
}
