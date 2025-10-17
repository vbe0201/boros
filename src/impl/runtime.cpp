// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "runtime.hpp"

namespace boros::impl {

    namespace {

        thread_local IoRing g_current_io_ring;

    }

    namespace {

        extern "C" auto RuntimeContextNew(PyTypeObject *tp, PyObject *args, PyObject *kwds) noexcept -> PyObject* {
            BOROS_UNUSED(tp, args, kwds);

            PyErr_SetString(PyExc_TypeError,
                "Cannot instantiate RuntimeContext directly; use RuntimeContext.get() instead");
            return nullptr;
        }

        extern "C" auto RuntimeContextGet(PyObject *self, PyObject *args) noexcept -> PyObject* {
            BOROS_UNUSED(args);

            auto *tp = Py_TYPE(self);
            auto *tp_alloc = reinterpret_cast<allocfunc>(PyType_GetSlot(tp, Py_tp_alloc));

            auto *ring = &g_current_io_ring;
            if (!ring->IsCreated()) {
                PyErr_SetString(PyExc_RuntimeError,
                    "No runtime is currently active on this thread");
                return nullptr;
            }

            auto *ctx = reinterpret_cast<RuntimeContextObj*>(tp_alloc(tp, 0));
            if (self != nullptr) {
                ctx->ring = ring;
            }

            return reinterpret_cast<PyObject*>(ctx);
        }

        PyMethodDef g_runtime_context_methods[] = {
            {"get", &RuntimeContextGet, METH_CLASS | METH_NOARGS, ""},
            {nullptr, nullptr, 0, nullptr}
        };

        PyType_Slot g_runtime_context_slots[] = {
            {Py_tp_new, reinterpret_cast<void*>(&RuntimeContextNew)},
            {Py_tp_methods, g_runtime_context_methods},
            {0, nullptr}
        };

        PyType_Spec g_runtime_context_spec = {
            "RuntimeContext",
            sizeof(RuntimeContextObj),
            0,
            Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
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
