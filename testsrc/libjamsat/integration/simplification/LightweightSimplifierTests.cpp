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
#include <libjamsat/solver/FirstUIPLearning.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/Trail.h>
#include <libjamsat/utils/StampMap.h>

#include <toolbox/testutils/ClauseUtils.h>

#include <algorithm>
#include <vector>

#include <boost/range/algorithm_ext/erase.hpp>

namespace jamsat {
using TrailT = Trail<Clause>;
using PropagationT = Propagation<TrailT>;
using ConflictAnalyzerT = FirstUIPLearning<TrailT, PropagationT>;
using LightweightSimplifierT = LightweightSimplifier<PropagationT, TrailT, ConflictAnalyzerT>;

class IntegrationLightweightSimplifier : public ::testing::Test {
protected:
    IntegrationLightweightSimplifier()
      : ::testing::Test()
      , m_trail(CNFVar{24})
      , m_propagation(CNFVar{24}, m_trail)
      , m_stamps(getMaxLit(CNFVar{24}).getRawValue())
      , underTest(CNFVar{24}, m_propagation, m_trail) {}

    std::unique_ptr<Clause> createAndRegClause(std::vector<CNFLit> const &lits) {
        auto result = createHeapClause(lits.size());
        std::copy(lits.begin(), lits.end(), result->begin());
        m_propagation.registerClause(*result);
        return result;
    }

    std::unique_ptr<Clause> createAndRegClause(std::initializer_list<CNFLit> lits) {
        auto result = createClause(lits);
        m_propagation.registerClause(*result);
        return result;
    }

