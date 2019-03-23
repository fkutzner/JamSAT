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
#include <iterator>

#include <gtest/gtest.h>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/ClauseDBReductionPolicies.h>

#include <cstdint>

namespace jamsat {

// In many tests, no actual clauses are needed, just pointers to clauses.
class TrivialClause {
public:
    template <typename LBDType>
    auto getLBD() -> LBDType {
        return 0;
    }

    template <typename LBDType>
    void setLBD(LBDType) {}

    auto size() const noexcept -> std::size_t { return 2; }
};

using TrivialClauseSeq = std::vector<TrivialClause*>;

TEST(UnitSolver, GlucoseClauseDBReductionPolicy_forbidsReductionWhenNoClauseHasBeenLearned) {
    TrivialClauseSeq emptyClauseList;
    GlucoseClauseDBReductionPolicy<TrivialClause, TrivialClauseSeq, int> underTest{10,
                                                                                   emptyClauseList};
    EXPECT_FALSE(underTest.shouldReduceDB());
}

TEST(UnitSolver, GlucoseClauseDBReductionPolicy_firstReductionPossibleAtArbitraryTime) {
    TrivialClause l1;
    TrivialClauseSeq learntClauses{&l1};
    GlucoseClauseDBReductionPolicy<TrivialClause, TrivialClauseSeq, int> underTest{10,
                                                                                   learntClauses};

    ASSERT_TRUE(underTest.shouldReduceDB());
    for (int i = 0; i < 24; ++i) {
        underTest.registerConflict();
        ASSERT_TRUE(underTest.shouldReduceDB());
    }
}

TEST(UnitSolver, GlucoseClauseDBReductionPolicy_reductionForbiddenJustAfterFirstReduction) {
    TrivialClause l1;
    TrivialClauseSeq learntClauses{&l1};
    GlucoseClauseDBReductionPolicy<TrivialClause, TrivialClauseSeq, int> underTest{10,
                                                                                   learntClauses};
    ASSERT_TRUE(underTest.shouldReduceDB());
    underTest.getClausesMarkedForDeletion(0);
    EXPECT_FALSE(underTest.shouldReduceDB());
}

TEST(UnitSolver, GlucoseClauseDBReductionPolicy_reductionIntervalsAreIncreased) {
    TrivialClause l1;
    TrivialClauseSeq learntClauses{&l1};
    GlucoseClauseDBReductionPolicy<TrivialClause, TrivialClauseSeq, int> underTest{5,
                                                                                   learntClauses};

    ASSERT_TRUE(underTest.shouldReduceDB());
    underTest.getClausesMarkedForDeletion(0);
    for (int i = 0; i < 5; ++i) {
        ASSERT_FALSE(underTest.shouldReduceDB());
        underTest.registerConflict();
    }

    ASSERT_TRUE(underTest.shouldReduceDB());
    underTest.getClausesMarkedForDeletion(0);
    for (int i = 0; i < 10; ++i) {
        ASSERT_FALSE(underTest.shouldReduceDB());
        underTest.registerConflict();
    }

    ASSERT_TRUE(underTest.shouldReduceDB());
}

TEST(UnitSolver, GlucoseClauseDBReductionPolicy_noReductionForTooManyKnownGoodClauses) {
    TrivialClause l1, l2, l3;
    TrivialClauseSeq learntClauses{&l1, &l2, &l3};
    GlucoseClauseDBReductionPolicy<TrivialClause, TrivialClauseSeq, int> underTest{10,
                                                                                   learntClauses};
    ASSERT_TRUE(underTest.shouldReduceDB());
    underTest.getClausesMarkedForDeletion(4);
    EXPECT_FALSE(underTest.shouldReduceDB());
}

namespace {
/**
 * \brief Tests whether the correct clauses are selected for reduction.
 *
 * \param LBDs                     A sequence of LBDs. For each value, a clause with the
 *                                 corresponding LBD value is created.
 * \param knownGoods               The amount of "known good" clauses, passed to the policy.
 * \param expectedDeletedIndices   The indices of the clauses expected to be deleted, given as
 *                                 indices to \p LBDs
 */
void test_GlucoseClauseDBReductionPolicy_markedForDeletion(
    const std::vector<int>& LBDs,
    uint16_t knownGoods,
    const std::vector<uint16_t>& expectedDeletedIndices) {
    std::vector<std::unique_ptr<Clause>> clauses;
    for (auto lbd : LBDs) {
        clauses.push_back(createHeapClause(3));
        clauses.back()->setLBD(lbd);
    }

    std::vector<Clause*> learntClauses;
    for (auto& clause : clauses) {
        learntClauses.push_back(clause.get());
    }
    auto originalLearntClauses = learntClauses;

    GlucoseClauseDBReductionPolicy<Clause, std::vector<Clause*>, int> underTest{10, learntClauses};
    ASSERT_TRUE(underTest.shouldReduceDB());
    auto toDeleteBegin = underTest.getClausesMarkedForDeletion(knownGoods);

    for (auto idx : expectedDeletedIndices) {
        Clause* expected = originalLearntClauses[idx];
        ASSERT_TRUE(std::find(toDeleteBegin, learntClauses.end(), expected) != learntClauses.end())
            << "Clause at index " << idx << " has not been marked for deletion.";
    }

    ASSERT_TRUE(std::distance(toDeleteBegin, learntClauses.end()) ==
                static_cast<int>(expectedDeletedIndices.size()))
        << "More clauses marked for deletion than expected";
}
}

TEST(UnitSolver, GlucoseClauseDBReductionPolicy_worstHalfOfClausesIsMarkedForDeletion) {
    test_GlucoseClauseDBReductionPolicy_markedForDeletion({6, 2, 4, 3}, 0, {0, 2});
}

TEST(UnitSolver, GlucoseClauseDBReductionPolicy_knownGoodValueShrinksRangeOfDeletedClauses) {
    test_GlucoseClauseDBReductionPolicy_markedForDeletion({6, 2, 4, 3}, 2, {0});
}

TEST(UnitSolver, GlucoseClauseDBReductionPolicy_noClausesMarkedForDeletionWhenLBDTooLow) {
    test_GlucoseClauseDBReductionPolicy_markedForDeletion({2, 2, 3, 6}, 0, {});
}
}
