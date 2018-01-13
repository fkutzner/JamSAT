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
#include <cstdlib>
#include <memory>

#include <libjamsat/utils/Assert.h>

namespace jamsat {
namespace clausedb_detail {

/**
 * \ingroup JamSAT_ClauseDB
 *
 * \brief Simple allocator for a fixed-size chunk of heap memory.
 */
class Heaplet {
public:
    using size_type = uintptr_t;

    /**
     * \brief Constructs a Heaplet.
     *
     * No memory is allocated during construction. The heaplet can only be
     * used after the initialize() method has been called.
     *
     * \param size  The size of the heaplet in bytes.
     */
    explicit Heaplet(size_type size) noexcept
      : m_memory(nullptr), m_firstFree(nullptr), m_size(size), m_free(0) {}

    ~Heaplet() {
        if (m_memory != nullptr) {
            std::free(m_memory);
            m_memory = nullptr;
        }
    }

    /**
     * \brief Initializes the Heaplet.
     *
     * \throws std::bad_alloc when memory allocation failed.
     */
    void initialize() {
        JAM_ASSERT(m_memory == nullptr, "Cannot initialize a heaplet twice");
        m_memory = std::malloc(m_size);
        if (m_memory == nullptr) {
            throw std::bad_alloc{};
        }
        clear();
    }

    /**
     * \brief Returns true iff the Heaplet has been initialized.
     *
     * \returns true iff the Heaplet has been initialized.
     */
    bool isInitialized() const noexcept { return m_memory != nullptr; }

    /**
     * \brief Empties the Heaplet.
     *
     * This method may only be called for initialized Heaplets. After calling
     * this method, the Heaplet is reset to its state just after initialization.
     */
    void clear() noexcept {
        JAM_ASSERT(isInitialized(), "Cannot reset an uninitialized Heaplet");
        m_firstFree = m_memory;
        m_free = m_size;
    }

    /**
     * \brief Allocates memory in the Heaplet.
     *
     * \param size      The size of the allocated object, in bytes. \p size must not be smaller
     *                  than `sizeof(T)`.
     * \returns         If the allocation was successful, a pointer to the first byte
     *                  of the allocated memory is returned. Otherwise, `nullptr` is returned.
     *                  If the returned pointer is not nullptr, it is aligned such that an
     *                  object of type `T` can be stored at that location.
     * \tparam T        The non-void type of the allocated object.
     */
    template <typename T>
    T *allocate(size_type size) noexcept {
        JAM_ASSERT(isInitialized(), "Cannot allocate on an uninitialized Heaplet");
        JAM_ASSERT(size >= sizeof(T), "Fewer bytes allocated than required by type");
        void *result = std::align(alignof(T), size, m_firstFree, m_free);
        m_free -= size;
        m_firstFree = reinterpret_cast<char *>(m_firstFree) + size;
        return reinterpret_cast<T *>(result);
    }

    /**
     * \brief Returns the amount of bytes which are available for allocation.
     *
     * \returns The amount of bytes which are available for allocation.
     */
    size_type getFreeSize() const noexcept { return m_free; }

private:
    void *m_memory;

    // Pointer to the first free byte of the Heaplet. In case the Heaplet is full,
    // this is a past-the-end pointer.
    // Invariant: m_memory != nullptr => (m_memory <= m_firstFree <= m_memory + m_size)
    void *m_firstFree;

    size_type m_size;

    // Invariant: m_free <= m_size
    size_type m_free;
};
}
}
