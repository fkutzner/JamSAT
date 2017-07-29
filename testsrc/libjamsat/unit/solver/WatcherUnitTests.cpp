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
}
}
