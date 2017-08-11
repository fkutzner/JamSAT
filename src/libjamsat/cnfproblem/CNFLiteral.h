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
constexpr CNFSign invert(CNFSign sign) noexcept {
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
   * \class jamsat::CNFVar::CNFVarIndex
   *
   * \brief Indexer class for CNFVar objects, e.g. for use with ArrayBackedMap.
   */
  class Index {
  public:
    /**
     * \brief Gets an index value for the given variable.
     *
     * \param variable    A variable.
     * \returns           The variable's raw value, to be used for indexing.
     */
    static constexpr CNFVar::RawVariable getIndex(CNFVar variable) {
      return variable.getRawValue();
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
   * \brief Equality operator for CNFVar.
   *
   * \param rhs   The right-hand-side literal.
   * \returns \p true iff this literal is equal to \p rhs.
   */
  constexpr bool operator==(const CNFLit &rhs) const noexcept;

  /**
   * \brief Inequality operator for CNFVar.
   *
   * \param rhs   The right-hand-side literal.
   * \returns \p true iff this literal is inequal to \p rhs.
   */
  constexpr bool operator!=(const CNFLit &rhs) const noexcept;

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
   * \class jamsat::CNFLit::CNFLitIndex
   *
   * \brief Indexer class for CNFLit objects, e.g. for use with ArrayBackedMap.
   */
  class Index {
  public:
    /**
     * \brief Gets an index value for the given literal.
     *
     * \param literal    A literal.
     * \returns           The literal's raw value, to be used for indexing.
     */
    static constexpr CNFLit::RawLiteral getIndex(CNFLit literal) {
      return literal.getRawValue();
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

constexpr bool operator==(const CNFVar &lhs, const CNFVar &rhs) noexcept;
constexpr bool operator!=(const CNFVar &lhs, const CNFVar &rhs) noexcept;

/********** Implementation ****************************** */

constexpr CNFVar::CNFVar() noexcept
    : m_value(std::numeric_limits<int>::max() >> 1) {}

constexpr CNFVar::CNFVar(RawVariable variableValue) noexcept
    : m_value(variableValue) {}

constexpr CNFVar::RawVariable CNFVar::getRawValue() const noexcept {
  return m_value;
}

constexpr CNFVar CNFVar::getUndefinedVariable() noexcept { return CNFVar{}; }

constexpr CNFVar::RawVariable CNFVar::getMaxRawValue() noexcept {
  return CNFVar::getUndefinedVariable().getRawValue() - 1;
}

constexpr bool operator==(const CNFVar &lhs, const CNFVar &rhs) noexcept {
  return lhs.getRawValue() == rhs.getRawValue();
}

constexpr bool operator!=(const CNFVar &lhs, const CNFVar &rhs) noexcept {
  return lhs.getRawValue() != rhs.getRawValue();
}

constexpr CNFLit::CNFLit(CNFVar variable, CNFSign sign) noexcept
    : m_value((variable.getRawValue() << 1) | static_cast<int>(sign)) {
  JAM_ASSERT(variable != CNFVar::getUndefinedVariable(),
             "The variable must be smaller than CNFVar::undefinedVariable");
}

constexpr CNFLit::CNFLit() noexcept
    : m_value(std::numeric_limits<int>::max()) {}

constexpr CNFVar CNFLit::getVariable() const noexcept {
  return CNFVar{m_value >> 1};
}

constexpr CNFSign CNFLit::getSign() const noexcept {
  return static_cast<CNFSign>(m_value & 1);
}

constexpr CNFLit CNFLit::operator~() const noexcept {
  JAM_ASSERT(*this != CNFLit::getUndefinedLiteral(),
             "Cannot negate an undefined literal");
  return CNFLit{getVariable(), invert(getSign())};
}

constexpr bool CNFLit::operator==(const CNFLit &rhs) const noexcept {
  return rhs.m_value == m_value;
}

constexpr bool CNFLit::operator!=(const CNFLit &rhs) const noexcept {
  return !(*this == rhs);
}

constexpr CNFLit::RawLiteral CNFLit::getRawValue() const noexcept {
  return m_value;
}

constexpr CNFLit CNFLit::getUndefinedLiteral() noexcept { return CNFLit{}; }
}

namespace std {
template <> struct hash<jamsat::CNFVar> {
  using argument_type = jamsat::CNFVar;
  using result_type = std::size_t;

  result_type operator()(argument_type variable) const noexcept {
    return std::hash<jamsat::CNFVar::RawVariable>{}(variable.getRawValue());
  }
};

template <> struct hash<jamsat::CNFLit> {
  using argument_type = jamsat::CNFLit;
  using result_type = std::size_t;

  result_type operator()(argument_type literal) const noexcept {
    return std::hash<jamsat::CNFLit::RawLiteral>{}(literal.getRawValue());
  }
};
}
