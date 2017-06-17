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

#include <libjamsat/cnfproblem/CNFLiteral.h>

namespace jamsat {

/**
 * \defgroup JamSAT_Solver
 */

/**
 * \ingroup JamSAT_Solver
 *
 * \class Trail
 *
 * \brief The solver's trail data structure. The trail is used to keep track of
 * the assignment sequence, which is partitioned into individually accessible
 * decision levels. The assignment sequence is kept as a sequence of literals.
 */
class Trail {
private:
  std::vector<CNFLit> m_trail;
  std::vector<decltype(m_trail)::size_type> m_trailLimits;

public:
  using size_type = decltype(m_trail)::size_type;
  using DecisionLevel = decltype(m_trailLimits)::size_type;
  using const_iterator = decltype(m_trail)::const_iterator;

  /**
   * \brief Constructs a new trail.
   *
   * \param maxVar    The maximum variable which will occur on the trail.
   */
  explicit Trail(CNFVar maxVar);

  /**
   * \brief Sets up a new decision level on the trail, beginning from the next
   * added literal.
   */
  void newDecisionLevel() noexcept;

  /**
   * \brief Gets the current decision level.
   *
   * \returns The current decision level.
   */
  DecisionLevel getCurrentDecisionLevel() const noexcept;

  /**
   * \brief Removes all literals from the trail which belong to a decision level
   * greater than or equal to the given one. After this operation, the trail's
   * decision level matches the given decision level.
   */
  void shrinkToDecisionLevel(DecisionLevel level) noexcept;

  /**
   * \brief Adds a literal to the end of the trail. Note that this literal will
   * belong to the current decision level.
   */
  void addLiteral(CNFLit literal) noexcept;

  /**
   * \brief Gets the literals of the requested decision level.
   *
   * \param level   The requested decision level.
   * \returns       a pair of random-access const iterators pointing to
   * literals. The first iterator points to the beginning of the literals
   * belonging to the decision level \p level, while the second iterator points
   * to one element past the literals belonging to the decision level \p level
   * and may not be dereferenced. If \p level is empty or not a valid decision
   * level, the two iterators are equal.
   */
  std::pair<const_iterator, const_iterator>
  getDecisionLevelLiterals(DecisionLevel level) const noexcept;
};
}
