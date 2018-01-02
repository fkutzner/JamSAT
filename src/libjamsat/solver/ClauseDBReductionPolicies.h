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

#include <algorithm>
#include <cstdint>

#include <boost/range.hpp>

#include <libjamsat/utils/Assert.h>

#if defined(JAM_ENABLE_LOGGING) && defined(JAM_ENABLE_REDUCE_LOGGING)
#include <boost/log/trivial.hpp>
#define JAM_LOG_REDUCE(x, y) BOOST_LOG_TRIVIAL(x) << "[reduce] " << y
#else
#define JAM_LOG_REDUCE(x, y)
#endif

namespace jamsat {
/**
 * \ingroup JamSAT_Solver
 *
 * \brief A policy deciding which clauses should be reduced from the main clause database
 *        modeled after the one used in Glucose.
 *
 * With this policy, ClauseDB reduction is admitted `K` conflicts after the previous reduction,
 * with `K` increasing by a fixed value at each reduction. The first reduction may be performed
 * any time when at least one clause has been learned.
 *
 * \tparam ClauseT              The clause type. `LBD l = c.getLBD()` must be a valid expression
 *                              for all ClauseT objects `c`.
 * \tparam LearntClauseSeq      A sequence container type for pointers to ClauseT.
 * \tparam LBD                  The LBD type, which must be an integral type.
 */
template <class ClauseT, class LearntClauseSeq, typename LBD>
class GlucoseClauseDBReductionPolicy {
public:
    /**
     * \brief Constructs a new ClauseDBReductionPolicy instance.
     *
     * \param intervalIncrease  The constant by which the intervals of conflicts between clause
     *                          DB reductions are increased at each reduction.
     * \param learntClauses     A reference to a sequence container containing pointers to the
     *                          learnt clauses.
     */
    GlucoseClauseDBReductionPolicy(uint32_t intervalIncrease,
                                   LearntClauseSeq &learntClauses) noexcept;

    /**
     * \brief Notifies the policy that the solver has performed a restart.
     */
    void registerConflict() noexcept;

    /**
     * \brief Determines whether a clause DB reduction should be performed.
     *
     * \returns true iff a clause DB reduction should be performed.
     */
    bool shouldReduceDB() const noexcept;

    /**
     * \brief Returns a range of clauses in \p learntClauses which should be purged from the
     *        clause database.
     *
     * The elements in \p learntClauses are rearranged by this method.
     *
     * A clause is selected for removal if its LBD value is higher than that of 50% of all
     * learnt clauses. If there are more "known good" clauses than clauses in \p learntClauses
     * or if a clause with LBD <= 3 would have to be removed, an empty range is returned.
     *
     * \param knownGoodClauses  The amount of "known good" learnt clauses which will never be
     *                          removed from the clause database and are not included in
     *                          \p learntClauses.
     * \returns The range of range of clauses in \p learntClauses which should be purged from
     *          the clause database.
     */
    boost::iterator_range<typename LearntClauseSeq::iterator>
    getClausesMarkedForDeletion(typename LearntClauseSeq::size_type knownGoodClauses) noexcept;

private:
    const uint32_t m_intervalIncrease;
    LearntClauseSeq &m_learntClauses;
    uint64_t m_intervalSize;
    uint64_t m_conflictsRemaining;
};

/********** Implementation ****************************** */

template <class ClauseT, class LearntClauseSeq, typename LBDType>
GlucoseClauseDBReductionPolicy<ClauseT, LearntClauseSeq, LBDType>::GlucoseClauseDBReductionPolicy(
    uint32_t intervalIncrease, LearntClauseSeq &learntClauses) noexcept
  : m_intervalIncrease(intervalIncrease)
  , m_learntClauses(learntClauses)
  , m_intervalSize(0)
  , m_conflictsRemaining(0) {}

template <class ClauseT, class LearntClauseSeq, typename LBDType>
void GlucoseClauseDBReductionPolicy<ClauseT, LearntClauseSeq,
                                    LBDType>::registerConflict() noexcept {
    if (m_conflictsRemaining > 0) {
        --m_conflictsRemaining;
    }
}

template <class ClauseT, class LearntClauseSeq, typename LBDType>
bool GlucoseClauseDBReductionPolicy<ClauseT, LearntClauseSeq, LBDType>::shouldReduceDB() const
    noexcept {
    return m_conflictsRemaining == 0 && !m_learntClauses.empty();
}

template <class ClauseT, class LearntClauseSeq, typename LBDType>
boost::iterator_range<typename LearntClauseSeq::iterator>
GlucoseClauseDBReductionPolicy<ClauseT, LearntClauseSeq, LBDType>::getClausesMarkedForDeletion(
    typename LearntClauseSeq::size_type knownGoodClauses) noexcept {

    using LearntClauseItRange = boost::iterator_range<typename LearntClauseSeq::iterator>;
    using LearntClauseIdx = typename LearntClauseSeq::size_type;

    JAM_ASSERT(shouldReduceDB(), "Clause DB reduction not allowed at this point");
    JAM_LOG_REDUCE(info, "Determining clauses to be removed...");

    m_intervalSize += m_intervalIncrease;
    m_conflictsRemaining = m_intervalSize;

    LearntClauseIdx midIndex = (knownGoodClauses + m_learntClauses.size()) / 2;
    if (midIndex >= m_learntClauses.size()) {
        JAM_LOG_REDUCE(info, "Selecting no clauses for reduction: too few learnt clauses");
        return LearntClauseItRange{m_learntClauses.end(), m_learntClauses.end()};
        ;
    }

    std::sort(m_learntClauses.begin(), m_learntClauses.end(), [](ClauseT *lhs, ClauseT *rhs) {
        return lhs->template getLBD<LBDType>() < rhs->template getLBD<LBDType>();
    });

    if (m_learntClauses[midIndex]->template getLBD<LBDType>() <= static_cast<LBDType>(3)) {
        JAM_LOG_REDUCE(info, "Selecting no clauses for reduction: LBD values are too low");
        return LearntClauseItRange{m_learntClauses.end(), m_learntClauses.end()};
        ;
    }

    JAM_LOG_REDUCE(info,
                   "Selecting " << (m_learntClauses.size() - midIndex) << " clauses for reduction");
    return LearntClauseItRange{m_learntClauses.begin() + midIndex, m_learntClauses.end()};
}
}
