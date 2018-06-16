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

#pragma once

#include <cstdint>
#include <memory>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/Casts.h>
#include <libjamsat/utils/OverApproximatingSet.h>

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
private:
    using flag_type = uint16_t;

public:
    using value_type = CNFLit;
    using size_type = uint32_t;
    using iterator = CNFLit*;
    using const_iterator = const CNFLit*;
    using lbd_type = uint16_t;

    enum class Flag : flag_type { SCHEDULED_FOR_DELETION = 1, REDUNDANT = 2 };

    /**
     * \brief Returns a reference to a literal within the clause.
     *
     * \param index The index of the target literal. \p index must be smaller than
     * the clause size.
     * \returns A reference to the literal with index \p index.
     */
    CNFLit& operator[](size_type index) noexcept;
    const CNFLit& operator[](size_type index) const noexcept;

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
     * \param LBD  the clause's new LBD value. `LBD >= 1` must hold.
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
     * \brief Erases a literal from the clause.
     *
     * \param pos   The position of the literal to be erased.
     *
     * \return The iterator following the last removed element. If the last element
     *         has been erased, end() is returned.
     */
    iterator erase(const_iterator pos) noexcept;

    /**
     * \brief Erases literals from the clause.
     *
     * \param begin The position of the first to be erased.
     * \param end   The position of the first literal after \p begin not to be erased,
     *              or `end()` if no such position exists.
     *
     * \return The iterator following the last removed element. If the last element
     *         has been erased, end() is returned.
     */
    iterator erase(const_iterator begin, const_iterator end) noexcept;

    /**
     * \brief Assignment operator
     *
     * \param other     The clause to be copied. \p other must not be larger than this clause.
     * \returns Reference to this clause
     */
    Clause& operator=(const Clause& other) noexcept;

    Clause& operator=(Clause&& other) = delete;
    Clause(const Clause& other) = delete;
    Clause(Clause&& other) = delete;

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
    static Clause* constructIn(void* target, size_type size);

    /**
     * \brief Sets the given flag for the clause.
     *
     * \param flag      A value of Clause::Flags; the flag to be set.
     */
    void setFlag(Flag flag) noexcept;

    /**
     * \brief Clears the given flag for the clause.
     *
     * \param flag      A value of Clause::Flags; the flag to be cleared.
     */
    void clearFlag(Flag flag) noexcept;

    /**
     * \brief Checks whether the given flag is set.
     *
     * \param flag      A value of Clause::Flags; the flag to be set.
     * \return          true iff \p flag is set.
     */
    bool getFlag(Flag flag) const noexcept;

    /**
     * \brief Fast over-approximating check if the clause contains a given literal.
     *
     * \param lit   An arbitrary literal.
     *
     * \return      If `false` is returned, the clause definitely does not contain `lit`.
     *              If `true` is returned, the clause might contain `lit`.
     */
    bool mightContain(CNFLit lit) const noexcept;

    /**
     * \brief Fast over-approximating check if the clause is a subset of another clause.
     *
     * \param other   An arbitrary clause.
     *
     * \return        If `false` is returned, the clause is definitely not a subset of `other`.
     *                If `true` is returned, the clause might be a subset of `other`.
     */
    bool mightBeSubsetOf(Clause const& other) const noexcept;

    /**
     * \brief Notifies the clause that its contents have been updated.
     */
    void clauseUpdated() noexcept;

    friend std::unique_ptr<Clause> createHeapClause(size_type size);

    bool operator==(Clause const& rhs) const noexcept;
    bool operator!=(Clause const& rhs) const noexcept;

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
    flag_type m_flags;
    OverApproximatingSet<64, CNFLit::Index> m_approximatedClause;
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

std::ostream& operator<<(std::ostream& stream, const Clause& clause);

/********** Implementation ****************************** */

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
    JAM_ASSERT(LBD > 0, "LBD out of range");
    if (LBD <= std::numeric_limits<decltype(m_lbd)>::max()) {
        m_lbd = static_checked_cast<decltype(m_lbd)>(LBD);
    } else {
        m_lbd = std::numeric_limits<decltype(m_lbd)>::max();
    }
}

inline Clause::Clause(size_type size) noexcept
  : m_size(size), m_lbd(0), m_flags(0), m_anchor(CNFLit::getUndefinedLiteral()) {}

