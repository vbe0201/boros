// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "binding/python.hpp"

#include "ring.hpp"

namespace boros {

    /// Defines the global policy for creating per-thread event loops.
    /// This is kept in per-module state when configured so that the
    /// loops can access it for lazy creation on demand.
    struct EventLoopPolicy {
        unsigned int sq_entries = 0;
        unsigned int cq_entries = 0;
        int wqfd                = -1;

        auto GetSqEntries() const -> unsigned int { return sq_entries; }
        auto SetSqEntries(unsigned int v) -> void { sq_entries = v; }

        auto GetCqEntries() const -> unsigned int { return cq_entries; }
        auto SetCqEntries(unsigned int v) -> void { cq_entries = v; }

        auto GetWqFd() const -> int { return wqfd; }
        auto SetWqFd(int v) -> void { wqfd = v; }

        /// Registers the EventLoopPolicy as a Python class onto the module.
        static auto Register(PyObject *mod) -> PyTypeObject *;
    };

    struct EventLoop {
    private:
        IoRing m_ring;

    public:
        /// Creates a new event loop for the current thread given a
        /// creation policy and returns its instance.
        static auto Create(python::Module mod, PyObject *policy) -> PyObject *;

        /// Destroys the active event loop on the current thread.
        /// No-op if  the event loop is not running.
        static auto Destroy(python::Module mod) -> void;

        /// Gets the active event loop on the current thread, or
        /// raises an exception if not set.
        static auto Get(python::Module mod) -> PyObject *;

        /// Registers the EventLoop as a Python class onto the module.
        static auto Register(PyObject *mod) -> PyTypeObject *;

    public:
        /// Performs one tick of the event loop. This advances all
        /// subsystems and resumes ready coroutines.
        auto Tick() -> void;

    public:
        // TODO: Just a test for now.

        static auto Nop(python::Module mod, int res) -> void;
    };

}  // namespace boros
