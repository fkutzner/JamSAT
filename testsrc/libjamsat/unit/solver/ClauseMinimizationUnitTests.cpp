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
#include <libjamsat/solver/ClauseMinimization.h>
#include <libjamsat/utils/StampMap.h>

#include "TestAssignmentProvider.h"
#include "TestReasonProvider.h"

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;

struct CNFVarKey {
    using Type = CNFVar;

    static size_t getIndex(CNFVar variable) { return static_cast<size_t>(variable.getRawValue()); }
};

struct CNFLitKey {
    using Type = CNFLit;

    static size_t getIndex(CNFLit literal) { return static_cast<size_t>(literal.getRawValue()); }
};

template <typename Container>
bool isPermutation(const Container &c1, const Container &c2) {
    return std::is_permutation(c1.begin(), c1.end(), c2.begin(), c2.end());
}

TEST(UnitSolver, eraseRedundantLiterals_fixpointOnEmptyClause) {
    TestReasonProvider<TrivialClause> reasonProvider;
    TestAssignmentProvider dlProvider;

    TrivialClause emptyClause;
    StampMap<int, CNFVarKey> tempStamps{1024};

    eraseRedundantLiterals(emptyClause, reasonProvider, dlProvider, tempStamps);

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

    StampMap<int, CNFVarKey> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 1);

    TrivialClause expected = testData;
    boost::remove_erase(expected, CNFLit{CNFVar{3}, CNFSign::NEGATIVE});

    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_removesTwoLevelRedundancy) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor3{
        CNFLit{CNFVar{3}, CNFSign::POSITIVE},
        CNFLit{CNFVar{4}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{5}, CNFSign::NEGATIVE},
    };
    reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

    TrivialClause reasonFor5{
        CNFLit{CNFVar{5}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{8}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{9}, CNFSign::NEGATIVE},
    };
    reasonProvider.setAssignmentReason(CNFVar{5}, reasonFor5);

    TrivialClause testData{
        CNFLit{CNFVar{1}, CNFSign::POSITIVE}, CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{4}, CNFSign::NEGATIVE}, CNFLit{CNFVar{8}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{9}, CNFSign::POSITIVE}};

    StampMap<int, CNFVarKey> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);

    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    for (CNFVar::RawVariable i = 1; i < 10; ++i) {
        dlProvider.setAssignmentDecisionLevel(CNFVar{i}, 1);
    }

    TrivialClause expected = testData;
    boost::remove_erase(expected, CNFLit{CNFVar{3}, CNFSign::NEGATIVE});

    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_removesSingleLevelRedundancyWithUnit) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor3{
        CNFLit{CNFVar{3}, CNFSign::POSITIVE},
        CNFLit{CNFVar{4}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{5}, CNFSign::NEGATIVE},
    };
    reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

    TrivialClause testData{CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                           CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                           CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};

    StampMap<int, CNFVarKey> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{5}, 0);

    TrivialClause expected = testData;
    boost::remove_erase(expected, CNFLit{CNFVar{3}, CNFSign::NEGATIVE});

    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_removesUnitLiteral) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause testData{CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                           CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                           CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};

    StampMap<int, CNFVarKey> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 0);

    TrivialClause expected = testData;
    boost::remove_erase(expected, CNFLit{CNFVar{4}, CNFSign::NEGATIVE});

    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, eraseRedundantLiterals_doesNotRemoveNonredundantLiteral) {
    TestReasonProvider<TrivialClause> reasonProvider;

    TrivialClause reasonFor3{CNFLit{CNFVar{3}, CNFSign::POSITIVE},
                             CNFLit{CNFVar{4}, CNFSign::NEGATIVE},
                             CNFLit{CNFVar{5}, CNFSign::POSITIVE}};
    reasonProvider.setAssignmentReason(CNFVar{3}, reasonFor3);

    TrivialClause testData{CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                           CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                           CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};

    StampMap<int, CNFVarKey> tempStamps{1024};
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

    TrivialClause reasonFor1{CNFLit{CNFVar{1}, CNFSign::NEGATIVE},
                             CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};
    reasonProvider.setAssignmentReason(CNFVar{1}, reasonFor1);

    TrivialClause testData{CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                           CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                           CNFLit{CNFVar{4}, CNFSign::NEGATIVE}};

    StampMap<int, CNFVarKey> tempStamps{1024};
    TestAssignmentProvider dlProvider;
    dlProvider.setCurrentDecisionLevel(2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{1}, 2);
    dlProvider.setAssignmentDecisionLevel(CNFVar{3}, 1);
    dlProvider.setAssignmentDecisionLevel(CNFVar{4}, 1);

    TrivialClause expected = testData;
    eraseRedundantLiterals(testData, reasonProvider, dlProvider, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}

TEST(UnitSolver, resolveWithBinaries_emptyClauseIsFixpoint) {
    CNFLit resolveAt{CNFVar{10}, CNFSign::POSITIVE};
    // representing binary clauses as a map from first literals to a list of
    // second literals
    std::unordered_map<CNFLit, std::vector<CNFLit>> binaryClauses;
    binaryClauses[resolveAt] = {CNFLit{CNFVar{9}, CNFSign::POSITIVE},
                                CNFLit{CNFVar{8}, CNFSign::POSITIVE}};

    TrivialClause empty;
    StampMap<int, CNFLitKey> tempStamps{1024};

    resolveWithBinaries(empty, binaryClauses, resolveAt, tempStamps);

    EXPECT_TRUE(empty.empty());
}

TEST(UnitSolver, resolveWithBinaries_clauseWithoutResOpportunityIsFixpoint) {
    CNFLit resolveAt{CNFVar{10}, CNFSign::POSITIVE};
    // representing binary clauses as a map from first literals to a list of
    // second literals
    std::unordered_map<CNFLit, std::vector<CNFLit>> binaryClauses;
    binaryClauses[resolveAt] = {CNFLit{CNFVar{12}, CNFSign::POSITIVE},
                                CNFLit{CNFVar{13}, CNFSign::POSITIVE}};

    TrivialClause noResPossible{CNFLit{CNFVar{7}, CNFSign::POSITIVE},
                                CNFLit{CNFVar{10}, CNFSign::POSITIVE},
                                CNFLit{CNFVar{11}, CNFSign::POSITIVE}};
    StampMap<int, CNFLitKey> tempStamps{1024};
    TrivialClause expected = noResPossible;
    resolveWithBinaries(noResPossible, binaryClauses, resolveAt, tempStamps);

    EXPECT_TRUE(isPermutation(noResPossible, expected));
}

TEST(UnitSolver, resolveWithBinaries_noResolutionWhenNoBinaryClauses) {
    CNFLit resolveAt{CNFVar{10}, CNFSign::POSITIVE};
    // representing binary clauses as a map from first literals to a list of
    // second literals
    std::unordered_map<CNFLit, std::vector<CNFLit>> binaryClauses;

    TrivialClause noResPossible{CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                                CNFLit{CNFVar{2}, CNFSign::POSITIVE}};
    StampMap<int, CNFLitKey> tempStamps{1024};
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
        CNFLit{CNFVar{12}, CNFSign::POSITIVE},
        CNFLit{CNFVar{15}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{17}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{30}, CNFSign::POSITIVE},
    };

    TrivialClause testData{
        CNFLit{CNFVar{12}, CNFSign::NEGATIVE}, CNFLit{CNFVar{15}, CNFSign::POSITIVE},
        CNFLit{CNFVar{30}, CNFSign::NEGATIVE}, CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
        CNFLit{CNFVar{5}, CNFSign::POSITIVE},
    };

    StampMap<int, CNFLitKey> tempStamps{1024};
    TrivialClause expected = {CNFLit{CNFVar{3}, CNFSign::NEGATIVE},
                              CNFLit{CNFVar{5}, CNFSign::POSITIVE}};

    resolveWithBinaries(testData, binaryClauses, resolveAt, tempStamps);

    EXPECT_TRUE(isPermutation(testData, expected));
}
}
