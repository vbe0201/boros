// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <cstddef>
#include <utility>

#include <sys/mman.h>

#include "macros.hpp"

namespace boros {

    /// A RAII handle to a memory-mapped region.
    struct Mmap {
        /// The base pointer to the mapping, or nullptr if unmapped.
        std::byte *ptr = nullptr;
        /// The size of the mapping in bytes, or 0 if unmapped.
        std::size_t size = 0;

        ALWAYS_INLINE Mmap() = default;

        ALWAYS_INLINE ~Mmap() {
            this->Unmap();
        }

        Mmap(const Mmap&) = delete;
        auto operator=(const Mmap&) -> Mmap& = delete;

        ALWAYS_INLINE Mmap(Mmap &&rhs) : ptr(rhs.ptr), size(rhs.size) {
            rhs.ptr  = nullptr;
            rhs.size = 0;
        }

        ALWAYS_INLINE auto operator=(Mmap &&rhs) -> Mmap& {
            if (this != &rhs) {
                this->Unmap();
                std::swap(ptr, rhs.ptr);
                std::swap(size, rhs.size);
            }
            return *this;
        }

        /// Whether this instance is currently mapped to memory.
        ALWAYS_INLINE auto IsMapped() const -> bool {
            return ptr != nullptr;
        }

        /// Maps a file with a given size and offset in bytes. Returns
        /// 0 on success, or a negative errno value.
        auto Map(int fd, off_t offset, std::size_t len) -> int;

        /// Unmaps an existing mapping and resets pointer and size.
        /// Does nothing if this instance is not mapped.
        auto Unmap() -> void;

        /// Disables access to the memory for child processes after a
        /// fork. Returns 0 on success, or a negative errno value.
        auto DontFork() -> int;
    };

}
