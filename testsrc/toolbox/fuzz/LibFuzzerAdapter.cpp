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

#include "FuzzingEntryPoint.h"
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <cstring>

using FuzzStream = boost::iostreams::stream<boost::iostreams::array_source>;

/**
 * \ingroup JamSAT_TestInfrastructure
 *
 * \brief An entry point for fuzzing with LLVM's LibFuzzer.
 *
 * See http://llvm.org/docs/LibFuzzer.html
 *
 * This adapter passes the fuzzer-generated input to jamsat::JamSATFuzzingEntryPoint.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // TODO: make this nicer by getting rid of the copy and the reinterpret_cast.
    auto *workingCopy = new uint8_t[size];
    std::memcpy(workingCopy, data, size);
    char *streamBase = reinterpret_cast<char *>(workingCopy);

    FuzzStream fuzzerInput{streamBase, size};
    jamsat::JamSATFuzzingEntryPoint(fuzzerInput);
    delete[] workingCopy;
    return 0;
}
