// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

/// Strong hint to the compiler to always inline a function.
#define ALWAYS_INLINE __attribute__((always_inline)) inline

/// Strong hint to the compiler to always inline a lambda body.
#define ALWAYS_INLINE_LAMBDA __attribute__((always_inline))
