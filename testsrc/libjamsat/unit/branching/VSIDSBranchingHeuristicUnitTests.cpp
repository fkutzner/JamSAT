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

#include <gtest/gtest.h>

#include <unordered_map>

#include <boost/log/trivial.hpp>

#include <libjamsat/branching/VSIDSBranchingHeuristic.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {
class FakeAssignmentProvider {
public:
    FakeAssignmentProvider(TBool defaultAssignment) : m_defaultAssignment(defaultAssignment) {}

    TBool getAssignment(CNFVar variable) const noexcept {
        auto result = m_assignments.find(variable);
        if (result == m_assignments.end()) {
            return m_defaultAssignment;
        }
        return result->second;
    }

    void setAssignment(CNFVar variable, TBool assignment) {
        m_assignments[variable] = assignment;
        m_phases[variable] = assignment;
    }

    void setPhase(CNFVar variable, TBool assignment) { m_phases[variable] = assignment; }

    TBool getPhase(CNFVar variable) const noexcept {
        auto result = m_phases.find(variable);
        if (result == m_phases.end()) {
            return TBools::FALSE;
        }
        return result->second;
    }

private:
    TBool m_defaultAssignment;
    std::unordered_map<CNFVar, TBool> m_assignments;
    std::unordered_map<CNFVar, TBool> m_phases;
};

TEST(UnitBranching, VSIDSBranchingHeuristic_allAssignedCausesUndefToBePicked) {
    CNFVar maxVar{10};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::TRUE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{maxVar, fakeAssignmentProvider};
    EXPECT_EQ(underTest.pickBranchLiteral(), CNFLit::getUndefinedLiteral());
}

TEST(UnitBranching, VSIDSBranchingHeuristic_singleVariableGetsPicked) {
    CNFVar maxVar{0};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::INDETERMINATE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{maxVar, fakeAssignmentProvider};

    underTest.setEligibleForDecisions(CNFVar{0}, true);

    CNFLit result = underTest.pickBranchLiteral();
    EXPECT_EQ(result.getVariable(), CNFVar{0});
}

TEST(UnitBranching, VSIDSBranchingHeuristic_variablesInitiallyHaveSameActivities) {
    CNFVar maxVar{10};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::INDETERMINATE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{maxVar, fakeAssignmentProvider};

    for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
        underTest.setEligibleForDecisions(CNFVar{i}, true);
    }

    underTest.seenInConflict(CNFVar{4});

    CNFLit result = underTest.pickBranchLiteral();
    EXPECT_NE(result, CNFLit::getUndefinedLiteral());
    EXPECT_EQ(result.getVariable(), CNFVar{4});
}

namespace {
template <typename Heuristic>
void expectVariableSequence(Heuristic &underTest, const std::vector<CNFVar> &expectedSequence) {
    for (auto var : expectedSequence) {
        CNFLit pick = underTest.pickBranchLiteral();
        EXPECT_NE(pick, CNFLit::getUndefinedLiteral());
        auto pickedVar = pick.getVariable();
        EXPECT_EQ(pickedVar, var);
    }
}

template <typename Heuristic>
void expectLiteralSequence(Heuristic &underTest, const std::vector<CNFLit> &expectedSequence) {
    for (auto lit : expectedSequence) {
        CNFLit pick = underTest.pickBranchLiteral();
        EXPECT_EQ(pick, lit);
    }
}

template <typename Heuristic>
void addDefaultConflictSequence(Heuristic &underTest) {
    for (CNFVar::RawVariable i = 0; i <= 10; ++i) {
        underTest.setEligibleForDecisions(CNFVar{i}, true);
    }

    underTest.seenInConflict(CNFVar{4});
    underTest.seenInConflict(CNFVar{5});
    underTest.seenInConflict(CNFVar{4});
    underTest.seenInConflict(CNFVar{5});
    underTest.seenInConflict(CNFVar{5});
    underTest.seenInConflict(CNFVar{3});
}
}

TEST(UnitBranching, VSIDSBranchingHeuristic_usingVariablesInConflictCausesReordering) {
    CNFVar maxVar{10};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::INDETERMINATE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{maxVar, fakeAssignmentProvider};

    addDefaultConflictSequence(underTest);

    expectVariableSequence(underTest, {CNFVar{5}, CNFVar{4}, CNFVar{3}});
}

TEST(UnitBranching, VSIDSBranchingHeuristic_ineligibleVariableDoesNotGetPicked) {
    CNFVar maxVar{10};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::INDETERMINATE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{maxVar, fakeAssignmentProvider};

    addDefaultConflictSequence(underTest);
    underTest.setEligibleForDecisions(CNFVar{5}, false);

    expectVariableSequence(underTest, {CNFVar{4}, CNFVar{3}});
}

