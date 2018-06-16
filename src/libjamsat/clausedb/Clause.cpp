/* Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)

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

#include "Clause.h"
#include <libjamsat/utils/Assert.h>

#include <algorithm>

namespace jamsat {
std::ostream& operator<<(std::ostream& stream, const Clause& clause) {
    stream << "( ";
    for (auto lit : clause) {
        stream << lit << " ";
    }
    stream << ")";
    return stream;
}

std::unique_ptr<Clause> createHeapClause(Clause::size_type size) {
    Clause* result;
    if (size == 0) {
        result = new Clause(0);
    } else {
        auto memorySize = Clause::getAllocationSize(size);
        void* rawMemory = operator new(memorySize);
        result = Clause::constructIn(rawMemory, size);
    }

    for (auto& lit : *result) {
        lit = CNFLit::getUndefinedLiteral();
    }

    return std::unique_ptr<Clause>(result);
}
}
