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

#include <gtest/gtest.h>

#include <libjamsat/simplification/VariableElimination.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/Printers.h>

#include <toolbox/testutils/TestAssignmentProvider.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace jamsat {

using TestClause = TestAssignmentProviderClause;

namespace {
class TestClauseDeletedQuery {
public:
    auto operator()(TestClause const* clause) const noexcept {
        return clause->getFlag(TestClause::Flag::SCHEDULED_FOR_DELETION);
    }
};

template <typename Iterable1, typename Iterable2>
auto isPermutationOfPermutations(Iterable1 const& iterable1, Iterable2 const& iterable2) -> bool {
    for (auto& item1 : iterable1) {
        bool found = false;
        for (auto& item2 : iterable2) {
            if (std::distance(item1.begin(), item1.end()) !=
                std::distance(item2.begin(), item2.end())) {
                continue;
            }

            found |= std::is_permutation(item1.begin(), item1.end(), item2.begin());
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

template <typename CNFLitContainers>
auto toString(CNFLitContainers const& litContainers) {
    std::stringstream result;
    for (auto& litContainer : litContainers) {
        result << "(" << toString(litContainer.begin(), litContainer.end()) << ")\n";
    }
    return result.str();
}

void testDistribution(std::vector<TestClause> input,
                      CNFVar distributeAt,
                      ClauseDistribution::DistributionStatus expectedStatus,
                      std::vector<TestClause> expectedDistributionClauses) {
    constexpr static CNFLit maxLit = 1024_Lit;

    OccurrenceMap<TestClause, TestClauseDeletedQuery> litOccurrences{maxLit};
    for (TestClause& c : input) {
        litOccurrences.insert(c);
    }

    ClauseDistribution underTest{maxLit.getVariable()};
    ClauseDistribution::DistributionResult result =
        underTest.distribute(litOccurrences, distributeAt);

    ASSERT_EQ(result.status, expectedStatus);
    ASSERT_EQ(result.numClauses, expectedDistributionClauses.size())
        << "Expected clauses:\n"
        << toString(expectedDistributionClauses) << "But got:\n"
        << toString(result.clauses);

    if (result.status == ClauseDistribution::DistributionStatus::OK) {
        EXPECT_TRUE(isPermutationOfPermutations(result.clauses, expectedDistributionClauses))
            << "Expected clauses:\n"
            << toString(expectedDistributionClauses) << "But got:\n"
            << toString(result.clauses);
    } else {
        EXPECT_EQ(result.numClauses, 0);
        auto distClauses = result.clauses;
        EXPECT_EQ(distClauses.begin(), distClauses.end());
    }
}

void testDistributionWorthwileCheck(std::vector<TestClause> input,
                                    CNFVar distributeAt,
                                    bool expectedWorthwile) {
    constexpr static CNFLit maxLit = 1024_Lit;

    OccurrenceMap<TestClause, TestClauseDeletedQuery> litOccurrences{maxLit};
    for (TestClause& c : input) {
        litOccurrences.insert(c);
    }

    ClauseDistribution underTest{maxLit.getVariable()};
    EXPECT_EQ(underTest.isDistributionWorthwile(litOccurrences, distributeAt), expectedWorthwile);
}
}

TEST(UnitSimplification, ClauseDistributionProducesNoClausesForEmptyInput) {
    testDistribution({}, 1_Var, ClauseDistribution::DistributionStatus::OK, {});
}

TEST(UnitSimplification, ClauseDistributionProducesNoClausesForIrrelevantInput) {
    testDistribution(
        {{2_Lit, 3_Lit}, {5_Lit, ~7_Lit}}, 1_Var, ClauseDistribution::DistributionStatus::OK, {});
}

TEST(UnitSimplification, ClauseDistributionEliminatesPureLiteralClauses) {
    testDistribution({{1_Lit, 3_Lit, 10_Lit}, {5_Lit, 1_Lit, 20_Lit}},
                     1_Var,
                     ClauseDistribution::DistributionStatus::OK,
                     {});
}

TEST(UnitSimplification, ClauseDistributionCanProduceUnaryClauses) {
    testDistribution({{1_Lit, 20_Lit}, {~1_Lit, 20_Lit}},
                     1_Var,
                     ClauseDistribution::DistributionStatus::OK,
                     {{20_Lit}});
}

TEST(UnitSimplification, ClauseDistributionEliminatesRedundantClauses) {
    testDistribution({{1_Lit, 2_Lit, 20_Lit}, {~1_Lit, ~2_Lit, ~20_Lit}},
                     1_Var,
                     ClauseDistribution::DistributionStatus::OK,
                     {});
}

TEST(UnitSimplification, ClauseDistributionComputesAllResolvents) {
    testDistribution({{4_Lit, 2_Lit, 3_Lit},
                      {5_Lit, 4_Lit, 6_Lit},
                      {~4_Lit, ~2_Lit},
                      {~7_Lit, ~4_Lit, 8_Lit},
                      {9_Lit, ~4_Lit, ~10_Lit}},
                     4_Var,
                     ClauseDistribution::DistributionStatus::OK,
                     {
                         {2_Lit, 3_Lit, ~7_Lit, 8_Lit},
                         {2_Lit, 3_Lit, 9_Lit, ~10_Lit},
                         {5_Lit, 6_Lit, ~2_Lit},
                         {5_Lit, 6_Lit, ~7_Lit, 8_Lit},
                         {5_Lit, 6_Lit, 9_Lit, ~10_Lit},
                     });
}

// TODO: test multiple invocations

TEST(UnitSimplification, ClauseDistributionNotWorthwileForEmptyClauseSet) {
    testDistributionWorthwileCheck({}, 1_Var, false);
}

TEST(UnitSimplification, ClauseDistributionWorthwileForPureLiteralClauses) {
    testDistributionWorthwileCheck({{1_Lit, 3_Lit, 10_Lit}, {5_Lit, 1_Lit, 20_Lit}}, 1_Var, true);
}

TEST(UnitSimplification, ClauseDistributionNotWorthwileWhenAsManyClausesGenerated) {
    testDistributionWorthwileCheck({{4_Lit, 2_Lit, 3_Lit},
                                    {5_Lit, 4_Lit, 6_Lit},
                                    {~4_Lit, ~2_Lit},
                                    {~7_Lit, ~4_Lit, 8_Lit},
                                    {9_Lit, ~4_Lit, ~10_Lit}},
                                   4_Var,
                                   false);
}

TEST(UnitSimplification, ClauseDistributionWorthwileWhenAsFewerClausesGenerated) {
    testDistributionWorthwileCheck({{4_Lit, 2_Lit, 3_Lit},
                                    {5_Lit, 4_Lit, 6_Lit},
                                    {~4_Lit, ~2_Lit},
                                    {~7_Lit, ~4_Lit, ~6_Lit},
                                    {9_Lit, ~4_Lit, ~10_Lit}},
                                   4_Var,
                                   true);
}


}