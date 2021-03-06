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

#include <boost/log/trivial.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <vector>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/solver/Assignment.h>
#include <libjamsat/utils/Casts.h>
#include <libjamsat/utils/ControlFlow.h>

/*
 * This file contains a simple implementation of a DPLL SAT solver, serving as
 * an integration test of the Trail, Clause and Propagation classes.
 */

namespace jamsat {

namespace {

class SimpleDPLL {
public:
  SimpleDPLL(const CNFProblem& problem)
    : m_assignment(problem.getMaxVar()), m_clauses(), m_maxVar(problem.getMaxVar())
  {

    std::vector<CNFLit> units;
    for (auto clause : problem.getClauses()) {
      JAM_ASSERT(!clause.empty(), "Can't add empty clauses");
      if (clause.size() == 1) {
        units.push_back(clause[0]);
      }
      else {
        addClause(clause);
      }
    }

    for (auto unit : units) {
      addUnitClause(unit);
    }

    // Decision level 0 is finished here
    m_assignment.newLevel();
  }

  TBool solve()
  {
    if (allVariablesAssigned()) {
      return TBools::TRUE;
    }

    CNFVar firstBranchingVariable = getBranchingVariable();
    JAM_ASSERT(firstBranchingVariable != CNFVar::getUndefinedVariable(),
               "Illegal branching variable");
    CNFLit firstBranchingLit{firstBranchingVariable, CNFSign::NEGATIVE};
    return toTBool(solve(firstBranchingLit) || solve(~firstBranchingLit));
  }

private:
  void addUnitClause(const CNFLit unitLit)
  {
    if (!isDeterminate(m_assignment.getAssignment(unitLit))) {
      m_assignment.append(unitLit);
    }
  }

  std::unique_ptr<Clause> createInternalClause(const CNFClause& from)
  {
    auto newClause = createHeapClause(static_checked_cast<Clause::size_type>(from.size()));
    boost::copy(from, newClause->begin());
    return newClause;
  }

  void addClause(const CNFClause& clause)
  {
    m_clauses.push_back(createInternalClause(clause));
    m_assignment.registerClause(*(m_clauses.back()));
  }

  CNFVar getBranchingVariable()
  {
    JAM_ASSERT(m_assignment.getCurrentLevel() > 0, "Can't branch on decision level 0");
    CNFVar result = CNFVar{static_cast<CNFVar::RawVariable>(m_assignment.getCurrentLevel() - 1)};

    // TODO: comparison operators on variables would apparently be really nice
    while (result.getRawValue() <= m_maxVar.getRawValue() &&
           isDeterminate(m_assignment.getAssignment(result))) {
      // TODO: an increment operator on variables would be nice, too.
      result = CNFVar{result.getRawValue() + 1};
    }

    if (result.getRawValue() > m_maxVar.getRawValue() ||
        isDeterminate(m_assignment.getAssignment(result))) {
      return CNFVar::getUndefinedVariable();
    }

    return result;
  }

  bool allVariablesAssigned() { return m_assignment.isComplete(); }

  bool solve(CNFLit branchingLit)
  {
    auto currentDecisionLevel = m_assignment.getCurrentLevel();
    OnExitScope autoBacktrack{
        [this, currentDecisionLevel]() { this->m_assignment.undoToLevel(currentDecisionLevel); }};

    m_assignment.newLevel();
    if (m_assignment.append(branchingLit) != nullptr) {
      // conflicting clause found -> current assignment falsifies the formula
      return false;
    }

    if (allVariablesAssigned()) {
      // all variables assignmend without conflicts -> current assignment
      // satisfies the formula
      return true;
    }

    CNFVar branchingVariable = getBranchingVariable();
    JAM_ASSERT(branchingVariable != CNFVar::getUndefinedVariable(), "Illegal branching variable");
    CNFLit nextBranchingLit{branchingVariable, CNFSign::NEGATIVE};


    return solve(nextBranchingLit) || solve(~nextBranchingLit);
  }

