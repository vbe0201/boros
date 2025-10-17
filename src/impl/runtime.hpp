// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

#include "ring.hpp"

namespace boros::impl {

    /// A Python structure providing access to the current runtime state.
    /// This should be the preferred way of interfacing with the runtime.
    extern "C" struct RuntimeContextObj {
        PyObject_HEAD
        IoRing *ring;

        /// Makes the RuntimeContext class accessible to Python code through
        /// the given module object.
        static auto Register(PyObject *module) noexcept -> PyObject*;
    };

}
