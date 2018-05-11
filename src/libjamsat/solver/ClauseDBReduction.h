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

#include <boost/range.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <libjamsat/solver/LiteralBlockDistance.h>

#include <type_traits>
#include <vector>

namespace jamsat {

// TODO: document reduceClauseDB
template <typename ClauseTy, typename ClauseDBTy, typename PropagationTy, typename TrailTy,
          typename ClauseRangeTy>
void reduceClauseDB(ClauseDBTy &clauseDB, PropagationTy &propagation, TrailTy &trail,
                    ClauseRangeTy toDeleteRange, std::vector<ClauseTy *> &problemClauses,
                    std::vector<ClauseTy *> &learntClauses) {

    if (toDeleteRange.begin() == toDeleteRange.end()) {
        return;
    }

    for (auto toDelete : toDeleteRange) {
        if (!propagation.isAssignmentReason(*toDelete, trail)) {
            toDelete->setFlag(ClauseTy::Flag::SCHEDULED_FOR_DELETION);
        }
    }

    auto clausesInPropOrder = propagation.getClausesInPropagationOrder();

    std::vector<ClauseTy *> clausesAfterRelocation;
    clauseDB.retain(boost::adaptors::filter(clausesInPropOrder,
                                            [](ClauseTy const *clause) {
                                                return !clause->getFlag(
                                                    ClauseTy::Flag::SCHEDULED_FOR_DELETION);
                                            }),
                    [&propagation, &trail](ClauseTy const &clause) {
                        return propagation.isAssignmentReason(clause, trail);
                    },
                    [&propagation](ClauseTy const &reason, ClauseTy const &relocatedTo) {
                        propagation.updateAssignmentReason(reason, relocatedTo);
                    },
                    boost::optional<decltype(std::back_inserter(clausesAfterRelocation))>{
                        std::back_inserter(clausesAfterRelocation)});

    // Re-register relocated clauses:
    problemClauses.clear();
    learntClauses.clear();
    // The reasons have already been updated to point at the relocated clauses, so keep them:
    propagation.clear();
    for (auto clausePtr : clausesAfterRelocation) {
        clausePtr->clearFlag(ClauseTy::Flag::SCHEDULED_FOR_DELETION);
        if (clausePtr->template getLBD<LBD>() != 0) {
            learntClauses.push_back(clausePtr);
        } else {
            problemClauses.push_back(clausePtr);
        }
        propagation.registerEquivalentSubstitutingClause(*clausePtr);
    }
}
}
