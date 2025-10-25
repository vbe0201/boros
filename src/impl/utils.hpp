// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <sys/mman.h>

#include "macros.hpp"

namespace boros {

    /// A handle to a memory-mapped region.
    ///
    /// When this type is destroyed, the memory is freed. Users must make sure
    /// that any backing file handles are only closed after the mapping has
    /// been destroyed. Can be safely moved around, but not copied.
    struct Mmap {
        /// The base address of the mapping, or nullptr if unmapped.
        void *Address;
        /// The size of the mapping in bytes, or 0 if unmapped.
        std::size_t Size;

        /// Creates an unmapped instance.
        ALWAYS_INLINE Mmap() : Address(nullptr), Size(0) {}

        Mmap(const Mmap&) = delete;
        auto operator=(const Mmap&) -> Mmap& = delete;

        Mmap(Mmap &&rhs) noexcept;
        auto operator=(Mmap &&rhs) noexcept -> Mmap&;

        ~Mmap() noexcept;

        /// Creates a mapping from the given information.
        auto Create(int fd, off_t offset, std::size_t len) noexcept -> int;

        /// Destroys an existing mapping.
        /// Does nothing if the instance is not mapped.
        auto Destroy() noexcept -> void;

        /// Whether this instance is mapped into memory or not.
        ALWAYS_INLINE auto IsMapped() const noexcept -> bool {
            return Address != nullptr;
        }

        /// Gets a pointer of type ``T`` to a given offset into the mapping.
        template <typename T>
        ALWAYS_INLINE auto Offset(std::uint32_t offset) const noexcept -> T* {
            return reinterpret_cast<T*>(static_cast<std::byte*>(Address) + offset);
        }
    };

    /// Atomically loads the value from ``ptr`` with a given ordering.
    ALWAYS_INLINE auto AtomicLoad(unsigned *ptr, std::memory_order order) noexcept -> unsigned {
        return std::atomic_ref(*ptr).load(order);
    }

    /// Atomically writes ``v`` to ``ptr`` with a given ordering.
    ALWAYS_INLINE auto AtomicStore(unsigned *ptr, unsigned v, std::memory_order order) noexcept -> void {
        std::atomic_ref(*ptr).store(v, order);
    }

}
