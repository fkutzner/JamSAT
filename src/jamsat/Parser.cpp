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

#include <jamsat/Parser.h>
#include <libjamsat/api/ipasir/JamSatIpasir.h>

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <ostream>
#include <zlib.h>

namespace jamsat {

CNFParserError::CNFParserError(std::string const &what) : std::runtime_error(what) {}

CNFParserError::~CNFParserError() {}

namespace {
class GZFileResource {
public:
    explicit GZFileResource(char const *location) {
        m_file = (std::string{location} == "-") ? gzdopen(0, "rb") : gzopen(location, "rb");
        if (m_file == nullptr) {
            std::perror(location);
            throw CNFParserError{"Could not open input file."};
        }
    }

    ~GZFileResource() {
        if (m_file != nullptr) {
            gzclose(m_file);
        }
    }

    auto getFile() noexcept -> gzFile { return m_file; }

private:
    gzFile m_file;
};

struct DIMACSHeader {
    uint32_t m_numVariables;
    uint32_t m_numClauses;
};


// General TODO: when using gzgetc(), the compiler warns about disabled recursive
// macro expansion. Look into whether gzgetc is usable here instead of gzread.

/**
 * \brief Reads a character from a gzip-compressed file.
 *
 * \ingroup JamSAT_Frontend
 *
 * \param[in] file                  The file from which the character shall be read.
 *
 * \throws CNFParserError           An I/O or parsing error has occured while
 *                                  reading \p file, or EOF has been reached.
 */
auto readCharFromGzFile(gzFile file) -> char {
    char character = 0;
    int charsRead = gzread(file, &character, 1);
    if (charsRead <= 0) {
        if (gzeof(file)) {
            throw CNFParserError{"Syntax error: unexpected end of input file"};
        } else {
            int errnum = 0;
            char const *message = gzerror(file, &errnum);
            throw CNFParserError{message};
        }
    }
    return character;
}

/**
 * \brief Advances a gz-compressed file to the next line.
 *
 * \ingroup JamSAT_Frontend
 *
 * \param[in] file                  The file from which the line shall be read.
 *
 * \throws CNFParserError           An I/O or parsing error has occured while
 *                                  reading \p file, or EOF has been reached before
 *                                  a newline symbol has been read.
 */
void skipLine(gzFile file) {
    while (readCharFromGzFile(file) != '\n')
        ;
}

/**
 * \brief Slowly reads a newline-terminated line from a gz-compressed file.
 *
 * \ingroup JamSAT_Frontend
 *
 * \param[in] file                  The file from which the line shall be read.
 * \returns                         The line read from \p file.
 *
 * \throws CNFParserError           An I/O or parsing error has occured while
 *                                  reading \p file, or EOF has been reached.
 */
auto readLine(gzFile file) -> std::string {
    std::string result;
    result.reserve(512);
    char character;
    while ((character = readCharFromGzFile(file)) != '\n') {
        result += character;
    }
    return result;
}

/**
 * \brief Reads the DIMACS CNF header from a gz-compressed problem file.
 *
 * \ingroup JamSAT_Frontend
 *
 * \param[in] file                  The file from which the header shall be read.
 *                                  The file is advanced just beyond the header line.
 * \return problemHeader            The problem file's DIMACS header.
 *
 * \throws CNFParserError           An I/O or parsing error has occured while
 *                                  reading \p file.
 */
auto readHeader(gzFile file) -> DIMACSHeader {
    // Skip comment lines, i.e. those starting with 'c'
    char lineBegin = 'c';
    while ((lineBegin = readCharFromGzFile(file)) == 'c') {
        skipLine(file);
    }

    // The comment block should be immediately followed by the
    // problem description line, starting with 'p'
    if (lineBegin != 'p') {
        throw CNFParserError{"Syntax error: missing problem description line"};
    }

    // Expected: p cnf <NumVars> <NumClauses>
    std::string headerLine = readLine(file);
    std::stringstream headerStream{headerLine};

    std::string cnfToken;
    headerStream >> cnfToken;
    if (headerStream.fail() || cnfToken != "cnf") {
        throw CNFParserError{"Syntax error: malformed problem description line"};
    }

    uint32_t numVariables = 0;
    headerStream >> numVariables;
    if (headerStream.fail()) {
        throw CNFParserError{"Syntax error: malformed problem description line"};
    }

    uint32_t numClauses = 0;
    headerStream >> numClauses;
    if (headerStream.fail()) {
        throw CNFParserError{"Syntax error: malformed problem description line"};
    }

    return DIMACSHeader{numVariables, numClauses};
}

/**
 * \brief Reads data from a gz-compressed file without cutting off literals.
 *
 * \ingroup JamSAT_Frontend
 *
 * \param[in] file                  The file to read.
 * \param[in] preferredChunkSize    The desired amount of data to read.
 * \param[in] buffer                The target buffer.
 * \return                          The amount of bytes read.
 *
 * \throws CNFParserError           An I/O error has occured while reading \p file.
 *
 * readChunk reads \p preferredChunkSize characters from \p file into \p buffer
 * (with the first character written to the first position of \p buffer) and
 * returns the amount of bytes read. When returning, \p buffer contains the
 * characters read from \p file, terminated with a 0 character.
 *
 * If the end of \p file is reached, the amount of bytes actually read may be
 * smaller than \p preferredChunkSize.
 *
 * If the end of \p file has not been reached and the last character read from
 * \p file is not a whitespace character, more characters are read from \p file
 * and appended to \p buffer until a whitespace character has been read.
 */
auto readChunk(gzFile file, unsigned int preferredChunkSize, std::vector<char> &buffer) -> int {
    if (preferredChunkSize == 0) {
        buffer.push_back(0);
        return 0;
    }

    buffer.resize(preferredChunkSize);
    int bytesRead = gzread(file, buffer.data(), preferredChunkSize);

    if (bytesRead >= 0) {
        buffer.resize(static_cast<std::vector<char>::size_type>(bytesRead));
    }

    if (gzeof(file) && bytesRead >= 0) {
        buffer.push_back(0);
        return bytesRead;
    }

    if (bytesRead <= 0) {
        int errnum = 0;
        char const *message = gzerror(file, &errnum);
        throw CNFParserError{message};
    }

    if (std::isspace(buffer.back()) == 0) {
        // Stopped reading in the middle of a literal ~> read the rest of that literal, too
        bool consumedWhitespace = false;

        while (!consumedWhitespace) {
            char character = 0;
            int charsRead = gzread(file, &character, 1);

            if (charsRead <= 0) {
                if (gzeof(file)) {
                    buffer.push_back(0);
                    return buffer.size() - 1;
                } else {
                    int errnum = 0;
                    char const *message = gzerror(file, &errnum);
                    throw CNFParserError{message};
                }
            }

            buffer.push_back(character);
            consumedWhitespace = (std::isspace(character) != 0);
        }
    }

    buffer.push_back(0);
    return buffer.size() - 1;
}

/**
 * \brief Reads all CNF clauses from a gz-compressed file, adding the
 * clauses to an IPASIR solver.
 *
 * \ingroup JamSAT_Frontend
 *
 * \param[in] solver                The IPASIR solver receiving the clauses.
 * \param[in] file                  The file from which the clauses shall be read.
 *                                  \p file must only contain whitespace and
 *                                  string-representation of integers.
 * \param[in] problemHeader         The problem file's DIMACS header.
 *
 * \throws CNFParserError           An I/O or parsing error has occured while
 *                                  reading \p file.
 */
void readClauses(void *solver, gzFile file, DIMACSHeader problemHeader) {
    std::vector<char> buffer;

    uint32_t effectiveClauses = 0;

    int bytesRead = 1;
    while ((bytesRead = readChunk(file, 1048576, buffer)) != 0) {
        char *cursor = buffer.data();
        char *endCursor = cursor;
        char *end = buffer.data() + buffer.size();

        while (cursor != end) {
            long literal = strtol(cursor, &endCursor, 10);

            if (errno == ERANGE && (literal == std::numeric_limits<long>::min() ||
                                    literal == std::numeric_limits<long>::max())) {
                throw CNFParserError{"Literal out of range"};
            }

            // Don't check for ERANGE error, instead directly check if
            // the literal fits in the range of int:
            if (literal < std::numeric_limits<int>::min() ||
                literal > std::numeric_limits<int>::max()) {
                throw CNFParserError{"Literal out of range"};
            }

            if (cursor == endCursor) {
                // No conversion could be perfomed. Possible reasons: the string
                // is blank or contains something that cannot be parsed as a long.
                for (char *c = cursor; cursor < end; ++cursor) {
                    if (std::isspace(*c) == 0) {
                        throw CNFParserError{"Syntax error: invalid character with code " +
                                             std::to_string(*c)};
                    }
                }

                // Read next chunk.
                break;
            }

            if (literal == 0) {
                ++effectiveClauses;
            }

            ipasir_add(solver, static_cast<int>(literal));
            cursor = endCursor;
        }
    }

    if (effectiveClauses != problemHeader.m_numClauses) {
        std::string moreFewer = (effectiveClauses < problemHeader.m_numClauses) ? "fewer" : "more";
        throw CNFParserError{"Error: input file contains " + moreFewer +
                             " clauses than specified in the DIMACS header"};
    }
}
}

void readProblem(void *solver, std::string const &location, std::ostream &msgStream) {
    GZFileResource fileRAII{location.c_str()};
    gzFile file = fileRAII.getFile();
    DIMACSHeader problemHeader = readHeader(file);
    msgStream << "Reading a problem with " << problemHeader.m_numClauses << " clauses and "
              << problemHeader.m_numVariables << " variables\n";
    readClauses(solver, file, problemHeader);
}
}
