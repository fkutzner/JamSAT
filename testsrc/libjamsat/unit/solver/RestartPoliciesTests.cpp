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

#include <vector>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/RestartPolicies.h>
#include <libjamsat/utils/LubySequence.h>

namespace jamsat {
using TrivialClause = std::vector<CNFLit>;

TEST(UnitSolver, LubyRestartPolicy_noRestartWithinGraceTime)
{
  LubyRestartPolicy::Options options;
  options.graceTime = 50;
  options.log2OfScaleFactor = 2;
  LubyRestartPolicy underTest{options};

  for (int i = 0; i < 50; ++i) {
    underTest.registerConflict(LubyRestartPolicy::RegisterConflictArgs{});
    EXPECT_FALSE(underTest.shouldRestart())
        << "Erroneous restart after " << (i + 1) << " conflicts";
  }
}

namespace {
void fastForward(LubyRestartPolicy& target, int conflicts)
{
  for (int i = 0; i < conflicts; ++i) {
    target.registerConflict(LubyRestartPolicy::RegisterConflictArgs{});
  }
}

int failsRestartSequenceAt(LubyRestartPolicy& underTest, int scaleFactor, int lubySteps)
{
  LubySequence lubySequence;
  int executedSteps = 0;

  for (int i = 0; i < lubySteps; ++i) {
    LubySequence::Element currentBudget = lubySequence.current() * scaleFactor;
    for (; currentBudget != 0; --currentBudget) {
      if (underTest.shouldRestart()) {
        return executedSteps;
      }
      underTest.registerConflict(LubyRestartPolicy::RegisterConflictArgs{});
      ++executedSteps;
    }
    if (!underTest.shouldRestart()) {
      return executedSteps;
    }
    underTest.registerRestart();
    lubySequence.next();
  }

  return -1;
}
}

TEST(UnitSolver, LubyRestartPolicy_restartFrequencyMatchesLubyAfterGraceTime)
{
  LubyRestartPolicy::Options options;
  options.graceTime = 10;
  // scale by 2^0: restarts after 1, 1, 2, ... conflicts
  options.log2OfScaleFactor = 0;
  LubyRestartPolicy underTest{options};
  fastForward(underTest, 10);

  auto failureAt = failsRestartSequenceAt(underTest, 1, 10);
  EXPECT_EQ(failureAt, -1) << "Detected luby restart sequence failure at conflict " << failureAt;
}

TEST(UnitSolver, LubyRestartPolicy_restartAdvicePersistsWhenNoRestart)
{
  LubyRestartPolicy::Options options;
  options.graceTime = 4;
  options.log2OfScaleFactor = 1;
  LubyRestartPolicy underTest{options};

  while (!underTest.shouldRestart()) {
    underTest.registerConflict(LubyRestartPolicy::RegisterConflictArgs{});
  }

  for (int i = 0; i < 1024; ++i) {
    underTest.registerConflict(LubyRestartPolicy::RegisterConflictArgs{});
    EXPECT_TRUE(underTest.shouldRestart())
        << "Restart advice failed after " << (i + 1) << " conflicts";
  }
}

TEST(UnitSolver, LubyRestartPolicy_scaleFactorScalesSequence)
{
  LubyRestartPolicy::Options options;
  options.graceTime = 0;
  options.log2OfScaleFactor = 3;
  LubyRestartPolicy underTest{options};

  auto failureAt = failsRestartSequenceAt(underTest, 8, 32);
  EXPECT_EQ(failureAt, -1) << "Detected luby restart sequence failure at conflict " << failureAt;
}

TEST(UnitSolver, GlucoseRestartPolicy_noRestartWhenTooFewConflicts)
{
  GlucoseRestartPolicy::Options options;
  options.movingAverageWindowSize = 10ull;
  GlucoseRestartPolicy underTest{options};

  EXPECT_FALSE(underTest.shouldRestart());
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{10});
  EXPECT_FALSE(underTest.shouldRestart());
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{20});
  EXPECT_FALSE(underTest.shouldRestart());
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{30});
  EXPECT_FALSE(underTest.shouldRestart());
}

TEST(UnitSolver, GlucoseRestartPolicy_noRestartWhenTooFewConflictsSinceLastRestart)
{
  GlucoseRestartPolicy::Options options;
  options.movingAverageWindowSize = 3ull;
  options.K = 10.0f;
  GlucoseRestartPolicy underTest{options};

  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{10});
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{20});
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{30});
  EXPECT_TRUE(underTest.shouldRestart());
  underTest.registerRestart();
  EXPECT_FALSE(underTest.shouldRestart());
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{20});
  EXPECT_FALSE(underTest.shouldRestart());
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{30});
  EXPECT_FALSE(underTest.shouldRestart());
}

TEST(UnitSolver, GlucoseRestartPolicy_restartWhenAverageLBDTooBad)
{
  GlucoseRestartPolicy::Options options;
  options.movingAverageWindowSize = 3ull;
  options.K = 0.8f;
  GlucoseRestartPolicy underTest{options};
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{2});
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{2});
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{2});
  EXPECT_FALSE(underTest.shouldRestart());
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{20});
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{30});
  underTest.registerConflict(GlucoseRestartPolicy::RegisterConflictArgs{40});
  EXPECT_TRUE(underTest.shouldRestart());
}
}
