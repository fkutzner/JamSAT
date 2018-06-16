/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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

#include <gtest/gtest.h>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/HeapletClauseDB.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/ClauseDBReduction.h>
#include <libjamsat/solver/LiteralBlockDistance.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/Trail.h>

#include <boost/range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/join.hpp>

#include <algorithm>
#include <cstdint>
#include <random>
#include <set>
#include <unordered_set>

namespace jamsat {

namespace {
using ClauseFingerprint = uint64_t;

template <typename ClauseTy>
auto fingerprintClause(ClauseTy const& clause) -> ClauseFingerprint {
    uint64_t result = clause.size();
    result ^= (static_cast<uint64_t>(clause.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION)) << 32);
    result ^= (static_cast<uint64_t>(clause.template getLBD<LBD>()) << 32);

    for (auto lit : clause) {
        uint64_t variable = static_cast<uint64_t>(lit.getVariable().getRawValue());
        result ^= (variable << (lit.getSign() == CNFSign::POSITIVE ? 0 : 32));
    }

    return result;
}

template <typename ClausePtrRange>
auto fingerprint(ClausePtrRange const& clauses) -> ClauseFingerprint {
    uint64_t result = 0ULL;
    for (auto clause : clauses) {
        result ^= fingerprintClause(*clause);
    }
    return result;
}

template <typename ClauseTy,
          typename ClauseDBTy,
          typename PropagationTy,
          typename TrailTy,
          typename ClauseRangeTy>
void checkedReduceClauseDB(ClauseDBTy& clauseDB,
                           PropagationTy& propagation,
                           TrailTy& trail,
                           ClauseRangeTy toDeleteRange,
                           std::vector<ClauseTy*>& problemClauses,
                           std::vector<ClauseTy*>& learntClauses) {

    auto assignmentReasonPred = [&propagation, &trail](Clause* clause) {
        return propagation.isAssignmentReason(*clause, trail);
    };

    ClauseFingerprint permanentsFingerprint = fingerprint(problemClauses);
    ClauseFingerprint learntsFingerprint = fingerprint(learntClauses);
    ClauseFingerprint toDeleteFingerprint = fingerprint(toDeleteRange);
    ClauseFingerprint learntReasonsFingerprint =
        fingerprint(boost::adaptors::filter(toDeleteRange, assignmentReasonPred));


    auto permanentsPreSize = problemClauses.size();
    auto learntsPreSize = learntClauses.size();
    auto amntToDelete =
        (toDeleteRange.end() - toDeleteRange.begin()) -
        std::count_if(toDeleteRange.begin(), toDeleteRange.end(), assignmentReasonPred);

    reduceClauseDB(clauseDB, propagation, trail, toDeleteRange, problemClauses, learntClauses);


    // Check that exactly the clauses that should be deleted have indeed
    // been deleted:
    EXPECT_EQ(problemClauses.size(), permanentsPreSize);
    EXPECT_EQ(learntClauses.size(), learntsPreSize - amntToDelete);

    ClauseFingerprint postPermanentsFingerprint = fingerprint(problemClauses);
    ClauseFingerprint postLearntsFingerprint = fingerprint(learntClauses);

    EXPECT_EQ(postPermanentsFingerprint, permanentsFingerprint);
    // Fingerprints of clause ranges are computed by XORing clause fingerprints
    // ~> "subtract" and "add" fingerprints via XOR
    EXPECT_EQ(postLearntsFingerprint,
              learntsFingerprint ^ toDeleteFingerprint ^ learntReasonsFingerprint);

    // Check that the watchers contain exactly the pointers to the relocated clauses
    auto relocatedClauses = boost::range::join(problemClauses, learntClauses);
    std::vector<Clause const*> watchedClausesVec;
    auto watchedClauses = propagation.getClausesInPropagationOrder();

    // Each clause occurs twice in watchedClauses => compute unique clauses
    std::copy(watchedClauses.begin(), watchedClauses.end(), std::back_inserter(watchedClausesVec));
    std::sort(watchedClausesVec.begin(), watchedClausesVec.end());
    auto watchedClausesVecEnd = std::unique(watchedClausesVec.begin(), watchedClausesVec.end());
    watchedClausesVec.erase(watchedClausesVecEnd, watchedClausesVec.end());

    ASSERT_EQ(watchedClausesVec.size(), problemClauses.size() + learntClauses.size());
    EXPECT_TRUE(std::is_permutation(
        watchedClausesVec.begin(), watchedClausesVec.end(), relocatedClauses.begin()));

    // Check that reason clauses have been preserved:
    for (CNFLit assignment : trail.getAssignments(0)) {
        if (Clause const* reason = propagation.getAssignmentReason(assignment.getVariable())) {
            bool found = (std::find(relocatedClauses.begin(),
                                    relocatedClauses.end(),
                                    const_cast<Clause*>(reason)) != relocatedClauses.end());
            EXPECT_TRUE(found) << "Reason clause not preserved";
        }
    }
}

// Inserts \p nClauses random clauses of lengths in [2...20] into ClauseDB
// and returns a vector containing pointers to the inserted clauses.
auto makeClauses(HeapletClauseDB<Clause>& clauseDB,
                 Propagation<Trail<Clause>>& trail,
                 int nClauses,
                 CNFVar maxVar,
                 bool isLearnt) -> std::vector<Clause*> {
    std::vector<Clause*> result;

    std::mt19937_64 rng; // using no random seed => deterministic results

    std::uniform_int_distribution<CNFVar::RawVariable> randomVarDistribution{0,
                                                                             maxVar.getRawValue()};
    std::uniform_int_distribution<int> randomSignDistribution{0, 1};
    std::uniform_int_distribution<unsigned int> clauseLengthDistribution{2, 20};
    std::uniform_int_distribution<LBD> lbdDistribution{1, 20};

    std::set<CNFVar::RawVariable> usedVariables;

    for (int i = 0; i < nClauses; ++i) {
        usedVariables.clear();
        Clause& clause = clauseDB.allocate(clauseLengthDistribution(rng));
        for (unsigned int j = 0; j < clause.size(); ++j) {
            CNFVar variable;
            do {
                variable = CNFVar{randomVarDistribution(rng)};
            } while (usedVariables.find(variable.getRawValue()) != usedVariables.end());
            CNFSign sign =
                (randomSignDistribution(rng) == 1 ? CNFSign::POSITIVE : CNFSign::NEGATIVE);
            clause[j] = CNFLit{variable, sign};
        }

        if (isLearnt) {
            clause.setLBD(lbdDistribution(rng));
        }

        trail.registerClause(clause);
        result.push_back(&clause);
    }

    return result;
}
}

