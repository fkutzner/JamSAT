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

#include <cstdint>
#include <vector>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
/**
 * \ingroup JamSAT_Solver
 *
 * \class jamsat::VariableState
 *
 * \brief A class for keeping track of variable states.
 */
class VariableState {
public:
  using TruthValue = CNFSign;

  /**
   * \brief Constructs a new VariableState object with the given maximum amount
   * of variables.
   *
   * \param maxVar  The maximum variable whose state will be represented by the
   * VariableState object.
   */
  explicit VariableState(CNFVar maxVar);

  /**
   * \brief Sets the assignment for the given variable.
   *
   * \param variable  The target variable. Must not be greater than \p maxVar
   * passed to the constructor.
   * \param value     The variable's new value.
   */
  void setAssignment(CNFVar variable, TruthValue value) noexcept;

  /**
   * \brief Gets the assignment for the given variable.
   *
   * \param variable  The target variable. Must not be greater than \p maxVar
   * passed to the constructor.
   * \returns The variable's current assignment.
   */
  TruthValue getAssignment(CNFVar variable) const noexcept;

  /**
   * \brief Marks variables as eligible for being used in branching decisions.
   *
   * \param variable  The target variable. Must not be greater than \p maxVar
   * passed to the constructor.
   * \param isEligible  Iff true, \p variable may be used as a branching
   * decision variable.
   */
  void setEligibleForDecisions(CNFVar variable, bool isEligible) noexcept;

  /**
   * \brief Determines whether a given variable is eligible for being used in
   * branching decisions.
   *
   * \param variable  The target variable. Must not be greater than \p maxVar
   * passed to the constructor.
   * \returns true iff \p variable is eligible for being used in branching
   * decisions
   */
  bool isEligibleForDecisions(CNFVar variable) const noexcept;

  /**
   * \brief Marks the given variable as eliminated from the SAT problem instance
   * being solved.
   *
   * \param variable The target variable. Must not be greater than \p maxVar
   * passed to the constructor.
   */
  void setEliminated(CNFVar variable) noexcept;

  /**
   * \brief Determines whether the given variable as eliminated from the SAT
   * problem instance being solved.
   *
   * \param variable The target variable. Must not be greater than \p maxVar
   * passed to the constructor.
   * \returns true iff setEliminated(\p variable) has been called before.
   */
  bool isEliminated(CNFVar variable) const noexcept;

private:
  std::vector<CNFSign> m_assignments;
  std::vector<Bool> m_decisionVariables;
  std::vector<Bool> m_eliminatedVariables;
};
}
