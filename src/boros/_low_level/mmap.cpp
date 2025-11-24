// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "mmap.hpp"

#include <cerrno>

namespace boros {

    auto Mmap::Map(int fd, off_t offset, std::size_t len) -> int {
        void *raw = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, offset);
        if (raw == MAP_FAILED) [[unlikely]] {
            return -errno;
        }

        ptr  = static_cast<std::byte*>(raw);
        size = len;
        return 0;
    }

    auto Mmap::Unmap() -> void {
        if (this->IsMapped()) {
            munmap(ptr, size);
            ptr  = nullptr;
            size = 0;
        }
    }

    auto Mmap::DontFork() -> int {
        if (madvise(ptr, size, MADV_DONTFORK) != 0) [[unlikely]] {
            return -errno;
        }

        return 0;
    }

}
