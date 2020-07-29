/* Copyright (c) 2019 Felix Kutzner (github.com/fkutzner)

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

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/IterableClauseDB.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/OccurrenceMap.h>
#include <libjamsat/utils/StampMap.h>

#include <boost/range.hpp>

#include <cstdint>
#include <vector>

namespace jamsat {
/**
 * \brief Variable elimination by distribution
 * 
 * For a description of elimination via distribution, see:
 * Een, Niklas; Biere, Armin. Effective preprocessing in SAT through variable and
 * clause elimination. In: International conference on theory and applications of
 * satisfiability testing. Springer, Berlin, Heidelberg, 2005. S. 61-75. 
 * 
 * \ingroup JamSAT_Simplification
 */
class ClauseDistribution {
private:
    constexpr static std::size_t databaseRegionSize = 1048576;

public:
    using iterator = IterableClauseDB<Clause>::iterator;
    using size_type = std::size_t;

    enum class DistributionStatus { OK, OK_DETECTED_UNSATISFIABILITY, FAILED };

    struct DistributionResult {
    public:
        DistributionStatus status;

        /** If `status` is `OK`, `clauses` is the range of clauses produced by variable elimination. */
        boost::iterator_range<iterator> clauses;

        /** If `status` is `OK`, `clauses` is the number of clauses produced by variable elimination. */
        size_type numClauses;
    };

    /**
     * Constructs a ClauseDistribution object.
     * 
     * \param maxVar        The maximum variable occurring in clauses passed to the
     *                      constructed object.
     * 
     * \throws std::bad_alloc
     */
    explicit ClauseDistribution(CNFVar maxVar);

    /**
     * \brief Clears any previously computed distributed clauses and computes the set of clauses
     * and computes result of performing clause distribution at a given literal.
     * 
     * The set `R` of resulting clauses can be obtained via `getDistributedClauses()`. `R`
     * contains no trivially satisfied clauses. The clauses in the union of 
     * `litOccurrences[distributeAt]` and `litOccurrences[~distributeAt]` may be replaced by adding
     * the clauses contained in `R` to the SAT problem instance represented by `litOccurrences`,
     * preserving equisatisfiability (but not equivalence).
     * 
     * \tparam OccurrenceMapT       An occurrence map for CNFLit objects, with arbitrary container
     *                              type
     * 
     * \param litOccurrences        The problem to be simplified, in form of a literal occurrence map.
     * \param distributeAt          The variable which shall be eliminated, with `distributeAt`,
     *                              `~distributeAt` being contained in the domain of `litOccurrences`.
     * 
     * \returns A DistributionResult whose status is `OK_DETECTED_UNSATISFIABILITY` when the problem
     *          has been detected to be unsatisfiable (i.e. the empty clause has been obtained via
     *          resolution); `FAILED` if the operation could not be completed; `OK` otherwise.
     *          The result's `clauses` range remains valid until `distribute` is called again or the
     *          corresponding ClauseDistribution object is destroyed.
     */
    template <typename OccurrenceMapT>
    auto distribute(OccurrenceMapT& litOccurrences, CNFVar distributeAt) noexcept
        -> DistributionResult;


    /**
     * \brief Checks whether variable elimination via distribution is worthwile
     * 
     * \tparam OccurrenceMapT       An occurrence map for CNFLit objects, with arbitrary container
     *                              type
     * 
     * \param litOccurrences        The problem to be simplified, in form of a literal occurrence map.
     * \param distributeAt          A variable with `distributeAt`, `~distributeAt` being contained in
     *                              the domain of `litOccurrences`.
     * 
     * \returns `true` iff eliminating `distributeAt` via distribution would reduce the
     *          amount of problem clauses.
     */
    template <typename OccurrenceMapT>
    auto isDistributionWorthwile(OccurrenceMapT& litOccurrences, CNFVar distributeAt) noexcept
        -> bool;

private:
    void clearDistributedClauses() noexcept;

    template <typename OccurrenceMapT>
    auto computeDistributedClauses(OccurrenceMapT& litOccurrences, CNFVar distributeAt) noexcept
        -> DistributionResult;

    StampMap<std::uint16_t, CNFLit::Index> m_seenLits;
    IterableClauseDB<Clause> m_clauses;
    size_type m_numDistributedClauses;
};