    TrailT m_trail;
    PropagationT m_propagation;
    StampMap<uint16_t, CNFLit::Index> m_stamps;
    LightweightSimplifierT underTest;
};

TEST_F(IntegrationLightweightSimplifier, doesNotCreateNewClausesOnEmptyProblem) {
    // Tests clauses subsumed by unary clauses are scheduled for deletion:

    std::vector<CNFLit> unaryClauses;
    std::vector<Clause *> possiblyIrredundantClauses;
    std::vector<Clause *> redundantClauses;

    underTest.simplify(unaryClauses, possiblyIrredundantClauses, redundantClauses, m_stamps);

    EXPECT_TRUE(unaryClauses.empty());
    EXPECT_TRUE(possiblyIrredundantClauses.empty());
    EXPECT_TRUE(redundantClauses.empty());
}

TEST_F(IntegrationLightweightSimplifier, minimizesUsingUnaries) {
    // Tests that negates of literals occurring in unary clauses are
    // removed from all problem clauses:

    std::vector<CNFLit> rawClause1{1_Lit, ~2_Lit, 3_Lit};
    std::vector<CNFLit> rawClause2{5_Lit, 2_Lit, 6_Lit};
    std::vector<CNFLit> rawClause3{8_Lit, ~9_Lit};

    std::vector<CNFLit> unaries{1_Lit, ~2_Lit};

    auto clause1 = createAndRegClause(rawClause1);
    auto clause2 = createAndRegClause(rawClause2);
    auto clause3 = createAndRegClause(rawClause3);

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

TEST_F(IntegrationLightweightSimplifier, eliminatesFailedLiteralsViaRestrictedFLE) {
    // Tests that failed-literal elimination correctly eliminates failed
    // literals where the lemma's asserting literal is different from the
    // originally detected failed literal (with restricted FLE):

    auto clause1 = createAndRegClause({~1_Lit, 2_Lit});
    auto clause2 = createAndRegClause({~2_Lit, 3_Lit});
    auto clause3 = createAndRegClause({~3_Lit, 4_Lit});
    auto clause4 = createAndRegClause({~3_Lit, 5_Lit});
    auto clause5 = createAndRegClause({~4_Lit, 6_Lit});
    auto clause6 = createAndRegClause({~5_Lit, ~6_Lit});
    auto clause7 = createAndRegClause({1_Lit, ~8_Lit, 20_Lit});


    // should detect that 3_Lit needs to be set to false:
    std::vector<CNFLit> unaries{10_Lit};
    std::vector<Clause *> possiblyIrrClauses{clause1.get(), clause2.get(), clause3.get(),
                                             clause4.get(), clause5.get(), clause6.get(),
                                             clause7.get()};
    std::vector<Clause *> redundantClauses;
    underTest.simplify(unaries, possiblyIrrClauses, redundantClauses, m_stamps);

    std::vector<CNFLit> expectedUnaries{~3_Lit, ~2_Lit, ~1_Lit, 10_Lit};

    ASSERT_EQ(unaries.size(), expectedUnaries.size());
    EXPECT_TRUE(std::is_permutation(unaries.begin(), unaries.end(), expectedUnaries.begin()));

    EXPECT_TRUE(clause1->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
    EXPECT_TRUE(clause2->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
    EXPECT_TRUE(clause3->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
    EXPECT_TRUE(clause4->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
    EXPECT_FALSE(clause5->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
    EXPECT_FALSE(clause6->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
    EXPECT_FALSE(clause7->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));

    expectClauseEqual(*clause5, {~4_Lit, 6_Lit});
    expectClauseEqual(*clause6, {~5_Lit, ~6_Lit});
    expectClauseEqual(*clause7, {~8_Lit, 20_Lit});
}

TEST_F(IntegrationLightweightSimplifier, detectsUnsatViaRestrictedFailedLiteralElimination) {
    // Tests that failed-literal elimination correctly handles situations where
    // both a and -a are failed literals for some variable a, indicating that the
    // problem is unsatisfiable (with restricted FLE):

    auto clause1 = createAndRegClause({~1_Lit, 2_Lit});
    auto clause2 = createAndRegClause({~2_Lit, 3_Lit});
    auto clause3 = createAndRegClause({~3_Lit, 4_Lit});
    auto clause4 = createAndRegClause({~3_Lit, 5_Lit});
    auto clause5 = createAndRegClause({~4_Lit, 6_Lit});
    auto clause6 = createAndRegClause({~5_Lit, ~6_Lit});
    auto clause7 = createAndRegClause({1_Lit, 2_Lit});


    // Each assignment of 1_Lit leads to a conflict. The simplifier should
    // append conflicting unaries to the end of the unaries vector:
    std::vector<CNFLit> unaries{10_Lit};
    std::vector<Clause *> possiblyIrrClauses{clause1.get(), clause2.get(), clause3.get(),
                                             clause4.get(), clause5.get(), clause6.get(),
                                             clause7.get()};
    std::vector<Clause *> redundantClauses;
    underTest.simplify(unaries, possiblyIrrClauses, redundantClauses, m_stamps);

    ASSERT_GE(unaries.size(), 2ULL);
    EXPECT_TRUE(unaries.back() == ~unaries[unaries.size() - 2]);
}

TEST_F(IntegrationLightweightSimplifier, eliminatesFailedLiteralsWithDecoupledUIPViaRestrictedFLE) {
    // Tests that failed-literal elimination correctly handles situations like
    // (a -> b), (a -> c), ((b and c) -> d), (d -> e), (d -> -e)
    // Here, d is detected as UIP, but setting -d does not force the assignment -a
    // in a single propagation step. Check that both -d and -a are learned
    // (with restricted FLE):

    auto clause1 = createAndRegClause({~1_Lit, 2_Lit});
    auto clause2 = createAndRegClause({~1_Lit, 3_Lit});
    auto clause3 = createAndRegClause({~2_Lit, ~3_Lit, 4_Lit});
    auto clause4 = createAndRegClause({~4_Lit, 5_Lit});
    auto clause5 = createAndRegClause({~4_Lit, 6_Lit});
    auto clause6 = createAndRegClause({~5_Lit, 7_Lit});
    auto clause7 = createAndRegClause({~6_Lit, ~7_Lit});

    // For FLE with literal 1_Lit, the asserting literal is ~4_Lit,
    // but in this case ~1_Lit is not directly obtained by propagating
    // ~4_Lit. (This is due to clause3)

    std::vector<CNFLit> unaries{10_Lit};
    std::vector<Clause *> possiblyIrrClauses{clause1.get(), clause2.get(), clause3.get(),
                                             clause4.get(), clause5.get(), clause6.get(),
                                             clause7.get()};
    std::vector<Clause *> redundantClauses;
    underTest.simplify(unaries, possiblyIrrClauses, redundantClauses, m_stamps);

    std::vector<CNFLit> expectedUnaries{10_Lit, ~1_Lit, ~4_Lit};
    ASSERT_EQ(unaries.size(), expectedUnaries.size());
    EXPECT_TRUE(std::is_permutation(unaries.begin(), unaries.end(), expectedUnaries.begin()));
}
}
