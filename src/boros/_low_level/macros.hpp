// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <limits.h>

/// Strong hint to the compiler to always inline a function.
#define ALWAYS_INLINE __attribute__((always_inline)) inline

/// Strong hint to the compiler to always inline a lambda body.
#define ALWAYS_INLINE_LAMBDA __attribute__((always_inline))

/// Discards unused arguments without side effects.
#define BOROS_UNUSED(...) ::boros::impl::Unused(__VA_ARGS__)

namespace boros::impl {

    template <typename... Args>
    constexpr auto Unused(Args &&...args) -> void {
        (static_cast<void>(args), ...);
    }

}  // namespace boros::impl
