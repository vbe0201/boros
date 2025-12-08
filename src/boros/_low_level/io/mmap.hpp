// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <cstddef>
#include <utility>

#include <sys/mman.h>

namespace boros::io {

    /// A RAII handle to a memory-mapped region. This is used to
    /// setup the shared memory buffers for the io_uring.
    struct Mmap {
        /// Base pointer to the mapping, or nullptr if unmapped.
        std::byte *ptr = nullptr;
        /// The size of the mapping in bytes, or 0 if unmapped.
        std::size_t size = 0;

        Mmap() noexcept = default;
        ~Mmap();

        Mmap(const Mmap &)            = delete;
        Mmap &operator=(const Mmap &) = delete;

        Mmap(Mmap &&rhs) noexcept;
        Mmap &operator=(Mmap &&rhs) noexcept;

        /// Whether this instance is currently mapped to memory.
        bool IsMapped() const {
            return ptr != nullptr;
        }

        /// Maps a file with a given offset in bytes. Returns 0 on
        /// success, or a negative errno value.
        int Map(int fd, off_t offset, std::size_t len);

        /// Unmaps an existing mapping and resets pointer and size.
        /// Does nothing if this instance is not mapped.
        void Unmap();
    };

}  // namespace boros::io
