// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "io_uring/mmap.hpp"

#include <cerrno>

namespace boros {

    auto Mmap::Map(int fd, off_t offset, std::size_t len) noexcept -> int {
        void *ptr = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, offset);
        if (ptr == MAP_FAILED) [[unlikely]] {
            return -errno;
        }

        address = ptr;
        size    = len;
        return 0;
    }

    auto Mmap::Unmap() noexcept -> void {
        if (this->IsMapped()) {
            munmap(address, size);
            address = nullptr;
            size    = 0;
        }
    }

}
