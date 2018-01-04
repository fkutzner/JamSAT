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
#include <iterator>

#include <boost/optional.hpp>

#include <libjamsat/utils/Assert.h>

namespace jamsat {

/**
 * \ingroup JamSAT_Utils
 *
 * \brief A const iterator type providing a flat view on nested STL containers.
 *
 * Let
 *
 *  - `C` and `D` be types satisfying the STL Container concept, with objects of `C`
 *    containing objects of type `D`,
 *  - `c` be an object of type `C`,
 *  - `i1` and `i2` be valid iterators of `c` with `i1 <= i2` (if `i1` and `i2` are random
 *    access iterators) or `i2 == c.end()`.
 *
 * this iterator traverses the objects contained in the objects in `[i1, i2)`. The nested
 * containers are traversed in the order given by `i1`.
 *
 * Restriction: Only a single level of container nesting is supported by this iterator type.
 *
 * Usage example: Use this iterator to iterate over all the integers contained in an
 *   `std::vector<std::vector<int>>` object.
 *
 * This type satisfies the STL's InputIterator concept. If both C::const_iterator and
 * D::const_iterator satisfy the STL's ForwardIterator concept, so does NestedConstIterator<C>.
 *
 * \tparam C    An STL container type whose objects contain STL containers.
 */
template <typename C>
class NestedConstIterator {
private:
    using OuterIt = typename C::const_iterator;
    using InnerContainer = typename std::iterator_traits<OuterIt>::value_type;
    using InnerIt = typename InnerContainer::const_iterator;

public:
    using value_type = typename std::iterator_traits<InnerIt>::value_type;
    using reference = typename std::iterator_traits<InnerIt>::reference;
    using pointer = typename std::iterator_traits<InnerIt>::pointer;
    using iterator_category = std::input_iterator_tag;
    using difference_type = int64_t;

    /**
     * \brief Constructs a new NestedConstIterator.
     *
     * \param begin     An iterator pointing to the first nested container to be traversed.
     * \param end       An iterator pointing past the last container to be traversed.
     */
    NestedConstIterator(typename C::const_iterator begin, typename C::const_iterator end);

    reference operator*() const;
    const value_type *operator->() const;

    void operator++();
    NestedConstIterator operator++(int dummy);

    bool operator==(const NestedConstIterator<C> &rhs) const noexcept;
    bool operator!=(const NestedConstIterator<C> &rhs) const noexcept;

    NestedConstIterator &operator=(const NestedConstIterator<C> &other) = default;
    NestedConstIterator &operator=(NestedConstIterator<C> &&other) = default;
    NestedConstIterator(const NestedConstIterator<C> &other) = default;
    NestedConstIterator(NestedConstIterator<C> &&other) = default;
    ~NestedConstIterator() = default;

private:
    // Class invariant A: m_outerIt is either dereferencable or equal to m_outerEndIt
    OuterIt m_outerIt;
    OuterIt m_outerEndIt;

    // Class invariant B: m_outerIt != m_outerEndIt <=> m_innerIt exists
    // Class invariant C: m_innerIt exists <=> m_innerEndIt exists
    // Class invariant D: if m_innerIt exists, *m_innerIt is dereferencable
    boost::optional<InnerIt> m_innerIt;
    boost::optional<InnerIt> m_innerEndIt;
};
}

namespace std {
template <typename C>
struct iterator_traits<jamsat::NestedConstIterator<C>> {
    using value_type = typename jamsat::NestedConstIterator<C>::value_type;
    using reference = typename jamsat::NestedConstIterator<C>::reference;
    using pointer = typename jamsat::NestedConstIterator<C>::pointer;
    using iterator_category = typename jamsat::NestedConstIterator<C>::iterator_category;
    using difference_type = typename jamsat::NestedConstIterator<C>::difference_type;
};
}

/********** Implementation ****************************** */

namespace jamsat {

template <typename C>
NestedConstIterator<C>::NestedConstIterator(typename C::const_iterator begin,
                                            typename C::const_iterator end)
  : m_outerIt(begin), m_outerEndIt(end), m_innerIt(), m_innerEndIt() {
    while (m_outerIt != m_outerEndIt && m_outerIt->empty()) {
        ++m_outerIt;
    }

    if (m_outerIt != m_outerEndIt) {
        m_innerIt = boost::optional<InnerIt>{m_outerIt->begin()};
        m_innerEndIt = boost::optional<InnerIt>{m_outerIt->end()};
    }
}

template <typename C>
typename NestedConstIterator<C>::reference NestedConstIterator<C>::operator*() const {
    JAM_ASSERT(m_innerIt, "Nested iterator pointing past the end dereferenced");
    return **m_innerIt;
}

template <typename C>
const typename NestedConstIterator<C>::value_type *NestedConstIterator<C>::operator->() const {
    return &(this->operator*());
}

template <typename C>
void NestedConstIterator<C>::operator++() {
    if (!m_innerIt) {
        // Due to class invariant B, there are no further elements to traverse
        return;
    }

    // Try to advance to the next element of the nested container:
    if (*m_innerIt != *m_innerEndIt) {
        auto newInnerIt = *m_innerIt;
        ++newInnerIt;
        if (newInnerIt != *m_innerEndIt) {
            m_innerIt = boost::optional<InnerIt>{newInnerIt};
            return;
        }
    }

    // Try to advance to the first element of the next nonempty nested container:
    // m_outerIt can be advanced at least once due to class invariant B
    do {
        ++m_outerIt;
    } while (m_outerIt != m_outerEndIt && m_outerIt->empty());

    if (m_outerIt != m_outerEndIt) {
        m_innerIt = boost::optional<InnerIt>{m_outerIt->begin()};
        m_innerEndIt = boost::optional<InnerIt>{m_outerIt->end()};
        return;
    }

    // No more elements to traverse
    m_innerIt = boost::optional<InnerIt>{};
    m_innerEndIt = boost::optional<InnerIt>{};
}

template <typename C>
typename NestedConstIterator<C>::NestedConstIterator NestedConstIterator<C>::operator++(int dummy) {
    (void)dummy;
    NestedConstIterator<C> copy = *this;
    this->operator++();
    return copy;
}

template <typename C>
bool NestedConstIterator<C>::operator==(const NestedConstIterator<C> &rhs) const noexcept {
    if (this == &rhs) {
        return true;
    }

    // Optimize for comparing to an end iterator that is not equal to *this:
    if (this->m_innerIt && !rhs.m_innerIt) {
        return false;
    }

    return (this->m_outerIt == rhs.m_outerIt && this->m_innerIt == rhs.m_innerIt &&
            this->m_outerEndIt == rhs.m_outerEndIt && this->m_innerEndIt == rhs.m_innerEndIt);
}

template <typename C>
bool NestedConstIterator<C>::operator!=(const NestedConstIterator<C> &rhs) const noexcept {
    return !this->operator==(rhs);
}
}
