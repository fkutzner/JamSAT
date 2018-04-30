/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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

#include "TestAssignmentProvider.h"
#include "TestReasonProvider.h"

#include <libjamsat/solver/AssignmentAnalysis.h>
#include <libjamsat/utils/Printers.h>
#include <libjamsat/utils/StampMap.h>

#include <algorithm>
#include <vector>

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;

TEST(UnitSolver, AssignmentAnalysisProducesUnitaryResultForReasonlessConflict) {
    TestAssignmentProvider decisionLevelProvider;
    TestReasonProvider<TrivialClause> reasonProvider;
    StampMap<int, CNFVarKey> tempStamps{CNFVar{1024}.getRawValue()};
    CNFLit lit{CNFVar{3}, CNFSign::POSITIVE};
    decisionLevelProvider.setCurrentDecisionLevel(0);
    decisionLevelProvider.addAssignment(lit);
    decisionLevelProvider.setAssignmentDecisionLevel(lit.getVariable(), 0);

    auto result = analyzeAssignment(reasonProvider, decisionLevelProvider, tempStamps, lit);
    EXPECT_EQ(result, std::vector<CNFLit>{lit});
}

TEST(UnitSolver, AssignmentAnalysisProducesFailingAssumptionsForReasonfulConflict) {
    TestAssignmentProvider decisionLevelProvider;
    TestReasonProvider<TrivialClause> reasonProvider;
    StampMap<int, CNFVarKey> tempStamps{CNFVar{1024}.getRawValue()};

    CNFLit lit1{CNFVar{3}, CNFSign::POSITIVE}, lit2{CNFVar{6}, CNFSign::POSITIVE},
        lit3{CNFVar{8}, CNFSign::NEGATIVE}, lit4{CNFVar{16}, CNFSign::NEGATIVE},
        lit5{CNFVar{20}, CNFSign::POSITIVE}, lit6{CNFVar{22}, CNFSign::NEGATIVE},
        lit7{CNFVar{25}, CNFSign::NEGATIVE};

    decisionLevelProvider.setCurrentDecisionLevel(0);
    decisionLevelProvider.addAssignment(lit1);
    decisionLevelProvider.addAssignment(lit2);
    decisionLevelProvider.setCurrentDecisionLevel(1);
    decisionLevelProvider.addAssignment(lit3);
    decisionLevelProvider.addAssignment(lit4);
    decisionLevelProvider.addAssignment(lit5);
    decisionLevelProvider.addAssignment(lit6);
    decisionLevelProvider.addAssignment(lit7);

    decisionLevelProvider.setAssignmentDecisionLevel(lit1.getVariable(), 0);
    decisionLevelProvider.setAssignmentDecisionLevel(lit2.getVariable(), 0);
    decisionLevelProvider.setAssignmentDecisionLevel(lit3.getVariable(), 1);
    decisionLevelProvider.setAssignmentDecisionLevel(lit4.getVariable(), 1);
    decisionLevelProvider.setAssignmentDecisionLevel(lit5.getVariable(), 1);
    decisionLevelProvider.setAssignmentDecisionLevel(lit6.getVariable(), 1);
    decisionLevelProvider.setAssignmentDecisionLevel(lit7.getVariable(), 1);

    TrivialClause reasonFor2{~lit1, lit2};
    TrivialClause reasonFor4{~lit2, ~lit1, ~lit6, ~lit3, lit4};
    TrivialClause reasonFor5{~lit2, ~lit7, ~lit3, ~lit4, lit5};
    reasonProvider.setAssignmentReason(lit2.getVariable(), reasonFor2);
    reasonProvider.setAssignmentReason(lit4.getVariable(), reasonFor4);
    reasonProvider.setAssignmentReason(lit5.getVariable(), reasonFor5);

    std::vector<CNFLit> expected{lit3, lit4, lit5, lit6, lit7};
    auto result = analyzeAssignment(reasonProvider, decisionLevelProvider, tempStamps, lit5);
    ASSERT_EQ(result.size(), expected.size());
    EXPECT_TRUE(std::is_permutation(expected.begin(), expected.end(), result.begin()))
        << "Expected a permutation of " << toString(expected.begin(), expected.end()) << " but got "
        << toString(result.begin(), result.end());
}
}
