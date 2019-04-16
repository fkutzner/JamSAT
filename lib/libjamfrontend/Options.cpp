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

#include "Options.h"

#include <chrono>
#include <stdexcept>
#include <string>

namespace jamsat {

namespace {

/**
 * \brief Parses the timeout value part of a timeout argument.
 *
 * \param[in] timeoutValue  The timeout value, representing a nonnegative number of seconds
 *
 * \returns The timeout value, respresented as a std::chrono::seconds object
 *
 * \throws invalid_argument when `timeoutValue` does not represent an integral value,
 *                          `timeoutValue` represents a negative value or `timeoutValue`
 *                          can not be represented as a `long` value.
 */
auto parseTimeoutArgument(std::string const& timeoutValue) -> std::chrono::seconds {
    // checking the sign early: MSVC17 doesn't seem to throw an out_of_range exception
    // for negative stoul inputs
    if (!timeoutValue.empty() && timeoutValue[0] == '-') {
        throw std::invalid_argument{"Error: negative timeout value"};
    }

    std::chrono::seconds result;
    try {
        result = std::chrono::seconds{std::stoul(timeoutValue)};
    } catch (std::invalid_argument&) {
        throw std::invalid_argument{"Error: invalid timeout value"};
    } catch (std::out_of_range&) {
        throw std::invalid_argument{"Error: timeout value out of range"};
    }

    return result;
}

/**
 * \brief Parses a single JamSAT argument
 *
 * \param[in] argument      The argument to be parsed
 * \param[in,out] result    The JamSATOptions object where the result shall be stored
 *
 * \returns true iff the argument has been successfully parsed and registered in `result`.
 *
 * \throws invalid_argument when `argument` is not a valid JamSAT argument.
 *
 * Strong exception safety guarantee: when an exception is thrown, `result` has not been
 * modified by this function.
 */
auto parseArgument(std::string const& argument, JamSATOptions& result) -> bool {
    std::string const timeoutArgPrefix = "--timeout=";
    if (argument == "--version") {
        result.m_printVersion = true;
        return true;
    }
	if (argument == "--help") {
        result.m_printHelp = true;
        return true;
    }
	if (argument == "--wait") {
        result.m_waitForUserInput = true;
        return true;
    }
	if (argument.compare(0, timeoutArgPrefix.size(), timeoutArgPrefix) == 0) {
        std::string timeoutValue{argument.begin() + timeoutArgPrefix.size(), argument.end()};
        result.m_timeout = parseTimeoutArgument(timeoutValue);
        return true;
    }
	if (argument.compare(0, 2, "--") == 0) {
        // Not a frontend option ~> pass it to the backend
        result.m_backendOptions.push_back(argument);
        return true;
    }
    return false;
}
}

auto parseOptions(int argc, char const* const* argv) -> JamSATOptions {
    if (argc < 2) {
        throw std::invalid_argument{"<FILE> argument missing"};
    }

    JamSATOptions result;
    bool lastArgIsFilename = false;

    for (int i = 1; i < argc; ++i) {
        std::string argument{argv[i]};
        bool parseSucceeded = parseArgument(argument, result);
        if (!parseSucceeded) {
            if (i != argc - 1) {
                throw std::invalid_argument{std::string{"Error: unknown argument "} + argument};
            }

            lastArgIsFilename = true;
        }
    }

    // Only --help and --version may be specified without a file argument:
    if (!result.m_printHelp && !result.m_printVersion && !lastArgIsFilename) {
        throw std::invalid_argument{"<FILE> argument missing"};
    }

    if (lastArgIsFilename) {
        result.m_problemFilename = argv[argc - 1];
    }

    return result;
}

namespace {
void printIndentedLine(std::ostream& output, unsigned int indent, std::string const& line) {
    std::string indentString;
    indentString.resize(indent, ' ');
    output << indentString << line << "\n";
}
}

void printOptions(std::ostream& output, unsigned int indent) noexcept {
    printIndentedLine(output, indent, "Options:");
    printIndentedLine(
        output, indent, " --version              Print the version of JamSAT and exit.");
    printIndentedLine(output, indent, " --timeout=N            Stop solving after N seconds.");
    printIndentedLine(
        output,
        indent,
        "                        N must be a nonnegative integer not greater than 2^32-1.");
    printIndentedLine(output, indent, " --help                 Print usage information and exit.");
}
}
