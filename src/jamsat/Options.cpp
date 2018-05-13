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

#include <jamsat/Options.h>

namespace jamsat {
auto parseOptions(int argc, char **argv) -> JamSATOptions {
    if (argc < 2) {
        throw std::invalid_argument{"<FILE> argument missing"};
    }

    JamSATOptions result;

    std::string timeoutArgPrefix = "--timeout=";
    for (int i = 1; i < argc - 1; ++i) {
        std::string argument{argv[i]};
        if (argument == "--version") {
            result.m_printVersion = true;
        } else if (argument == "--help") {
            result.m_printHelp = true;
        } else if (argument.compare(0, timeoutArgPrefix.size(), timeoutArgPrefix) == 0) {
            std::string timeoutValue{argument.begin() + timeoutArgPrefix.size(), argument.end()};
            try {
                result.m_timeout = std::chrono::seconds{std::stoul(timeoutValue)};
            } catch (std::exception &) {
                throw std::invalid_argument{"Error: invalid timeout value"};
            }
        } else {
            // Not a frontend option ~> pass it to the backend
            result.m_backendOptions.push_back(argument);
        }
    }

    result.m_problemFilename = argv[argc - 1];
    return result;
}

namespace {
void printIndentedLine(std::ostream &output, unsigned int indent, std::string const &line) {
    std::string indentString;
    indentString.resize(indent, ' ');
    output << indentString << line << "\n";
}
}

void printOptions(std::ostream &output, unsigned int indent) noexcept {
    printIndentedLine(output, indent, "Options:");
    printIndentedLine(output, indent,
                      " --version              Print the version of JamSAT and exit.");
    printIndentedLine(output, indent, " --timeout=N            Stop solving after N seconds.");
    printIndentedLine(
        output, indent,
        "                        N must be a nonnegative integer not greater than 2^32-1.");
    printIndentedLine(output, indent, " --help                 Print usage information and exit.");
}
}
