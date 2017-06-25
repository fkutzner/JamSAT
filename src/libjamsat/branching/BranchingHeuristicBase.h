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

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/ArrayMap.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
/**
 * \defgroup JamSAT_Branching
 */

/**
 * \ingroup JamSAT_Branching
 *
 * \class BranchingHeuristicBase
 *
 * \brief The base class for CDCL branching heuristics in JamSAT. Concrete
 * branching heuristics should be derived from this class.
 */
class BranchingHeuristicBase {
public:
  explicit BranchingHeuristicBase(CNFVar maxVar) noexcept;

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
   * \returns true iff \p variable has been marked eligible for being used in
   * branching decisions. If the variables's eligibility has not been set yet,
   * false is returned.
   */
  bool isEligibleForDecisions(CNFVar variable) const noexcept;

private:
  ArrayMap<CNFVar, Bool> m_decisionVariables;
};
}
