// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "wrapper/python_header.hpp"

#include "wrapper/conversion.hpp"

namespace boros::python {

    /// Allocates a new Python object from the given type object.
    /// This is suitable for implementing a type's __new__.
    template <typename T> requires PythonObject<T>
    auto Alloc(PyTypeObject *tp) noexcept -> T* {
        auto *tp_alloc = reinterpret_cast<allocfunc>(PyType_GetSlot(tp, Py_tp_alloc));

        auto *self = reinterpret_cast<T*>(tp_alloc(tp, 0));
        if (self != nullptr) [[likely]] {
            // Make sure the constructor does not destroy the state
            // of the Python object header by backing it up.
            auto base = self->ob_base;
            std::construct_at(self);
            self->ob_base = base;
        }

        return self;
    }

    /// Implements __new__ for a function by allocating an instance
    /// and default-constructing it with its C++ constructor.
    template <typename T> requires PythonObject<T>
    auto DefaultNew(PyTypeObject *tp, PyObject *args, PyObject *kwds) noexcept -> PyObject* {
        BOROS_UNUSED(args, kwds);
        return reinterpret_cast<PyObject*>(Alloc<T>(tp));
    }

    /// Deallocates a Python object by calling its C++ destructor,
    /// then freeing the allocation.
    template <typename T> requires PythonObject<T>
    auto DefaultDealloc(PyObject *self) noexcept -> void {
        PyTypeObject *tp = Py_TYPE(self);
        auto *tp_free = reinterpret_cast<freefunc>(PyType_GetSlot(tp, Py_tp_free));

        std::destroy_at(reinterpret_cast<T*>(self));
        tp_free(self);

        Py_DECREF(tp);
    }

}
