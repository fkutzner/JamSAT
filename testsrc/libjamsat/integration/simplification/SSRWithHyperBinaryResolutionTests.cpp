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
#include <libjamsat/simplification/SSRWithHyperBinaryResolution.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/Trail.h>
#include <libjamsat/utils/OccurrenceMap.h>
#include <libjamsat/utils/StampMap.h>

#include <toolbox/testutils/ClauseUtils.h>

#include <algorithm>
#include <functional>
#include <unordered_set>

namespace jamsat {
using TrailT = Trail<Clause>;
using PropagationT = Propagation<TrailT>;

class ClauseDeletedQuery {
public:
    bool operator()(Clause const *x) const noexcept {
        return x->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
    }
};

class IntegrationSSRWithHyperBinaryResolution : public ::testing::Test {
protected:
    IntegrationSSRWithHyperBinaryResolution()
      : ::testing::Test()
      , m_trail(CNFVar{1024})
      , m_propagation(CNFVar{1024}, m_trail)
      , m_stamps(getMaxLit(CNFVar{1024}).getRawValue())
      , m_notifyClauseModification([this](Clause *c) {
          m_propagation.notifyClauseModificationAhead(*c);
          m_notifiedModifications.insert(c);
      })
      , m_occurrenceMap(getMaxLit(CNFVar{1024}))
      , m_notifiedModifications() {}


    auto createAndRegClause(std::initializer_list<CNFLit> literals) -> std::unique_ptr<Clause> {
        auto result = createClause(literals);
        m_occurrenceMap.insert(*result);
        m_propagation.registerClause(*result);
        return result;
    }

    auto performSSRWithHBR(CNFLit resolveAt) -> SimplificationStats {
        return ssrWithHyperBinaryResolution(m_occurrenceMap, m_notifyClauseModification,
                                            m_propagation, m_trail, m_stamps, resolveAt);
    }

    void expectDeleted(Clause const &c) {
        EXPECT_TRUE(c.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
        EXPECT_TRUE(m_notifiedModifications.find(&c) != m_notifiedModifications.end());
    }

    void expectUnmodified(Clause const &c) {
        EXPECT_FALSE(c.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
        EXPECT_TRUE(m_notifiedModifications.find(&c) == m_notifiedModifications.end());
    }

    void expectModifiedButNotDeleted(Clause const &c) {
        EXPECT_FALSE(c.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION));
        EXPECT_TRUE(m_notifiedModifications.find(&c) != m_notifiedModifications.end());
    }

    TrailT m_trail;
    PropagationT m_propagation;
    StampMap<uint16_t, CNFLit::Index> m_stamps;
    std::function<void(Clause *)> m_notifyClauseModification;
    OccurrenceMap<Clause, ClauseDeletedQuery> m_occurrenceMap;

    std::unordered_set<const Clause *> m_notifiedModifications;
};

TEST_F(IntegrationSSRWithHyperBinaryResolution, DeletesClausesDirectlySubsumedByBinaries) {
    auto subsuming = createAndRegClause({1_Lit, ~2_Lit});
    auto subsumed = createAndRegClause({5_Lit, 1_Lit, 6_Lit, ~2_Lit});
    performSSRWithHBR(~2_Lit);
    expectDeleted(*subsumed);
    expectUnmodified(*subsuming);
}

TEST_F(IntegrationSSRWithHyperBinaryResolution, DeletesClausesIndirectlySubsumed) {
    auto subsuming = createAndRegClause({1_Lit, ~10_Lit});
    auto subsuming2 = createAndRegClause({10_Lit, ~2_Lit});
    auto subsumed = createAndRegClause({5_Lit, 1_Lit, 6_Lit, ~2_Lit});
    performSSRWithHBR(~2_Lit);
    expectDeleted(*subsumed);
    expectUnmodified(*subsuming);
    expectUnmodified(*subsuming2);
}

TEST_F(IntegrationSSRWithHyperBinaryResolution, StrengthensClausesWithBinaries) {
    auto strengthening = createAndRegClause({8_Lit, 9_Lit});
    auto strengthened = createAndRegClause({~8_Lit, 10_Lit, 6_Lit, 9_Lit});
    performSSRWithHBR(9_Lit);
    expectUnmodified(*strengthening);
    expectModifiedButNotDeleted(*strengthened);
    expectClauseEqual(*strengthened, {10_Lit, 6_Lit, 9_Lit});
}

TEST_F(IntegrationSSRWithHyperBinaryResolution, StrengthensClausesWithIndirectBinaries) {
    auto strengthening1 = createAndRegClause({11_Lit, 9_Lit});
    auto strengthening2 = createAndRegClause({12_Lit, 9_Lit});
    auto strengthening3 = createAndRegClause({~11_Lit, ~12_Lit, ~8_Lit});
    auto strengthened = createAndRegClause({8_Lit, 10_Lit, 6_Lit, 9_Lit});
    performSSRWithHBR(9_Lit);

    expectUnmodified(*strengthening1);
    expectUnmodified(*strengthening2);
    expectUnmodified(*strengthening3);
    expectModifiedButNotDeleted(*strengthened);
    expectClauseEqual(*strengthened, {10_Lit, 6_Lit, 9_Lit});
}

TEST_F(IntegrationSSRWithHyperBinaryResolution, ReasonClausesAreNotModified) {
    auto bin1 = createAndRegClause({1_Lit, 2_Lit});
    auto bin2 = createAndRegClause({1_Lit, 3_Lit});
    auto reasonFor4 = createAndRegClause({1_Lit, ~2_Lit, ~3_Lit, 4_Lit});
    performSSRWithHBR(1_Lit);
    expectUnmodified(*bin1);
    expectUnmodified(*bin2);
    expectUnmodified(*reasonFor4);
}

TEST_F(IntegrationSSRWithHyperBinaryResolution, NoClausesModifiedForFailedLiterals) {
    auto bin1 = createAndRegClause({~1_Lit, 2_Lit});
    auto bin2 = createAndRegClause({~2_Lit, 3_Lit});
    auto bin3 = createAndRegClause({~3_Lit, ~1_Lit});

    EXPECT_THROW(performSSRWithHBR(~1_Lit), FailedLiteralException<Clause>);

    expectUnmodified(*bin1);
    expectUnmodified(*bin2);
    expectUnmodified(*bin3);
}

TEST_F(IntegrationSSRWithHyperBinaryResolution, NoClausesModifiedForUnitLiterals) {
    auto bin1 = createAndRegClause({~1_Lit, 2_Lit});
    auto bin2 = createAndRegClause({~1_Lit, 2_Lit, 3_Lit});

    m_trail.addAssignment(1_Lit);
    performSSRWithHBR(1_Lit);
    expectUnmodified(*bin1);
    expectUnmodified(*bin2);
}

TEST_F(IntegrationSSRWithHyperBinaryResolution, OnlyClausesContainingResolveAtAreStrengthened) {
    auto clause1 = createAndRegClause({1_Lit, 2_Lit});
    auto clause2 = createAndRegClause({1_Lit, ~2_Lit, 3_Lit, 4_Lit});
    auto clause3 = createAndRegClause({~2_Lit, ~1_Lit});

    performSSRWithHBR(1_Lit);
    performSSRWithHBR(~2_Lit);

    expectUnmodified(*clause1);
    expectModifiedButNotDeleted(*clause2);
    expectClauseEqual(*clause2, {1_Lit, 3_Lit, 4_Lit});
    expectUnmodified(*clause3);
}
}
