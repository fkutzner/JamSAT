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
#include <boost/log/trivial.hpp>
#include <istream>
#include <sstream>

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

void CNFProblem::addClause(CNFClause &&clause) noexcept {
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

void CNFProblem::clear() noexcept {
  m_clauses.clear();
  m_maxVar = CNFVar::undefinedVariable;
}

std::ostream &operator<<(std::ostream &output, const CNFProblem &problem) {
  if (problem.isEmpty()) {
    output << "p cnf 0 0" << std::endl;
    return output;
  }

  output << "p cnf " << problem.getMaxVar() << " " << problem.getSize()
         << std::endl;
  for (auto &clause : problem.getClauses()) {
    output << clause << std::endl;
  }

  return output;
}

std::ostream &operator<<(std::ostream &output, const CNFClause &clause) {
  for (auto literal : clause) {
    output << literal << " ";
  }
  output << "0";

  return output;
}

struct DIMACSHeader {
  bool valid;
  unsigned int variableCount;
  unsigned int clauseCount;
};

namespace {

// Reads a DIMACS header (i.e. the line beginning with "p") from the given
// input stream, setting the stream's fail bit if a parsing failed (wrt. the
// DIMACS CNF format). The resulting DIMACSHeader's "valid" member is set to
// true iff the header could be parsed correctly.
DIMACSHeader readDIMACSHeader(std::istream &input) {
  std::string lineBuffer = "c";
  while (input && (lineBuffer.empty() || lineBuffer[0] != 'p')) {
    input >> std::ws;
    std::getline(input, lineBuffer);
  }

  if (!lineBuffer.empty() && lineBuffer[0] == 'p') {
    std::stringstream headerLine{lineBuffer};
    std::string cnfToken;
    DIMACSHeader result = {true, 0, 0};

    // Read "p"
    headerLine >> cnfToken;
    result.valid &= !headerLine.fail() && cnfToken == "p";

    // Read "cnf"
    headerLine >> cnfToken;
    result.valid &= !headerLine.fail() && cnfToken == "cnf";

    headerLine >> result.variableCount;
    result.valid &= !headerLine.fail();
    headerLine >> result.clauseCount;
    result.valid &= !headerLine.fail();

    return result;
  } else {
    BOOST_LOG_TRIVIAL(warning) << "Could not find the DIMACS header";
  }
  return {false, 0, 0};
}

// Reads as many clauses as specified in the DIMACS problem header from input
// and stores them into the given CNFProblem object. If the data read from the
// stream does not satisfy the DIMACS CNF format, the problem is cleared and the
// stream's fail bit is set.
std::istream &readDIMACSClauses(std::istream &input, DIMACSHeader problemHeader,
                                CNFProblem &problem) {
  for (unsigned int i = 1; i <= problemHeader.clauseCount; ++i) {
    CNFClause newClause;
    input >> newClause;
    if (input.fail()) {
      problem.clear();
      BOOST_LOG_TRIVIAL(warning) << "Failed parsing DIMACS clause no. " << i;
      return input;
    }
    problem.addClause(std::move(newClause));

    if (static_cast<unsigned int>(problem.getMaxVar().getRawValue()) + 1 >
        problemHeader.variableCount) {
      input.setstate(std::ios::failbit);
      problem.clear();
      BOOST_LOG_TRIVIAL(warning) << "Illegal variable " << i;
      BOOST_LOG_TRIVIAL(warning) << "Failed parsing DIMACS clause no. " << i;
      return input;
    }
  }

  return input;
}
}

std::istream &operator>>(std::istream &input, CNFProblem &problem) {
  const DIMACSHeader dimacsHeader = readDIMACSHeader(input);
  if (!dimacsHeader.valid) {
    input.setstate(std::ios::failbit);
    BOOST_LOG_TRIVIAL(warning) << "Unable to parse the DIMACS header";
    return input;
  }
  return readDIMACSClauses(input, dimacsHeader, problem);
}

std::istream &operator>>(std::istream &input, CNFClause &clause) {
  std::string buffer;
  auto originalClauseSize = clause.size();

  while (input) {
    input >> buffer;

    // Try to parse a literal.
    std::stringstream converter{buffer};
    int encodedLiteral;
    converter >> encodedLiteral;

    // If parsing a literal failed, a comment might begin with "c" or the input
    // stream is not DIMACS-formatted.
    if (converter.fail()) {
      if (buffer == "c") {
        input.clear(input.rdstate() & ~std::ios::failbit);
        std::getline(input, buffer);
        continue;
      }
      input.setstate(std::ios::failbit);
      BOOST_LOG_TRIVIAL(warning) << "Illegal token in clause: " << buffer;
      clause.resize(originalClauseSize);
      return input;
    }

    // A "0" token signifies the end of the clause.
    if (encodedLiteral == 0) {
      return input;
    }
    CNFSign literalSign =
        encodedLiteral > 0 ? CNFSign::POSITIVE : CNFSign::NEGATIVE;
    CNFVar literalVariable{std::abs(encodedLiteral) - 1};
    clause.push_back(CNFLit{literalVariable, literalSign});
  }

  // This can only be reached if the clause is not properly terminated.
  clause.resize(originalClauseSize);
  return input;
}
}
