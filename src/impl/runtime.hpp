// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "wrapper/python.hpp"

#include "implmodule.hpp"
#include "io_uring/ring.hpp"

namespace boros {

    /// The runtime instance state. This is kept in thread-local storage
    /// and is exclusively managed through the RuntimeContext from Python.
    struct Runtime {
        IoRing ring;

        /// Sets up this runtime instance by initializing all resources.
        auto Create(unsigned sq_entries, io_uring_params &p) noexcept -> bool;

        /// Tears down this runtime instance if it is active. It can be
        /// reenabled by another call to Create.
        auto Destroy() noexcept -> void;

        /// Indicates if this runtime is currently ready for use.
        auto IsCreated() const noexcept -> bool;

        /// Gets the runtime instance for the current thread.
        static auto Get() noexcept -> Runtime&;
    };

    /// A Python structure providing access to the current runtime state.
    /// This should be the preferred way of interacting with the runtime.
    struct RuntimeContext {
        PyObject_HEAD
        Runtime *rt;

        /// The implementation of the __new__ operator for this class.
        static auto New(PyTypeObject *tp, PyObject *args, PyObject *kwds) noexcept -> PyObject*;

        /// Enters the runtime context on the current thread.
        static auto Enter(PyTypeObject *tp, unsigned long sqes, unsigned long cqes, long wq_fd) noexcept -> RuntimeContext*;

        /// Exits the runtime context on the current thread.
        static auto Exit(std::nullptr_t) noexcept -> void;

        /// Gets the associated runtime instance for this thread.
        static auto Get(PyTypeObject *tp) noexcept -> RuntimeContext*;

        /// Drives a coroutine to completion and returns its result.
        auto Run(PyObject *coro) noexcept -> PyObject*;

        /// Gets the file descriptor of the associated io_uring instance,
        /// or -1 if it is closed already.
        auto GetRingFd() const noexcept -> long;

        /// Enables the associated io_uring instance after it was created.
        /// Only then it is possible to submit operations to the ring.
        auto EnableRing() const noexcept -> void;

        /// Exposes the RuntimeContext class to a given Python module.
        static auto Register(ImplModule mod) noexcept -> PyObject*;
    };

}
