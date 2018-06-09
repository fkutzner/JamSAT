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

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/simplification/LightweightSimplifier.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/Trail.h>
#include <libjamsat/utils/StampMap.h>

#include <algorithm>
#include <vector>

#include <boost/range/algorithm_ext/erase.hpp>

namespace jamsat {
using TrailT = Trail<Clause>;
using PropagationT = Propagation<TrailT>;
using LightweightSimplifierT = LightweightSimplifier<PropagationT, TrailT>;

class IntegrationLightweightSimplifier : public ::testing::Test {
protected:
    IntegrationLightweightSimplifier()
      : ::testing::Test()
      , m_trail(CNFVar{1024})
      , m_propagation(CNFVar{1024}, m_trail)
      , m_stamps(getMaxLit(CNFVar{1024}).getRawValue())
      , underTest(CNFVar{1024}, m_propagation, m_trail) {}

    TrailT m_trail;
    PropagationT m_propagation;
    StampMap<uint16_t, CNFLit::Index> m_stamps;
    LightweightSimplifierT underTest;
};

TEST_F(IntegrationLightweightSimplifier, doesNotCreateNewClausesOnEmptyProblem) {
    std::vector<CNFLit> unaryClauses;
    std::vector<Clause *> possiblyIrredundantClauses;
    std::vector<Clause *> redundantClauses;

    underTest.simplify(unaryClauses, possiblyIrredundantClauses, redundantClauses, m_stamps);

    EXPECT_TRUE(unaryClauses.empty());
    EXPECT_TRUE(possiblyIrredundantClauses.empty());
    EXPECT_TRUE(redundantClauses.empty());
}

namespace {
std::unique_ptr<Clause> createClause(std::vector<CNFLit> const &lits) {
    auto result = createHeapClause(lits.size());
    std::copy(lits.begin(), lits.end(), result->begin());
    return result;
}
}

TEST_F(IntegrationLightweightSimplifier, minimizesUsingUnaries) {
    std::vector<CNFLit> rawClause1{1_Lit, ~2_Lit, 3_Lit};
    std::vector<CNFLit> rawClause2{5_Lit, 2_Lit, 6_Lit};
    std::vector<CNFLit> rawClause3{8_Lit, ~9_Lit};

    std::vector<CNFLit> unaries{1_Lit, ~2_Lit};

    auto clause1 = createClause(rawClause1);
    auto clause2 = createClause(rawClause2);
    auto clause3 = createClause(rawClause3);

    // The segmentation in possibly irredundant clauses and redundant
    // clauses is arbitrary in this test
    std::vector<Clause *> possiblyIrrClauses{clause1.get(), clause3.get()};
    std::vector<Clause *> redundantClauses{clause2.get()};

    underTest.simplify(unaries, possiblyIrrClauses, redundantClauses, m_stamps);

    EXPECT_TRUE(clause1->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
    EXPECT_FALSE(clause2->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
    EXPECT_FALSE(clause3->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));

    ASSERT_EQ(clause2->size(), 2ULL);
    ASSERT_EQ(clause3->size(), 2ULL);

    // Check that clause3 has not been changed:
    EXPECT_TRUE(std::equal(clause3->begin(), clause3->end(), rawClause3.begin()));

    // Check that clause2 has been strengthened:
    boost::remove_erase(rawClause2, 2_Lit);
    EXPECT_TRUE(std::is_permutation(clause2->begin(), clause2->end(), rawClause2.begin()));
}
}
