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

#include <functional>
#include <limits>
#include <ostream>

#include <libjamsat/utils/Assert.h>

namespace jamsat {
/**
 * \defgroup JamSAT_CNFProblem  JamSAT CNF problem instance data structures
 * This module contains types and data structures for representing CNF problem instances.
 */

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \enum jamsat::CNFSign
 * \brief A sign datatype for CNF literals.
 */
enum class CNFSign {
    /// The negative literal sign.
    NEGATIVE = 0,
    /// The positive literal sign.
    POSITIVE = 1
};

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \brief Inverts the given CNF literal sign.
 *
 * \param sign      The sign to be inverted.
 * \returns         The inverted sign.
 */
constexpr CNFSign invert(CNFSign sign) noexcept {
    int rawSign = static_cast<int>(sign);
    return static_cast<CNFSign>(1 - rawSign);
}

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \class jamsat::CNFVar
 * \brief A CNF variable class.
 *
 * CNFVar is a regular type.
 */
class CNFVar {
public:
    /** The underlying variable type. */
    using RawVariable = uint32_t;

    /**
     * \brief Constructs a CNFVar object.
     *
     * \param variableValue  The non-negative raw variable identifier.
     */
    constexpr explicit CNFVar(RawVariable variableValue) noexcept;

    /**
     * \brief Constructs an undefined variable.
     */
    constexpr CNFVar() noexcept;

    /**
     * \brief Gets the variable's raw value.
     *
     * \returns the variable's raw value.
     */
    constexpr RawVariable getRawValue() const noexcept;

    /**
     * \brief Gets the "undefined" marker variable
     *
     * \returns The undefined marker variable.
     */
    static constexpr CNFVar getUndefinedVariable() noexcept;

    /**
     * \brief Gets the maximal raw value a regular variable can have.
     *
     * \returns The maximal raw value a regular variable can have.
     */
    static constexpr RawVariable getMaxRawValue() noexcept;

    /**
     * \ingroup JamSAT_CNFProblem
     *
     * \brief Index type for CNFVar
     *
     * This type is a model of the concept `Index`.
     */
    class Index {
    public:
        using Type = CNFVar;

        /**
         * \brief Gets an index value for the given variable.
         *
         * \param variable    A variable.
         * \returns           The index for \p variable
         */
        static constexpr std::size_t getIndex(CNFVar variable) {
            return static_cast<std::size_t>(variable.getRawValue());
        }
    };

    CNFVar(const CNFVar &other) = default;
    CNFVar &operator=(const CNFVar &other) = default;
    CNFVar(CNFVar &&other) = default;
    CNFVar &operator=(CNFVar &&other) = default;

private:
    RawVariable m_value;
};

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \class jamsat::CNFLit
 * \brief A CNF literal class.
 *
 * CNFLit is a regular type.
 */
class CNFLit {
public:
    /** The underlying literal type. */
    using RawLiteral = uint32_t;

    /**
     * \brief Constructs a CNFLit object.
     *
     * \param variable  The literal's variable. The variable's value must be
     * nonnegative and smaller than CNFVar::undefinedVariable.
     * \param sign      The literal's sign.
     */
    constexpr CNFLit(CNFVar variable, CNFSign sign) noexcept;

    /**
     * \brief Constructs an undefined literal.
     */
    constexpr CNFLit() noexcept;

    /**
     * \brief Gets the literal's variable.
     *
     * \returns  The literal's variable.
     */
    constexpr CNFVar getVariable() const noexcept;

    /**
     * \brief Gets the literal's sign.
     *
     * \returns  The literal's sign.
     */
    constexpr CNFSign getSign() const noexcept;

    /**
     * \brief Gets the literal's negate.
     *
     * \returns The literal's negate.
     */
    constexpr CNFLit operator~() const noexcept;

    /**
     * \brief Gets the literal's raw value.
     *
     * Literal raw values are monotonically increasing wrt. the raw value of the
     * corresponding variables.
     *
     * \returns the literal's raw value.
     */
    constexpr RawLiteral getRawValue() const noexcept;

    /**
     * \brief The undefined marker literal.
     */
    static constexpr CNFLit getUndefinedLiteral() noexcept;

    /**
     * \ingroup JamSAT_CNFProblem
     *
     * \brief Index type for CNFLit
     *
     * This type is a model of the concept `Index`.
     */
    class Index {
    public:
        using Type = CNFLit;

