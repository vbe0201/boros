// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "operation.hpp"

#include "runtime.hpp"

namespace boros::impl {

    namespace {

        extern "C" auto OperationNew(PyTypeObject *tp, PyObject *args, PyObject *kwds) noexcept -> PyObject* {
            auto *tp_alloc = reinterpret_cast<allocfunc>(PyType_GetSlot(tp, Py_tp_alloc));

            auto *op = reinterpret_cast<OperationObj*>(tp_alloc(tp, 0));
            if (op != nullptr) [[likely]] {
                op->sqe = nullptr;
            }

            return reinterpret_cast<PyObject*>(op);
        }

        PyMethodDef g_operation_methods[] = {
            {nullptr, nullptr, 0, nullptr}
        };

        PyType_Slot g_operation_slots[] = {
            {Py_tp_new, reinterpret_cast<void*>(&OperationNew)},
            {0, nullptr}
        };

        PyType_Spec g_operation_spec = {
            "Operation",
            sizeof(OperationObj),
            0,
            Py_TPFLAGS_DEFAULT,
            g_operation_slots
        };

    }

    auto OperationObj::Register(PyObject *module) noexcept -> PyObject* {
        auto *cls = PyType_FromModuleAndSpec(module, &g_operation_spec, nullptr);
        if (cls == nullptr) [[unlikely]] {
            return nullptr;
        }

        if (PyModule_AddObjectRef(module, "Operation", cls) < 0) [[unlikely]] {
            return nullptr;
        }

        return cls;
    }

}
