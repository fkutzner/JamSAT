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

/**
 * \file utils/FaultInjector.h
 * \brief An exception injector for testing
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_set>

namespace jamsat {
/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::FaultInjector
 *
 * \brief A simple fault injector singleton for testing.
 *
 * Usage example: In locations where faults may occur which need to be
 * handled, check with the FaultInjector if a synthetic fault should be
 * generated for testing.
 */
class FaultInjector {
public:
  using const_iterator = std::unordered_set<std::string>::const_iterator;

  /**
   * \brief Enables faults matching the given name.
   *
   * \param which   The name of the faults to be marked as enabled, e.g. "low memory".
   */
  void enableFaults(const std::string& which);

  /**
   * \brief Determines whether synthetic faults matching the given name
   *        are enabled.
   *
   * Note: All synthetic faults are disabled by default.
   *
   * \param which   A name of faults.
   * \returns true iff \p which denotes a class of faults which should be synthesized for
   *          testing purposes.
   */
  bool isFaultEnabled(const std::string& which) const noexcept;

  /**
   * \brief Marks all synthetic faults as disabled.
   */
  void reset() noexcept;

  /**
   * \brief Returns a begin iterator of the names of enabled faults.
   *
   * \returns a begin iterator of the names of enabled faults.
   */
  const_iterator begin() const noexcept;

  /**
   * \brief Returns an end iterator of the names of enabled faults.
   *
   * \returns an end iterator of the names of enabled faults.
   */
  const_iterator end() const noexcept;

  /**
   * \brief Returns the singleton FaultInjector instance.
   *
   * \returns the singleton FaultInjector instance.
   */
  static FaultInjector& getInstance() noexcept
  {
    static FaultInjector instance;
    return instance;
  }

  FaultInjector(const FaultInjector& other) = delete;
  FaultInjector(FaultInjector&& other) = delete;
  FaultInjector& operator=(const FaultInjector& other) = delete;
  FaultInjector& operator=(FaultInjector&& other) = delete;

private:
  FaultInjector();
  std::unordered_set<std::string> m_enabledFaults;
};

/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::FaultInjectorResetRAII
 *
 * \brief An RAII object that, when destructed, restores the FaultInjector
 *        singleton's state to the state it had when this object was constructed.
 *
 * Usage example: Use FaultInjectorResetRAII in tests using fault injection to
 * ensure that none of the test's injected faults affect other tests.
 */
class FaultInjectorResetRAII {
public:
  /**
   * \brief Constructs the FaultInjectorResetRAII instance, storing a snapshot
   *        of the current FaultInjector singleton's state.
   */
  FaultInjectorResetRAII() noexcept;

  /**
   * \brief Destructs the FaultInjectorResetRAII instance, restoring the
   *        FaultInjector singleton's state.
   */
  ~FaultInjectorResetRAII() noexcept;

  FaultInjectorResetRAII(FaultInjectorResetRAII const& rhs) = delete;
  auto operator=(FaultInjectorResetRAII const& rhs) -> FaultInjectorResetRAII& = delete;
  FaultInjectorResetRAII(FaultInjectorResetRAII&& rhs) = default;
  auto operator=(FaultInjectorResetRAII&& rhs) -> FaultInjectorResetRAII& = default;

private:
  std::unordered_set<std::string> m_enabledFaults;
};

/**
 * \ingroup JamSAT_Utils
 *
 * \brief Throws an exception of the specified type when fault injection is
 * enabled for the given fault (via the FaultInjector singleton).
 *
 * \param fault     The fault's name.
 * \param ...       Arguments forwarded to the exception's constructor.
 *
 * \tparam Thrown   The type of the exception to be thrown.
 * \tparam ...      The types of exception constructor's arguments.
 */
template <typename Thrown, typename... Args>
void throwOnInjectedTestFault(const std::string& fault, Args&&... args)
{
  if (FaultInjector::getInstance().isFaultEnabled(fault)) {
    throw Thrown(std::forward<Args>(args)...);
  }
}
}
