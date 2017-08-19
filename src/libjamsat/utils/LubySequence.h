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

#include <cstdint>

namespace jamsat {

/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::LubySequence
 *
 * \brief Computes the Luby sequence and encapsulates the state of the
 * computation.
 *
 */
class LubySequence {
public:
  using Element = int64_t;

  /**
   * \brief Constructs a fresh LubySequence.
   */
  LubySequence() noexcept;

  /**
   * \brief Returns the next element of the Luby sequence.
   *
   * \returns the (N+1)th element of the Luby sequence iff next() has been
   * called exactly N times before.
   */
  Element next() noexcept;

  /**
   * \brief Returns the current element of the Luby sequence.
   *
   * \returns the (N+1)th element of the Luby sequence iff next() has been
   * called exactly N times before.
   */
  Element current() const noexcept;

private:
  struct LubyState {
    LubySequence::Element u;
    LubySequence::Element v;
  };

  LubyState m_state;
};

inline LubySequence::LubySequence() noexcept : m_state({1, 1}) {}

inline LubySequence::Element LubySequence::next() noexcept {
  Element u = m_state.u;
  Element v = m_state.v;
  m_state = ((u & -u) == v) ? LubyState{u + 1, 1} : LubyState{u, 2 * v};
  return current();
}

inline LubySequence::Element LubySequence::current() const noexcept {
  return m_state.v;
}
}
