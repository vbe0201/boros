// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

/// Strong hint to the compiler to always inline a function.
#define ALWAYS_INLINE __attribute__((always_inline)) inline

/// Strong hint to the compiler to always inline a lambda body.
#define ALWAYS_INLINE_LAMBDA __attribute__((always_inline))

/// Discards unused variables without side effects.
#define BOROS_UNUSED(...) ::boros::impl::DiscardUnused(__VA_ARGS__)

namespace boros::impl {

    template <typename... Args>
    ALWAYS_INLINE void DiscardUnused(Args &&...args) noexcept {
        (static_cast<void>(args), ...);
    }

}
