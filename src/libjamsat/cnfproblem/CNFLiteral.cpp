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

#include "CNFLiteral.h"

#include <limits>

namespace jamsat {

CNFLit::CNFLit() noexcept : m_value(std::numeric_limits<int>::max()) {}

CNFVar::CNFVar() noexcept : m_value(std::numeric_limits<int>::max() >> 1) {}

const CNFLit CNFLit::undefinedLiteral = CNFLit{};
const CNFVar CNFVar::undefinedVariable = CNFVar{};
const CNFVar::RawVariable CNFVar::maxRawValue =
    CNFVar::undefinedVariable.getRawValue() - 1;

std::ostream &operator<<(std::ostream &stream, const CNFVar &variable) {
  stream << (variable.getRawValue() + 1);
  return stream;
}

std::ostream &operator<<(std::ostream &stream, const CNFLit &literal) {
  stream << (literal.getSign() == CNFSign::POSITIVE ? " " : "-");
  stream << literal.getVariable();
  return stream;
}
}
