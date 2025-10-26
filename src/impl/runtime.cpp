// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "runtime.hpp"
#include "macros.hpp"

namespace boros {

    namespace {

        thread_local Runtime g_current_runtime;

        ALWAYS_INLINE auto RaiseIoError(int res) noexcept -> void {
            errno = -res;
            PyErr_SetFromErrno(PyExc_OSError);
        }

    }

    auto Runtime::Create(unsigned sq_entries, io_uring_params &p) noexcept -> bool {
        if (int res = ring.Create(sq_entries, p); res < 0) [[unlikely]] {
            RaiseIoError(res);
            return false;
        }

        return true;
    }

    auto Runtime::Destroy() noexcept -> void {
        ring.Destroy();
    }

    auto Runtime::IsCreated() const noexcept -> bool {
        return ring.IsCreated();
    }

    auto Runtime::Get() noexcept -> Runtime& {
        return g_current_runtime;
    }

    auto RuntimeContext::New(PyTypeObject *tp, PyObject *args, PyObject *kwds) noexcept -> PyObject* {
        BOROS_UNUSED(tp, args, kwds);
        PyErr_SetString(PyExc_TypeError,
            "Cannot instantiate RuntimeContext directly; use RuntimeContext.get() instead");
        return nullptr;
    }

    auto RuntimeContext::Enter(PyTypeObject *tp, unsigned long sqes, unsigned long cqes, long wq_fd) noexcept -> RuntimeContext* {
        io_uring_params params{};

        if (g_current_runtime.IsCreated()) {
            PyErr_SetString(PyExc_RuntimeError,
                "Runtime context was already entered in the current thread");
            return nullptr;
        }

        // Set up a completion queue size separately from the submission
        // queue size, if given.
        if (cqes != 0) {
            params.flags |= IORING_SETUP_CQSIZE;
            params.cq_entries = cqes;
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
        // Decouple async event reaping and retrying from regular system
        // calls. If this isn't set, then io_uring uses normal task_work
        // for this and we could end up running it way too often if users
        // perform other syscalls too. This flag defers task_work to when
        // the event loop enters the kernel anyway to wait for new events.
        params.flags |= IORING_SETUP_DEFER_TASKRUN;
        // Inform the kernel that only a single thread submits to the ring.
        // This enables internal performance optimizations since our ring
        // is only designed for single-threaded usage anyway.
        params.flags |= IORING_SETUP_SINGLE_ISSUER;

        // If requested, share the async worker thread backend of another
        // ring identified by its file descriptor with this instance.
        if (wq_fd != -1) {
            params.flags |= IORING_SETUP_ATTACH_WQ;
            params.wq_fd = wq_fd;
        }

        if (!g_current_runtime.Create(sqes, params)) [[unlikely]] {
            return nullptr;
        }

        auto *self = python::Alloc<RuntimeContext>(tp);
        if (self != nullptr) [[likely]] {
            self->rt = &g_current_runtime;
        }
        return self;
    }

    auto RuntimeContext::Exit(std::nullptr_t) noexcept -> void {
        g_current_runtime.Destroy();
    }

    auto RuntimeContext::Get(PyTypeObject *tp) noexcept -> RuntimeContext* {
        if (!g_current_runtime.IsCreated()) {
            PyErr_SetString(PyExc_RuntimeError,
                "No runtime is currently active on this thread");
            return nullptr;
        }

        auto *self = python::Alloc<RuntimeContext>(tp);
        if (self != nullptr) [[likely]] {
            self->rt = &g_current_runtime;
        }

        return self;
    }

    auto RuntimeContext::GetRingFd() const noexcept -> long {
        return rt->ring.GetRingFd();
    }

    auto RuntimeContext::EnableRing() const noexcept -> void {
        if (int res = rt->ring.Enable(); res < 0) [[unlikely]] {
            RaiseIoError(res);
        }
    }

    namespace {

        auto g_runtime_context_methods = python::MethodTable(
            python::Method<&RuntimeContext::Enter>("enter"),
            python::Method<&RuntimeContext::Exit>("exit"),
            python::Method<&RuntimeContext::Get>("get"),
            python::Method<&RuntimeContext::GetRingFd>("get_ring_fd"),
            python::Method<&RuntimeContext::EnableRing>("enable_ring")
        );

        auto g_runtime_context_slots = python::TypeSlotTable(
            python::TypeSlot(Py_tp_new, &RuntimeContext::New),
            python::TypeSlot(Py_tp_methods, g_runtime_context_methods.data())
        );

        auto g_runtime_context_spec = python::TypeSpec<RuntimeContext>(
            "RuntimeContext",
            g_runtime_context_slots.data()
        );

    }

    auto RuntimeContext::Register(python::Module mod) noexcept -> PyObject* {
        return mod.Add(g_runtime_context_spec);
    }

}
