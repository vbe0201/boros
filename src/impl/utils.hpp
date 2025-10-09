#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <sys/mman.h>

#define ALWAYS_INLINE __attribute__((always_inline)) inline

namespace boros::impl {

    struct Mmap {
        void *Address;
        std::size_t Size;

        ALWAYS_INLINE Mmap() : Address(nullptr), Size(0) {}

        Mmap(const Mmap&) = delete;
        Mmap &operator=(const Mmap&) = delete;

        ALWAYS_INLINE Mmap(Mmap &&rhs) noexcept : Address(rhs.Address), Size(rhs.Size) {
            rhs.Address = nullptr;
            rhs.Size    = 0;
        }

        ALWAYS_INLINE Mmap &operator=(Mmap &&rhs) noexcept {
            if (this != &rhs) {
                this->Destroy();
                std::swap(Address, rhs.Address);
                std::swap(Size, rhs.Size);
            }
            return *this;
        }

        ~Mmap() noexcept {
            this->Destroy();
        }

        auto Create(int fd, off_t offset, std::size_t len) noexcept -> int;
        auto Destroy() noexcept -> void;

        ALWAYS_INLINE auto IsMapped() const noexcept -> bool {
            return Address != nullptr;
        }

        template <typename T>
        ALWAYS_INLINE auto Offset(std::uint32_t offset) const noexcept -> T* {
            return reinterpret_cast<T*>(static_cast<std::byte*>(Address) + offset);
        }
    };

    ALWAYS_INLINE auto AtomicLoad(unsigned *ptr, std::memory_order order) noexcept -> unsigned {
        return std::atomic_ref(*ptr).load(order);
    }

    ALWAYS_INLINE auto AtomicStore(unsigned *ptr, unsigned v, std::memory_order order) noexcept -> void {
        std::atomic_ref(*ptr).store(v, order);
    }

}
