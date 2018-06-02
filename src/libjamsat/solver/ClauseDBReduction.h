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

#pragma once

#include <boost/range.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <libjamsat/solver/LiteralBlockDistance.h>

#include <type_traits>
#include <vector>

namespace jamsat {

// TODO: document reduceClauseDB
template <typename ClauseDBTy, typename PropagationTy, typename TrailTy, typename ClauseRangeTy>
void reduceClauseDB(ClauseDBTy &clauseDB, PropagationTy &propagation, TrailTy &trail,
                    ClauseRangeTy toDeleteRange,
                    std::vector<typename ClauseDBTy::Clause *> &problemClauses,
                    std::vector<typename ClauseDBTy::Clause *> &learntClauses) {
    using Clause = typename ClauseDBTy::Clause;
    static_assert(std::is_same<typename ClauseDBTy::Clause, typename PropagationTy::Clause>::value,
                  "ClauseDBTy and PropagationTy must have the same Clause type");
    static_assert(std::is_same<typename ClauseDBTy::Clause, typename TrailTy::Clause>::value,
                  "ClauseDBTy and TrailTy must have the same Clause type");

    if (toDeleteRange.begin() == toDeleteRange.end()) {
        return;
    }

    for (auto toDelete : toDeleteRange) {
        if (!propagation.isAssignmentReason(*toDelete, trail)) {
            toDelete->setFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
        }
    }

    auto clausesInPropOrder = propagation.getClausesInPropagationOrder();

    std::vector<Clause *> clausesAfterRelocation;
    clauseDB.retain(boost::adaptors::filter(clausesInPropOrder,
                                            [](Clause const *clause) {
                                                return !clause->getFlag(
                                                    Clause::Flag::SCHEDULED_FOR_DELETION);
                                            }),
                    [&propagation, &trail](Clause const &clause) {
                        return propagation.isAssignmentReason(clause, trail);
                    },
                    [&propagation](Clause const &reason, Clause const &relocatedTo) {
                        propagation.updateAssignmentReason(reason, relocatedTo);
                    },
                    boost::optional<decltype(std::back_inserter(clausesAfterRelocation))>{
                        std::back_inserter(clausesAfterRelocation)});

    // Re-register relocated clauses:
    problemClauses.clear();
    learntClauses.clear();
    propagation.clear();
    for (auto clausePtr : clausesAfterRelocation) {
        clausePtr->clearFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
        if (clausePtr->template getLBD<LBD>() != 0) {
            learntClauses.push_back(clausePtr);
        } else {
            problemClauses.push_back(clausePtr);
        }
        propagation.registerEquivalentSubstitutingClause(*clausePtr);
    }
}
}
