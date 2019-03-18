/* Copyright (c) 2019 Felix Kutzner (github.com/fkutzner)

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

#include <libjamfrontend/Options.h>

#include <limits>
#include <stdexcept>
#include <string>

auto operator==(jamsat::JamSATOptions const& lhs, jamsat::JamSATOptions const& rhs) -> bool {
    if (&lhs == &rhs) {
        return true;
    }

    return lhs.m_printHelp == rhs.m_printHelp && lhs.m_printVersion == rhs.m_printVersion &&
           lhs.m_problemFilename == rhs.m_problemFilename && lhs.m_timeout == rhs.m_timeout &&
           lhs.m_waitForUserInput == rhs.m_waitForUserInput &&
           lhs.m_backendOptions == rhs.m_backendOptions;
}

auto operator!=(jamsat::JamSATOptions const& lhs, jamsat::JamSATOptions const& rhs) -> bool {
    return !(lhs == rhs);
}

TEST(UnitFrontendOptions, ParserReturnsThrowsInvalidArgumentWhenFileIsMissing) {
    char const* minOptions[1] = {nullptr};
    EXPECT_THROW(jamsat::parseOptions(0, minOptions), std::invalid_argument);

    char const* optionsWithBinaryName[2] = {"binaryName", nullptr};
    EXPECT_THROW(jamsat::parseOptions(1, optionsWithBinaryName), std::invalid_argument);
}

TEST(UnitFrontendOptions, ParserReturnsDefaultSettingsWhenOnlyFileIsSpecified) {
    std::string filename = "foo.cnf.gz";

    char const* rawOptions[3] = {"binaryName", filename.data(), nullptr};
    jamsat::JamSATOptions expectedOptions;
    expectedOptions.m_problemFilename = filename;
    EXPECT_TRUE(jamsat::parseOptions(2, rawOptions) == expectedOptions);
}

TEST(UnitFrontendOptions, ParserRecognizesUnknownDoubleDashArgumentsAsBackendOptions) {
    std::string filename = "foo.cnf.gz";

    char const* rawOptions[6] = {
        "binaryName", "--opt1", "--opt2", "--opt3", filename.data(), nullptr};
    jamsat::JamSATOptions expectedOptions;
    expectedOptions.m_problemFilename = filename;
    expectedOptions.m_backendOptions = {"--opt1", "--opt2", "--opt3"};
    EXPECT_TRUE(jamsat::parseOptions(5, rawOptions) == expectedOptions);
}

TEST(UnitFrontendOptions, ParserRefusesUnknownArgument) {
    std::string filename = "foo.cnf.gz";
    char const* rawOptions[4] = {"binaryName", "unknown-arg", filename.data(), nullptr};
    EXPECT_THROW(jamsat::parseOptions(3, rawOptions), std::invalid_argument);
}

TEST(UnitFrontendOptions, ParserDetectsHelpArg) {
    char const* rawOptions[3] = {"binaryName", "--help", nullptr};
    jamsat::JamSATOptions expectedOptions;
    expectedOptions.m_printHelp = true;
    EXPECT_TRUE(jamsat::parseOptions(2, rawOptions) == expectedOptions);
}

TEST(UnitFrontendOptions, ParserDetectsVersionArg) {
    char const* rawOptions[3] = {"binaryName", "--version", nullptr};
    jamsat::JamSATOptions expectedOptions;
    expectedOptions.m_printVersion = true;
    EXPECT_TRUE(jamsat::parseOptions(2, rawOptions) == expectedOptions);
}

TEST(UnitFrontendOptions, ParserDetectsWaitArg) {
    std::string filename = "foo.cnf.gz";
    char const* rawOptions[4] = {"binaryName", "--wait", filename.data(), nullptr};
    jamsat::JamSATOptions expectedOptions;
    expectedOptions.m_problemFilename = filename;
    expectedOptions.m_waitForUserInput = true;
    EXPECT_TRUE(jamsat::parseOptions(3, rawOptions) == expectedOptions);
}

TEST(UnitFrontendOptions, ParserRefusesWaitArgWithoutFilename) {
    char const* rawOptions[3] = {"binaryName", "--wait", nullptr};
    EXPECT_THROW(jamsat::parseOptions(2, rawOptions), std::invalid_argument);
}

TEST(UnitFrontendOptions, ParserDetectsTimeoutArg) {
    std::string filename = "foo.cnf.gz";
    char const* rawOptions[4] = {"binaryName", "--timeout=30", filename.data(), nullptr};
    jamsat::JamSATOptions expectedOptions;
    expectedOptions.m_problemFilename = filename;
    expectedOptions.m_timeout = std::chrono::seconds{30};
    EXPECT_TRUE(jamsat::parseOptions(3, rawOptions) == expectedOptions);
}

TEST(UnitFrontendOptions, ParserRefusesNegativeTimeout) {
    std::string filename = "foo.cnf.gz";
    char const* rawOptions[4] = {"binaryName", "--timeout=-30", filename.data(), nullptr};
    EXPECT_THROW(jamsat::parseOptions(3, rawOptions), std::invalid_argument);
}

TEST(UnitFrontendOptions, ParserRefusesNonIntegralTimeout) {
    std::string filename = "foo.cnf.gz";
    char const* rawOptions[4] = {"binaryName", "--timeout=foo", filename.data(), nullptr};
    EXPECT_THROW(jamsat::parseOptions(3, rawOptions), std::invalid_argument);
}

TEST(UnitFrontendOptions, ParserRefusesOutOfRangeTimeout) {
    std::string filename = "foo.cnf.gz";
    std::string longTimeout =
        std::string{"--timeout="} + std::to_string(std::numeric_limits<unsigned long>::max()) + "0";
    char const* rawOptions[4] = {"binaryName", longTimeout.data(), filename.data(), nullptr};
    EXPECT_THROW(jamsat::parseOptions(3, rawOptions), std::invalid_argument);
}

TEST(UnitFrontendOptions, ParserRefusesEmptyTimeout) {
    std::string filename = "foo.cnf.gz";
    char const* rawOptions[4] = {"binaryName", "--timeout=", filename.data(), nullptr};
    EXPECT_THROW(jamsat::parseOptions(3, rawOptions), std::invalid_argument);
}

TEST(UnitFrontendOptions, ParserRefusesTimeoutArgWithoutFileArg) {
    char const* rawOptions[3] = {"binaryName", "--timeout=", nullptr};
    EXPECT_THROW(jamsat::parseOptions(2, rawOptions), std::invalid_argument);
}

TEST(UnitFrontendOptions, ParserAcceptsCombinationOfAllArgsWithoutFn) {
    char const* rawOptions[7] = {
        "binaryName", "--timeout=1000", "--wait", "--version", "--help", "foo.cnf.gz", nullptr};
    jamsat::JamSATOptions expectedOptions;
    expectedOptions.m_problemFilename = "foo.cnf.gz";
    expectedOptions.m_timeout = std::chrono::seconds{1000};
    expectedOptions.m_printHelp = true;
    expectedOptions.m_printVersion = true;
    expectedOptions.m_waitForUserInput = true;

    auto actualOptions = jamsat::parseOptions(6, rawOptions);
    EXPECT_TRUE(actualOptions == expectedOptions);
}
