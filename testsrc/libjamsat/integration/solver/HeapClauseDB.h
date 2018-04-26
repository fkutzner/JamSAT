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

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Casts.h>

namespace jamsat {

/**
 * \ingroup JamSAT_ClauseDB
 *
 * \class jamsat::HeapClauseDB
 *
 * \brief A simple clause database, allocating clauses individually on the heap.
 *
 * Usage example: Use HeapClauseDB to keep track of allocated clauses in a CDCL
 * implementation, adding learnt clauses to the database and regularly removing
 * batches of learnt clauses which are deemed superfluous.
 *
 * Note that this implementation is not cache-efficient and only serves as a
 * baseline implementation.
 *
 * \tparam ClauseT    The clause type. If ClauseT is constructible, it must have
 * a constructor with the clause size (in literals) as its single numeric
 * parameter. If ClauseT is not constructible, ClauseT must have a static method
 * getAllocationSize(ClauseT::size_type n) where ClauseT::size_type is a numeric
 * type and n denotes the clause size (in literals), returning the amount of
 * bytes needed to store a clause of the given size. In the latter case, ClauseT
 * must also have a static method with the signature "static ClauseT
 * *constructIn(void *, size_type)" constructing a clause of the given size (in
 * literals, second parameter) in the memory region given by the first
 * parameter. ClauseT must have non-const begin() and end() methods returning
 * iterators to the range of literals stored in the clause.
 */
template <class ClauseT>
class HeapClauseDB {
private:
    using ClauseStore = std::vector<std::unique_ptr<ClauseT>>;

public:
    using size_type = typename ClauseStore::size_type;

    /**
     * \brief Constructs an empty HeapClauseDB instance.
     */
    HeapClauseDB();

    /**
     * \brief Creates a new clause in the clause database.
     *
     * \param literals    The nonempty range of literals to be contained in the
     * new clause.
     * \returns           Reference to the new clause, which contains exactly the
     * literals given in \p literals .
     */
    template <typename ForwardRange>
    ClauseT &insertClause(const ForwardRange &literals);

    /**
     * \brief Creates a new, undeletable clause in the clause database.
     *
     * Clauses created using this method may not be passed to \p destroy() .
     *
     * \param literals    The nonempty range of literals to be contained in the
     * new clause.
     * \returns           Reference to the new clause, which contains exactly the
     * literals given in \p literals .
     */
    template <typename ForwardRange>
    ClauseT &insertUndestroyableClause(const ForwardRange &literals);

    /**
     * \brief Lazily destroys a clause.
     *
     * The given clause is removed from the clause database and is then destroyed.
     * The clause destruction is lazy in the sense that the clause may be
     * postponed until the completion of the next \p purgeDestroyedClause() call.
     *
     * \param clause      The clause to be destroyed. \p clause must not have been
     * added to the database via \p insertUndestroyableClause() .
     */
    void destroy(ClauseT &clause);

    /**
     * \brief Determines whether the given clause is marked for (lazy)
     * destruction.
     *
     * \param clause      A clause.
     * \returns           \p true iff \p clause is currently marked for (lazy)
     * destruction.
     */
    bool isDestroyed(ClauseT &clause) const noexcept;

    /**
     * \brief Forces all remaining clauses marked for (lazy) destruction to be
     * actually destroyed.
     */
    void purgeDestroyedClauses();

    /**
     * \brief Checks if the given clause is contained in the database.
     *
     * Note: This method may be implemented inefficiently and is part of the
     * interface for testing purposes and for checking assertions.
     */
    bool contains(ClauseT &clause) const noexcept;

    /**
     * \brief Returns the size of the clause database, in clauses.
     *
     * \returns           The current size of the clause database, in clauses.
     */
    size_type size() const noexcept;

private:
    ClauseStore m_clauses;
    std::unordered_set<ClauseT *> m_deleted;
};

/********** Implementation ****************************** */

namespace clausedb_detail {
template <class ClauseT>
typename std::enable_if<std::is_constructible<ClauseT>::value, std::unique_ptr<ClauseT>>::type
heapAllocateClause(typename ClauseT::size_type size) {
    return std::unique_ptr<ClauseT>(new ClauseT{size});
}

template <class ClauseT>
typename std::enable_if<!std::is_constructible<ClauseT>::value, std::unique_ptr<ClauseT>>::type
heapAllocateClause(typename ClauseT::size_type size) {
    void *rawMemory = operator new(ClauseT::getAllocationSize(size));
    return std::unique_ptr<ClauseT>(ClauseT::constructIn(rawMemory, size));
}
}

template <class ClauseT>
HeapClauseDB<ClauseT>::HeapClauseDB() : m_clauses(), m_deleted() {}

template <class ClauseT>
template <typename ForwardRange>
ClauseT &HeapClauseDB<ClauseT>::insertClause(const ForwardRange &literals) {
    JAM_ASSERT(literals.begin() - literals.end() != 0,
               "The range of literals to be added must be nonempty");
    auto size = literals.end() - literals.begin();
    auto clauseSize = static_checked_cast<typename ClauseT::size_type>(size);
    m_clauses.push_back(clausedb_detail::heapAllocateClause<ClauseT>(clauseSize));
    boost::copy(literals, m_clauses.back()->begin());
    return *(m_clauses.back());
}

template <class ClauseT>
template <typename ForwardRange>
ClauseT &HeapClauseDB<ClauseT>::insertUndestroyableClause(const ForwardRange &literals) {
    return insertClause(literals);
}

template <class ClauseT>
void HeapClauseDB<ClauseT>::destroy(ClauseT &clause) {
    m_deleted.insert(&clause);
}

template <class ClauseT>
bool HeapClauseDB<ClauseT>::isDestroyed(ClauseT &clause) const noexcept {
    return m_deleted.find(&clause) != m_deleted.end();
}

template <class ClauseT>
void HeapClauseDB<ClauseT>::purgeDestroyedClauses() {
    auto isDeleted = [this](std::unique_ptr<ClauseT> &clause) {
        return (this->m_deleted.find(clause.get()) != this->m_deleted.end());
    };
    boost::remove_erase_if(m_clauses, isDeleted);
}

template <class ClauseT>
typename HeapClauseDB<ClauseT>::size_type HeapClauseDB<ClauseT>::size() const noexcept {
    return m_clauses.size();
}

template <class ClauseT>
bool HeapClauseDB<ClauseT>::contains(ClauseT &clause) const noexcept {
    auto containsClause = [&clause](const std::unique_ptr<ClauseT> &storedClause) {
        return storedClause.get() == &clause;
    };

    return (boost::find_if(m_clauses, containsClause) != m_clauses.end());
}
}
