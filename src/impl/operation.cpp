// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "operation.hpp"

#include "runtime.hpp"

namespace boros {

    auto Operation::New(PyTypeObject *tp, PyObject *args, PyObject *kwds) noexcept -> PyObject* {
        BOROS_UNUSED(args, kwds);

        auto *self = python::New<Operation>(tp);
        if (self != nullptr) [[likely]] {
            self->sqe = nullptr;
        }

        return python::ToPython(self);
    }

    namespace {

        auto g_operation_methods = python::MethodTable();

        auto g_operation_slots = python::TypeSlotTable(
            python::TypeSlot(Py_tp_new, &Operation::New),
            python::TypeSlot(Py_tp_methods, g_operation_methods.data())
        );

        auto g_operation_spec = python::TypeSpec<Operation>(
            "Operation",
            g_operation_slots.data()
        );

    }

    auto Operation::Register(python::Module mod) noexcept -> PyObject* {
        return mod.Add(g_operation_spec);
    }

}