inline CNFLit& Clause::operator[](size_type index) noexcept {
    JAM_ASSERT(index < m_size, "Index out of bounds");
    return *(&m_anchor + index);
}

inline const CNFLit& Clause::operator[](size_type index) const noexcept {
    JAM_ASSERT(index < m_size, "Index out of bounds");
    return *(&m_anchor + index);
}

inline Clause::size_type Clause::size() const noexcept {
    return m_size;
}

inline void Clause::resize(size_type newSize) noexcept {
    JAM_ASSERT(newSize <= m_size, "newSize may not be larger than the current size");
    m_size = newSize;
}

inline Clause::iterator Clause::begin() noexcept {
    return &m_anchor;
}

inline Clause::iterator Clause::end() noexcept {
    return &m_anchor + m_size;
}

inline Clause::const_iterator Clause::begin() const noexcept {
    return &m_anchor;
}

inline Clause::const_iterator Clause::end() const noexcept {
    return &m_anchor + m_size;
}

inline Clause::iterator Clause::erase(const_iterator pos) noexcept {
    iterator replacement = begin() + (m_size - 1);
    iterator writablePos = begin() + (pos - begin());

    *writablePos = *replacement;
    --m_size;
    return writablePos;
}

inline Clause::iterator Clause::erase(const_iterator begin, const_iterator end) noexcept {
    iterator writableBegin = this->begin() + (begin - this->begin());
    iterator result = writableBegin;

    auto eraseDist = std::distance(begin, end);
    iterator replacement = this->begin() + (m_size - eraseDist);
    if (writableBegin + eraseDist > replacement) {
        replacement = writableBegin + eraseDist;
    }

    while (replacement != this->end()) {
        *writableBegin = *replacement;
        ++writableBegin;
        ++replacement;
    }

    m_size -= eraseDist;
    return result;
}

inline Clause& Clause::operator=(const Clause& other) noexcept {
    if (this == &other) {
        return *this;
    }

    JAM_ASSERT(this->m_size >= other.m_size,
               "Illegal argument: other clause must not be larger than the assignee");

    this->m_lbd = other.m_lbd;
    this->m_flags = other.m_flags;
    this->m_size = other.m_size;
    std::copy(other.begin(), other.end(), this->begin());
    return *this;
}

inline size_t Clause::getAllocationSize(Clause::size_type clauseSize) {
    JAM_ASSERT(clauseSize > 0, "clauseSize must be nonzero");
    size_t size = sizeof(Clause) + (clauseSize - 1) * sizeof(CNFLit);
    size_t overlapToAligned = size % alignof(Clause);
    if (overlapToAligned != 0) {
        size += alignof(Clause) - overlapToAligned;
    }
    return size;
}

inline Clause* Clause::constructIn(void* target, size_type size) {
    return new (target) Clause(size);
}

inline bool Clause::operator==(Clause const& rhs) const noexcept {
    if (this == &rhs) {
        return true;
    }
    if (size() != rhs.size() || this->m_lbd != rhs.m_lbd) {
        return false;
    }
    return std::equal(begin(), end(), rhs.begin());
}

inline bool Clause::operator!=(Clause const& rhs) const noexcept {
    return !(*this == rhs);
}


inline void Clause::setFlag(Flag flag) noexcept {
    m_flags |= static_cast<std::underlying_type_t<Flag>>(flag);
}

inline void Clause::clearFlag(Flag flag) noexcept {
    m_flags &= ~(static_cast<std::underlying_type_t<Flag>>(flag));
}

inline bool Clause::getFlag(Flag flag) const noexcept {
    return (m_flags & static_cast<std::underlying_type_t<Flag>>(flag)) != 0;
}

inline bool Clause::mightContain(CNFLit lit) const noexcept {
    return m_approximatedClause.mightContain(lit);
}

inline bool Clause::mightBeSubsetOf(Clause const& other) const noexcept {
    return m_approximatedClause.mightBeSubsetOf(other.m_approximatedClause);
}

inline void Clause::clauseUpdated() noexcept {
    m_approximatedClause.clear();
    for (auto lit : *this) {
        m_approximatedClause.insert(lit);
    }
}
}
