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

#pragma once

#include <functional>
#include <libjamsat/utils/Assert.h>

namespace jamsat {
/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::OnExitScope
 *
 * \brief Executes a user-defined operation when being destructed. The main
 * application of this class is being allocated with automatic storage duration,
 * executing the user-defined operation when the program execution leaves the
 * rsp. scope.
 */
class OnExitScope {
public:
  /**
   * \brief Constructs a new OnExitScope instance.
   *
   * \param callOnExit    A function which will be called when this instance is
   * destroyed. This function may not throw.
   */
  explicit OnExitScope(std::function<void()> callOnExit) noexcept
      : m_callOnExit(callOnExit) {
    JAM_ASSERT(callOnExit,
               "callOnExit must be a callable function, but is not");
    m_callOnExit = callOnExit;
  }

  OnExitScope &operator=(const OnExitScope &other) = delete;
  OnExitScope(const OnExitScope &other) = delete;

  OnExitScope &operator=(OnExitScope &&other) noexcept {
    if (this != &other) {
      this->m_callOnExit = other.m_callOnExit;
      other.m_callOnExit = std::function<void()>{};
    }
    return *this;
  }

  OnExitScope(OnExitScope &&other) noexcept {
    this->m_callOnExit = other.m_callOnExit;
    other.m_callOnExit = std::function<void()>{};
  }

  /**
   * \brief Destructs the instance, calling the \p callOnExit function which has
   * been passed to the constructor.
   */
  ~OnExitScope() noexcept {
    if (m_callOnExit) {
      m_callOnExit();
    }
  }

private:
  std::function<void()> m_callOnExit;
};
}
