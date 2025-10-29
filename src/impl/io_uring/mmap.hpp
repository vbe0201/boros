// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <cstddef>
#include <utility>

#include <sys/mman.h>

#include "macros.hpp"

namespace boros {

    /// A handle to a memory-mapped region.
    struct Mmap {
        /// The base address of the mapping, or nullptr if unmapped.
        void *address = nullptr;
        /// The size of the mapping in bytes, or 0 if unmapped.
        std::size_t size = 0;

        ALWAYS_INLINE Mmap() noexcept = default;

        ALWAYS_INLINE ~Mmap() noexcept {
            this->Unmap();
        }

        Mmap(const Mmap&) = delete;
        auto operator=(const Mmap&) -> Mmap& = delete;

        ALWAYS_INLINE Mmap(Mmap &&rhs) noexcept : address(rhs.address), size(rhs.size) {
            rhs.address = nullptr;
            rhs.size    = 0;
        }

        ALWAYS_INLINE auto operator=(Mmap &&rhs) noexcept -> Mmap& {
            if (this != std::addressof(rhs)) {
                this->Unmap();
                std::swap(address, rhs.address);
                std::swap(size, rhs.size);
            }
            return *this;
        }

        /// Indicates if this instance is mapped into memory.
        ALWAYS_INLINE auto IsMapped() const noexcept -> bool {
            return address != nullptr;
        }

        /// Maps the given file with a given size and offset in bytes.
        /// Returns 0 on success, or a negative errno value on error.
        auto Map(int fd, off_t offset, std::size_t len) noexcept -> int;

        /// Unmaps an existing mapping and resets address and size.
        /// Does nothing if this instance is not mapped.
        auto Unmap() noexcept -> void;

        /// Gets a pointer of type T to a given offset into the mapping.
        template <typename T>
        ALWAYS_INLINE auto GetPointer(unsigned offset) const noexcept -> T* {
            return reinterpret_cast<T*>(static_cast<std::byte*>(address) + offset);
        }
    };

}
