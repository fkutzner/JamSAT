/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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

/**
 * \file Statistics.h
 * \brief Statistics collector for CDCL search
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <type_traits>

#include <boost/format.hpp>

#include <libjamsat/utils/SimpleMovingAverage.h>

namespace jamsat {

/**
 * \brief Optimization statistics
 *
 * \ingroup JamSAT_Solver
 */
struct OptimizationStats {
  uint64_t amntFactsDerived = 0;
  uint64_t amntLitsRemoved = 0;
  uint64_t amntClausesRemoved = 0;
  uint64_t amntClausesAdded = 0;
  uint64_t amntVarsEliminated = 0;
  uint64_t amntVarsAdded = 0;

  auto operator+=(OptimizationStats const& rhs) -> OptimizationStats&;
};

auto operator<<(std::ostream& output, OptimizationStats const& stats) -> std::ostream&;
auto to_string(OptimizationStats const& stats) -> std::string;


/**
 * \brief Configuration struct for the Statistics class, enabling all statistics
 *
 * \ingroup JamSAT_Solver
 */
struct AllEnabledStatisticsConfig {
  using CountConflicts = std::true_type;
  using CountPropagations = std::true_type;
  using CountDecisions = std::true_type;
  using CountRestarts = std::true_type;
  using MeasureLemmaSize = std::true_type;
  using CountLemmaDeletions = std::true_type;
  using CountOptimizationStats = std::true_type;
};

/**
 * \brief Storage for statistics
 */
struct StatisticsEra {
  uint64_t m_conflictCount = 0;
  uint64_t m_propagationCount = 0;
  uint64_t m_decisionCount = 0;
  uint64_t m_restartCount = 0;
  uint64_t m_unitLemmas = 0;
  uint64_t m_binaryLemmas = 0;
  uint64_t m_lemmaDeletions = 0;
  OptimizationStats m_optimizationStats;
  SimpleMovingAverage<uint32_t> m_avgLemmaSize{1000};
  double m_avgLBD = 0.0;
  std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> m_startTime;
  std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> m_stopTime;
};

/**
 * \brief A class for accumulating solver statistics
 *
 * \tparam StatisticsConfig A type controlling what statistics shall be kept. \p StatisticsConfig
 * must have the following member types:
 *  - CountConflicts
 *  - CountPropagations
 *  - CountDecisions
 *  - CountRestarts
 *  - MeasureLemmaSize
 *  - CountLemmaDeletions
 * Each of these member types must be either std::true_type or std::false_type. Their meaning
 * directly follows from their name: the value of CountConflicts::value determines whether
 * conflicts shall be counted, etc.
 *
 * Usage example: collection of statistics in a SAT solver, with an "era" being the duration
 * of one solver invocation.
 *
 * \ingroup JamSAT_Solver
 */
template <typename StatisticsConfig = AllEnabledStatisticsConfig>
class Statistics {
public:
  /**
   * \brief Notifies the statistics system that a conflict has occurred.
   */
  void registerConflict() noexcept;

  /**
   * \brief Notifies the statistics system that propagations have been performed.
   *
   * \param count The amount of propagations having been performed.
   */
  void registerPropagations(uint64_t count) noexcept;

  /**
   * \brief Notifies the statistics system that a decision has been performed.
   */
  void registerDecision() noexcept;

  /**
   * \brief Notifies the statistics system that a restart has been performed.
   */
  void registerRestart() noexcept;


  /**
   * \brief Notifies the statistics that a lemma has been added
   *
   * \param length    The amount of literals contained in the added lemma
   */
  void registerLemma(uint32_t size) noexcept;

  /**
   * \brief Notifies the statistics that lemmas have been deleted.
   *
   * \param amount    The amount of lemmas having been deleted.
   */
  void registerLemmaDeletion(uint32_t amount);

  /**
   * \brief Notifies the statistics system about optimizations performed
   *   on the problem instance.
   */
  void registerOptimizationStatistics(OptimizationStats const& stats);

  /**
   * \brief Notifies the statistics system that the solver entered its main search
   * routine.
   */
  void registerSolvingStart() noexcept;

  /**
   * \brief Notifies the statistics system that the solver has finished the search.
   */
  void registerSolvingStop() noexcept;

  /**
   * \brief Notifies the statistics system that the current era has ended.
   *
   * The statistics data for the current era is made available via getPreviousEra()
   * and all further statistics are recorded into a new StatisticsEra object, with all
   * statistics values reset to their initial value.
   */
  void concludeEra() noexcept;

  /**
   * \brief Returns the recorded statistics of the current era.
   *
   * \return The recorded statistics of the current era.
   */
  auto getCurrentEra() const noexcept -> StatisticsEra const&;

  /**
   * \brief Returns the recorded statistics of the era that ended with
   * the last call to concludeEra().
   *
   * \return The recorded statistics of the previous era.
   */
  auto getPreviousEra() const noexcept -> StatisticsEra const&;


  /**
   * \brief Returns the description string.
   *
   * \return a string describing the abbreviations used when
   *   converting the statistics object to a string rsp. printing it
   *   to a stream.
   */
  auto getStatisticsDescription() const -> std::string;

private:
  StatisticsEra m_previousEra;
  StatisticsEra m_currentEra;
};

template <typename StatisticsConfig>
auto operator<<(std::ostream& stream, const Statistics<StatisticsConfig>& stats) noexcept
    -> std::ostream&;

template <typename T>
auto to_string(Statistics<T> const& stats) -> std::string;

