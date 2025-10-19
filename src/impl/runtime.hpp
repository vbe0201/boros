// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

#include "ring.hpp"

namespace boros::impl {

    class Runtime {
    private:
        IoRing m_ring;

    public:
        Runtime() noexcept = default;
        ~Runtime() noexcept = default;

        /// Gets the runtime instance for the current thread.
        static auto Get() noexcept -> Runtime&;

        /// Creates a new runtime instance by initializing all resources
        /// and setting up an io_uring instance from the given parameters.
        auto Create(unsigned sq_entries, io_uring_params &p) noexcept -> PyObject*;

        /// Whether this runtime has already been created or not.
        auto IsCreated() const noexcept -> bool;

        /// Gets the file descriptor of the underlying io_uring instance.
        auto GetRingFd() const noexcept -> PyObject*;

        /// Enables the underlying io_uring instance.
        auto EnableRing() const noexcept -> PyObject*;
    };

    /// A Python structure providing access to the current runtime state.
    /// This should be the preferred way of interacting with the runtime.
    struct RuntimeContextObj {
        PyObject_HEAD
        Runtime *rt;

        /// Makes the RuntimeContext class accessible to Python code through
        /// the given module object.
        static auto Register(PyObject *module) noexcept -> PyObject*;
    };

}
