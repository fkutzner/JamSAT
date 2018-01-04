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
 * \brief A const iterator type providing a flat view on a sequence of STL containers.
 *
 * Let
 *
 *  - `C` be a type satisfying the STL container concept,
 *  - `I` be a type satisfying the STP InputIterator concept, with the value type of `I` being `C`,
 *  - `i1` and `i2` be objects of type `I` such that `i1 = i2` can be achieved by incrementing `i1`
 *
 * this iterator traverses the objects contained in the objects in `[i1, i2)`. The nested
 * containers are traversed in the order given by `i1`.
 *
 * Restriction: Only a single level of container nesting is supported by this iterator type.
 *
 * Usage example: Use this iterator to iterate over all the integers contained in an
 *   `std::vector<std::vector<int>>` object.
 *
 * This type satisfies the STL's InputIterator concept. If both `I` and
 * `D::const_iterator` satisfy the STL's ForwardIterator concept, so does `FlatteningIterator<I>`.
 *
 * \tparam I    a type satisfying the STP InputIterator concept, with the value type of `I`
 *              satisfying the STL container concept.
 */
template <typename I>
class FlatteningIterator {
private:
    using OuterIt = I;
    using InnerContainer = typename std::iterator_traits<OuterIt>::value_type;
    using InnerIt = typename InnerContainer::const_iterator;

public:
    using value_type = typename std::iterator_traits<InnerIt>::value_type;
    using reference = typename std::iterator_traits<InnerIt>::reference;
    using pointer = typename std::iterator_traits<InnerIt>::pointer;
    using iterator_category = std::input_iterator_tag;
    using difference_type = int64_t;

    /**
     * \brief Constructs a new FlatteningIterator.
     *
     * \param begin     An iterator pointing to the first nested container to be traversed.
     * \param end       An iterator pointing past the last container to be traversed.
     */
    FlatteningIterator(I begin, I end);

    /**
     * \brief Constructs a past-the-end FlatteningIterator.
     */
    FlatteningIterator();

    reference operator*() const;
    const value_type *operator->() const;

    void operator++();
    FlatteningIterator operator++(int dummy);

    bool operator==(const FlatteningIterator<I> &rhs) const noexcept;
    bool operator!=(const FlatteningIterator<I> &rhs) const noexcept;

    FlatteningIterator &operator=(const FlatteningIterator<I> &other) = default;
    FlatteningIterator &operator=(FlatteningIterator<I> &&other) = default;
    FlatteningIterator(const FlatteningIterator<I> &other) = default;
    FlatteningIterator(FlatteningIterator<I> &&other) = default;
    ~FlatteningIterator() = default;

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
template <typename I>
struct iterator_traits<jamsat::FlatteningIterator<I>> {
    using value_type = typename jamsat::FlatteningIterator<I>::value_type;
    using reference = typename jamsat::FlatteningIterator<I>::reference;
    using pointer = typename jamsat::FlatteningIterator<I>::pointer;
    using iterator_category = typename jamsat::FlatteningIterator<I>::iterator_category;
    using difference_type = typename jamsat::FlatteningIterator<I>::difference_type;
};
}

/********** Implementation ****************************** */

namespace jamsat {

template <typename I>
FlatteningIterator<I>::FlatteningIterator(I begin, I end)
  : m_outerIt(begin), m_outerEndIt(end), m_innerIt(), m_innerEndIt() {
    while (m_outerIt != m_outerEndIt && m_outerIt->empty()) {
        ++m_outerIt;
    }

    if (m_outerIt != m_outerEndIt) {
        m_innerIt = boost::optional<InnerIt>{m_outerIt->begin()};
        m_innerEndIt = boost::optional<InnerIt>{m_outerIt->end()};
    }
}

template <typename I>
FlatteningIterator<I>::FlatteningIterator()
  : m_outerIt(), m_outerEndIt(), m_innerIt(), m_innerEndIt() {}

template <typename I>
typename FlatteningIterator<I>::reference FlatteningIterator<I>::operator*() const {
    JAM_ASSERT(m_innerIt, "Nested iterator pointing past the end dereferenced");
    return **m_innerIt;
}

template <typename I>
const typename FlatteningIterator<I>::value_type *FlatteningIterator<I>::operator->() const {
    return &(this->operator*());
}

template <typename I>
void FlatteningIterator<I>::operator++() {
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

template <typename I>
typename FlatteningIterator<I>::FlatteningIterator FlatteningIterator<I>::operator++(int dummy) {
    (void)dummy;
    FlatteningIterator<I> copy = *this;
    this->operator++();
    return copy;
}

template <typename I>
bool FlatteningIterator<I>::operator==(const FlatteningIterator<I> &rhs) const noexcept {
    if (this == &rhs) {
        return true;
    }

    // Optimize for comparing to end iterators:
    bool isLhsEnd = !static_cast<bool>(this->m_innerIt);
    bool isRhsEnd = !static_cast<bool>(rhs.m_innerIt);
    if (isLhsEnd != isRhsEnd) {
        return false;
    }
    if (isLhsEnd) {
        // Special case for end iterators (m_outerIt may differ, but all end iterators are equal)
        return true;
    }

    return (this->m_outerIt == rhs.m_outerIt && this->m_innerIt == rhs.m_innerIt &&
            this->m_outerEndIt == rhs.m_outerEndIt && this->m_innerEndIt == rhs.m_innerEndIt);
}

template <typename I>
bool FlatteningIterator<I>::operator!=(const FlatteningIterator<I> &rhs) const noexcept {
    return !this->operator==(rhs);
}
}
