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

/**
 * \file SubsumptionChecker.h
 * \brief Generic subsumption and self-subsuming resolution checker
 */

#pragma once

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/concepts/ClauseTraits.h>

namespace jamsat {

/**
 * \brief Representation of an optimization opportunity using self-subsuming resolution
 *
 * \ingroup JamSAT_Simplification
 */
template <typename ClauseT>
struct SSROpportunity {
    /** The index of the literal in `*clause` with which to resolve (contained in this object's `*clause`) */
    typename ClauseT::size_type resolveAtIdx = 0;

    /** The clause with which to resolve */
    ClauseT const* clause = nullptr;
};

/**
 * \brief Given a clause C, computes the set of clauses which are subsumed by C or can be strengthened
 *        by applying self-subsuming resolution with C.
 *
 * \ingroup JamSAT_Simplification
 *
 * \tparam ClauseT                      The clause type
 * \tparam ClausePtrRng                 An iterator range type with values convertible to `ClauseT const*`
 * \tparam ClausePtrOutputIter          A type satisfying OutputIterator for `ClauseT const*`
 * \tparam SSROpportunityOutputIter     A type satisfying OutputIterator for `SSROpportunity<ClauseT>`
 *
 * \param subsumer              The subsumer clause
 * \param subsumeeCandidates    The candidates for removal via subsumption or self-subsuming resolution (SSR)
 * \param maxSubsumeeSize       The maximum size of clauses in `subsumeeCandidates` to take into account. Clauses
 *                              larger than `maxSubsumeeSize` are not subsumption-/SSR-checked.
 * \param subsumedClauseOutputBegin An output iterator receiving `ClauseT const*` pointers `c` to clauses which
 *                              are subsumed by `subsumer`, with `c` obtained from iterating over
 *                              `subsumeeCandidates`.
 * \param ssrOpportunityBegin   An output iterator receiving `SSROpportunity<ClauseT>` objects o with clauses
 *                              `o.clause` of `subsumerCandidates` such that SSR can be performed with `c` and
 *                              `subsumeeCandidate`; `o.resolveAt` is the literal contained in `o.clause` which
 *                              can be removed via SSR while preserving satisfiability.
 *
 * \par Exception safety
 *
 * This function may throw only exceptions that can be thrown by any parameter's methods. If an exception
 * is thrown, `stampMap` is restored to a valid state. However, SSR opportunities may have been passed
 * to the SSR opportunity output iterator.
 */
template <typename ClauseT,
          typename ClausePtrRng,
          typename ClausePtrOutputIter,
          typename SSROpportunityOutputIter>
void getSubsumedClauses(ClauseT const& subsumerCandidate,
                        ClausePtrRng subsumeeCandidates,
                        typename ClauseT::size_type maxSubsumeeSize,
                        ClausePtrOutputIter subsumedClauseOutputBegin,
                        SSROpportunityOutputIter ssrOpportunityBegin);

/********** Implementation ****************************** */

namespace subsumption_checker_detail {
template <typename ClauseT>
auto compareClausesQuadratic(ClauseT const& subsumerCandidate,
                             ClauseT const& subsumeeCandidate,
                             SSROpportunity<ClauseT>& ssrOpportunity) -> bool {

    bool foundSSROpportunity = false;
    typename ClauseT::size_type ssrOpportunityResolveAtIdx = 0;

    for (CNFLit x : subsumerCandidate) {
        bool found = false;

        for (typename ClauseT::size_type yIdx = 0; yIdx < subsumeeCandidate.size(); ++yIdx) {
            CNFLit y = subsumeeCandidate[yIdx];
            if (x == y) {
                found = true;
                break;
            } else if (x == ~y && !foundSSROpportunity) {
                foundSSROpportunity = true;
                ssrOpportunityResolveAtIdx = yIdx;
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    if (foundSSROpportunity) {
        ssrOpportunity.clause = &subsumeeCandidate;
        ssrOpportunity.resolveAtIdx = ssrOpportunityResolveAtIdx;
        return false;
    }
    return true;
}
}

template <typename ClauseT,
          typename ClausePtrRng,
          typename ClausePtrOutputIter,
          typename SSROpportunityOutputIter>
void getSubsumedClauses(ClauseT const& subsumerCandidate,
                        ClausePtrRng subsumeeCandidates,
                        typename ClauseT::size_type maxSubsumeeSize,
                        ClausePtrOutputIter subsumedClauseOutputBegin,
                        SSROpportunityOutputIter ssrOpportunityBegin) {
    static_assert(is_clause<ClauseT>::value, "ClauseT must satisfy the Clause concept");

    for (ClauseT const* subsumeeCandidate : subsumeeCandidates) {
        if (subsumeeCandidate->size() < maxSubsumeeSize &&
            subsumerCandidate.mightShareAllVarsWith(*subsumeeCandidate)) {
            SSROpportunity<ClauseT> ssrOpportunity;
            bool isSubsumed = subsumption_checker_detail::compareClausesQuadratic(
                subsumerCandidate, *subsumeeCandidate, ssrOpportunity);

            if (isSubsumed) {
                *subsumedClauseOutputBegin = subsumeeCandidate;
                ++subsumedClauseOutputBegin;
            } else if (ssrOpportunity.clause != nullptr) {
                *ssrOpportunityBegin = ssrOpportunity;
                ++ssrOpportunityBegin;
            }
        }
    }
}
}
