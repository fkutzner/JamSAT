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
#include <boost/optional.hpp>
#include <istream>
#include <sstream>

#include <libjamsat/utils/Casts.h>

#if defined(JAM_ENABLE_LOGGING) && defined(JAM_ENABLE_CNFPROBLEM_LOGGING)
#include <boost/log/trivial.hpp>
#define JAM_LOG_CNFPROBLEM(x, y) BOOST_LOG_TRIVIAL(x) << "[cnfprb] " << y
#else
#define JAM_LOG_CNFPROBLEM(x, y)
#endif

namespace jamsat {

CNFProblem::CNFProblem() : m_clauses({}), m_maxVar(CNFVar::getUndefinedVariable()) {}

void CNFProblem::addClause(const CNFClause &clause) noexcept {
    for (auto literal : clause) {
        CNFVar variable = literal.getVariable();
        if (m_maxVar == CNFVar::getUndefinedVariable() ||
            variable.getRawValue() > m_maxVar.getRawValue()) {
            m_maxVar = variable;
        }
    }

    m_clauses.push_back(clause);
}

void CNFProblem::addClause(CNFClause &&clause) noexcept {
    for (auto literal : clause) {
        CNFVar variable = literal.getVariable();
        if (m_maxVar == CNFVar::getUndefinedVariable() ||
            variable.getRawValue() > m_maxVar.getRawValue()) {
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

bool CNFProblem::isEmpty() const noexcept {
    return m_clauses.empty();
}

CNFVar CNFProblem::getMaxVar() const noexcept {
    if (isEmpty()) {
        return CNFVar::getUndefinedVariable();
    }
    return m_maxVar;
}

void CNFProblem::clear() noexcept {
    m_clauses.clear();
    m_maxVar = CNFVar::getUndefinedVariable();
}

std::ostream &operator<<(std::ostream &output, const CNFProblem &problem) {
    if (problem.isEmpty()) {
        output << "p cnf 0 0" << std::endl;
        return output;
    }

    output << "p cnf " << problem.getMaxVar() << " " << problem.getSize() << std::endl;
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
        result.valid = result.valid && !headerLine.fail() && cnfToken == "p";

        // Read "cnf"
        headerLine >> cnfToken;
        result.valid = result.valid && !headerLine.fail() && cnfToken == "cnf";

        headerLine >> result.variableCount;
        result.valid = result.valid && !headerLine.fail();
        headerLine >> result.clauseCount;
        result.valid = result.valid && !headerLine.fail();

        result.valid = result.valid && (result.variableCount <= CNFVar::getMaxRawValue());

        return result;
    }

    JAM_LOG_CNFPROBLEM(warning, "Could not find the DIMACS header");
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
            JAM_LOG_CNFPROBLEM(warning, "Failed parsing DIMACS clause no. " << i);
            return input;
        }

        problem.addClause(std::move(newClause));

        if (static_checked_cast<unsigned int>(problem.getMaxVar().getRawValue()) + 1 >
            problemHeader.variableCount) {
            input.setstate(std::ios::failbit);
            problem.clear();
            JAM_LOG_CNFPROBLEM(warning, "Illegal variable " << i);
            JAM_LOG_CNFPROBLEM(warning, "Failed parsing DIMACS clause no. " << i);
            return input;
        }
    }

    return input;
}
} // namespace

std::istream &operator>>(std::istream &input, CNFProblem &problem) {
    const DIMACSHeader dimacsHeader = readDIMACSHeader(input);
    if (!dimacsHeader.valid) {
        input.setstate(std::ios::failbit);
        JAM_LOG_CNFPROBLEM(warning, "Unable to parse the DIMACS header");
        return input;
    }
    return readDIMACSClauses(input, dimacsHeader, problem);
}

namespace {
/**
 * \brief Decodes a DIMACS-encoded literal.
 *
 * \param encodedLiteral    The literal to be encoded.
 * \returns   an optional value containing the decoded literal iff decoding
 * encodedLiteral succeeded (ie. iff encodedLiteral is within the legal range of
 * literals).
 */
boost::optional<CNFLit> decodeCNFLit(int encodedLiteral) {
    if (encodedLiteral == std::numeric_limits<int>::min()) {
        JAM_LOG_CNFPROBLEM(warning, "Illegally large variable: " << encodedLiteral);
        return boost::optional<CNFLit>{};
    }

    CNFSign literalSign = encodedLiteral > 0 ? CNFSign::POSITIVE : CNFSign::NEGATIVE;
    auto rawVariable = static_checked_cast<CNFVar::RawVariable>(std::abs(encodedLiteral) - 1);

    if (rawVariable > CNFVar::getMaxRawValue()) {
        JAM_LOG_CNFPROBLEM(warning, "Illegally large variable: " << encodedLiteral);
        return boost::optional<CNFLit>{};
    }

    CNFVar literalVariable{rawVariable};
    return CNFLit{literalVariable, literalSign};
}

void parsingFailed(CNFClause &clause, std::istream &source) {
    source.setstate(std::ios::failbit);
    clause.clear();
}
} // namespace

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
            parsingFailed(clause, input);
            JAM_LOG_CNFPROBLEM(warning, "Illegal token in clause: " << buffer);
            return input;
        }

        // A "0" token signifies the end of the clause.
        if (encodedLiteral == 0) {
            return input;
        }

        boost::optional<CNFLit> decodedLiteral = decodeCNFLit(encodedLiteral);
        if (!decodedLiteral) {
            parsingFailed(clause, input);
            return input;
        }
        clause.push_back(*decodedLiteral);
    }

    // This can only be reached if the clause is not properly terminated.
    clause.resize(originalClauseSize);
    return input;
}
}
