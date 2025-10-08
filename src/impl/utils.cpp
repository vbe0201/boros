#include "utils.hpp"

#include <cerrno>

namespace boros::impl {

    auto Mmap::Create(int fd, off_t offset, std::size_t len) noexcept -> std::expected<Mmap, int> {
        void *ptr = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, offset);
        if (ptr == MAP_FAILED) [[unlikely]] {
            return std::unexpected(errno);
        }

        return Mmap{ptr, len};
    }

    Mmap::~Mmap() noexcept {
        munmap(Address, Size);
    }

    auto Mmap::DontFork() const noexcept -> std::expected<void, int> {
        int res = madvise(Address, Size, MADV_DONTFORK);
        if (res != 0) [[unlikely]] {
            return std::unexpected(errno);
        }

        return {};
    }

}
