// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <limits.h>

/// Strong hint to the compiler to always inline a function.
#define ALWAYS_INLINE __attribute__((always_inline)) inline

/// Strong hint to the compiler to always inline a lambda body.
#define ALWAYS_INLINE_LAMBDA __attribute__((always_inline))

/// Gets the size of a given type or value in bits.
#define BITSIZEOF(x) (sizeof(x) * CHAR_BIT)
