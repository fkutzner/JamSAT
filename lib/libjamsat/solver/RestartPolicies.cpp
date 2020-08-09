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

/** \file */

#include "RestartPolicies.h"
#include <libjamsat/utils/Assert.h>

namespace jamsat {
LubyRestartPolicy::LubyRestartPolicy(const LubyRestartPolicy::Options& options) noexcept
  : m_lubySeq()
  , m_conflictsUntilRestart(options.graceTime > 0
                                ? options.graceTime + 1
                                : m_lubySeq.current() << options.log2OfScaleFactor)
  , m_log2OfScaleFactor(options.log2OfScaleFactor)
{
}

void LubyRestartPolicy::registerConflict(RegisterConflictArgs&& args) noexcept
{
  (void)args;
  if (m_conflictsUntilRestart > 0) {
    --m_conflictsUntilRestart;
  }
}

void LubyRestartPolicy::registerRestart() noexcept
{
  LubySequence::Element nextLubyElem = m_lubySeq.next();
  m_conflictsUntilRestart = nextLubyElem << m_log2OfScaleFactor;
}

bool LubyRestartPolicy::shouldRestart() const noexcept
{
  return m_conflictsUntilRestart == 0;
}

GlucoseRestartPolicy::GlucoseRestartPolicy(const GlucoseRestartPolicy::Options& options) noexcept
  : m_averageLBD(options.movingAverageWindowSize)
  , m_K(options.K)
  , m_sumLBD(0.0F)
  , m_conflictCount(0ULL)
{
}

void GlucoseRestartPolicy::registerConflict(
    GlucoseRestartPolicy::RegisterConflictArgs&& args) noexcept
{
  ++m_conflictCount;
  m_sumLBD += args.learntClauseLBD;
  m_averageLBD.add(args.learntClauseLBD);
}

void GlucoseRestartPolicy::registerRestart() noexcept
{
  m_averageLBD.clear();
}

bool GlucoseRestartPolicy::shouldRestart() const noexcept
{
  return m_averageLBD.isFull() &&
         ((m_averageLBD.getAverage() * m_K) > (m_sumLBD / m_conflictCount));
}
}
