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

#include <functional>
#include <ostream>

#include <libjamsat/utils/Assert.h>

namespace jamsat {
/**
 * \defgroup JamSAT_CNFProblem Collection of CNF-related data types.
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
inline CNFSign invert(CNFSign sign) noexcept {
  int rawSign = static_cast<int>(sign);
  return static_cast<CNFSign>(1 - rawSign);
}

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \class jamsat::CNFVar
 * \brief A CNF variable class.
 */
class CNFVar {
public:
  /** The underlying variable type. */
  using RawVariableType = int;

  /**
   * \brief Constructs a CNFVar object.
   *
   * \param variableValue  The non-negative raw variable identifier.
   */
  explicit inline CNFVar(RawVariableType variableValue) noexcept;

  /**
   * \brief Constructs an undefined variable.
   */
  CNFVar() noexcept;

  /**
   * \brief Gets the variable's raw value.
   *
   * \returns the variable's raw value.
   */
  inline RawVariableType getRawValue() const noexcept;

  /**
   * \brief Equality operator for CNFVar.
   *
   * \param rhs   The right-hand-side variable.
   * \returns \p true iff this variable is equal to \p rhs.
   */
  inline bool operator==(const CNFVar &rhs) const noexcept;

  /**
   * \brief Inquality operator for CNFVar.
   *
   * \param rhs   The right-hand-side variable.
   * \returns \p true iff this variable is inequal to \p rhs.
   */
  inline bool operator!=(const CNFVar &rhs) const noexcept;

  /**
   * \brief The undefined marker variable.
   *
   * \param rhs   The right-hand-side variable.
   * \returns \p true iff this variable is equal to \p rhs.
   */
  static const CNFVar undefinedVariable;

private:
  RawVariableType m_value;
};

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \class jamsat::CNFLit
 * \brief A CNF literal class.
 */
class CNFLit {
public:
  /** The underlying literal type. */
  using RawLiteralType = int;

  /**
   * \brief Constructs a CNFLit object.
   *
   * \param variable  The literal's variable. The variable's value must be
   * nonnegative and smaller than CNFVar::undefinedVariable.
   * \param sign      The literal's sign.
   */
  inline CNFLit(CNFVar variable, CNFSign sign) noexcept;

  /**
   * \brief Constructs an undefined literal.
   */
  CNFLit() noexcept;

  /**
   * \brief Gets the literal's variable.
   *
   * \returns  The literal's variable.
   */
  inline CNFVar getVariable() const noexcept;

  /**
   * \brief Gets the literal's sign.
   *
   * \returns  The literal's sign.
   */
  inline CNFSign getSign() const noexcept;

  /**
   * \brief Gets the literal's negate.
   *
   * \returns The literal's negate.
   */
  inline CNFLit operator~() const noexcept;

  /**
   * \brief Equality operator for CNFVar.
   *
   * \param rhs   The right-hand-side literal.
   * \returns \p true iff this literal is equal to \p rhs.
   */
  inline bool operator==(const CNFLit &rhs) const noexcept;

  /**
   * \brief Inequality operator for CNFVar.
   *
   * \param rhs   The right-hand-side literal.
   * \returns \p true iff this literal is inequal to \p rhs.
   */
  inline bool operator!=(const CNFLit &rhs) const noexcept;

  /**
   * \brief Gets the literal's raw value.
   *
   * Literal raw values are monotonically increasing wrt. the raw value of the
   * corresponding variables.
   *
   * \returns the literal's raw value.
   */
  inline RawLiteralType getRawValue() const noexcept;

  /**
   * \brief The undefined marker literal.
   */
  static const CNFLit undefinedLiteral;

private:
  int m_value;
};

std::ostream &operator<<(std::ostream &stream, const CNFVar &variable);
std::ostream &operator<<(std::ostream &stream, const CNFLit &literal);

/********** Implementation ****************************** */

CNFVar::CNFVar(RawVariableType variableValue) noexcept
    : m_value(variableValue) {}

CNFVar::RawVariableType CNFVar::getRawValue() const noexcept { return m_value; }

bool CNFVar::operator==(const CNFVar &rhs) const noexcept {
  return rhs.m_value == m_value;
}

bool CNFVar::operator!=(const CNFVar &rhs) const noexcept {
  return rhs.m_value != m_value;
}

CNFLit::CNFLit(CNFVar variable, CNFSign sign) noexcept {
  JAM_ASSERT(variable.getRawValue() >= 0,
             "The variable of a literal must not be negative");
  JAM_ASSERT(variable != CNFVar::undefinedVariable,
             "The variable must be smaller than CNFVar::undefinedVariable");
  m_value = (variable.getRawValue() << 1) | static_cast<int>(sign);
}

CNFVar CNFLit::getVariable() const noexcept { return CNFVar{m_value >> 1}; }

CNFSign CNFLit::getSign() const noexcept {
  return static_cast<CNFSign>(m_value & 1);
}

CNFLit CNFLit::operator~() const noexcept {
  JAM_ASSERT(*this != CNFLit::undefinedLiteral,
             "Cannot negate an undefined literal");
  return CNFLit{getVariable(), invert(getSign())};
}

bool CNFLit::operator==(const CNFLit &rhs) const noexcept {
  return rhs.m_value == m_value;
}

bool CNFLit::operator!=(const CNFLit &rhs) const noexcept {
  return !(*this == rhs);
}

CNFLit::RawLiteralType CNFLit::getRawValue() const noexcept { return m_value; }
}

namespace std {
template <> struct hash<jamsat::CNFVar> {
  using argument_type = jamsat::CNFVar;
  using result_type = std::size_t;

  result_type operator()(argument_type variable) const noexcept {
    return std::hash<jamsat::CNFVar::RawVariableType>{}(variable.getRawValue());
  }
};
}