/********** Implementation ****************************** */

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerConflict() noexcept
{
  if (StatisticsConfig::CountConflicts::value == true) {
    ++m_currentEra.m_conflictCount;
  }
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerPropagations(uint64_t count) noexcept
{
  if (StatisticsConfig::CountPropagations::value == true) {
    m_currentEra.m_propagationCount += count;
  }
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerDecision() noexcept
{
  if (StatisticsConfig::CountDecisions::value == true) {
    ++m_currentEra.m_decisionCount;
  }
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerRestart() noexcept
{
  if (StatisticsConfig::CountRestarts::value == true) {
    ++m_currentEra.m_restartCount;
  }
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerSolvingStart() noexcept
{
  auto now = decltype(m_currentEra.m_startTime)::clock::now();
  m_currentEra.m_startTime = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerSolvingStop() noexcept
{
  auto now = decltype(m_currentEra.m_startTime)::clock::now();
  m_currentEra.m_stopTime = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerLemma(uint32_t size) noexcept
{
  if (StatisticsConfig::MeasureLemmaSize::value == true) {
    m_currentEra.m_avgLemmaSize.add(size);
    if (size == 1) {
      ++m_currentEra.m_unitLemmas;
    }
    else if (size == 2) {
      ++m_currentEra.m_binaryLemmas;
    }
  }
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerLemmaDeletion(uint32_t amount)
{
  if (StatisticsConfig::CountLemmaDeletions::value == true) {
    m_currentEra.m_lemmaDeletions += amount;
  }
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::registerOptimizationStatistics(OptimizationStats const& stats)
{
  if (StatisticsConfig::CountOptimizationStats::value == true) {
    m_currentEra.m_optimizationStats += stats;
  }
}

template <typename StatisticsConfig>
void Statistics<StatisticsConfig>::concludeEra() noexcept
{
  m_previousEra = m_currentEra;
  m_currentEra = StatisticsEra{};
}

template <typename StatisticsConfig>
auto Statistics<StatisticsConfig>::getCurrentEra() const noexcept -> StatisticsEra const&
{
  return m_currentEra;
}

template <typename StatisticsConfig>
auto Statistics<StatisticsConfig>::getPreviousEra() const noexcept -> StatisticsEra const&
{
  return m_previousEra;
}


template <typename StatisticsConfig>
auto Statistics<StatisticsConfig>::getStatisticsDescription() const -> std::string
{
  // clang-format off
    return std::string{"Statistics: " \
           "#C = amount of conflicts; " \
           "#P = amount of propagations; " \
           "#D = amount of decision literals picked;\n" \
           "  #R = amount of restarts performed; " \
           "T = time passed since last solve() invocation; " \
           "L = avg. lemma size;\n" \
           "  #U = amount of unit lemmas added; " \
           "#B = amount of binary lemmas added; " \
           "#LD = amount of lemmas deleted; " \
           "O = optimization stats"};
  // clang-format on
}


template <typename StatisticsConfig>
auto operator<<(std::ostream& stream, const Statistics<StatisticsConfig>& stats) noexcept
    -> std::ostream&
{
  auto& currentEra = stats.getCurrentEra();

  auto now = decltype(currentEra.m_startTime)::clock::now();

  uint64_t millisElapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - currentEra.m_startTime).count();
  stream << "T: " << millisElapsed << "ms ";

  if (StatisticsConfig::MeasureLemmaSize::value == true) {
    stream << boost::format("| L: %4.2f ") % currentEra.m_avgLemmaSize.getAverage();
    stream << "| #U: " << currentEra.m_unitLemmas << " ";
    stream << "| #B: " << currentEra.m_binaryLemmas << " ";
  }

  if (StatisticsConfig::CountLemmaDeletions::value == true) {
    stream << "| #LD: " << currentEra.m_lemmaDeletions << " ";
  }

  if (StatisticsConfig::CountConflicts::value == true) {
    stream << "| #C: " << currentEra.m_conflictCount << " ";
  }

  if (StatisticsConfig::CountPropagations::value == true) {
    stream << "| #P: " << currentEra.m_propagationCount << " ";
  }

  if (StatisticsConfig::CountDecisions::value == true) {
    stream << "| #D: " << currentEra.m_decisionCount << " ";
  }

  if (StatisticsConfig::CountRestarts::value == true) {
    stream << "| #R: " << currentEra.m_restartCount << " ";
  }

  stream << "\n  ";

  if (StatisticsConfig::CountOptimizationStats::value == true) {
    stream << "#O: " << to_string(currentEra.m_optimizationStats) << " ";
  }

  if (millisElapsed > 0) {
    double secsElapsed = millisElapsed;
    secsElapsed /= 1000;

    if (StatisticsConfig::CountConflicts::value == true) {
      stream << boost::format("| #C/s: %7.2f ") % (currentEra.m_conflictCount / secsElapsed);
    }

    if (StatisticsConfig::CountPropagations::value == true) {
      stream << boost::format("| #P/s: %7.2f ") % (currentEra.m_propagationCount / secsElapsed);
    }

    if (StatisticsConfig::CountDecisions::value == true) {
      stream << boost::format("| #D/s: %7.2f ") % (currentEra.m_decisionCount / secsElapsed);
    }

    if (StatisticsConfig::CountRestarts::value == true) {
      stream << boost::format("| #R/s: %4.2f ") % (currentEra.m_restartCount / secsElapsed);
    }
  }

  return stream;
}

template <typename T>
auto to_string(Statistics<T> const& stats) -> std::string
{
  std::stringstream recv;
  recv << stats;
  return recv.str();
}
}