  Assignment m_assignment;
  std::vector<std::unique_ptr<Clause>> m_clauses;
  CNFVar m_maxVar;
};
}

TEST(IntegrationSolver, SimpleDPLL_satisfiableFormula)
{
  std::stringstream conduit;
  conduit << "p cnf 10 3" << std::endl;
  conduit << "1 0" << std::endl;
  conduit << "2 1 3 0" << std::endl;
  conduit << "7 8 9 0" << std::endl;

  ASSERT_FALSE(conduit.fail());

  CNFProblem testData;
  conduit >> testData;

  SimpleDPLL underTest{testData};
  EXPECT_EQ(underTest.solve(), TBools::TRUE);
}

TEST(IntegrationSolver, SimpleDPLL_unsatisfiableFormula)
{
  std::stringstream conduit;
  conduit << "p cnf 11 10" << std::endl;
  conduit << "1 0" << std::endl;
  conduit << "3 0" << std::endl;
  conduit << "7 8 9 0" << std::endl;
  conduit << "-1 8 0" << std::endl;
  conduit << "-7 -3 0" << std::endl;
  conduit << "9 8 -1 0" << std::endl;

  conduit << " -10 -11 0" << std::endl;
  conduit << "  10 -11 0" << std::endl;
  conduit << " -10  11 0" << std::endl;
  conduit << "  10  11 0" << std::endl;

  ASSERT_FALSE(conduit.fail());

  CNFProblem testData;
  conduit >> testData;

  SimpleDPLL underTest{testData};
  EXPECT_EQ(underTest.solve(), TBools::FALSE);
}

TEST(IntegrationSolver, SimpleDPLL_random5SAT_satisfiableFormula)
{
  std::stringstream conduit;

  conduit << "p cnf 7 30" << std::endl;
  conduit << "-3 5 -1 -6 -7 0" << std::endl;
  conduit << "2 -7 5 6 -3 0" << std::endl;
  conduit << "-1 5 6 4 -3 0" << std::endl;
  conduit << "-6 -1 7 -5 2 0" << std::endl;
  conduit << "7 -1 -6 4 5 0" << std::endl;
  conduit << "-7 3 -5 6 -2 0" << std::endl;
  conduit << "2 3 -6 -7 5 0" << std::endl;
  conduit << "1 2 -7 -6 5 0" << std::endl;
  conduit << "-3 6 -1 -7 4 0" << std::endl;
  conduit << "7 2 -4 5 -1 0" << std::endl;
  conduit << "-3 2 -4 -7 1 0" << std::endl;
  conduit << "1 -4 -2 -5 -7 0" << std::endl;
  conduit << "-3 -6 2 1 -4 0" << std::endl;
  conduit << "-2 -6 7 -5 -3 0" << std::endl;
  conduit << "1 -7 -2 -4 -3 0" << std::endl;
  conduit << "1 -6 7 5 2 0" << std::endl;
  conduit << "3 6 2 7 -4 0" << std::endl;
  conduit << "-4 -1 -3 5 7 0" << std::endl;
  conduit << "-5 3 4 -1 7 0" << std::endl;
  conduit << "4 1 -5 2 -6 0" << std::endl;
  conduit << "2 3 6 7 -1 0" << std::endl;
  conduit << "5 -1 -4 -2 7 0" << std::endl;
  conduit << "3 -7 5 6 -2 0" << std::endl;
  conduit << "-4 5 -1 2 6 0" << std::endl;
  conduit << "4 2 -1 -3 5 0" << std::endl;
  conduit << "-2 5 6 7 -4 0" << std::endl;
  conduit << "-2 -6 -1 -7 -5 0" << std::endl;
  conduit << "-1 -3 5 -2 6 0" << std::endl;
  conduit << "4 -5 -3 2 -6 0" << std::endl;
  conduit << "3 -1 2 -4 -7 0" << std::endl;

  ASSERT_FALSE(conduit.fail());

  CNFProblem testData;
  conduit >> testData;

  SimpleDPLL underTest{testData};
  EXPECT_EQ(underTest.solve(), TBools::TRUE);
}

TEST(IntegrationSolver, SimpleDPLL_random4SAT_satisfiableFormula)
{
  std::stringstream conduit;

  conduit << "p cnf 4 20" << std::endl;
  conduit << "-3 2 -1 4 0" << std::endl;
  conduit << "-3 2 1 4 0" << std::endl;
  conduit << "-2 1 -3 -4 0" << std::endl;
  conduit << "-1 -4 -2 3 0" << std::endl;
  conduit << "1 4 -2 -3 0" << std::endl;
  conduit << "-1 -3 -2 -4 0" << std::endl;
  conduit << "3 1 4 2 0" << std::endl;
  conduit << "-2 4 -1 -3 0" << std::endl;
  conduit << "4 -3 -1 -2 0" << std::endl;
  conduit << "2 -4 1 3 0" << std::endl;
  conduit << "2 -3 1 -4 0" << std::endl;
  conduit << "-1 3 2 4 0" << std::endl;
  conduit << "-4 3 -2 1 0" << std::endl;
  conduit << "-3 1 -2 -4 0" << std::endl;
  conduit << "1 -3 2 4 0" << std::endl;
  conduit << "-4 1 -2 3 0" << std::endl;
  conduit << "-4 1 -3 -2 0" << std::endl;
  conduit << "4 2 3 1 0" << std::endl;
  conduit << "-2 3 -1 4 0" << std::endl;
  conduit << "4 -1 -2 3 0" << std::endl;

  ASSERT_FALSE(conduit.fail());

  CNFProblem testData;
  conduit >> testData;

  SimpleDPLL underTest{testData};
  EXPECT_EQ(underTest.solve(), TBools::TRUE);
}
}
