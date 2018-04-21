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

#include "Clause.h"
#include <libjamsat/utils/Assert.h>

#include <algorithm>

namespace jamsat {
Clause::Clause(size_type size) noexcept : m_size(size), m_anchor(CNFLit::getUndefinedLiteral()) {}

CNFLit &Clause::operator[](size_type index) noexcept {
    JAM_ASSERT(index < m_size, "Index out of bounds");
    return *(&m_anchor + index);
}

const CNFLit &Clause::operator[](size_type index) const noexcept {
    JAM_ASSERT(index < m_size, "Index out of bounds");
    return *(&m_anchor + index);
}

Clause::size_type Clause::size() const noexcept {
    return m_size;
}

void Clause::resize(size_type newSize) noexcept {
    JAM_ASSERT(newSize <= m_size, "newSize may not be larger than the current size");
    m_size = newSize;
}

Clause::iterator Clause::begin() noexcept {
    return &m_anchor;
}

Clause::iterator Clause::end() noexcept {
    return &m_anchor + m_size;
}

Clause::const_iterator Clause::begin() const noexcept {
    return &m_anchor;
}

Clause::const_iterator Clause::end() const noexcept {
    return &m_anchor + m_size;
}

Clause &Clause::operator=(const Clause &other) noexcept {
    if (this == &other) {
        return *this;
    }

    JAM_ASSERT(this->m_size >= other.m_size,
               "Illegal argument: other clause must not be larger than the assignee");

    this->m_lbd = other.m_lbd;
    this->m_size = other.m_size;
    std::copy(other.begin(), other.end(), this->begin());
    return *this;
}

size_t Clause::getAllocationSize(Clause::size_type clauseSize) {
    JAM_ASSERT(clauseSize > 0, "clauseSize must be nonzero");
    return sizeof(Clause) + (clauseSize - 1) * sizeof(CNFLit);
}

Clause *Clause::constructIn(void *target, size_type size) {
    return new (target) Clause(size);
}

std::unique_ptr<Clause> createHeapClause(Clause::size_type size) {
    Clause *result;
    if (size == 0) {
        result = new Clause(0);
    } else {
        auto memorySize = Clause::getAllocationSize(size);
        void *rawMemory = operator new(memorySize);
        result = Clause::constructIn(rawMemory, size);
    }

    for (auto &lit : *result) {
        lit = CNFLit::getUndefinedLiteral();
    }

    return std::unique_ptr<Clause>(result);
}

std::ostream &operator<<(std::ostream &stream, const Clause &clause) {
    stream << "( ";
    for (auto lit : clause) {
        stream << lit << " ";
    }
    stream << ")";
    return stream;
}

bool Clause::operator==(Clause const &rhs) const noexcept {
    if (this == &rhs) {
        return true;
    }
    if (size() != rhs.size() || this->m_lbd != rhs.m_lbd) {
        return false;
    }
    return std::equal(begin(), end(), rhs.begin());
}

bool Clause::operator!=(Clause const &rhs) const noexcept {
    return !(*this == rhs);
}
}
