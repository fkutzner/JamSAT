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

#include <libjamsat/simplification/UnaryOptimizations.h>
#include <libjamsat/solver/Statistics.h>

#include <regex>
#include <sstream>

namespace jamsat {
TEST(UnitSolver, StatisticsInitializesCountersToZero) {
    Statistics<AllEnabledStatisticsConfig> underTest;
    EXPECT_EQ(underTest.getCurrentEra().m_conflictCount, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_propagationCount, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_decisionCount, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_restartCount, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_avgLemmaSize.getAverage(), 0.0);
    EXPECT_EQ(underTest.getCurrentEra().m_avgLBD, 0.0);
}

namespace {
// Registers a conflict, 7 propagations, 3 decisions, 2 restarts, 3 lemmas (sizes: 2,5,11)
template <typename Statistics>
void addEvents(Statistics& underTest) {
    underTest.registerConflict();
    underTest.registerPropagations(4);
    underTest.registerPropagations(3);
    underTest.registerDecision();
    underTest.registerDecision();
    underTest.registerDecision();
    underTest.registerRestart();
    underTest.registerRestart();
    underTest.registerLemma(2);
    underTest.registerLemma(5);
    underTest.registerLemma(11);
    underTest.registerLemmaDeletion(5);

    SimplificationStats simpStats;
    simpStats.amntClausesRemovedBySubsumption = 1ULL;
    simpStats.amntClausesStrengthened = 2ULL;
    simpStats.amntLiteralsRemovedByStrengthening = 3ULL;
    underTest.registerSimplification(simpStats);
}
}

TEST(UnitSolver, StatisticsCountsAllItemsInAllEnabledMode) {
    Statistics<AllEnabledStatisticsConfig> underTest;
    addEvents(underTest);
    EXPECT_EQ(underTest.getCurrentEra().m_conflictCount, 1ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_propagationCount, 7ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_decisionCount, 3ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_restartCount, 2ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_avgLemmaSize.getAverage(), 6.0);
}

namespace {
template <typename StatisticsConfiguration>
void test_expectStatsDisabled(bool conflictsDisabled,
                              bool propsDisabled,
                              bool decsDisabled,
                              bool restartsDisabled,
                              bool lemmaSizeDisabled,
                              bool lemmaDelDisabled) {
    Statistics<StatisticsConfiguration> underTest;
    addEvents(underTest);

    EXPECT_EQ(underTest.getCurrentEra().m_conflictCount, conflictsDisabled ? 0ULL : 1ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_propagationCount, propsDisabled ? 0ULL : 7ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_decisionCount, decsDisabled ? 0ULL : 3ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_restartCount, restartsDisabled ? 0ULL : 2ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_avgLemmaSize.getAverage(),
              lemmaSizeDisabled ? 0ULL : 6.0);
    EXPECT_EQ(underTest.getCurrentEra().m_lemmaDeletions, lemmaDelDisabled ? 0ULL : 5ULL);
}
}

TEST(UnitSolver, StatisticsDoesNotCountConflictsWhenDisabled) {
    struct PropDisabledStatisticsConfig {
        using CountConflicts = std::false_type;
        using CountPropagations = std::true_type;
        using CountDecisions = std::true_type;
        using CountRestarts = std::true_type;
        using MeasureLemmaSize = std::true_type;
        using CountLemmaDeletions = std::true_type;
        using CountSimplificationStats = std::true_type;
    };
    test_expectStatsDisabled<PropDisabledStatisticsConfig>(true, false, false, false, false, false);
}

TEST(UnitSolver, StatisticsDoesNotCountPropagationsWhenDisabled) {
    struct PropDisabledStatisticsConfig {
        using CountConflicts = std::true_type;
        using CountPropagations = std::false_type;
        using CountDecisions = std::true_type;
        using CountRestarts = std::true_type;
        using MeasureLemmaSize = std::true_type;
        using CountLemmaDeletions = std::true_type;
        using CountSimplificationStats = std::true_type;
    };
    test_expectStatsDisabled<PropDisabledStatisticsConfig>(false, true, false, false, false, false);
}

TEST(UnitSolver, StatisticsDoesNotCountDecisionsWhenDisabled) {
    struct PropDisabledStatisticsConfig {
        using CountConflicts = std::true_type;
        using CountPropagations = std::true_type;
        using CountDecisions = std::false_type;
        using CountRestarts = std::true_type;
        using MeasureLemmaSize = std::true_type;
        using CountLemmaDeletions = std::true_type;
        using CountSimplificationStats = std::true_type;
    };
    test_expectStatsDisabled<PropDisabledStatisticsConfig>(false, false, true, false, false, false);
}

TEST(UnitSolver, StatisticsDoesNotCountRestartsWhenDisabled) {
    struct PropDisabledStatisticsConfig {
        using CountConflicts = std::true_type;
        using CountPropagations = std::true_type;
        using CountDecisions = std::true_type;
        using CountRestarts = std::false_type;
        using MeasureLemmaSize = std::true_type;
        using CountLemmaDeletions = std::true_type;
        using CountSimplificationStats = std::true_type;
    };
    test_expectStatsDisabled<PropDisabledStatisticsConfig>(false, false, false, true, false, false);
}

TEST(UnitSolver, StatisticsDoesNotMeasureLemmaSizeWhenDisabled) {
    struct PropDisabledStatisticsConfig {
        using CountConflicts = std::true_type;
        using CountPropagations = std::true_type;
        using CountDecisions = std::true_type;
        using CountRestarts = std::true_type;
        using MeasureLemmaSize = std::false_type;
        using CountLemmaDeletions = std::true_type;
        using CountSimplificationStats = std::true_type;
    };
    test_expectStatsDisabled<PropDisabledStatisticsConfig>(false, false, false, false, true, false);
}

TEST(UnitSolver, StatisticsDoesNotCountLemmaDeletionsWhenDisabled) {
    struct PropDisabledStatisticsConfig {
        using CountConflicts = std::true_type;
        using CountPropagations = std::true_type;
        using CountDecisions = std::true_type;
        using CountRestarts = std::true_type;
        using MeasureLemmaSize = std::true_type;
        using CountLemmaDeletions = std::false_type;
        using CountSimplificationStats = std::true_type;
    };
    test_expectStatsDisabled<PropDisabledStatisticsConfig>(false, false, false, false, false, true);
}

TEST(UnitSolver, StatisticsResetsCountersOnEraConclusion) {
    Statistics<AllEnabledStatisticsConfig> underTest;
    addEvents(underTest);
    underTest.concludeEra();
    EXPECT_EQ(underTest.getCurrentEra().m_conflictCount, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_propagationCount, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_decisionCount, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_restartCount, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_lemmaDeletions, 0ULL);

    EXPECT_EQ(underTest.getCurrentEra().m_simplificationStats.amntClausesRemovedBySubsumption,
              0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_simplificationStats.amntClausesStrengthened, 0ULL);
    EXPECT_EQ(underTest.getCurrentEra().m_simplificationStats.amntLiteralsRemovedByStrengthening,
              0ULL);
}

TEST(UnitSolver, StatisticsStoresPreviousEra) {
    Statistics<AllEnabledStatisticsConfig> underTest;
    addEvents(underTest);
    underTest.concludeEra();
    EXPECT_EQ(underTest.getPreviousEra().m_conflictCount, 1ULL);
    EXPECT_EQ(underTest.getPreviousEra().m_propagationCount, 7ULL);
    EXPECT_EQ(underTest.getPreviousEra().m_decisionCount, 3ULL);
    EXPECT_EQ(underTest.getPreviousEra().m_restartCount, 2ULL);
    EXPECT_EQ(underTest.getPreviousEra().m_lemmaDeletions, 5ULL);
    EXPECT_EQ(underTest.getPreviousEra().m_simplificationStats.amntClausesRemovedBySubsumption,
              1ULL);
    EXPECT_EQ(underTest.getPreviousEra().m_simplificationStats.amntClausesStrengthened, 2ULL);
    EXPECT_EQ(underTest.getPreviousEra().m_simplificationStats.amntLiteralsRemovedByStrengthening,
              3ULL);
}

TEST(UnitSolver, StatisticsPrintsCurrentEra) {
    Statistics<AllEnabledStatisticsConfig> underTest;
    underTest.concludeEra();
    addEvents(underTest);

    std::stringstream collector;
    collector << underTest;
    std::string result = collector.str();

    std::cout << result << std::endl;

    EXPECT_TRUE(static_cast<bool>(std::regex_search(result, std::regex{"#C: 1 "})));
    EXPECT_TRUE(static_cast<bool>(std::regex_search(result, std::regex{"#P: 7 "})));
    EXPECT_TRUE(static_cast<bool>(std::regex_search(result, std::regex{"#D: 3 "})));
    EXPECT_TRUE(static_cast<bool>(std::regex_search(result, std::regex{"#R: 2 "})));
    EXPECT_TRUE(static_cast<bool>(std::regex_search(result, std::regex{"#LD: 5 "})));
    EXPECT_TRUE(static_cast<bool>(std::regex_search(result, std::regex{"L: 6.00 "})));
    EXPECT_TRUE(static_cast<bool>(std::regex_search(result, std::regex{"#B: 1 "})));
    EXPECT_TRUE(static_cast<bool>(std::regex_search(result, std::regex{"#U: 0 "})));
}
}
