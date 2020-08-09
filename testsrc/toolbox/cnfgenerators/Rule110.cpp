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

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Casts.h>
#include <toolbox/cnfgenerators/Rule110.h>

#include <limits>


namespace jamsat {
Rule110PredecessorStateProblem::Rule110PredecessorStateProblem(const std::string& sourceStateSpec,
                                                               const std::string& targetStateSpec,
                                                               uint32_t numberOfIntermediateSteps)
  : m_targetStateSpec(targetStateSpec)
  , m_sourceStateSpec(sourceStateSpec)
  , m_numberOfIntermediateSteps(numberOfIntermediateSteps)
  , m_automatonWidth(static_checked_cast<uint32_t>(targetStateSpec.size()))
{
  JAM_ASSERT(targetStateSpec.size() == sourceStateSpec.size(),
             "Source and target automaton states must be of equal size");
}

CNFVar Rule110PredecessorStateProblem::getCellVariable(uint32_t step,
                                                       uint32_t cellIndex) const noexcept
{
  JAM_ASSERT(cellIndex < m_automatonWidth, "Argument cellIndex out of bounds");
  JAM_ASSERT(step < m_numberOfIntermediateSteps + 2, "Argument step out of bounds");
  CNFVar::RawVariable v = (step * m_automatonWidth) + cellIndex;
  return CNFVar{v};
}

std::vector<CNFClause> Rule110PredecessorStateProblem::createConstraints(
    uint32_t step, uint32_t cellIndex, std::vector<CNFLit>& freeInputs) const
{
  JAM_ASSERT(cellIndex < m_automatonWidth, "Argument cellIndex out of bounds");
  JAM_ASSERT(step < m_numberOfIntermediateSteps + 2, "Argument step out of bounds");

  std::vector<CNFClause> result;

  if (step == 0) {
    auto sign = (m_sourceStateSpec[cellIndex] == '1') ? CNFSign::POSITIVE : CNFSign::NEGATIVE;
    CNFLit lit{getCellVariable(step, cellIndex), sign};
    if (m_sourceStateSpec[cellIndex] != 'x') {
      result.push_back({lit});
    }
    else {
      freeInputs.push_back(~lit);
    }
    // return early since there are no other "incoming" constraints for this cell
    return result;
  }

  if (step == m_numberOfIntermediateSteps + 1) {
    if (m_targetStateSpec[cellIndex] != 'x') {
      auto sign = (m_targetStateSpec[cellIndex] == '1') ? CNFSign::POSITIVE : CNFSign::NEGATIVE;
      result.push_back({CNFLit{getCellVariable(step, cellIndex), sign}});
    }
  }

  auto mid = CNFLit{getCellVariable(step - 1, cellIndex), CNFSign::NEGATIVE};
  auto futureMid = CNFLit{getCellVariable(step, cellIndex), CNFSign::POSITIVE};

  if (cellIndex > 0 && cellIndex < (m_automatonWidth - 1)) {
    // Non-border cell: future state depends on right and left neighbour
    auto left = CNFLit{getCellVariable(step - 1, cellIndex - 1), CNFSign::NEGATIVE};
    auto right = CNFLit{getCellVariable(step - 1, cellIndex + 1), CNFSign::NEGATIVE};
    result.push_back({left, mid, right, ~futureMid});
    result.push_back({left, mid, ~right, futureMid});
    result.push_back({left, ~mid, right, futureMid});
    result.push_back({left, ~mid, ~right, ~futureMid});
    result.push_back({~left, mid, right, futureMid});
    result.push_back({~left, mid, ~right, futureMid});
    result.push_back({~left, ~mid, right, futureMid});
    result.push_back({~left, ~mid, ~right, ~futureMid});
  }
  else if (cellIndex == 0) {
    // Left border cell: left neighbour is always 0
    auto right = CNFLit{getCellVariable(step - 1, cellIndex + 1), CNFSign::NEGATIVE};
    result.push_back({mid, right, futureMid});
    result.push_back({mid, ~right, futureMid});
    result.push_back({~mid, right, futureMid});
    result.push_back({~mid, ~right, ~futureMid});
  }
  else {
    // Right border cell: right neighbour is always 0
    auto left = CNFLit{getCellVariable(step - 1, cellIndex - 1), CNFSign::NEGATIVE};
    result.push_back({left, mid, futureMid});
    result.push_back({left, ~mid, ~futureMid});
    result.push_back({~left, mid, futureMid});
    result.push_back({~left, ~mid, ~futureMid});
  }

  return result;
}

auto Rule110PredecessorStateProblem::getCNFEncoding() const -> Rule110Encoding
{
  Rule110Encoding result;
  for (uint32_t step = 0; step < (m_numberOfIntermediateSteps + 2); ++step) {
    for (uint32_t cellIndex = 0; cellIndex < m_targetStateSpec.size(); ++cellIndex) {
      auto encoding = createConstraints(step, cellIndex, result.freeInputs);
      for (auto clause : encoding) {
        result.cnfProblem.addClause(std::move(clause));
      }
    }
  }
  return result;
}
}
