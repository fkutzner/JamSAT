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
 * \file RestartPolicies.h
 * \brief Restart policies for CDCL search
 *
 * Restart policies are used to control when to restart the CDCL search
 * keeping derived lemmas and heuristic state.
 */

#pragma once

#include <cstdint>

#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/utils/LubySequence.h>
#include <libjamsat/utils/SimpleMovingAverage.h>

namespace jamsat {

/**
 * \ingroup JamSAT_Solver
 *
 * \brief A restart policy similar to the one used in the Glucose solver.
 *
 * This restart policy triggers a restart when `(AverageLBD * K) > (SumLBD / TotalConflictCount)`
 * with `AverageLBD` being the average LBD of the past 50 derived lemmas, K is a constant
 * (by default 0.8), SumLBD being the total sum of LBD values of derived lemmas and
 * TotalConflictCount being the total amount of lemmas derived.
 */
class GlucoseRestartPolicy {
public:
    struct Options {
        uint64_t movingAverageWindowSize = 50;
        double K = 0.8f;
    };

    struct RegisterConflictArgs {
        LBD learntClauseLBD;
    };

    /**
     * \brief Constructs a GlucoseRestartPolicy instance.
     *
     * \param[in] options   the configuration of the restart policy.
     */
    explicit GlucoseRestartPolicy(const Options& options) noexcept;

    /**
     * \brief Notifies the restart policy that the client has handled a conflict.
     *
     * \param[in] args      client state just after handling the conflict. See
     * GlucoseRestartPolicy::RegisterConflictArgs.
     */
    void registerConflict(RegisterConflictArgs&& args) noexcept;

    /**
     * \brief Notifies the restart policy that the client has handled a restart.
     */
    void registerRestart() noexcept;

    /**
     * \brief Indicates whether the client should restart.
     *
     * \returns true iff the client should restart.
     */
    bool shouldRestart() const noexcept;

private:
    SimpleMovingAverage<LBD> m_averageLBD;
    double m_K;
    double m_sumLBD;
    uint64_t m_conflictCount;
};

/**
 * \ingroup JamSAT_Solver
 *
 * \brief A static restart policy based on the Luby sequence.
 */
class LubyRestartPolicy {
public:
    struct Options {
        /** The size of the first restart interval. */
        uint64_t graceTime = 10000;

        /**
         * The logarithm (to the base of 2) of the restart interval scale factor.
         * If the Luby sequence is given as l1, l2, ..., lN, ..., restarts are
         * issued after log2OfScaleFactor*l1, log2OfScaleFactor*(l1 + l2) , ...
         * conflicts.
         */
        uint64_t log2OfScaleFactor = 7;
    };

    struct RegisterConflictArgs {};

    /**
     * \brief Constructs a LubyRestart instance.
     *
     * \param[in] options   the configuration of the restart policy.
     */
    explicit LubyRestartPolicy(const Options& options) noexcept;

    /**
     * \brief Notifies the restart policy that the client has handled a conflict.
     *
     * \param[in] args      client state just after handling the conflict. See
     * LubyRestartPolicy::RegisterConflictArgs.
     */
    void registerConflict(RegisterConflictArgs&& args) noexcept;

    /**
     * \brief Notifies the restart policy that the client has handled a restart.
     */
    void registerRestart() noexcept;

    /**
     * \brief Indicates whether the client should restart.
     *
     * \returns true iff the client should restart.
     */
    bool shouldRestart() const noexcept;

private:
    LubySequence m_lubySeq;
    uint64_t m_conflictsUntilRestart;
    const uint64_t m_log2OfScaleFactor;
};
}
