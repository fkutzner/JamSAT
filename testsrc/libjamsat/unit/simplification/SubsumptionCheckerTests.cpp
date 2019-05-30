/* Copyright (c) 2019 Felix Kutzner (github.com/fkutzner)

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

#include <toolbox/testutils/TestAssignmentProvider.h>

#include <iterator>
#include <ostream>
#include <vector>

namespace jamsat {
using TestClause = TestAssignmentProviderClause;

namespace {
struct SSRResult {
    std::vector<TestClause const*> subsumedClauses;
    std::vector<SSROpportunity<TestClause>> ssrOpportunities;
};

constexpr TestClause::size_type maxTestSubsumeeSize = 6;

SSRResult applySubsumptionCheck(TestClause const& subsumerCandidate,
                                std::vector<TestClause const*> const& subsumeeCandidates) {
    SSRResult result;
    getSubsumedClauses(subsumerCandidate,
                       subsumeeCandidates,
                       maxTestSubsumeeSize,
                       std::back_inserter(result.subsumedClauses),
                       std::back_inserter(result.ssrOpportunities));

    return result;
}
}

template <typename Clause>
auto operator==(SSROpportunity<Clause> const& lhs, SSROpportunity<Clause> const& rhs) -> bool {
    return lhs.resolveAtIdx == rhs.resolveAtIdx && lhs.clause == rhs.clause;
}

template <typename Clause>
auto operator<<(std::ostream& stream, SSROpportunity<Clause> const& toPrint) -> std::ostream& {
    stream << "(" << toPrint.resolveAtIdx << ", " << std::addressof(*toPrint.clause) << ")";
    return stream;
}

TEST(UnitSimplification, SubsumptionCheckWithEmptySubsumeeCandidateRange) {
    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit}, {});
    EXPECT_TRUE(result.subsumedClauses.empty());
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithTooLargeSubsumerClause) {
    TestClause subsumeeCandidate1{1_Lit, 2_Lit};
    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit}, {&subsumeeCandidate1});
    EXPECT_TRUE(result.subsumedClauses.empty());
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithSmallIrrelevantClause) {
    TestClause subsumeeCandidate1{1_Lit, 2_Lit, 3_Lit};
    SSRResult result = applySubsumptionCheck({1_Lit, 4_Lit}, {&subsumeeCandidate1});
    EXPECT_TRUE(result.subsumedClauses.empty());
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithSmallSubsumingClause) {
    TestClause subsumeeCandidate1{1_Lit, 2_Lit, 3_Lit};
    SSRResult result = applySubsumptionCheck({1_Lit, 3_Lit}, {&subsumeeCandidate1});
    EXPECT_THAT(result.subsumedClauses, testing::UnorderedElementsAre(&subsumeeCandidate1));
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithSmallSSRClause) {
    TestClause subsumeeCandidate1{1_Lit, 2_Lit, 3_Lit};
    SSRResult result = applySubsumptionCheck({1_Lit, ~2_Lit, 3_Lit}, {&subsumeeCandidate1});
    EXPECT_TRUE(result.subsumedClauses.empty());
    EXPECT_THAT(
        result.ssrOpportunities,
        testing::UnorderedElementsAre(SSROpportunity<TestClause>{1ull, &subsumeeCandidate1}));
}

TEST(UnitSimplification, SubsumptionCheckIgnoresOversizedClauses) {
    TestClause subsumeeCandidateTooLarge;

    for (CNFVar v{1}; v <= CNFVar{maxTestSubsumeeSize}; v = nextCNFVar(v)) {
        subsumeeCandidateTooLarge.push_back(CNFLit{v, CNFSign::POSITIVE});
    }
    subsumeeCandidateTooLarge.clauseUpdated();

    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit}, {&subsumeeCandidateTooLarge});
    EXPECT_TRUE(result.subsumedClauses.empty());
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckDetectsMaximumSizeSubsumeeCandidates) {
    TestClause subsumeeCandidate;

    for (CNFVar v{1}; v < CNFVar{maxTestSubsumeeSize}; v = nextCNFVar(v)) {
        subsumeeCandidate.push_back(CNFLit{v, CNFSign::POSITIVE});
    }
    subsumeeCandidate.clauseUpdated();

    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit}, {&subsumeeCandidate});
    EXPECT_THAT(result.subsumedClauses, testing::UnorderedElementsAre(&subsumeeCandidate));
    EXPECT_TRUE(result.ssrOpportunities.empty());
}

TEST(UnitSimplification, SubsumptionCheckWithMultiplePotentialSubsumers) {
    TestClause subsumeeCandidate1{1_Lit, 2_Lit, 3_Lit, 4_Lit};
    TestClause subsumeeCandidate2{5_Lit, 6_Lit};
    TestClause subsumeeCandidate3{1_Lit, 2_Lit, ~3_Lit};
    TestClause subsumeeCandidate4{1_Lit, ~2_Lit, 3_Lit};
    TestClause subsumeeCandidate5{1_Lit, 5_Lit, 3_Lit, 2_Lit};


    SSRResult result = applySubsumptionCheck({1_Lit, 2_Lit, 3_Lit},
                                             {&subsumeeCandidate1,
                                              &subsumeeCandidate2,
                                              &subsumeeCandidate3,
                                              &subsumeeCandidate4,
                                              &subsumeeCandidate5});

    EXPECT_THAT(result.subsumedClauses,
                testing::UnorderedElementsAre(&subsumeeCandidate1, &subsumeeCandidate5));
    EXPECT_THAT(result.ssrOpportunities,
                testing::UnorderedElementsAre(SSROpportunity<TestClause>{2, &subsumeeCandidate3},
                                              SSROpportunity<TestClause>{1, &subsumeeCandidate4}));
}

}