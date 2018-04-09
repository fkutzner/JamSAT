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

#include <algorithm>
#include <boost/range.hpp>
#include <vector>

#include <putl/state_ptr.hpp>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/ClauseMarkers.h>
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/FlatteningIterator.h>

namespace jamsat {
namespace detail_propagation {

// TODO: documentation, with \internal flag.

template <class ClauseT>
class Watcher {
public:
    Watcher(ClauseT &watchedClause, CNFLit otherWatchedLiteral, unsigned int index = 0) noexcept
      : m_clause(&watchedClause, index), m_otherWatchedLiteral(otherWatchedLiteral) {}

    ClauseT &getClause() noexcept { return *m_clause; }

    const ClauseT &getClause() const noexcept { return *m_clause; }

    CNFLit getOtherWatchedLiteral() const noexcept { return m_otherWatchedLiteral; }

    unsigned int getIndex() const noexcept {
        // Only using 1 bit of state so far, so no masking is required yet
        return m_clause.get_state();
    }

    void setOtherWatchedLiteral(CNFLit literal) noexcept { m_otherWatchedLiteral = literal; }

    bool operator==(const Watcher &rhs) const {
        return m_clause == rhs.m_clause && m_otherWatchedLiteral == rhs.m_otherWatchedLiteral;
    }

    bool operator!=(const Watcher &rhs) const {
        return m_clause != rhs.m_clause || m_otherWatchedLiteral != rhs.m_otherWatchedLiteral;
    }

private:
    // m_clause is a state pointer with at least one bit of state.
    // Its least significant state bit contains the watcher's index
    // (i.e. the index of the literal it is supposed to watch within
    // the rsp. clause).
    putl::state_ptr<ClauseT, uint32_t, 1> m_clause;
    CNFLit m_otherWatchedLiteral;
};

template <class WatcherT>
class WatcherTraversal {
public:
    using WatcherList = std::vector<WatcherT>;

    explicit WatcherTraversal(WatcherList *iteratee) noexcept
      : m_iteratee(iteratee), m_current(iteratee->begin()), m_toTraverse(iteratee->size()) {}

    void removeCurrent() noexcept {
        JAM_ASSERT(m_current != m_iteratee->end(), "Iterator is not pointing to a valid element");

        if (m_current != m_iteratee->end()) {
            *m_current = m_iteratee->back();
        }
        m_iteratee->pop_back();
        --m_toTraverse;
    }

    bool hasFinishedTraversal() const noexcept { return m_toTraverse == 0ull; }

    void finishedTraversal() noexcept {}

    WatcherTraversal &operator++() noexcept {
        JAM_ASSERT(m_toTraverse > 0ull, "Tried to traverse beyond the watcher list");
        ++m_current;
        --m_toTraverse;
        return *this;
    }

    bool operator==(const WatcherTraversal &rhs) const noexcept {
        return m_current == rhs.m_current && m_iteratee == rhs.m_iteratee;
    }

    bool operator!=(const WatcherTraversal &rhs) const noexcept {
        return m_current != rhs.m_current || m_iteratee != rhs.m_iteratee;
    }

    WatcherT &operator*() noexcept {
        JAM_ASSERT(m_current != m_iteratee->end(), "Iterator is not pointing to a valid element");
        return *m_current;
    }

    WatcherT *operator->() noexcept {
        JAM_ASSERT(m_current != m_iteratee->end(), "Iterator is not pointing to a valid element");
        return &(*m_current);
    }

private:
    WatcherList *m_iteratee;
    typename WatcherList::iterator m_current;
    typename WatcherList::size_type m_toTraverse;
};

template <class ClauseT>
class Watchers {
private:
    using WatcherList = std::vector<Watcher<ClauseT>>;

public:
    using WatcherT = Watcher<ClauseT>;

    using WatcherIterator =
        FlatteningIterator<typename BoundedMap<CNFLit, WatcherList>::const_iterator>;
    using WatcherRange = boost::iterator_range<WatcherIterator>;

    explicit Watchers(CNFVar maxVar) : m_maxVar(maxVar), m_watchers(getMaxLit(maxVar)) {}

    WatcherTraversal<WatcherT> getWatchers(CNFLit literal) noexcept {
        JAM_ASSERT(literal.getRawValue() < static_cast<CNFLit::RawLiteral>(m_watchers.size()),
                   "literal out of bounds");

        return WatcherTraversal<WatcherT>{&m_watchers[literal]};
    }

    void addWatcher(CNFLit literal, Watcher<ClauseT> watcher) {
        JAM_ASSERT(literal.getRawValue() < static_cast<CNFLit::RawLiteral>(m_watchers.size()),
                   "literal out of bounds");
        m_watchers[literal].push_back(watcher);
    }

    void clear() noexcept {
        for (CNFLit::RawLiteral i = 0; i <= m_maxVar.getRawValue(); ++i) {
            m_watchers[CNFLit{CNFVar{i}, CNFSign::NEGATIVE}].clear();
            m_watchers[CNFLit{CNFVar{i}, CNFSign::POSITIVE}].clear();
        }
    }

    WatcherRange getWatchersInTraversalOrder() const noexcept {
        auto watcherLists = m_watchers.values();
        return {WatcherIterator{watcherLists.begin(), watcherLists.end()}, WatcherIterator{}};
    }

    void eraseWatchersToBeDeleted() {
        for (CNFLit::RawLiteral i = 0; i <= m_maxVar.getRawValue(); ++i) {
            eraseWatchersToBeDeleted(m_watchers[CNFLit{CNFVar{i}, CNFSign::NEGATIVE}]);
            eraseWatchersToBeDeleted(m_watchers[CNFLit{CNFVar{i}, CNFSign::POSITIVE}]);
        }
    }

    void increaseMaxVarTo(CNFVar newMaxVar) {
        JAM_ASSERT(newMaxVar >= m_maxVar,
                   "Argument newMaxVar must not be smaller than the previous maximum variable");
        JAM_ASSERT(isRegular(newMaxVar), "Argument newMaxVar must be a regular variable.");
        m_watchers.increaseSizeTo(getMaxLit(newMaxVar));
        m_maxVar = newMaxVar;
    }

private:
    void eraseWatchersToBeDeleted(WatcherList &watcherList) {
        watcherList.erase(std::remove_if(watcherList.begin(), watcherList.end(),
                                         [](const Watcher<ClauseT> &w) {
                                             return isMarkedToBeDeleted(w.getClause());
                                         }),
                          watcherList.end());
    }

    CNFVar m_maxVar;
    BoundedMap<CNFLit, WatcherList> m_watchers;
};
}
}
