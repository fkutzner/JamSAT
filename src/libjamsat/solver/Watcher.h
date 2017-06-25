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

#include "Clause.h"
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/ArrayMap.h>
#include <libjamsat/utils/Assert.h>

namespace jamsat {
namespace detail_propagation {

// TODO: documentation, with \internal flag.

class Watcher {
public:
  Watcher(Clause &watchedClause, CNFLit otherWatchedLiteral, int index = 0)
      : m_clause(&watchedClause), m_otherWatchedLiteral(otherWatchedLiteral),
        m_index(index) {}

  Clause &getClause() noexcept { return *m_clause; }

  CNFLit getOtherWatchedLiteral() const noexcept {
    return m_otherWatchedLiteral;
  }

  int getIndex() const noexcept { return m_index; }

  void setOtherWatchedLiteral(CNFLit literal) noexcept {
    m_otherWatchedLiteral = literal;
  }

  bool operator==(const Watcher &rhs) const {
    return m_clause == rhs.m_clause &&
           m_otherWatchedLiteral == rhs.m_otherWatchedLiteral;
  }

  bool operator!=(const Watcher &rhs) const {
    return m_clause != rhs.m_clause ||
           m_otherWatchedLiteral != rhs.m_otherWatchedLiteral;
  }

private:
  Clause *m_clause;
  CNFLit m_otherWatchedLiteral;
  int m_index;
};

using WatcherList = std::vector<Watcher>;

class WatcherTraversal {
public:
  explicit WatcherTraversal(WatcherList *iteratee) noexcept
      : m_iteratee(iteratee), m_current(iteratee->begin()),
        m_toTraverse(iteratee->size()) {}

  void removeCurrent() noexcept {
    JAM_ASSERT(m_current != m_iteratee->end(),
               "Iterator is not pointing to a valid element");

    if (m_current != m_iteratee->end()) {
      *m_current = m_iteratee->back();
    }
    m_iteratee->pop_back();
    --m_toTraverse;
  }

  bool hasFinishedTraversal() const noexcept { return m_toTraverse == 0ull; }

  void finishedTraversal() noexcept {}

  WatcherTraversal &operator++() noexcept {
    JAM_ASSERT(m_toTraverse > 0ull,
               "Tried to traverse beyond the watcher list");
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

  Watcher &operator*() noexcept {
    JAM_ASSERT(m_current != m_iteratee->end(),
               "Iterator is not pointing to a valid element");
    return *m_current;
  }

  Watcher *operator->() noexcept {
    JAM_ASSERT(m_current != m_iteratee->end(),
               "Iterator is not pointing to a valid element");
    return &(*m_current);
  }

private:
  WatcherList *m_iteratee;
  WatcherList::iterator m_current;
  WatcherList::size_type m_toTraverse;
};

class Watchers {
public:
  explicit Watchers(CNFVar maxVar)
      : m_watchers(CNFLit{maxVar, CNFSign::POSITIVE}) {}

  WatcherTraversal getWatchers(CNFLit literal) noexcept {
    JAM_ASSERT(literal.getRawValue() <
                   static_cast<CNFLit::RawLiteralType>(m_watchers.size()),
               "literal out of bounds");

    return WatcherTraversal{&m_watchers[literal]};
  }

  void addWatcher(CNFLit literal, Watcher watcher) noexcept {
    JAM_ASSERT(literal.getRawValue() <
                   static_cast<CNFLit::RawLiteralType>(m_watchers.size()),
               "literal out of bounds");
    m_watchers[literal].push_back(watcher);
  }

private:
  ArrayMap<CNFLit, WatcherList> m_watchers;
};
}
}
