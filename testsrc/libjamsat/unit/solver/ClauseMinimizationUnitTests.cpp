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

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

#include <boost/algorithm/cxx11/is_permutation.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/ClauseMinimization.h>
#include <libjamsat/utils/StampMap.h>

#include "TestReasonProvider.h"

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;

struct CNFLitKey {
  using Type = CNFLit;

  static size_t getIndex(CNFLit literal) {
    return static_cast<size_t>(literal.getRawValue());
  }
};

template <typename Container>
bool isPermutation(const Container &c1, const Container &c2) {
  if (c1.size() != c2.size()) {
    return false;
  }
  return boost::algorithm::is_permutation(c1, c2.begin());
}

TEST(UnitSolver, eraseRedundantLiterals_fixpointOnEmptyClause) {
  TestReasonProvider<TrivialClause> reasonProvider;
  TrivialClause emptyClause;
  StampMap<int, CNFLitKey> tempStamps{1024};

  eraseRedundantLiterals(emptyClause, reasonProvider, tempStamps);

  EXPECT_TRUE(emptyClause.empty());
}

TEST(UnitSolver, eraseRedundantLiterals_removesSingleLevelRedundancy) {
  TestReasonProvider<TrivialClause> reasonProvider;
  TrivialClause reasonFor3{
      CNFLit{CNFVar{3}, CNFSign::POSITIVE},
      CNFLit{CNFVar{4}, CNFSign::NEGATIVE},
  };
  reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

  TrivialClause testData{CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                         CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                         CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};

  StampMap<int, CNFLitKey> tempStamps{1024};

  eraseRedundantLiterals(testData, reasonProvider, tempStamps);

  TrivialClause expected = testData;
  boost::remove_erase(expected, CNFLit{CNFVar{3}, CNFSign::NEGATIVE});

  EXPECT_TRUE(isPermutation(testData, expected));
}
}
