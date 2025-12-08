// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "io/mmap.hpp"

#include <cerrno>

namespace boros::io {

    Mmap::~Mmap() {
        this->Unmap();
    }

    Mmap::Mmap(Mmap &&rhs) noexcept : ptr(rhs.ptr), size(rhs.size) {
        rhs.ptr  = nullptr;
        rhs.size = 0;
    }

    Mmap &Mmap::operator=(Mmap &&rhs) noexcept {
        if (this != &rhs) {
            this->Unmap();
            std::swap(ptr, rhs.ptr);
            std::swap(size, rhs.size);
        }
        return *this;
    }

    int Mmap::Map(int fd, off_t offset, std::size_t len) {
        void *raw = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, offset);
        if (raw == MAP_FAILED) {
            return -errno;
        }

        ptr  = static_cast<std::byte *>(raw);
        size = len;
        return 0;
    }

    void Mmap::Unmap() {
        if (this->IsMapped()) {
            munmap(ptr, size);
            ptr  = nullptr;
            size = 0;
        }
    }

    int Mmap::DontFork() {
        if (madvise(ptr, size, MADV_DONTFORK) != 0) {
            return -errno;
        }

        return 0;
    }

}  // namespace boros::io
