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
#include <gtest/gtest.h>
#include <unordered_map>
#include <vector>

#include <boost/range/algorithm_ext/erase.hpp>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/simplification/ClauseMinimization.h>
#include <libjamsat/utils/StampMap.h>

#include <toolbox/testutils/TestAssignmentProvider.h>
#include <toolbox/testutils/TestReasonProvider.h>

namespace jamsat {
using TrivialClause = TestAssignmentProvider::Clause;

template <typename Container>
bool isPermutation(const Container &c1, const Container &c2) {
    return std::is_permutation(c1.begin(), c1.end(), c2.begin(), c2.end());
}

TEST(UnitSolver, eraseRedundantLiterals_fixpointOnEmptyClause) {
    TestReasonProvider<TrivialClause> reasonProvider;
    TestAssignmentProvider dlProvider;

    TrivialClause emptyClause;
    StampMap<int, CNFVar::Index> tempStamps{1024};

    eraseRedundantLiterals(emptyClause, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(emptyClause.empty());
}

TEST(UnitSolver, eraseRedundantLiterals_removesSingleLevelRedundancy) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor3{
        3_Lit,
        ~4_Lit,
    };
    reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

    TrivialClause testData{1_Lit, ~3_Lit, ~4_Lit};

    StampMap<int, CNFVar::Index> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 1);

    TrivialClause expected = testData;
    boost::remove_erase(expected, ~3_Lit);

    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_removesTwoLevelRedundancy) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor3{
        3_Lit,
        ~4_Lit,
        ~5_Lit,
    };
    reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

    TrivialClause reasonFor5{
        ~5_Lit,
        ~8_Lit,
        ~9_Lit,
    };
    reasonProvider.setAssignmentReason(CNFVar{5}, reasonFor5);

    TrivialClause testData{1_Lit, ~3_Lit, ~4_Lit, ~8_Lit, 9_Lit};

    StampMap<int, CNFVar::Index> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);

    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    for (CNFVar::RawVariable i = 1; i < 10; ++i) {
        dlProvider.setAssignmentDecisionLevel(CNFVar{i}, 1);
    }

    TrivialClause expected = testData;
    boost::remove_erase(expected, ~3_Lit);

    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_removesSingleLevelRedundancyWithUnit) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor3{
        3_Lit,
        ~4_Lit,
        ~5_Lit,
    };
    reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

    TrivialClause testData{1_Lit, ~3_Lit, ~4_Lit};

    StampMap<int, CNFVar::Index> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{5}, 0);

    TrivialClause expected = testData;
    boost::remove_erase(expected, ~3_Lit);

    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_removesUnitLiteral) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause testData{1_Lit, ~3_Lit, ~4_Lit};

    StampMap<int, CNFVar::Index> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 0);

    TrivialClause expected = testData;
    boost::remove_erase(expected, ~4_Lit);

    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_doesNotRemoveNonredundantLiteral) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor3{3_Lit, ~4_Lit, 5_Lit};
    reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

    TrivialClause testData{1_Lit, ~3_Lit, ~4_Lit};

    StampMap<int, CNFVar::Index> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{5}, 1);

    // Literal 3 is not redundant since literal 5 does not occur
    // in testData and is a decision literal (has no reason clause).

    TrivialClause expected = testData;
    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_doesNotRemoveLiteralsOnCurrentLevel) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor1{~1_Lit, ~4_Lit};
    reasonProvider.setAssignmentReason(CNFVar{1}, reasonFor1);

    TrivialClause testData{1_Lit, ~3_Lit, ~4_Lit};

    StampMap<int, CNFVar::Index> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 1);

    TrivialClause expected = testData;
    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_regression_doesNotMarkNonredundantLitAsRedundant) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor3{3_Lit, 7_Lit};
    reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

    TrivialClause reasonFor7{~7_Lit, 1_Lit};
    reasonProvider.setAssignmentReason(CNFVar{7}, reasonFor7);

    // Variable 1 has no reason clause.

    TrivialClause reasonFor2{7_Lit, 2_Lit};
    reasonProvider.setAssignmentReason(CNFVar{2}, reasonFor2);

    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{2}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{7}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{6}, 2);

    TrivialClause testData{6_Lit, ~3_Lit, ~2_Lit};

    StampMap<int, CNFVar::Index> tempStamps{1024};

    TrivialClause expected = testData;
    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    // neither -3 nor -2 is redundant, since var. 1 has no reason clause and is not unit
    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, resolveWithBinaries_emptyClauseIsFixpoint) {
    CNFLit resolveAt{CNFVar{10}, CNFSign::POSITIVE};
    // representing binary clauses as a map from first literals to a list of
    // second literals
    std::unordered_map<CNFLit, std::vector<CNFLit>> binaryClauses;
    binaryClauses[resolveAt] = {9_Lit, 8_Lit};

    TrivialClause empty;
    StampMap<int, CNFLit::Index> tempStamps{1024};

    resolveWithBinaries(empty, binaryClauses, resolveAt, tempStamps);

    EXPECT_TRUE(empty.empty());
}

TEST(UnitSolver, resolveWithBinaries_clauseWithoutResOpportunityIsFixpoint) {
    CNFLit resolveAt{CNFVar{10}, CNFSign::POSITIVE};
    // representing binary clauses as a map from first literals to a list of
    // second literals
    std::unordered_map<CNFLit, std::vector<CNFLit>> binaryClauses;
    binaryClauses[resolveAt] = {12_Lit, 13_Lit};

    TrivialClause noResPossible{7_Lit, 10_Lit, 11_Lit};
    StampMap<int, CNFLit::Index> tempStamps{1024};
    TrivialClause expected = noResPossible;
    resolveWithBinaries(noResPossible, binaryClauses, resolveAt, tempStamps);

    EXPECT_TRUE(isPermutation(noResPossible, expected));
}

TEST(UnitSolver, resolveWithBinaries_noResolutionWhenNoBinaryClauses) {
    CNFLit resolveAt{CNFVar{10}, CNFSign::POSITIVE};
    // representing binary clauses as a map from first literals to a list of
    // second literals
    std::unordered_map<CNFLit, std::vector<CNFLit>> binaryClauses;

    TrivialClause noResPossible{1_Lit, 2_Lit};
    StampMap<int, CNFLit::Index> tempStamps{1024};
    TrivialClause expected = noResPossible;
    resolveWithBinaries(noResPossible, binaryClauses, resolveAt, tempStamps);

    EXPECT_TRUE(isPermutation(noResPossible, expected));
}

TEST(UnitSolver, resolveWithBinaries_allResolutionOpportunitiesAreUsed) {
    CNFLit resolveAt{CNFVar{5}, CNFSign::POSITIVE};
    // representing binary clauses as a map from first literals to a list of
    // second literals
    std::unordered_map<CNFLit, std::vector<CNFLit>> binaryClauses;
    binaryClauses[resolveAt] = {
        12_Lit,
        ~15_Lit,
        ~17_Lit,
        30_Lit,
    };

    TrivialClause testData{
        ~12_Lit, 15_Lit, ~30_Lit, ~3_Lit, 5_Lit,
    };

    StampMap<int, CNFLit::Index> tempStamps{1024};
    TrivialClause expected = {~3_Lit, 5_Lit};

    resolveWithBinaries(testData, binaryClauses, resolveAt, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}
}
