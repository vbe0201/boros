// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

/// Discards unused arguments without side effects.
#define BOROS_UNUSED(...) ::boros::impl::Unused(__VA_ARGS__)

namespace boros::impl {

    template <typename... Args>
    constexpr auto Unused(Args &&...args) -> void {
        (static_cast<void>(args), ...);
    }

}  // namespace boros::impl
