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

#include <libjamsat/solver/Clause.h>
#include <libjamsat/solver/Watcher.h>

namespace jamsat {
namespace detail_propagation {
TEST(UnitSolver, watchersStoreClausesAndOtherLit) {
  auto testClause = createHeapClause(0);
  CNFLit otherLiteral{CNFVar{10}, CNFSign::NEGATIVE};
  Watcher underTest{*testClause, otherLiteral};

  EXPECT_EQ(&(underTest.getClause()), testClause.get());
  EXPECT_EQ(underTest.getOtherWatchedLiteral(), otherLiteral);
}

TEST(UnitSolver, watchersWithSameClauseAndOtherLitAreEqual) {
  auto testClause = createHeapClause(0);
  Watcher underTest1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  Watcher underTest2{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};

  EXPECT_TRUE(underTest1 == underTest2);
  EXPECT_FALSE(underTest1 != underTest2);
}

TEST(UnitSolver, watchersWithSameClauseAndDifferentLitAreInequal) {
  auto testClause = createHeapClause(0);
  Watcher underTest1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  Watcher underTest2{*testClause, CNFLit{CNFVar{11}, CNFSign::POSITIVE}};

  EXPECT_TRUE(underTest1 != underTest2);
  EXPECT_FALSE(underTest1 == underTest2);
}

TEST(UnitSolver, watchersWithDifferentClausesAreInequal) {
  auto testClause1 = createHeapClause(0);
  auto testClause2 = createHeapClause(0);
  Watcher underTest1{*testClause1, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  Watcher underTest2{*testClause2, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};

  EXPECT_TRUE(underTest1 != underTest2);
  EXPECT_FALSE(underTest1 == underTest2);
}

TEST(UnitSolver, traverseEmptyWatcherList) {
  WatcherList empty;
  WatcherTraversal underTest{&empty};
  EXPECT_TRUE(underTest.hasFinishedTraversal());
}

TEST(UnitSolver, dereferenceWatcherTraversal) {
  auto testClause = createHeapClause(0);

  Watcher watcher1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  WatcherList watchers{watcher1};

  WatcherTraversal underTest{&watchers};
  ASSERT_FALSE(underTest.hasFinishedTraversal());
  EXPECT_EQ(&((*underTest).getClause()), testClause.get());
  EXPECT_EQ(&(underTest->getClause()), testClause.get());
}

TEST(UnitSolver, traverseWatcherListWithSingleElement) {
  auto testClause = createHeapClause(0);

  Watcher watcher1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  WatcherList watchers{watcher1};

  WatcherTraversal underTest{&watchers};
  ASSERT_FALSE(underTest.hasFinishedTraversal());
  EXPECT_EQ(&(*underTest), &watchers[0]);

  ++underTest;
  EXPECT_TRUE(underTest.hasFinishedTraversal());
}

TEST(UnitSolver, traverseWatcherListWithThreeElements) {
  auto testClause = createHeapClause(0);

  Watcher watcher1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  Watcher watcher2{*testClause, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
  Watcher watcher3{*testClause, CNFLit{CNFVar{12}, CNFSign::NEGATIVE}};
  WatcherList watchers{watcher1, watcher2, watcher3};

  WatcherTraversal underTest{&watchers};
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
  auto testClause = createHeapClause(0);

  Watcher watcher1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  WatcherList watchers{watcher1};

  WatcherTraversal underTest{&watchers};
  underTest.removeCurrent();

  ASSERT_TRUE(underTest.hasFinishedTraversal());
  underTest.finishedTraversal();
  EXPECT_TRUE(watchers.empty());
}

TEST(UnitSolver, removeSingleElementInWatcherListWithThreeElements) {
  auto testClause = createHeapClause(0);

  Watcher watcher1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  Watcher watcher2{*testClause, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
  Watcher watcher3{*testClause, CNFLit{CNFVar{12}, CNFSign::NEGATIVE}};
  WatcherList watchers{watcher1, watcher2, watcher3};

  WatcherTraversal underTest{&watchers};
  ++underTest;
  underTest.removeCurrent();

  ASSERT_FALSE(underTest.hasFinishedTraversal());
  EXPECT_EQ(&(*underTest), &watchers[1]);

  underTest.finishedTraversal();
  WatcherList expectedWatchers{watcher1, watcher3};
  EXPECT_EQ(watchers, expectedWatchers);
}

TEST(UnitSolver, removeAllElementsInWatcherListWithThreeElements) {
  auto testClause = createHeapClause(0);

  Watcher watcher1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  Watcher watcher2{*testClause, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
  Watcher watcher3{*testClause, CNFLit{CNFVar{12}, CNFSign::NEGATIVE}};
  WatcherList watchers{watcher1, watcher2, watcher3};

  WatcherTraversal underTest{&watchers};
  underTest.removeCurrent();
  underTest.removeCurrent();
  underTest.removeCurrent();

  ASSERT_TRUE(underTest.hasFinishedTraversal());

  underTest.finishedTraversal();
  WatcherList expectedWatchers{};
  EXPECT_EQ(watchers, expectedWatchers);
}

TEST(UnitSolver, compareWatcherListTraversals) {
  auto testClause = createHeapClause(0);

  Watcher watcher1{*testClause, CNFLit{CNFVar{10}, CNFSign::NEGATIVE}};
  Watcher watcher2{*testClause, CNFLit{CNFVar{11}, CNFSign::NEGATIVE}};
  WatcherList watchers{watcher1, watcher2};

  WatcherTraversal lhs{&watchers};
  WatcherTraversal rhs{&watchers};

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
  Watchers underTest{CNFVar{10}};
  for (CNFVar::RawVariableType i = 0; i <= 10; ++i) {
    auto probeNeg = underTest.getWatchers(CNFLit{CNFVar{i}, CNFSign::NEGATIVE});
    EXPECT_TRUE(probeNeg.hasFinishedTraversal());
    auto probePos = underTest.getWatchers(CNFLit{CNFVar{i}, CNFSign::POSITIVE});
    EXPECT_TRUE(probePos.hasFinishedTraversal());
  }
}

TEST(UnitSolver, addedWatcherIsContainedInTraversal) {
  CNFLit secondWatchedLiteral{CNFVar{9}, CNFSign::POSITIVE};
  auto testClause = createHeapClause(0);
  Watcher watcher{*testClause, secondWatchedLiteral};

  Watchers underTest{CNFVar{10}};
  CNFLit watchedLiteral{CNFVar{10}, CNFSign::POSITIVE};
  underTest.addWatcher(watchedLiteral, watcher);

  auto probe = underTest.getWatchers(watchedLiteral);

  ASSERT_FALSE(probe.hasFinishedTraversal());
  EXPECT_TRUE(*probe == watcher);
  ++probe;
  EXPECT_TRUE(probe.hasFinishedTraversal());

  for (CNFVar::RawVariableType i = 0; i <= 10; ++i) {
    auto probeNeg = underTest.getWatchers(CNFLit{CNFVar{i}, CNFSign::NEGATIVE});
    EXPECT_TRUE(probeNeg.hasFinishedTraversal());
    if (i == 10) {
      continue;
    }
    auto probePos = underTest.getWatchers(CNFLit{CNFVar{i}, CNFSign::POSITIVE});
    EXPECT_TRUE(probePos.hasFinishedTraversal());
  }
}
}
}
