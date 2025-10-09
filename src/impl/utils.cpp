#include "utils.hpp"

#include <cerrno>

namespace boros::impl {

    auto Mmap::Create(int fd, off_t offset, std::size_t len) noexcept -> int {
        void *ptr = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, offset);
        if (ptr == MAP_FAILED) [[unlikely]] {
            return -errno;
        }

        Address = ptr;
        Size    = len;
        return 0;
    }

    auto Mmap::Destroy() noexcept -> void {
        if (this->IsMapped()) {
            munmap(Address, Size);
            Address = nullptr;
            Size    = 0;
        }
    }

}
