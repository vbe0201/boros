// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <atomic>
#include <concepts>

namespace boros::io::impl {

    /// Atomically loads a value with the given ordering.
    template <std::integral I>
    I AtomicLoad(I *ptr, std::memory_order order) {
        return std::atomic_ref<I>(*ptr).load(order);
    }

    /// Atomically stores a value with the given ordering.
    template <std::integral I>
    void AtomicStore(I *ptr, I value, std::memory_order order) {
        std::atomic_ref<I>(*ptr).store(value, order);
    }

}  // namespace boros::io::impl
