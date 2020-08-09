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

#include <algorithm>
#include <cstdint>
#include <toolbox/cnfgenerators/GateStructure.h>

namespace jamsat {
void insertAND(const std::vector<CNFLit> inputs, CNFLit output, CNFProblem& target)
{
  CNFClause fwd;
  for (auto inputLit : inputs) {
    fwd.push_back(~inputLit);
  }
  fwd.push_back(output);
  target.addClause(std::move(fwd));

  for (auto inputLit : inputs) {
    CNFClause bwd{inputLit, ~output};
    target.addClause(std::move(bwd));
  }
}

void insertOR(const std::vector<CNFLit> inputs, CNFLit output, CNFProblem& target)
{
  CNFClause bwd;
  for (auto inputLit : inputs) {
    bwd.push_back(inputLit);
  }
  bwd.push_back(~output);
  target.addClause(std::move(bwd));

  for (auto inputLit : inputs) {
    CNFClause fwd{~inputLit, output};
    target.addClause(std::move(fwd));
  }
}

void insertXOR(const std::vector<CNFLit> inputs, CNFLit output, CNFProblem& target)
{
  uint32_t max = (1 << inputs.size());
  for (uint32_t i = 0; i < max; ++i) {
    CNFClause clause;
    for (uint32_t j = 0; j < inputs.size(); ++j) {
      bool sign = ((i & (1ull << j)) != 0);
      clause.push_back(sign ? ~inputs[j] : inputs[j]);
    }

    // the output is forced to 1 iff exactly one bit is set
    bool outputPositive = i > 0 && ((i & (i - 1)) == 0);
    clause.push_back(outputPositive ? output : ~output);
    target.addClause(std::move(clause));
  }
}
}
