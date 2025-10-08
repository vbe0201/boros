#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <expected>

#include <sys/mman.h>

#define ALWAYS_INLINE __attribute__((always_inline)) inline

namespace boros::impl {

    struct Mmap {
    public:
        void *Address;
        std::size_t Size;

    public:
        static auto Create(int fd, off_t offset, std::size_t len) noexcept -> std::expected<Mmap, int>;
        ~Mmap() noexcept;

        auto DontFork() const noexcept -> std::expected<void, int>;

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
