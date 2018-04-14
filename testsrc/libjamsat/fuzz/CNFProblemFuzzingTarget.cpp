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

#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/utils/Assert.h>
#include <toolbox/fuzz/FuzzingEntryPoint.h>

#include <iostream>

// This file contains a fuzzing target for CNF problem parsing.

namespace jamsat {
void JamSATFuzzingEntryPoint(std::istream &fuzzerInput) {
    CNFProblem underTest;
    fuzzerInput >> underTest;

    if (fuzzerInput.fail()) {
        return;
    }

    for (auto &clause : underTest.getClauses()) {
        for (auto lit : clause) {
            JAM_ASSERT(lit.getVariable() <= underTest.getMaxVar(), "Invalid literal");
        }
    }
}
}
