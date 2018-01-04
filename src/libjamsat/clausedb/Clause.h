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

#pragma once

#include <cstdint>
#include <memory>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/Casts.h>

namespace jamsat {
/**
 * \defgroup JamSAT_ClauseDB  JamSAT internal clause types and clause databases
 * This module contains data structures for representing and storing (solver-internal) CNF clauses.
 */

/**
 * \ingroup JamSAT_ClauseDB
 *
 * \class jamsat::Clause
 *
 * \brief The internal clause data structure
 */
class Clause {
public:
    using size_type = uint32_t;
    using iterator = CNFLit *;
    using const_iterator = const CNFLit *;
    using lbd_type = uint16_t;

    /**
     * \brief Returns a reference to a literal within the clause.
     *
     * \param index The index of the target literal. \p index must be smaller than
     * the clause size.
     * \returns A reference to the literal with index \p index.
     */
    CNFLit &operator[](size_type index) noexcept;
    const CNFLit &operator[](size_type index) const noexcept;

    /**
     * \brief Returns the clause's size.
     *
     * \returns The clause's size.
     */
    size_type size() const noexcept;

    /**
     * \brief Returns the clause's LBD value.
     *
     * Note: if the clause's LBD value is larger than the maximum value
     * of LBDType, the maximum value of LBDType is returned.
     *
     * \returns the clause's LBD value.
     */
    template <typename LBDType>
    auto getLBD() const noexcept -> LBDType;

    /**
     * \brief Sets the clause's LBD value.
     *
     * Note: if the \p LBD is larger than the maximum LBD value storable
     * by the clause, that maximum is stored instead.
     *
     * \param LBD  the clause's new LBD value.
     */
    template <typename LBDType>
    void setLBD(LBDType LBD) noexcept;

    /**
     * \brief Reduces the length of the clause to the given size.
     *
     * \param newSize  The clause's new size, which must not be larger than the
     * current size.
     */
    void resize(size_type newSize) noexcept;

    /**
     * \brief Gets the begin random-access iterator for the literal list.
     *
     * \returns The begin iterator for the literal list.
     */
    iterator begin() noexcept;

    /**
     * \brief Gets the end random-access iterator for the literal list.
     *
     * \returns The end iterator for the literal list. This iterator may not be
     * dereferenced.
     */
    iterator end() noexcept;

    /**
     * \brief Gets the begin random-access const iterator for the literal list.
     *
     * \returns The begin const iterator for the literal list.
     */
    const_iterator begin() const noexcept;

    /**
     * \brief Gets the end random-access const iterator for the literal list.
     *
     * \returns The end const iterator for the literal list. This iterator may not
     * be dereferenced.
     */
    const_iterator end() const noexcept;

    /**
     * \brief Computes the size of a non-empty Clause object.
     *
     * \param clauseSize The nonzero length of the clause.
     * \returns The size of a Clause object, in bytes.
     */
    static size_t getAllocationSize(size_type clauseSize);

    /**
     * \brief Constructs a clause in the memory region pointed to by \p target.
     *
     * \param target    The pointer to the first byte of a memory region M where
     * the clause shall be constructed. The size of M must be at least \p
     * Clause::getAllocationSize(size) bytes.
     * \param size      The non-zero size (in literals) of the clause to be
     * constructed.
     * \returns         The pointer to the constructed clause.
     */
    static Clause *constructIn(void *target, size_type size);

    friend std::unique_ptr<Clause> createHeapClause(size_type size);

private:
    /**
     * \brief Constructs a clause object of the given size.
     *
     * Note that objects are expected to be constructed within a preallocated
     * memory buffer of sufficient size.
     *
     * \param size  The clause's size.
     */
    explicit Clause(size_type size) noexcept;

    size_type m_size;
    lbd_type m_lbd;
    CNFLit m_anchor;
};

/**
 * \brief Allocates a clause of the given size on the heap.
 *
 * \param size The clause's size.
 * \returns A pointer to a new clause of size \p size, allocated on the heap.
 * Ownership of this object is transferred to the caller.
 */
std::unique_ptr<Clause> createHeapClause(Clause::size_type size);

std::ostream &operator<<(std::ostream &stream, const Clause &clause);

template <typename LBDType>
auto Clause::getLBD() const noexcept -> LBDType {
    if (m_lbd <= std::numeric_limits<LBDType>::max()) {
        return static_checked_cast<LBDType>(m_lbd);
    } else {
        return std::numeric_limits<LBDType>::max();
    }
}

template <typename LBDType>
void Clause::setLBD(LBDType LBD) noexcept {
    JAM_ASSERT(LBD >= 0, "LBD out of range");
    if (LBD <= std::numeric_limits<decltype(m_lbd)>::max()) {
        m_lbd = static_checked_cast<decltype(m_lbd)>(LBD);
    } else {
        m_lbd = std::numeric_limits<decltype(m_lbd)>::max();
    }
}
}
