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

#include "CNFProblem.h"
#include <istream>

namespace jamsat {
CNFProblem::CNFProblem() : m_clauses({}), m_maxVar(CNFVar::undefinedVariable) {}

void CNFProblem::addClause(const CNFClause &clause) noexcept {
  for (auto literal : clause) {
    CNFVar variable = literal.getVariable();
    if (isEmpty() || variable.getRawValue() > m_maxVar.getRawValue()) {
      m_maxVar = variable;
    }
  }

  m_clauses.push_back(clause);
}

const std::vector<CNFClause> &CNFProblem::getClauses() const noexcept {
  return m_clauses;
}

CNFProblem::size_type CNFProblem::getSize() const noexcept {
  return m_clauses.size();
}

bool CNFProblem::isEmpty() const noexcept { return m_clauses.empty(); }

CNFVar CNFProblem::getMaxVar() const noexcept {
  if (isEmpty()) {
    return CNFVar::undefinedVariable;
  }
  return m_maxVar;
}

std::ostream &operator<<(std::ostream &stream, const CNFProblem &problem) {
  if (problem.isEmpty()) {
    stream << "p cnf 0 0" << std::endl;
    return stream;
  }

  stream << "p cnf " << problem.getMaxVar() << " " << problem.getSize()
         << std::endl;
  for (auto &clause : problem.getClauses()) {
    for (auto literal : clause) {
      stream << literal << " ";
    }
    stream << "0" << std::endl;
  }

  return stream;
}

std::istream &operator>>(std::istream &stream, CNFProblem &problem) {
  (void)problem;
  return stream;
}
}
