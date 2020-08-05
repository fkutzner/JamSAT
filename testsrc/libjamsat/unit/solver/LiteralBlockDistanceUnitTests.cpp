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

#include <gtest/gtest.h>

#include <vector>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/utils/StampMap.h>

#include <toolbox/testutils/TestAssignmentProvider.h>

namespace jamsat {
struct LevelKey {
    using Type = TestAssignmentProvider::Level;

    static size_t getIndex(TestAssignmentProvider::Level level) {
        return static_cast<size_t>(level);
    }
};

using TestStampMap = StampMap<unsigned int, LevelKey>;
using TestClause = TestAssignmentProvider::Clause;

TEST(UnitSolver, getLBD_LBDofEmptyClauseIs0) {
    TestStampMap tempStamps{128};
    TestAssignmentProvider dlProvider;
    TestClause empty;

    LBD result = getLBD(empty, dlProvider, tempStamps);
    EXPECT_EQ(result, 0ull);
}

TEST(UnitSolver, getLBD_LBDofUnaryClauseIs1) {
    TestStampMap tempStamps{128};
    TestAssignmentProvider dlProvider;
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 10);
    TestClause unary{~1_Lit};

    LBD result = getLBD(unary, dlProvider, tempStamps);
    EXPECT_EQ(result, 1ull);
}

TEST(UnitSolver, getLBD_LBDofMultiLiteralClause) {
    TestStampMap tempStamps{128};
    TestAssignmentProvider dlProvider;
    dlProvider.setAssignmentDecisionLevel(CNFVar{2}, 10);
    dlProvider.setAssignmentDecisionLevel(CNFVar{5}, 9);
    dlProvider.setAssignmentDecisionLevel(CNFVar{7}, 10);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 8);
    dlProvider.setAssignmentDecisionLevel(CNFVar{0}, 10);
    dlProvider.setAssignmentDecisionLevel(CNFVar{10}, 9);

    TestClause testData{
        ~2_Lit,
        5_Lit,
        ~7_Lit,
        ~1_Lit,
        0_Lit,
        ~10_Lit,
    };

    LBD result = getLBD(testData, dlProvider, tempStamps);
    EXPECT_EQ(result, 3ull);
}
}