TEST(UnitBranching, VSIDSBranchingHeuristic_assignedVariableDoesNotGetPicked) {
    CNFVar maxVar{10};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::INDETERMINATE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{maxVar, fakeAssignmentProvider};

    fakeAssignmentProvider.setAssignment(CNFVar{4}, TBools::TRUE);
    addDefaultConflictSequence(underTest);

    expectVariableSequence(underTest, {CNFVar{5}, CNFVar{3}});
}

TEST(UnitBranching, VSIDSBranchingHeuristic_variableActivityDecaysWhenTooLarge) {
    CNFVar maxVar{10};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::INDETERMINATE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{maxVar, fakeAssignmentProvider};

    addDefaultConflictSequence(underTest);

    underTest.setActivityBumpDelta(0.5e100);

    underTest.seenInConflict(CNFVar{4});

    // After this call, the activities should get scaled down:
    underTest.seenInConflict(CNFVar{4});

    underTest.seenInConflict(CNFVar{3});

    expectVariableSequence(underTest, {CNFVar{4}, CNFVar{3}, CNFVar{5}});
}

TEST(UnitBranching, VSIDSBranchingHeuristic_signsAreSelectedByPhase) {
    CNFVar maxVar{10};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::INDETERMINATE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{maxVar, fakeAssignmentProvider};
    addDefaultConflictSequence(underTest);

    fakeAssignmentProvider.setPhase(CNFVar{5}, TBools::TRUE);
    fakeAssignmentProvider.setPhase(CNFVar{4}, TBools::TRUE);
    fakeAssignmentProvider.setPhase(CNFVar{3}, TBools::FALSE);

    expectLiteralSequence(underTest, {CNFLit{CNFVar{5}, CNFSign::POSITIVE},
                                      CNFLit{CNFVar{4}, CNFSign::POSITIVE},
                                      CNFLit{CNFVar{3}, CNFSign::NEGATIVE}});
}

TEST(UnitBranching, VSIDSBranchingHeuristic_addedVariablesAreUsedForDecisions) {
    CNFVar initialMaxVar{5};
    FakeAssignmentProvider fakeAssignmentProvider{TBools::INDETERMINATE};
    VSIDSBranchingHeuristic<FakeAssignmentProvider> underTest{initialMaxVar,
                                                              fakeAssignmentProvider};

    for (CNFVar i = CNFVar{0}; i < CNFVar{5}; i = nextCNFVar(i)) {
        underTest.setEligibleForDecisions(i, true);
    }
    fakeAssignmentProvider.setPhase(CNFVar{0}, TBools::TRUE);
    fakeAssignmentProvider.setPhase(CNFVar{1}, TBools::TRUE);
    fakeAssignmentProvider.setPhase(CNFVar{2}, TBools::FALSE);
    fakeAssignmentProvider.setPhase(CNFVar{3}, TBools::FALSE);
    fakeAssignmentProvider.setPhase(CNFVar{4}, TBools::FALSE);
    fakeAssignmentProvider.setPhase(CNFVar{5}, TBools::FALSE);

    underTest.seenInConflict(CNFVar{2});
    underTest.seenInConflict(CNFVar{2});
    underTest.seenInConflict(CNFVar{2});
    underTest.seenInConflict(CNFVar{1});
    underTest.seenInConflict(CNFVar{1});
    underTest.seenInConflict(CNFVar{0});

    underTest.increaseMaxVarTo(CNFVar{8});
    fakeAssignmentProvider.setPhase(CNFVar{7}, TBools::TRUE);
    fakeAssignmentProvider.setPhase(CNFVar{8}, TBools::TRUE);
    underTest.setEligibleForDecisions(CNFVar{7}, true);
    underTest.setEligibleForDecisions(CNFVar{8}, true);
    for (int i = 0; i < 10; ++i) {
        underTest.seenInConflict(CNFVar{7});
    }
    for (int i = 0; i < 9; ++i) {
        underTest.seenInConflict(CNFVar{8});
    }

    expectLiteralSequence(
        underTest, {CNFLit{CNFVar{7}, CNFSign::POSITIVE}, CNFLit{CNFVar{8}, CNFSign::POSITIVE},
                    CNFLit{CNFVar{2}, CNFSign::NEGATIVE}, CNFLit{CNFVar{1}, CNFSign::POSITIVE},
                    CNFLit{CNFVar{0}, CNFSign::POSITIVE}});
}

// TODO: unit tests for decay and bumping
}
