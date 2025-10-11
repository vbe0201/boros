// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "utils.hpp"

#include <cerrno>

namespace boros::impl {

    Mmap::Mmap(Mmap &&rhs) noexcept : Address(rhs.Address), Size(rhs.Size) {
        rhs.Address = nullptr;
        rhs.Size    = 0;
    }

    auto Mmap::operator=(Mmap &&rhs) noexcept -> Mmap& {
        if (this != &rhs) {
            this->Destroy();
            std::swap(Address, rhs.Address);
            std::swap(Size, rhs.Size);
        }
        return *this;
    }

    Mmap::~Mmap() noexcept {
        this->Destroy();
    }

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
