// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <atomic>
#include <concepts>

#include "macros.hpp"

namespace boros {

    /// Atomically loads a value with the given ordering.
    template <std::integral I>
    ALWAYS_INLINE auto AtomicLoad(I *ptr, std::memory_order order) -> I {
        return std::atomic_ref<I>(*ptr).load(order);
    }

    /// Atomically stores a value with the given ordering.
    template <std::integral I>
    ALWAYS_INLINE auto AtomicStore(I *ptr, I value, std::memory_order order) -> void {
        std::atomic_ref<I>(*ptr).store(value, order);
    }

}