TEST(IntegrationClauseDBReduction, reduceIsConsistentOnEmptyProblem) {
    Trail<Clause> trail{CNFVar{100}};
    Propagation<Trail<Clause>> propagation{CNFVar{100}, trail};
    HeapletClauseDB<Clause> clauseDB{256ULL, 1048576ULL};

    std::vector<Clause*> problemClauses;
    std::vector<Clause*> learntClauses;
    std::vector<Clause*> toDelete;

    checkedReduceClauseDB(clauseDB, propagation, trail, toDelete, problemClauses, learntClauses);
}

TEST(IntegrationClauseDBReduction, reduceDeletesNonreasonClauses) {
    CNFVar maxVar{100};
    Trail<Clause> trail{maxVar};
    Propagation<Trail<Clause>> propagation{maxVar, trail};
    HeapletClauseDB<Clause> clauseDB{256ULL, 1048576ULL};

    std::vector<Clause*> problemClauses = makeClauses(clauseDB, propagation, 20, maxVar, false);
    std::vector<Clause*> learntClauses = makeClauses(clauseDB, propagation, 150, maxVar, true);
    auto toDelete = boost::make_iterator_range(learntClauses.end() - 100, learntClauses.end());
    checkedReduceClauseDB(clauseDB, propagation, trail, toDelete, problemClauses, learntClauses);
}

void tryCreateForcingAssignment(Trail<Clause>& trail,
                                Propagation<Trail<Clause>>& propagation,
                                Clause const* clause) {
    for (auto lit : *clause) {
        if (!isDeterminate(trail.getAssignment(lit))) {
            trail.addAssignment(~lit);
            propagation.propagateUntilFixpoint(~lit);
        }
    }
}

TEST(IntegrationClauseDBReduction, reducePreservesReasonClauses) {
    CNFVar maxVar{2000};
    Trail<Clause> trail{maxVar};
    Propagation<Trail<Clause>> propagation{maxVar, trail};
    HeapletClauseDB<Clause> clauseDB{256ULL, 1048576ULL};

    std::vector<Clause*> problemClauses = makeClauses(clauseDB, propagation, 20, maxVar, false);
    std::vector<Clause*> learntClauses = makeClauses(clauseDB, propagation, 150, maxVar, true);
    auto toDelete = boost::make_iterator_range(learntClauses.end() - 100, learntClauses.end());

    // pick clauses in problemClauses and try to make them reason clauses:
    trail.newDecisionLevel();
    for (size_t i = 0; i < 20; ++i) {
        tryCreateForcingAssignment(trail, propagation, problemClauses[i]);
    }

    ASSERT_TRUE(std::any_of(problemClauses.begin(),
                            problemClauses.end(),
                            [&propagation, &trail](Clause const* clause) {
                                return propagation.isAssignmentReason(*clause, trail);
                            }))
        << "Bad test data: did not create reason clauses in problemClauses";

    // pick clauses in toDelete and try to make them reason clauses:
    trail.newDecisionLevel();
    for (int i = 0; i < 50; ++i) {
        tryCreateForcingAssignment(trail, propagation, *(toDelete.begin() + i));
    }

    ASSERT_TRUE(std::any_of(toDelete.begin(),
                            toDelete.end(),
                            [&propagation, &trail](Clause const* clause) {
                                return propagation.isAssignmentReason(*clause, trail);
                            }))
        << "Bad test data: did not create reason clauses in toDelete";
    checkedReduceClauseDB(clauseDB, propagation, trail, toDelete, problemClauses, learntClauses);
}
}
