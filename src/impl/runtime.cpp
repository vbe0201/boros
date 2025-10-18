// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "runtime.hpp"

namespace boros::impl {

    namespace {

        thread_local IoRing g_current_io_ring;

    }

    namespace {

        // Kernel 6.1+ is required.
        // sq_entries must be a power of two
        // cq_entries must be a power of two and greater than sq_entries
        // TODO: sqpoll
        extern "C" auto RuntimeContextEnter(PyObject *self, PyObject *args, PyObject *kwds) noexcept -> PyObject* {
            constexpr const char *kwlist[] = {"sq_entries", "cq_entries", "wq_fd", nullptr};

            unsigned sqs, cqs = 0;
            int wq = -1;
            io_uring_params params{};

            if (g_current_io_ring.IsCreated()) {
                PyErr_SetString(PyExc_RuntimeError,
                    "Runtime context is already set up in the current thread");
                return nullptr;
            }

            if (!PyArg_ParseTupleAndKeywords(args, kwds, "I|$Ii", const_cast<char**>(kwlist), &sqs, &cqs, &wq)) {
                return nullptr;
            }

            // If a completion queue size is given, use it instead of making it
            // the same size as the submission queue.
            if (cqs != 0) {
                params.flags |= IORING_SETUP_CQSIZE;
                params.cq_entries = cqs;
            }

            // Clamp the submission queue size at the maximum number of entries.
            // This helps reduce the surface for user errors during ring setup.
            params.flags |= IORING_SETUP_CLAMP;
            // Create the ring in disabled state by default. This allows us to
            // do some setup before submissions are allowed.
            params.flags |= IORING_SETUP_R_DISABLED;
            // Submit all requests to the kernel even when one of them results
            // in an error. We do not care about that since we still receive a
            // regular completion entry with the error and can then handle it.
            params.flags |= IORING_SETUP_SUBMIT_ALL;
            // Do not interrupt a running task when a completion event arrives.
            // Since we make sure to timely handle completions ourselves, this
            // option saves us from a lot of overhead and improves performance.
            // TODO: This can only be set when IORING_SETUP_SQPOLL is disabled.
            params.flags |= IORING_SETUP_COOP_TASKRUN | IORING_SETUP_TASKRUN_FLAG;
            // Inform the kernel that only a single thread submits to the ring.
            // This enables internal performance optimizations since our IoRing
            // is only designed for single-threaded usage anyway.
            params.flags |= IORING_SETUP_SINGLE_ISSUER;

            // If requested, share the async worker thread backend of an existing
            // ring identified by its file descriptor with this ring instance.
            if (wq != -1) {
                params.flags |= IORING_SETUP_ATTACH_WQ;
                params.wq_fd = wq;
            }

            // Try to initialize the io_uring instance, or raise an exception.
            if (int res = g_current_io_ring.Create(sqs, params); res < 0) [[unlikely]] {
                errno = -res;
                PyErr_SetFromErrno(PyExc_OSError);
                return nullptr;
            }

            Py_RETURN_NONE;
        }

        extern "C" auto RuntimeContextGet(PyObject *cls, PyObject *args) noexcept -> PyObject* {
            BOROS_UNUSED(args);

            if (!g_current_io_ring.IsCreated()) {
                PyErr_SetString(PyExc_RuntimeError,
                    "No runtime is currently active on this thread");
                return nullptr;
            }

            auto *ctx = reinterpret_cast<RuntimeContextObj*>(PyObject_CallNoArgs(cls));
            if (ctx != nullptr) [[likely]] {
                ctx->ring = &g_current_io_ring;
            }

            return reinterpret_cast<PyObject*>(ctx);
        }

        PyMethodDef g_runtime_context_methods[] = {
            {"enter", reinterpret_cast<PyCFunction>(&RuntimeContextEnter), METH_CLASS | METH_VARARGS | METH_KEYWORDS, ""},
            {"get", &RuntimeContextGet, METH_CLASS | METH_NOARGS, ""},
            {nullptr, nullptr, 0, nullptr}
        };

        PyType_Slot g_runtime_context_slots[] = {
            {Py_tp_new, reinterpret_cast<void*>(&PyType_GenericNew)},
            {Py_tp_methods, g_runtime_context_methods},
            {0, nullptr}
        };

        PyType_Spec g_runtime_context_spec = {
            "RuntimeContext",
            sizeof(RuntimeContextObj),
            0,
            Py_TPFLAGS_DEFAULT,
            g_runtime_context_slots
        };

    }

    auto RuntimeContextObj::Register(PyObject* module) noexcept -> PyObject* {
        auto *cls = PyType_FromModuleAndSpec(module, &g_runtime_context_spec, nullptr);
        if (cls == nullptr) [[unlikely]] {
            return nullptr;
        }

        if (PyModule_AddObjectRef(module, "RuntimeContext", cls) < 0) [[unlikely]] {
            return nullptr;
        }

        return cls;
    }

}
