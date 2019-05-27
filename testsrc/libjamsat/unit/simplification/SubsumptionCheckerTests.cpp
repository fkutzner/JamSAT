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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/simplification/SubsumptionChecker.h>
#include <libjamsat/utils/StampMap.h>

#include <toolbox/testutils/TestAssignmentProvider.h>

#include <iterator>
#include <ostream>
#include <vector>

namespace jamsat {
using TestClause = TestAssignmentProviderClause;

namespace {
struct SSRResult {
    std::vector<SSROpportunity<TestClause>> ssrOpportunities;
    bool subsumed;
};

SSRResult applySubsumptionCheck(TestClause const& subsumeeCandidate,
                                std::vector<TestClause const*> const& subsumerCandidates) {
    StampMap<std::uint8_t, CNFLit::Index> stampMap{CNFLit::Index::getIndex(getMaxLit(CNFVar{20}))};
    std::vector<SSROpportunity<TestClause>> ssrOpportunities;

    bool subsumed = isSubsumedBy(
        subsumeeCandidate, subsumerCandidates, stampMap, std::back_inserter(ssrOpportunities));

    return SSRResult{ssrOpportunities, subsumed};
}
}

template <typename Clause>
auto operator==(SSROpportunity<Clause> const& lhs, SSROpportunity<Clause> const& rhs) -> bool {
    return lhs.resolveAt == rhs.resolveAt && lhs.clause == rhs.clause;
}

template <typename Clause>
auto operator<<(std::ostream& stream, SSROpportunity<Clause> const& toPrint) -> std::ostream& {
    stream << "(" << toPrint.resolveAt << ", " << std::addressof(*toPrint.clause) << ")";
    return stream;
}

TEST(UnitSimplification, SubsumptionCheckWithEmptySubsumerCandidateRange) {
    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit}, {});
    EXPECT_FALSE(result.subsumed);
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithTooLargeSubsumerClause) {
    TestClause subsumerCandidate1{1_Lit, 2_Lit, 3_Lit};
    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit}, {&subsumerCandidate1});
    EXPECT_FALSE(result.subsumed);
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithSmallIrrelevantClause) {
    TestClause subsumerCandidate1{1_Lit, 4_Lit};
    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit}, {&subsumerCandidate1});
    EXPECT_FALSE(result.subsumed);
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithSmallSubsumingClause) {
    TestClause subsumerCandidate1{1_Lit, 3_Lit};
    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit}, {&subsumerCandidate1});
    EXPECT_TRUE(result.subsumed);
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithSmallSSRClause) {
    TestClause subsumerCandidate1{1_Lit, ~2_Lit, 3_Lit};
    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit}, {&subsumerCandidate1});
    EXPECT_FALSE(result.subsumed);
    EXPECT_THAT(
        result.ssrOpportunities,
        testing::UnorderedElementsAre(SSROpportunity<TestClause>{~2_Lit, &subsumerCandidate1}));
}


namespace {
auto createLargeClause() -> TestClause {
    return {1_Lit, 2_Lit, 3_Lit, 4_Lit, 5_Lit, 6_Lit, 7_Lit, 8_Lit, 9_Lit, 10_Lit, 11_Lit};
}
}

TEST(UnitSimplification, SubsumptionCheckWithLargeIrrelevantClause) {
    TestClause subsumerCandidate1 = createLargeClause();
    subsumerCandidate1.resize(10);
    subsumerCandidate1[2] = ~subsumerCandidate1[2];
    subsumerCandidate1[4] = CNFLit{CNFVar{19}, CNFSign::POSITIVE};

    SSRResult result = applySubsumptionCheck(createLargeClause(), {&subsumerCandidate1});
    EXPECT_FALSE(result.subsumed);
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithLargeSubsumingClause) {
    TestClause subsumerCandidate1 = createLargeClause();
    subsumerCandidate1.resize(10);

    SSRResult result = applySubsumptionCheck(createLargeClause(), {&subsumerCandidate1});
    EXPECT_TRUE(result.subsumed);
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithLargeSSRClause) {
    TestClause subsumerCandidate1 = createLargeClause();
    subsumerCandidate1[8] = ~subsumerCandidate1[8];

    SSRResult result = applySubsumptionCheck(createLargeClause(), {&subsumerCandidate1});
    EXPECT_FALSE(result.subsumed);
    EXPECT_THAT(result.ssrOpportunities,
                testing::UnorderedElementsAre(
                    SSROpportunity<TestClause>{subsumerCandidate1[8], &subsumerCandidate1}));
}

TEST(UnitSimplification, SubsumptionCheckWithMultiplePotentialSubsumers) {
    TestClause subsumerCandidate1{1_Lit, 2_Lit};
    TestClause subsumerCandidate2{5_Lit, 6_Lit};
    TestClause subsumerCandidate3{1_Lit, 2_Lit, ~3_Lit};
    TestClause subsumerCandidate4{1_Lit, ~2_Lit, 3_Lit};


    SSRResult result = applySubsumptionCheck(
        {1_Lit, 2_Lit, 3_Lit},
        {&subsumerCandidate1, &subsumerCandidate2, &subsumerCandidate3, &subsumerCandidate4});
    EXPECT_TRUE(result.subsumed);
    EXPECT_THAT(
        result.ssrOpportunities,
        testing::UnorderedElementsAre(SSROpportunity<TestClause>{~3_Lit, &subsumerCandidate3},
                                      SSROpportunity<TestClause>{~2_Lit, &subsumerCandidate4}));
}
}