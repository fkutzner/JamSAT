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

#pragma once

#include <libjamsat/utils/Assert.h>

#include <cstdint>

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

/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::TBool
 *
 * \brief The ternary logic truth type
 */
class TBool {
public:
    using UnderlyingType = uint8_t;

    /**
     * \brief Constructs a TBool value equal to TBools::FALSE.
     */
    constexpr TBool() : m_value(0) {}

    TBool(const TBool &other) = default;
    TBool &operator=(const TBool &other) = default;

    constexpr bool operator==(TBool const other) const {
        return (m_value > 1 && other.m_value > 1) || (m_value == other.m_value);
    }

    constexpr bool operator!=(TBool const other) const { return !(*this == other); }

    /**
     * \brief Returns the integer used to represent the TBool value.
     * \returns The integer used to represent the TBool value.
     */
    constexpr UnderlyingType getUnderlyingValue() const { return m_value; }

    /**
     * \brief Creates a TBool object.
     *
     * This method is intended for optimized code. Regularly, the TBools::*
     * constants should be used.
     *
     * \param value If 0, a TBool equal to TBools::FALSE is returned.
     *              If 1, a TBool equal to TBools::TRUE is returned.
     *              Otherwise, a TBool equal to TBools::INDETERMINATE is returned.
     * \returns The TBool object corresponding to \p value.
     */
    constexpr static TBool fromUnderlyingValue(UnderlyingType value) { return TBool{value}; }

private:
    constexpr explicit TBool(UnderlyingType value) : m_value(value) {}
    friend struct TBools;
    UnderlyingType m_value;
};

/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::TBools
 *
 * \brief Values of ternary logic: TRUE, FALSE and INDETERMINATE.
 */
struct TBools {
    static constexpr TBool FALSE{0};
    static constexpr TBool TRUE{1};
    static constexpr TBool INDETERMINATE{2};
};


/**
 * \ingroup JamSAT_Utils
 *
 * \brief Converts a TBool value to a bool value.
 *
 * \param value   The value to be converted. \p value must not be
 * TBools::INDETERMINATE.
 * \returns       true iff \p value equals TBools::TRUE.
 */
bool toBool(TBool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Checks if the given TBool value is determinate.
 *
 * \param value   The TBool value to be checked.
 * \returns       true iff \p value == TBools::INDETERMINATE.
 */
constexpr bool isDeterminate(TBool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Checks if the given TBool value is true.
 *
 * \param value   The TBool value to be checked.
 * \returns       true iff \p value == TBools::TRUE.
 */
constexpr bool isTrue(TBool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Checks if the given TBool value is false.
 *
 * \param value   The TBool value to be checked.
 * \returns       true iff \p value == TBools::FALSE.
 */
constexpr bool isFalse(TBool value);

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Converts a bool value to a TBool value.
 *
 * \param value   The bool value to be converted.
 * \returns       TBools::TRUE iff \p value == true; TBools::FALSE otherwise.
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
 * \returns   TBools::TRUE iff \p a == TBools::FALSE; TBools::FALSE iff \p a ==
 * TBools::TRUE; TBools::INDETERMINATE otherwise.
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
 * \returns     TBools::TRUE if both \p lhs and \p rhs are equal to TBools::TRUE;
 * TBools::FALSE if any of \p lhs and \p rhs are equal to TBools::FALSE;
 * TBools::INDETERMINATE otherwise.
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
 * \returns     TBools::TRUE if any of \p lhs and \p rhs are equal to
 * TBools::TRUE; TBools::FALSE if both \p lhs and \p rhs are equal to
 * TBools::FALSE; TBools::INDETERMINATE otherwise.
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
    JAM_ASSERT(value != TBools::INDETERMINATE, "Can't convert indeterminate TBool to bool");
    return value == TBools::TRUE;
}

constexpr bool isDeterminate(TBool value) {
    return value.getUnderlyingValue() <= 1;
}

constexpr bool isTrue(TBool value) {
    return value.getUnderlyingValue() == TBools::TRUE.getUnderlyingValue();
}

constexpr bool isFalse(TBool value) {
    return value.getUnderlyingValue() == TBools::FALSE.getUnderlyingValue();
}

inline TBool toTBool(bool value) {
    return value ? TBools::TRUE : TBools::FALSE;
}

inline TBool negate(const TBool a) {
    auto raw = a.getUnderlyingValue();
    return TBool::fromUnderlyingValue(raw ^ 1U);
}

inline TBool operator*(const TBool lhs, const TBool rhs) {
    if (lhs == TBools::TRUE && rhs == TBools::TRUE) {
        return TBools::TRUE;
    }
    if (lhs == TBools::FALSE || rhs == TBools::FALSE) {
        return TBools::FALSE;
    }
    return TBools::INDETERMINATE;
}

inline TBool &operator*=(TBool &lhs, const TBool rhs) {
    lhs = lhs * rhs;
    return lhs;
}

inline TBool operator+(const TBool lhs, const TBool rhs) {
    if (lhs == TBools::TRUE || rhs == TBools::TRUE) {
        return TBools::TRUE;
    }
    if (lhs == TBools::FALSE && rhs == TBools::FALSE) {
        return TBools::FALSE;
    }
    return TBools::INDETERMINATE;
}

inline TBool &operator+=(TBool &lhs, const TBool rhs) {
    lhs = lhs + rhs;
    return lhs;
}
}
