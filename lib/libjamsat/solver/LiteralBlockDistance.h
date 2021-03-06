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

/**
 * \file LiteralBlockDistance.h
 * \brief LBD-related utilities.
 */

#pragma once

#include <cstdint>
#include <libjamsat/concepts/SolverTypeTraits.h>
#include <libjamsat/utils/StampMap.h>

namespace jamsat {
using LBD = uint32_t;
/**
 * \ingroup JamSAT_Solver
 *
 * \brief Computes the literal block distance (LBD) of the given range of
 * literals.
 *
 * Given a set L of literals, the LBD of L is defined as the number of distinct
 * decision levels of variables occuring in L.
 *
 * Usage example: determine the value of a derived lemma by the LBD value of its literals.
 *
 * \param literals                a forward range of literals.
 * \param decisionLevelProvider   a decision level provider.
 * \param tempStamps              a clean StampMap supporting stamping the
 *     decision levels of \p literals . When \p getLBD returns, \p tempStamps is clean.
 * \returns                       the LBD of \p literals wrt. \p decisionLevelProvider .
 *
 * \tparam ForwardRange           a forward range of literals.
 * \tparam DLProvider             A type satisfying the DecisionLevelProvider concept.
 * \tparam StampMapT              A StampMap specialization supporting stamping of
 *                                DLProvider::LevelKey objects
 * \tparam LBD                    The literal block distance type, an integral type.
 */
template <typename ForwardRange, typename DLProvider, typename StampMapT>
LBD getLBD(const ForwardRange& literals,
           const DLProvider& decisionLevelProvider,
           StampMapT& tempStamps) noexcept
{
  static_assert(is_decision_level_provider<DLProvider>::value,
                "Template argument DLProvider must satisfy the DecisionLevelProvider concept,"
                " but does not");
  static_assert(is_stamp_map<StampMapT, typename DLProvider::Level>::value,
                "Template argument StampMapT must be a specialization of StampMap<...> supporting"
                " stamping of DLProvider::LevelKey objects");

  auto stampContext = tempStamps.createContext();
  auto stamp = stampContext.getStamp();
  LBD result = 0;

  for (auto& literal : literals) {
    auto variable = literal.getVariable();
    auto level = decisionLevelProvider.getLevel(variable);
    if (!tempStamps.isStamped(level, stamp)) {
      tempStamps.setStamped(level, stamp, true);
      ++result;
    }
  }

  return result;
}
}
