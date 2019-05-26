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
#include <libjamsat/utils/Concepts.h>
#include <libjamsat/utils/StampMap.h>

#include <iterator>
#include <type_traits>

namespace jamsat {

template <typename ClauseT>
struct SSROpportunity {
    CNFLit resolveAt = CNFLit::getUndefinedLiteral();
    ClauseT const* clause = nullptr;
};

namespace subsumption_checker_detail {
template <typename ClauseT>
auto compareClausesQuadratic(ClauseT const& subsumeeCandidate,
                             ClauseT const& subsumerCandidate,
                             SSROpportunity<ClauseT>& ssrOpportunity) -> bool {

    CNFLit ssrOpportunityResolveAt = CNFLit::getUndefinedLiteral();

    for (CNFLit x : subsumerCandidate) {
        bool found = false;

        for (CNFLit y : subsumeeCandidate) {
            if (x == y) {
                found = true;
                break;
            } else if (x == ~y && ssrOpportunityResolveAt == CNFLit::getUndefinedLiteral()) {
                ssrOpportunityResolveAt = x;
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    if (ssrOpportunityResolveAt != CNFLit::getUndefinedLiteral()) {
        ssrOpportunity.clause = &subsumerCandidate;
        ssrOpportunity.resolveAt = ssrOpportunityResolveAt;
        return false;
    }
    return true;
}


template <typename ClauseT, typename StampMapT>
auto compareClausesLinear(StampMapT const& subsumeeCandidateLits,
                          typename StampMapT::Stamp stamp,
                          ClauseT const& subsumerCandidate,
                          SSROpportunity<ClauseT>& ssrOpportunity) -> bool {
    CNFLit ssrOpportunityResolveAt = CNFLit::getUndefinedLiteral();

    // Linear, but cache-unfriendly comparison for large clauses:
    for (CNFLit l : subsumerCandidate) {
        if (subsumeeCandidateLits.isStamped(l, stamp)) {
            continue;
        }

        if (ssrOpportunityResolveAt == CNFLit::getUndefinedLiteral() &&
            subsumeeCandidateLits.isStamped(~l, stamp)) {
            ssrOpportunityResolveAt = l;
        } else {
            return false;
        }
    }

    if (ssrOpportunityResolveAt != CNFLit::getUndefinedLiteral()) {
        ssrOpportunity.clause = &subsumerCandidate;
        ssrOpportunity.resolveAt = ssrOpportunityResolveAt;
        return false;
    }
    return true;
}


template <typename ClauseT, typename StampMapT>
auto compareClauses(ClauseT const& subsumeeCandidate,
                    ClauseT const& subsumerCandidate,
                    StampMapT& stampMap,
                    typename StampMapT::Stamp stamp,
                    SSROpportunity<ClauseT>& ssrOpportunity) -> bool {
    if (subsumerCandidate.size() > subsumeeCandidate.size()) {
        return false;
    }

    if (subsumerCandidate.size() < 10) {
        return compareClausesQuadratic(subsumeeCandidate, subsumerCandidate, ssrOpportunity);
    }
    return compareClausesLinear(stampMap, stamp, subsumerCandidate, ssrOpportunity);
}
}

template <typename ClauseT,
          typename ClausePtrRng,
          typename StampMapT,
          typename SSROpportunityOutputIter>
auto isSubsumedBy(ClauseT const& subsumeeCandidate,
                  ClausePtrRng subsumerCandidates,
                  StampMapT& stampMap,
                  SSROpportunityOutputIter ssrOpportunityBegin) -> bool {
    static_assert(is_clause<ClauseT>::value, "ClauseT must satisfy the Clause concept");
    static_assert(is_stamp_map<StampMapT, CNFLit>::value,
                  "StampMapT must be a StampMap supporting CNFLit");
    // todo: add missing assertions about ranges and iterators

    auto stampingContext = stampMap.createContext();
    auto stamp = stampingContext.getStamp();

    for (auto lit : subsumeeCandidate) {
        stampMap.setStamped(lit, stamp, true);
    }

    bool isSubsumed = false;
    for (ClauseT const* potentialSubsumer : subsumerCandidates) {
        if (potentialSubsumer->mightShareAllVarsWith(subsumeeCandidate)) {
            SSROpportunity<ClauseT> ssrOpportunity;
            isSubsumed |= subsumption_checker_detail::compareClauses(
                subsumeeCandidate, *potentialSubsumer, stampMap, stamp, ssrOpportunity);
            if (ssrOpportunity.resolveAt != CNFLit::getUndefinedLiteral()) {
                *ssrOpportunityBegin = ssrOpportunity;
                ++ssrOpportunityBegin;
            }
        }
    }

    return isSubsumed;
}
}