/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>

#include <boost/range.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <cstdint>
#include <vector>

namespace jamsat {
/**
 * \class OccurrenceMap
 *
 * \brief A map-like type for keeping track of element occurrences in containers.
 *
 * \ingroup JamSAT_Utils
 *
 * \tparam Container                A type partially satisfying the STL `Container` concept:
 *                                  For objects `c` of type `Container`, the expressions
 *                                  `c.begin()` and `c.end()` must be valid, with the same
 *                                  semantics as defined by the STL `Container` concept.
 *                                  `Container::value_type` must be the type of the objects
 *                                  contained in instances of `Container`.
 * \tparam ContainerDeletedQuery    A type satisfying the STL UnaryPredicate concept for
 *                                  `Container const*` arguments, with instances of
 *                                  `ContainerDeletedQuery`indicating whether the container
 *                                  has been marked for deletion.
 * \tparam ContainerValueIndex      A type that is a model of the concept `Index` with indexed
 *                                  type `Container::value_type`.
 */
template <typename Container, typename ContainerDeletedQuery,
          typename ContainerValueIndex = typename Container::value_type::Index>
class OccurrenceMap {
public:
    using ContainerRange = boost::iterator_range<typename std::vector<Container *const>::iterator>;

    /**
     * \brief Constructs an OccurrenceMap
     *
     * \param maxElement    The maximum element that may occur in the containers
     *                      added to the OccurrenceMap.
     */
    OccurrenceMap(typename Container::value_type maxElement);

    /**
     * \brief Increases the maximum element that may occur in the containers added
     *        to the OccurrenceMap
     *
     * \param maxElement    The maximum element that may occur in the containers
     *                      added to the OccurrenceMap.
     */
    void increaseMaxElementTo(typename Container::value_type maxElement);

    /**
     * \brief Adds a container to the occurrence map.
     *
     * \param container     The container to be added.
     */
    void insert(Container &container);

    /**
     * \brief Returns a range of pointers to the containers in which \p index occurs.
     *
     * \param value         A value not greater than the current maximum element.
     * \returns             A range of pointers to the containers in which \p index
     *                      occurs.
     */
    auto operator[](typename Container::value_type value) noexcept -> ContainerRange;

    /**
     * \brief Marks a container as to-be-deleted from the occurrence map.
     *
     * Unless elements have been removed from \p container, \p container will not be
     * returned in future calls to `operator[]()`.
     *
     * Precondition: \p container must be marked as to-be-deleted by all
     * `ContainerDeletedQuery` objects.
     *
     * \param container     The container to be marked as to-be-deleted from the
     *                      occurrence map.
     */
    void remove(Container const &container) noexcept;

    OccurrenceMap(OccurrenceMap const &rhs) = delete;
    auto operator=(OccurrenceMap const &rhs) -> OccurrenceMap & = delete;

    OccurrenceMap(OccurrenceMap &&rhs) noexcept = default;
    auto operator=(OccurrenceMap &&rhs) noexcept -> OccurrenceMap & = default;

private:
    using value_type = typename Container::value_type;
    struct OccurrenceListWithFlags {
        bool m_requiresUpdate;
        std::vector<Container *> m_occList;
    };

    BoundedMap<typename Container::value_type, OccurrenceListWithFlags, ContainerValueIndex>
        m_occurrences;
    ContainerDeletedQuery m_deletedQuery;
};

/********** Implementation ****************************** */

template <typename Container, typename ContainerDeletedQuery, typename ContainerValueIndex>
OccurrenceMap<Container, ContainerDeletedQuery, ContainerValueIndex>::OccurrenceMap(
    typename Container::value_type maxElement)
  : m_occurrences(maxElement), m_deletedQuery() {}

template <typename Container, typename ContainerDeletedQuery, typename ContainerValueIndex>
void OccurrenceMap<Container, ContainerDeletedQuery, ContainerValueIndex>::increaseMaxElementTo(
    typename Container::value_type maxElement) {
    m_occurrences.increaseSizeTo(maxElement);
}

template <typename Container, typename ContainerDeletedQuery, typename ContainerValueIndex>
void OccurrenceMap<Container, ContainerDeletedQuery, ContainerValueIndex>::insert(
    Container &container) {
    for (auto &element : container) {
        m_occurrences[element].m_occList.push_back(&container);
    }
}

template <typename Container, typename ContainerDeletedQuery, typename ContainerValueIndex>
void OccurrenceMap<Container, ContainerDeletedQuery, ContainerValueIndex>::remove(
    Container const &container) noexcept {
    JAM_ASSERT(m_deletedQuery(&container),
               "Only remove containers marked for deletion may be deleted");
    for (auto &element : container) {
        m_occurrences[element].m_requiresUpdate = true;
    }
}

template <typename Container, typename ContainerDeletedQuery, typename ContainerValueIndex>
auto OccurrenceMap<Container, ContainerDeletedQuery, ContainerValueIndex>::
operator[](typename Container::value_type value) noexcept -> ContainerRange {
    auto &occList = m_occurrences[value].m_occList;
    if (m_occurrences[value].m_requiresUpdate) {
        boost::remove_erase_if(occList, m_deletedQuery);
        m_occurrences[value].m_requiresUpdate = false;
    }
    return boost::make_iterator_range(occList.cbegin(), occList.cend());
}
}