/********** Implementation ****************************** */

template <typename OccurrenceMapT>
auto ClauseDistribution::distribute(OccurrenceMapT& litOccurrences, CNFVar distributeAt) noexcept
    -> DistributionResult {
    static_assert(is_occurrence_map<OccurrenceMapT, CNFLit>::value,
                  "OccurrenceMapT must be an occurrence map for CNFLit objects");
    clearDistributedClauses();
    return computeDistributedClauses(litOccurrences, distributeAt);
}

template <typename OccurrenceMapT>
auto ClauseDistribution::computeDistributedClauses(OccurrenceMapT& litOccurrences,
                                                   CNFVar distributeAt) noexcept
    -> DistributionResult {
    CNFLit distributeAtPos = CNFLit{distributeAt, CNFSign::POSITIVE};
    std::vector<CNFLit> partialDistributedClause;

    for (auto const* posClause : litOccurrences[distributeAtPos]) {
        auto stampingContext = m_seenLits.createContext();
        auto stamp = stampingContext.getStamp();
        partialDistributedClause.clear();

        for (CNFLit lit : *posClause) {
            if (lit.getVariable() != distributeAt) {
                m_seenLits.setStamped(lit, stamp, true);
                try {
                    partialDistributedClause.push_back(lit);
                } catch (std::bad_alloc& e) {
                    return {DistributionStatus::FAILED, {}, 0};
                }
            }
        }

        auto baseSize = partialDistributedClause.size();

        for (auto const* negClause : litOccurrences[~distributeAtPos]) {
            for (CNFLit lit : *negClause) {
                if (lit.getVariable() == distributeAt || m_seenLits.isStamped(lit, stamp)) {
                    continue; // lit is irrelevant or already contained in partialDistributedClause
                }
                if (m_seenLits.isStamped(~lit, stamp)) {
                    goto outer_continue; // The resulting clause would be trivially satisfied -> ignore
                }

                try {
                    partialDistributedClause.push_back(lit);
                } catch (std::bad_alloc& e) {
                    return {DistributionStatus::FAILED, {}, 0};
                }
            }

            if (partialDistributedClause.empty()) {
                return {DistributionStatus::OK_DETECTED_UNSATISFIABILITY, {}, 0};
            }

            if (auto distClauseOpt = m_clauses.createClause(partialDistributedClause.size())) {
                std::copy(partialDistributedClause.begin(),
                          partialDistributedClause.end(),
                          (*distClauseOpt)->begin());
                ++m_numDistributedClauses;
            } else {
                return {DistributionStatus::FAILED, {}, 0};
            }

        outer_continue:
            partialDistributedClause.resize(baseSize);
        }
    }

    return {DistributionStatus::OK, m_clauses.getClauses(), m_numDistributedClauses};
}


template <typename OccurrenceMapT>
auto ClauseDistribution::isDistributionWorthwile(OccurrenceMapT& litOccurrences,
                                                 CNFVar distributeAt) noexcept -> bool {
    CNFLit distributeAtPos = CNFLit{distributeAt, CNFSign::POSITIVE};
    auto posClausesRange = litOccurrences[distributeAtPos];
    auto negClausesRange = litOccurrences[~distributeAtPos];

    std::size_t numTriviallySatResolvents = 0;

    for (auto const* lClause : posClausesRange) {
        auto stampingContext = m_seenLits.createContext();
        auto stamp = stampingContext.getStamp();

        for (CNFLit lit : *lClause) {
            m_seenLits.setStamped(lit, stamp, true);
        }

        for (auto const* rClause : negClausesRange) {
            bool isResolventTriviallySatisfied = false;
            for (CNFLit lit : *rClause) {
                if (lit.getVariable() != distributeAt && m_seenLits.isStamped(~lit, stamp)) {
                    ++numTriviallySatResolvents;
                    isResolventTriviallySatisfied = true;
                    break;
                }
            }
        }
    }

    std::uint64_t numClPos = std::distance(posClausesRange.begin(), posClausesRange.end());
    std::uint64_t numClNeg = std::distance(negClausesRange.begin(), negClausesRange.end());
    std::uint64_t numClTotal = numClPos + numClNeg;
    std::uint64_t numDistCl = (numClPos * numClNeg) - numTriviallySatResolvents;

    return numDistCl < numClTotal;
}
}