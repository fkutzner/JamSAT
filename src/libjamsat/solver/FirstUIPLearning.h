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

#include <vector>

#include "Clause.h"

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
/**
 * \ingroup JamSAT_Solver
 *
 * \class jamsat::FirstUIPLearning
 *
 * \brief A CDCL first-UIP-base clause learning implementation.
 *
 * Usage example: Use FirstUIPLearning with a Trail implementation as a
 * DLProvider and Propagation as a ReasonProvider to compute conflict clauses
 * after a conflict occurred.
 *
 * \tparam DLProvider   A class type having the method T <tt>
 * getAssignmentDecisionLevel(CNFVar variable) const noexcept </tt> (with T
 * being an integral type) which returns the current decision level of \p
 * variable, and T <tt> getCurrentDecisionLevel() const noexcept </tt> returning
 * the current decision level.
 * \tparam ReasonProvider A class type having the method <tt> const Clause*
 * getAssignmentReason(CNFVar variable) const noexcept </tt> returning the
 * reason for the assignment of \p variable.
 */
template <class DLProvider, class ReasonProvider> class FirstUIPLearning {
public:
  /**
   * \brief Constructs a new FirstUIPLearning instance.
   *
   * \param maxVar    The maximum variable occuring in the problem to be solved.
   * \param dlProvider  The decision level providing object. Needs to live as
   * long as the constructed object.
   * \param reasonProvider The assignment reason providing object. Needs to live
   * as long as the constructed object.
   */
  FirstUIPLearning(CNFVar maxVar, DLProvider &dlProvider,
                   ReasonProvider &reasonProvider);

  /**
   * \brief Given a conflicting clause, computes a conflict clause.
   *
   * \param conflictingClause  The conflicting clause, ie. a clause being
   * falsified through propagation under the current assignment.
   * \returns The conflict clause determined via resolutions of the conflicting
   * clause with reason clauses.
   */
  std::vector<CNFLit> computeConflictClause(Clause &conflictingClause) noexcept;

private:
  DLProvider &m_dlProvider;
  ReasonProvider &m_reasonProvider;
};

template <class DLProvider, class ReasonProvider>
FirstUIPLearning<DLProvider, ReasonProvider>::FirstUIPLearning(
    CNFVar maxVar, DLProvider &dlProvider, ReasonProvider &reasonProvider)
    : m_dlProvider(dlProvider), m_reasonProvider(reasonProvider) {}

template <class DLProvider, class ReasonProvider>
std::vector<CNFLit>
FirstUIPLearning<DLProvider, ReasonProvider>::computeConflictClause(
    Clause &conflictingClause) noexcept {
  return {};
}
}
