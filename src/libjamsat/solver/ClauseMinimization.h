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

#include <boost/range/algorithm_ext/erase.hpp>
#include <vector>

#include <libjamsat/utils/Assert.h>

namespace jamsat {

namespace erl_detail {
template <class ReasonProvider, class DecisionLevelProvider, class StampMapT>
bool isRedundant(CNFLit literal, const ReasonProvider &reasonProvider,
                 const DecisionLevelProvider &dlProvider, StampMapT &tempStamps,
                 typename StampMapT::Stamp currentStamp) {
  if (dlProvider.getAssignmentDecisionLevel(literal.getVariable()) ==
      dlProvider.getCurrentDecisionLevel()) {
    return false;
  }

  std::vector<CNFVar> work{literal.getVariable()};

  while (!work.empty()) {
    CNFVar workItem = work.back();
    work.pop_back();

    auto clausePtr = reasonProvider.getAssignmentReason(workItem);
    JAM_ASSERT(clausePtr != nullptr,
               "Can't determine redundancy of reasonless literals");

    for (CNFLit lit : *clausePtr) {
      CNFVar var = lit.getVariable();
      auto varLevel = dlProvider.getAssignmentDecisionLevel(var);

      if (varLevel == 0 || tempStamps.isStamped(var, currentStamp)) {
        continue;
      }

      if (reasonProvider.getAssignmentReason(var) != nullptr) {
        tempStamps.setStamped(var, currentStamp, true);
        work.push_back(var);
      } else {
        return false;
      }
    }
  }

  return true;
}
}

/**
 * \ingroup JamSAT_Solver
 *
 * \brief Erases redundant literals from the given clause.
 *
 * Erases literals from \p clause which are redundant wrt. reason clauses given
 * via \p reasonProvider .
 *
 * A literal l is said to be <i>redundant</i> if l has an assigned value, and
 * either
 *  - occurs on decision level 0 or
 *  - l is not a decision literal and every false-assigned literal in l's reason
 * is either contained in \p clause or is redundant.
 *
 * [Knuth, The Art of Computer Programming, chapter 7.2.2.2, exercise 257]
 *
 * Usage example: Remove redundant literals from a conflicting clause returned
 * by conflict analysis, using Propagation as an assignment reason provider and
 * Trail as a decision level provider.
 *
 *
 * \param[in,out] clause      The clause from which redundant literals should be
 * erased.
 * \param[in] reasonProvider  An assignment reason provider (TODO: document
 * assignment reason provider concept)
 * \param[in] dlProvider      A decision level provider (TODO: document decision
 * level provider concept)
 * \param[in,out] tempStamps  a clean StampMap supporting stamping CNFVar values
 * occuring in \p clause and any reason clause in \p reasonProvider . When this
 * function returns, \p tempStamps is clean.
 *
 * TODO: document template parameters
 */
template <class ClauseT, class ReasonProvider, class DecisionLevelProvider,
          class StampMapT>
void eraseRedundantLiterals(ClauseT &clause,
                            const ReasonProvider &reasonProvider,
                            const DecisionLevelProvider &dlProvider,
                            StampMapT &tempStamps) noexcept {
  const auto stampContext = tempStamps.createContext();
  const auto stamp = stampContext.getStamp();

  for (auto literal : clause) {
    tempStamps.setStamped(literal.getVariable(), stamp, true);
  }

  auto isRedundant = [&reasonProvider, &dlProvider, &tempStamps,
                      &stamp](CNFLit literal) {
    auto reason = reasonProvider.getAssignmentReason(literal.getVariable());
    if (reason != nullptr) {
      return erl_detail::isRedundant(literal, reasonProvider, dlProvider,
                                     tempStamps, stamp);
    }
    return dlProvider.getAssignmentDecisionLevel(literal.getVariable()) == 0;
  };

  boost::remove_erase_if(clause, isRedundant);
}
}