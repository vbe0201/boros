// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "wrapper/python_header.hpp"

#include <type_traits>

#include "macros.hpp"

namespace boros::python {

    /// Specifies if a C++ type matches the requirements for Python objects.
    template <typename T>
    concept PythonObject =
        std::is_same_v<T, PyObject> ||
        std::is_same_v<T, PyTypeObject> ||
        (
            std::is_standard_layout_v<T> &&
            requires (T &obj) {
                // The documentation states what PyObject_HEAD expands to.
                // It should be fine to hardcode the name.
                { obj.ob_base } -> std::same_as<PyObject&>;
            }
        );

    template <typename T> requires PythonObject<T>
    ALWAYS_INLINE auto FromPython(T *&out, PyObject *obj) noexcept -> void {
        out = reinterpret_cast<T*>(obj);
    }

    template <typename T> requires PythonObject<T>
    ALWAYS_INLINE auto ToPython(T *value) noexcept -> PyObject* {
        return reinterpret_cast<PyObject*>(value);
    }

    ALWAYS_INLINE auto FromPython(std::nullptr_t &out, PyObject *obj) noexcept -> void {
        if (obj != nullptr) [[unlikely]] {
            PyErr_SetString(PyExc_ValueError, "Expected a null pointer");
            return;
        }
        out = nullptr;
    }

    ALWAYS_INLINE auto ToPython(std::nullptr_t value) noexcept -> PyObject* {
        return value;
    }

    ALWAYS_INLINE auto FromPython(bool &out, PyObject *obj) noexcept -> void {
        if (!PyBool_Check(obj)) [[unlikely]] {
            PyErr_SetString(PyExc_TypeError, "Expected a boolean value");
            return;
        }
        out = (obj == Py_True);
    }

    ALWAYS_INLINE auto ToPython(bool value) noexcept -> PyObject* {
        return PyBool_FromLong(value);
    }

    ALWAYS_INLINE auto FromPython(long &out, PyObject *obj) noexcept -> void {
        out = PyLong_AsLong(obj);
    }

    ALWAYS_INLINE auto ToPython(long value) noexcept -> PyObject* {
        return PyLong_FromLong(value);
    }

    ALWAYS_INLINE auto FromPython(unsigned long &out, PyObject *obj) noexcept -> void {
        out = PyLong_AsUnsignedLong(obj);
    }

    ALWAYS_INLINE auto ToPython(unsigned long value) noexcept -> PyObject* {
        return PyLong_FromUnsignedLong(value);
    }

    /// Specifies if a C++ type T is convertible to and from a Python object.
    template <typename T>
    concept PythonConvertible = requires (PyObject *obj, T &out, T value) {
        { FromPython(out, obj) } noexcept -> std::same_as<void>;
        { ToPython(value)      } noexcept -> std::same_as<PyObject*>;
    };

}
