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

#include <libjamsat/simplification/UnaryOptimizations.h>
#include <libjamsat/utils/OccurrenceMap.h>

#include <toolbox/testutils/TestAssignmentProvider.h>


namespace jamsat {
using TrivialClause = TestAssignmentProviderClause;

namespace {
class TrivialClauseDeletedQuery {
public:
    auto operator()(TrivialClause const *clause) const noexcept {
        return clause->getFlag(TrivialClause::Flag::SCHEDULED_FOR_DELETION);
    }
};
}

class UnitUnaryOptimization : public ::testing::Test {
protected:
    UnitUnaryOptimization() : ::testing::Test() {
        lit1 = CNFLit{CNFVar{0}, CNFSign::NEGATIVE};
        lit2 = CNFLit{CNFVar{6}, CNFSign::POSITIVE};
        lit3 = CNFLit{CNFVar{10}, CNFSign::NEGATIVE};
        lit4 = CNFLit{CNFVar{22}, CNFSign::POSITIVE};
        lit5 = CNFLit{CNFVar{32}, CNFSign::POSITIVE};

        clause1 = TrivialClause{lit1, lit5, lit2};
        clause2 = TrivialClause{lit1, lit4, lit2};
        clause3 = TrivialClause{lit4, ~lit5, lit2};
        clause4 = TrivialClause{lit5, lit3, lit4};
    }

    void SetUp() override {
        testData.insert(clause1);
        testData.insert(clause2);
        testData.insert(clause3);
        testData.insert(clause4);
    }

    OccurrenceMap<TrivialClause, TrivialClauseDeletedQuery> testData{getMaxLit(CNFVar{32})};
    CNFLit lit1;
    CNFLit lit2;
    CNFLit lit3;
    CNFLit lit4;
    CNFLit lit5;
    TrivialClause clause1;
    TrivialClause clause2;
    TrivialClause clause3;
    TrivialClause clause4;
};

TEST_F(UnitUnaryOptimization, unarySubsumptionExactlyDeletesSubsumedClauses) {
    std::vector<CNFLit> unaries{lit1, lit5};

    auto delMarker = [](TrivialClause *) {};
    scheduleClausesSubsumedByUnariesForDeletion(testData, delMarker, unaries);
}

TEST_F(UnitUnaryOptimization, unarySubsumptionNotifiesPropagationAboutDeletions) {
    std::vector<CNFLit> unaries{lit1, lit5};

    std::vector<TrivialClause *> markedForDel;
    auto delMarker = [&markedForDel](TrivialClause *cla) { markedForDel.push_back(cla); };
    scheduleClausesSubsumedByUnariesForDeletion(testData, delMarker, unaries);

    std::vector<TrivialClause *> expectedToDel{&clause1, &clause2, &clause4};
    ASSERT_EQ(markedForDel.size(), expectedToDel.size());
    EXPECT_TRUE(
        std::is_permutation(markedForDel.begin(), markedForDel.end(), expectedToDel.begin()));
}

namespace {
template <typename A, typename B>
void expectPermutation(A const &seq1, B const &seq2) {
    ASSERT_EQ(seq1.size(), seq2.size());
    EXPECT_TRUE(std::is_permutation(seq1.begin(), seq1.end(), seq2.begin()));
}
}

TEST_F(UnitUnaryOptimization, strengthenWithUnariesExactlyStrenghensSuitableClauses) {
    std::vector<CNFLit> unaries{~lit3, lit5};

    auto delMarker = [](TrivialClause *) {};
    strengthenClausesWithUnaries(testData, delMarker, unaries);

    expectPermutation(clause1, TrivialClause{lit1, lit5, lit2});
    expectPermutation(clause2, TrivialClause{lit1, lit4, lit2});
    expectPermutation(clause3, TrivialClause{lit4, lit2});
    expectPermutation(clause4, TrivialClause{lit5, lit4});
}

TEST_F(UnitUnaryOptimization, strenghtenWithUnariesNotifiesPropagationAboutClauseModifications) {
    std::vector<CNFLit> unaries{~lit3, lit5};

    std::vector<TrivialClause *> markedForMod;
    auto modMarker = [&markedForMod](TrivialClause *cla) { markedForMod.push_back(cla); };
    strengthenClausesWithUnaries(testData, modMarker, unaries);

    std::vector<TrivialClause *> expectedToMod{&clause3, &clause4};
    ASSERT_EQ(markedForMod.size(), expectedToMod.size());
    EXPECT_TRUE(
        std::is_permutation(markedForMod.begin(), markedForMod.end(), expectedToMod.begin()));
}
}