        /**
         * \brief Gets an index value for the given literal.
         *
         * \param literal    A literal.
         * \returns          The index for \p literal.
         */
        static constexpr std::size_t getIndex(CNFLit literal) {
            return static_cast<std::size_t>(literal.getRawValue());
        }
    };

    CNFLit(const CNFLit &other) = default;
    CNFLit &operator=(const CNFLit &other) = default;
    CNFLit(CNFLit &&other) = default;
    CNFLit &operator=(CNFLit &&other) = default;

private:
    CNFLit::RawLiteral m_value;
};

std::ostream &operator<<(std::ostream &stream, const CNFVar &variable);
std::ostream &operator<<(std::ostream &stream, const CNFLit &literal);

/**
 * \brief Gets the next-higher CNF variable.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * \param var   A CNFVar whose raw value is not greater than CNFVar::getMaxRawValue().
 * \returns The next-higher CNFVar wrt. the raw value of \p var. If no such CNFVar exists,
 *   the undefined variable is returned.
 */
CNFVar nextCNFVar(CNFVar var) noexcept;

/**
 * \brief Returns true iff the given variable is regular.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * A variable is regular if its raw value is not greater than the maximimum raw value for CNFVar
 * objects. "Special" variables such as the undefined variable are not regular.
 */
constexpr bool isRegular(CNFVar variable) noexcept;


/**
 * \brief Equality operator for CNFVar.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * Two variables are considered equal iff their raw values are equal.
 *
 * \param lhs   The light-hand-side variable.
 * \param rhs   The right-hand-side variable.
 * \returns \p true iff \p lhs is equal to \p rhs.
 */
constexpr bool operator==(const CNFVar &lhs, const CNFVar &rhs) noexcept;

/**
 * \brief Equality operator for CNFVar.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * Two variables are considered inequal iff their raw values are inequal.
 *
 * \param lhs   The light-hand-side variable.
 * \param rhs   The right-hand-side variable.
 * \returns \p false iff \p lhs is equal to \p rhs.
 */
constexpr bool operator!=(const CNFVar &lhs, const CNFVar &rhs) noexcept;

/**
 * \brief Less-than operator for CNFVar.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * For any two literals \p x and \p y, we have <tt>x < y</tt> iff the raw value
 * of \p x is smaller than the raw value of \p y .
 *
 * Each CNFVar which is not the undefined variable is less than the undefined
 * variable.
 *
 * This operator establishes a strict weak ordering for CNFLit.
 *
 * \param lhs   The light-hand-side variable.
 * \param rhs   The right-hand-side variable.
 * \returns \p true iff \p lhs less than \p rhs .
 */
constexpr bool operator<(const CNFVar &lhs, const CNFVar &rhs) noexcept;

/**
 * \brief Greater-than operator for CNFVar.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * For any two CNFVar \p x and \p y, we have <tt>x > y</tt> iff <tt>x != y</tt>
 * and not \p <tt>x < y</tt>.
 *
 * \param lhs   The light-hand-side variable.
 * \param rhs   The right-hand-side variable.
 * \returns \p true iff \p lhs greater than \p rhs .
 */
constexpr bool operator>(const CNFVar &lhs, const CNFVar &rhs) noexcept;

/**
 * \brief Smaller-than-or-equal-to operator for CNFVar.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * \param lhs   The light-hand-side variable.
 * \param rhs   The right-hand-side variable.
 * \returns \p true iff \p lhs smaller than or equal to \p rhs .
 */
constexpr bool operator<=(const CNFVar &lhs, const CNFVar &rhs) noexcept;

/**
 * \brief Greater-than-or-equal-to operator for CNFVar.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * \param lhs   The light-hand-side variable.
 * \param rhs   The right-hand-side variable.
 * \returns \p true iff \p lhs greater than or equal to \p rhs .
 */
constexpr bool operator>=(const CNFVar &lhs, const CNFVar &rhs) noexcept;

/**
 * \brief Equality operator for CNFLit.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * Two literals are considered equal iff they have equal signs and equal
 * variables. The undefined literal is inequal to each other literal.
 *
 * \param lhs   The light-hand-side literal.
 * \param rhs   The right-hand-side literal.
 * \returns \p true iff \p lhs is equal to \p rhs.
 */
constexpr bool operator==(const CNFLit &lhs, const CNFLit &rhs) noexcept;

/**
 * \brief Inequality operator for CNFLit.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * Two literals are considered inequal iff they have inequal signs or inequal
 * variables.
 *
 * \param lhs   The light-hand-side literal.
 * \param rhs   The right-hand-side literal.
 * \returns \p false iff \p lhs is equal to \p rhs.
 */
constexpr bool operator!=(const CNFLit &lhs, const CNFLit &rhs) noexcept;

/**
 * \brief Less-than operator for CNFLit.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * For any two literals \p x and \p y, we have <tt>x < y</tt> iff either of the
 * following is true:
 *   - The variables of \p x and \p y are inequal, and the variable of \p x is
 * smaller than the variable of \p y .
 *   - The variables of \p x and \p y are equal, and \p x has a negative sign
 * and \p y has a positive sign.
 *
 * Each literal which is not the undefined literal is less than the undefined
 * literal.
 *
 * This operator establishes a strict weak ordering for CNFLit.
 *
 * \param lhs   The light-hand-side literal.
 * \param rhs   The right-hand-side literal.
 * \returns \p true iff \p lhs less than \p rhs .
 */
constexpr bool operator<(const CNFLit &lhs, const CNFLit &rhs) noexcept;

/**
 * \brief Greater-than operator for CNFLit.
 *
 * \ingroup JamSAT_CNFProblem
 *
 * For any two literals \p x and \p y, we have <tt>x > y</tt> iff <tt>x !=
 * y</tt> and not \p <tt>x < y</tt>.
 *
 * \param lhs   The light-hand-side literal.
 * \param rhs   The right-hand-side literal.
 * \returns \p true iff \p lhs greater than \p rhs .
 */
constexpr bool operator>(const CNFLit &lhs, const CNFLit &rhs) noexcept;

/**
 * \brief Less-than-or-equal-to operator for CNFLit.
 *
 * \ingroup JamSAT_CNFProblem
 *
 *
 * \param lhs   The light-hand-side literal.
 * \param rhs   The right-hand-side literal.
 * \returns \p true iff \p lhs less than or equal to \p rhs .
 */
constexpr bool operator<=(const CNFLit &lhs, const CNFLit &rhs) noexcept;

/**
 * \brief Greater-than-or-equal-to operator for CNFLit.
 *
 * \ingroup JamSAT_CNFProblem
 *
 *
 * \param lhs   The light-hand-side literal.
 * \param rhs   The right-hand-side literal.
 * \returns \p true iff \p lhs greater than or equal to \p rhs .
 */
constexpr bool operator>=(const CNFLit &lhs, const CNFLit &rhs) noexcept;

/**
 * \brief Computes the literal L with variable \p var such that L > ~L
 *
 * \ingroup JamSAT_CNFProblem
 *
 * \param var   A CNF variable
 * \return      The literal L with variable \p var such that L > ~L
 */
constexpr CNFLit getMaxLit(CNFVar var) noexcept;

/**
 * \brief User-defined literal for variables
 *
 * \ingroup JamSAT_CNFProblem
 *
 * \param var The variable value.
 * \return `CNFVar{var}`
 */
constexpr CNFVar operator"" _Var(unsigned long long var);

/**
 * \brief User-defined literal for literals
 *
 * \ingroup JamSAT_CNFProblem
 *
 * \param var The variable value.
 * \return `CNFLit{CNFVar{var}, CNFSign::POSITIVE}`
 */
constexpr CNFLit operator"" _Lit(unsigned long long var);

/********** Implementation ****************************** */

constexpr CNFVar::CNFVar() noexcept : m_value(std::numeric_limits<int>::max() >> 1) {}

constexpr CNFVar::CNFVar(RawVariable variableValue) noexcept : m_value(variableValue) {}

constexpr CNFVar::RawVariable CNFVar::getRawValue() const noexcept {
    return m_value;
}

constexpr CNFVar CNFVar::getUndefinedVariable() noexcept {
    return CNFVar{};
}

constexpr CNFVar::RawVariable CNFVar::getMaxRawValue() noexcept {
    return CNFVar::getUndefinedVariable().getRawValue() - 1;
}

constexpr bool operator==(const CNFVar &lhs, const CNFVar &rhs) noexcept {
    return lhs.getRawValue() == rhs.getRawValue();
}

constexpr bool operator!=(const CNFVar &lhs, const CNFVar &rhs) noexcept {
    return lhs.getRawValue() != rhs.getRawValue();
}

constexpr bool operator<(const CNFVar &lhs, const CNFVar &rhs) noexcept {
    return lhs.getRawValue() < rhs.getRawValue();
}

constexpr bool operator>(const CNFVar &lhs, const CNFVar &rhs) noexcept {
    return lhs.getRawValue() > rhs.getRawValue();
}

constexpr bool operator<=(const CNFVar &lhs, const CNFVar &rhs) noexcept {
    return lhs.getRawValue() <= rhs.getRawValue();
}

constexpr bool operator>=(const CNFVar &lhs, const CNFVar &rhs) noexcept {
    return lhs.getRawValue() >= rhs.getRawValue();
}

inline CNFVar nextCNFVar(CNFVar var) noexcept {
    auto varRaw = var.getRawValue();
    JAM_ASSERT(varRaw <= CNFVar::getMaxRawValue(), "Illegal argument");
    if (varRaw != CNFVar::getMaxRawValue()) {
        return CNFVar{varRaw + 1};
    } else {
        return CNFVar::getUndefinedVariable();
    }
}

constexpr bool isRegular(CNFVar variable) noexcept {
    return variable.getRawValue() <= CNFVar::getMaxRawValue();
}

constexpr CNFLit::CNFLit(CNFVar variable, CNFSign sign) noexcept
  : m_value((variable.getRawValue() << 1) | static_cast<int>(sign)) {
    JAM_ASSERT(variable != CNFVar::getUndefinedVariable(),
               "The variable must be smaller than CNFVar::undefinedVariable");
}

constexpr CNFLit::CNFLit() noexcept : m_value(std::numeric_limits<int>::max()) {}

constexpr CNFVar CNFLit::getVariable() const noexcept {
    return CNFVar{m_value >> 1};
}

constexpr CNFSign CNFLit::getSign() const noexcept {
    return static_cast<CNFSign>(m_value & 1);
}

constexpr CNFLit CNFLit::operator~() const noexcept {
    JAM_ASSERT(*this != CNFLit::getUndefinedLiteral(), "Cannot negate an undefined literal");
    return CNFLit{getVariable(), invert(getSign())};
}

constexpr CNFLit::RawLiteral CNFLit::getRawValue() const noexcept {
    return m_value;
}

constexpr CNFLit CNFLit::getUndefinedLiteral() noexcept {
    return CNFLit{};
}

constexpr bool operator==(const CNFLit &lhs, const CNFLit &rhs) noexcept {
    return lhs.getRawValue() == rhs.getRawValue();
}

constexpr bool operator!=(const CNFLit &lhs, const CNFLit &rhs) noexcept {
    return !(lhs == rhs);
}

constexpr bool operator<(const CNFLit &lhs, const CNFLit &rhs) noexcept {
    return lhs.getRawValue() < rhs.getRawValue();
}

constexpr bool operator>(const CNFLit &lhs, const CNFLit &rhs) noexcept {
    return lhs.getRawValue() > rhs.getRawValue();
}

constexpr bool operator<=(const CNFLit &lhs, const CNFLit &rhs) noexcept {
    return lhs.getRawValue() <= rhs.getRawValue();
}

constexpr bool operator>=(const CNFLit &lhs, const CNFLit &rhs) noexcept {
    return lhs.getRawValue() >= rhs.getRawValue();
}

constexpr CNFLit getMaxLit(CNFVar var) noexcept {
    return CNFLit{var, CNFSign::POSITIVE};
}

constexpr CNFVar operator"" _Var(unsigned long long var) {
    return CNFVar{static_cast<CNFVar::RawVariable>(var)};
}

constexpr CNFLit operator"" _Lit(unsigned long long var) {
    return CNFLit{CNFVar{static_cast<CNFVar::RawVariable>(var)}, CNFSign::POSITIVE};
}
}

namespace std {
template <>
struct hash<jamsat::CNFVar> {
    using argument_type = jamsat::CNFVar;
    using result_type = std::size_t;

    result_type operator()(argument_type variable) const noexcept {
        return std::hash<jamsat::CNFVar::RawVariable>{}(variable.getRawValue());
    }
};

template <>
struct hash<jamsat::CNFLit> {
    using argument_type = jamsat::CNFLit;
    using result_type = std::size_t;

    result_type operator()(argument_type literal) const noexcept {
        return std::hash<jamsat::CNFLit::RawLiteral>{}(literal.getRawValue());
    }
};
}
