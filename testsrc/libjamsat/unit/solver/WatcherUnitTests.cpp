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

#include <gtest/gtest.h>

#include <libjamsat/solver/ClauseMarkers.h>
#include <libjamsat/solver/Watcher.h>
#include <toolbox/testutils/RangeUtils.h>

namespace jamsat {
namespace detail_propagation {

using TrivialClause = std::vector<CNFLit>;
using TestWatcher = Watcher<TrivialClause>;
using TestWatchers = Watchers<TrivialClause>;
using TestWatcherTraversal = WatcherTraversal<TestWatcher>;
using TestWatcherList = TestWatcherTraversal::WatcherList;

TEST(UnitSolver, watchersStoreClausesAndOtherLit) {
    TrivialClause testClause;
    CNFLit otherLiteral{CNFVar{10}, CNFSign::NEGATIVE};
    TestWatcher underTest{testClause, otherLiteral};

    EXPECT_EQ(&(underTest.getClause()), &testClause);
    EXPECT_EQ(underTest.getOtherWatchedLiteral(), otherLiteral);
}

TEST(UnitSolver, watchersWithSameClauseAndOtherLitAreEqual) {
    TrivialClause testClause;
    TestWatcher underTest1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcher underTest2{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};

    EXPECT_TRUE(underTest1 == underTest2);
    EXPECT_FALSE(underTest1 != underTest2);
}

TEST(UnitSolver, watchersWithSameClauseAndDifferentLitAreInequal) {
    TrivialClause testClause;
    TestWatcher underTest1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcher underTest2{testClause, CNFLit{CNFVar{11}, CNFSign::POSITIVE}};

    EXPECT_TRUE(underTest1 != underTest2);
    EXPECT_FALSE(underTest1 == underTest2);
}

TEST(UnitSolver, watchersWithDifferentClausesAreInequal) {
    TrivialClause testClause1;
    TrivialClause testClause2;
    TestWatcher underTest1{testClause1, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcher underTest2{testClause2, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};

    EXPECT_TRUE(underTest1 != underTest2);
    EXPECT_FALSE(underTest1 == underTest2);
}

TEST(UnitSolver, traverseEmptyWatcherList) {
    TestWatcherList empty;
    TestWatcherTraversal underTest{&empty};
    EXPECT_TRUE(underTest.hasFinishedTraversal());
}

TEST(UnitSolver, dereferenceWatcherTraversal) {
    TrivialClause testClause;

    TestWatcher watcher1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcherList watchers{watcher1};

    TestWatcherTraversal underTest{&watchers};
    ASSERT_FALSE(underTest.hasFinishedTraversal());
    EXPECT_EQ(&((*underTest).getClause()), &testClause);
    EXPECT_EQ(&(underTest->getClause()), &testClause);
}

TEST(UnitSolver, traverseWatcherListWithSingleElement) {
    TrivialClause testClause;

    TestWatcher watcher1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcherList watchers{watcher1};

    TestWatcherTraversal underTest{&watchers};
    ASSERT_FALSE(underTest.hasFinishedTraversal());
    EXPECT_EQ(&(*underTest), &watchers[0]);

    ++underTest;
    EXPECT_TRUE(underTest.hasFinishedTraversal());
}

TEST(UnitSolver, traverseWatcherListWithThreeElements) {
    TrivialClause testClause;

    TestWatcher watcher1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcher watcher2{testClause, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
    TestWatcher watcher3{testClause, CNFLit{CNFVar{12}, CNFSign::NEGATIVE}};
    TestWatcherList watchers{watcher1, watcher2, watcher3};

    TestWatcherTraversal underTest{&watchers};
    ASSERT_FALSE(underTest.hasFinishedTraversal());
    EXPECT_EQ(&(*underTest), &watchers[0]);

    ++underTest;
    ASSERT_FALSE(underTest.hasFinishedTraversal());
    EXPECT_EQ(&(*underTest), &watchers[1]);

    ++underTest;
    ASSERT_FALSE(underTest.hasFinishedTraversal());
    EXPECT_EQ(&(*underTest), &watchers[2]);

    ++underTest;
    EXPECT_TRUE(underTest.hasFinishedTraversal());
}

TEST(UnitSolver, removeElementInSingleElementWatcherList) {
    TrivialClause testClause;

    TestWatcher watcher1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcherList watchers{watcher1};

    TestWatcherTraversal underTest{&watchers};
    underTest.removeCurrent();

    ASSERT_TRUE(underTest.hasFinishedTraversal());
    underTest.finishedTraversal();
    EXPECT_TRUE(watchers.empty());
}

TEST(UnitSolver, removeSingleElementInWatcherListWithThreeElements) {
    TrivialClause testClause;

    TestWatcher watcher1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcher watcher2{testClause, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
    TestWatcher watcher3{testClause, CNFLit{CNFVar{12}, CNFSign::NEGATIVE}};
    TestWatcherList watchers{watcher1, watcher2, watcher3};

    TestWatcherTraversal underTest{&watchers};
    ++underTest;
    underTest.removeCurrent();

    ASSERT_FALSE(underTest.hasFinishedTraversal());
    EXPECT_EQ(&(*underTest), &watchers[1]);

    underTest.finishedTraversal();
    TestWatcherList expectedWatchers{watcher1, watcher3};
    EXPECT_EQ(watchers, expectedWatchers);
}

TEST(UnitSolver, removeAllElementsInWatcherListWithThreeElements) {
    TrivialClause testClause;

    TestWatcher watcher1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcher watcher2{testClause, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
    TestWatcher watcher3{testClause, CNFLit{CNFVar{12}, CNFSign::NEGATIVE}};
    TestWatcherList watchers{watcher1, watcher2, watcher3};

    TestWatcherTraversal underTest{&watchers};
    underTest.removeCurrent();
    underTest.removeCurrent();
    underTest.removeCurrent();

    ASSERT_TRUE(underTest.hasFinishedTraversal());

    underTest.finishedTraversal();
    TestWatcherList expectedWatchers{};
    EXPECT_EQ(watchers, expectedWatchers);
}

TEST(UnitSolver, compareWatcherListTraversals) {
    TrivialClause testClause;

    TestWatcher watcher1{testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
    TestWatcher watcher2{testClause, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
    TestWatcherList watchers{watcher1, watcher2};

    TestWatcherTraversal lhs{&watchers};
    TestWatcherTraversal rhs{&watchers};

    EXPECT_TRUE(lhs == rhs);
    EXPECT_FALSE(lhs != rhs);
    ++rhs;
    EXPECT_FALSE(lhs == rhs);
    EXPECT_TRUE(lhs != rhs);
    ++lhs;
    EXPECT_TRUE(lhs == rhs);
    EXPECT_FALSE(lhs != rhs);
    ++lhs;
    EXPECT_FALSE(lhs == rhs);
    EXPECT_TRUE(lhs != rhs);
    ++rhs;
    EXPECT_TRUE(lhs == rhs);
    EXPECT_FALSE(lhs != rhs);
}

TEST(UnitSolver, emptyWatchersProducesEmptyTraversals) {
    TestWatchers underTest{CNFVar{10}};
    for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
        auto probeNeg = underTest.getWatchers(CNFLit{CNFVar{i}, CNFSign::NEGATIVE});
        EXPECT_TRUE(probeNeg.hasFinishedTraversal());
        auto probePos = underTest.getWatchers(CNFLit{CNFVar{i}, CNFSign::POSITIVE});
        EXPECT_TRUE(probePos.hasFinishedTraversal());
    }
}

TEST(UnitSolver, addedWatcherIsContainedInTraversal) {
    CNFLit secondWatchedLiteral{CNFVar{9}, CNFSign::POSITIVE};
    TrivialClause testClause;
    TestWatcher watcher{testClause, secondWatchedLiteral};

    TestWatchers underTest{CNFVar{10}};
    CNFLit watchedLiteral{CNFVar{10}, CNFSign::POSITIVE};
    underTest.addWatcher(watchedLiteral, watcher);

    auto probe = underTest.getWatchers(watchedLiteral);

    ASSERT_FALSE(probe.hasFinishedTraversal());
    EXPECT_TRUE(*probe == watcher);
    ++probe;
    EXPECT_TRUE(probe.hasFinishedTraversal());

    for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
        auto probeNeg = underTest.getWatchers(CNFLit{CNFVar{i}, CNFSign::NEGATIVE});
        EXPECT_TRUE(probeNeg.hasFinishedTraversal());
        if (i == 10) {
            continue;
        }
        auto probePos = underTest.getWatchers(CNFLit{CNFVar{i}, CNFSign::POSITIVE});
        EXPECT_TRUE(probePos.hasFinishedTraversal());
    }
}

TEST(UnitSolver, watchersMaxVarCanBeIncreased) {
    TestWatchers underTest{CNFVar{10}};
    underTest.increaseMaxVarTo(CNFVar{20});
    auto watchersFor20 = underTest.getWatchers(CNFLit{CNFVar{20}, CNFSign::POSITIVE});
    EXPECT_TRUE(watchersFor20.hasFinishedTraversal());

    CNFLit secondWatchedLiteral{CNFVar{9}, CNFSign::POSITIVE};
    TrivialClause testClause;
    TestWatcher watcher{testClause, secondWatchedLiteral};
    CNFLit watchedLiteral{CNFVar{20}, CNFSign::POSITIVE};
    underTest.addWatcher(watchedLiteral, watcher);

    auto postAddWatchersFor20 = underTest.getWatchers(CNFLit{CNFVar{20}, CNFSign::POSITIVE});
    ASSERT_FALSE(postAddWatchersFor20.hasFinishedTraversal());
    EXPECT_EQ(&((*postAddWatchersFor20).getClause()), &testClause);
    ++postAddWatchersFor20;
    EXPECT_TRUE(postAddWatchersFor20.hasFinishedTraversal());
}

TEST(UnitSolver, watchersAreNotModifiedWhenEraseIsCalledButNoClauseMarkedToBeDeleted) {
    CNFLit firstWatchedLiteral{CNFVar{0}, CNFSign::POSITIVE};
    CNFLit secondWatchedLiteral{CNFVar{1}, CNFSign::POSITIVE};
    CNFLit thirdLiteral{CNFVar{2}, CNFSign::POSITIVE};
    TrivialClause testClause{firstWatchedLiteral, secondWatchedLiteral, thirdLiteral};

    TestWatcher watcher1{testClause, secondWatchedLiteral};
    TestWatcher watcher2{testClause, firstWatchedLiteral};

    TestWatchers underTest{CNFVar{2}};
    underTest.addWatcher(firstWatchedLiteral, watcher1);
    underTest.addWatcher(secondWatchedLiteral, watcher2);

    underTest.eraseWatchersToBeDeleted();

    auto traversal1 = underTest.getWatchers(firstWatchedLiteral);
    EXPECT_EQ(&(traversal1->getClause()), &testClause);
    ++traversal1;
    EXPECT_TRUE(traversal1.hasFinishedTraversal());

    auto traversal2 = underTest.getWatchers(secondWatchedLiteral);
    EXPECT_EQ(&(traversal2->getClause()), &testClause);
    ++traversal2;
    EXPECT_TRUE(traversal2.hasFinishedTraversal());

    auto traversal3 = underTest.getWatchers(thirdLiteral);
    EXPECT_TRUE(traversal3.hasFinishedTraversal());
}

namespace {
void addClausesToWatchers(TestWatchers &target, const std::vector<TrivialClause *> clauses) {
    for (auto clause : clauses) {
        JAM_ASSERT(clause->size() >= 2, "Can only add clauses with size > 2");
        TestWatcher w1{*clause, clause->at(1)};
        TestWatcher w2{*clause, clause->at(0)};
        target.addWatcher(clause->at(0), w1);
        target.addWatcher(clause->at(1), w2);
    }
}
}

TEST(UnitSolver, watchersMarkedToBeDeletedAreRemovedByErase) {
    TrivialClause c1{CNFLit{CNFVar{0}, CNFSign::POSITIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                     CNFLit{CNFVar{2}, CNFSign::POSITIVE}};

    TrivialClause c2{CNFLit{CNFVar{0}, CNFSign::POSITIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                     CNFLit{CNFVar{5}, CNFSign::POSITIVE}};

    TestWatchers underTest{CNFVar{5}};
    addClausesToWatchers(underTest, {&c1, &c2});

    markToBeDeleted(c1);
    underTest.eraseWatchersToBeDeleted();

    auto traversal1 = underTest.getWatchers(CNFLit{CNFVar{0}, CNFSign::POSITIVE});
    while (!traversal1.hasFinishedTraversal()) {
        ASSERT_NE(&(traversal1->getClause()), &c1);
        ++traversal1;
    }

    auto traversal2 = underTest.getWatchers(CNFLit{CNFVar{1}, CNFSign::POSITIVE});
    while (!traversal2.hasFinishedTraversal()) {
        ASSERT_NE(&(traversal2->getClause()), &c1);
        ++traversal2;
    }
}

TEST(UnitSolver, watchersAreClearedByEraseWhenAllClausesMarkedToBeDeleted) {
    TrivialClause c1{CNFLit{CNFVar{0}, CNFSign::POSITIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                     CNFLit{CNFVar{2}, CNFSign::POSITIVE}};

    TrivialClause c2{CNFLit{CNFVar{0}, CNFSign::POSITIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                     CNFLit{CNFVar{5}, CNFSign::POSITIVE}};

    TestWatchers underTest{CNFVar{5}};
    addClausesToWatchers(underTest, {&c1, &c2});

    markToBeDeleted(c1);
    markToBeDeleted(c2);
    underTest.eraseWatchersToBeDeleted();

    for (CNFVar i = CNFVar{0}; i <= CNFVar{5}; i = nextCNFVar(i)) {
        CNFLit lit = CNFLit{i, CNFSign::POSITIVE};
        auto traversal1 = underTest.getWatchers(lit);
        ASSERT_TRUE(traversal1.hasFinishedTraversal()) << "Watchers for " << lit << " not empty";
        auto traversal2 = underTest.getWatchers(~lit);
        ASSERT_TRUE(traversal2.hasFinishedTraversal()) << "Watchers for " << lit << " not empty";
    }
}

TEST(UnitSolver, completeWatchersTraversalEmptyWhenNoWatchersExist) {
    TestWatchers underTest{CNFVar{4}};
    auto watcherRange = underTest.getWatchersInTraversalOrder();
    EXPECT_TRUE(watcherRange.begin() == watcherRange.end());
}

TEST(UnitSolver, watchersAllOccurInCompleteWatchersTraversal) {
    std::vector<TrivialClause> clauses = {
        {CNFLit{CNFVar{0}, CNFSign::POSITIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE}},
        {CNFLit{CNFVar{0}, CNFSign::POSITIVE}, CNFLit{CNFVar{2}, CNFSign::POSITIVE}},
        {CNFLit{CNFVar{1}, CNFSign::POSITIVE}, CNFLit{CNFVar{3}, CNFSign::POSITIVE}},
        {CNFLit{CNFVar{2}, CNFSign::POSITIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE}},
    };

    std::vector<TestWatcher> watchers;
    for (auto &clause : clauses) {
        watchers.emplace_back(clause, clause[1]);
        watchers.emplace_back(clause, clause[0]);
    }

    TestWatchers underTest{CNFVar{10}};
    for (decltype(watchers)::size_type i = 0; i < watchers.size(); i += 2) {
        underTest.addWatcher(clauses[i / 2][0], watchers[i]);
        underTest.addWatcher(clauses[i / 2][1], watchers[i + 1]);
    }

    std::vector<TestWatcher> expected;
    // The concrete ordering of the watcher sequences is deliberately omitted from the Watcher
    // interface's documentation. Thus, this test depends on a "deeper" implementation detail and
    // will need to be adjusted if the ordering mechanism of the watchers is changed.
    for (CNFVar i = CNFVar{0}; i <= CNFVar{10}; i = nextCNFVar(i)) {
        for (CNFSign s : {CNFSign::NEGATIVE, CNFSign::POSITIVE}) {
            auto traversal = underTest.getWatchers(CNFLit{i, s});
            while (!traversal.hasFinishedTraversal()) {
                expected.push_back(*traversal);
                ++traversal;
            }
        }
    }

    auto watcherRange = underTest.getWatchersInTraversalOrder();
    expectRangeElementsSequencedEqual(watcherRange, expected);
}

TEST(UnitSolver, binaryWatchersOccurInBlockerMap) {
    std::vector<TrivialClause> clauses = {
        {CNFLit{CNFVar{0}, CNFSign::POSITIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE}},
        {CNFLit{CNFVar{0}, CNFSign::POSITIVE}, CNFLit{CNFVar{2}, CNFSign::POSITIVE}},
        {CNFLit{CNFVar{1}, CNFSign::POSITIVE}, CNFLit{CNFVar{3}, CNFSign::POSITIVE}},
        {CNFLit{CNFVar{2}, CNFSign::POSITIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE}},
    };

    std::vector<TestWatcher> watchers;
    for (auto &clause : clauses) {
        watchers.emplace_back(clause, clause[1]);
        watchers.emplace_back(clause, clause[0]);
    }

    TestWatchers underTest{CNFVar{10}};
    for (decltype(watchers)::size_type i = 0; i < watchers.size(); i += 2) {
        underTest.addWatcher(clauses[i / 2][0], watchers[i]);
        underTest.addWatcher(clauses[i / 2][1], watchers[i + 1]);
    }

    auto blockerMap = underTest.getBlockerMap();

    auto blockersOfP0 = blockerMap[CNFLit{CNFVar{0}, CNFSign::POSITIVE}];
    std::vector<CNFLit> expectedBlockersOfP0{CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                                             CNFLit{CNFVar{2}, CNFSign::POSITIVE}};
    ASSERT_EQ(blockersOfP0.size(), expectedBlockersOfP0.size());
    EXPECT_TRUE(std::is_permutation(blockersOfP0.begin(), blockersOfP0.end(),
                                    expectedBlockersOfP0.begin()));

    auto blockersOfP2 = blockerMap[CNFLit{CNFVar{2}, CNFSign::POSITIVE}];
    std::vector<CNFLit> expectedBlockersOfP2{CNFLit{CNFVar{0}, CNFSign::POSITIVE},
                                             CNFLit{CNFVar{1}, CNFSign::POSITIVE}};
    ASSERT_EQ(blockersOfP2.size(), expectedBlockersOfP2.size());
    EXPECT_TRUE(std::is_permutation(blockersOfP2.begin(), blockersOfP2.end(),
                                    expectedBlockersOfP2.begin()));

    auto blockersOfP10 = blockerMap[CNFLit{CNFVar{10}, CNFSign::POSITIVE}];
    EXPECT_TRUE(blockersOfP10.empty());


    auto blockersOfP3 = blockerMap[CNFLit{CNFVar{3}, CNFSign::POSITIVE}];
    ASSERT_EQ(blockersOfP3.size(), 1ULL);
    EXPECT_EQ(*(blockersOfP3.begin()), (CNFLit{CNFVar{1}, CNFSign::POSITIVE}));
}
}
}
