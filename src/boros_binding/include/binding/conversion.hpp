// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

#include <concepts>

#include "object.hpp"

namespace boros::python {

    // Defines some machinery for converting between C++ and Python.
    //
    // This removes the boilerplate that comes with argument parsing
    // for functions written in C++. Thus, these functions should be
    // written for universal usefulness.
    //
    // # API breakdown
    //
    // The FromPython() functions define conversion behavior for a
    // PyObject* to a requested output type (provided as an output
    // pointer).
    //
    // The ToPython() functions define conversion behavior from C++
    // values to PyObject* values.

    inline bool FromPython(PyObject **out, PyObject *obj) {
        *out = obj;
        return true;
    }

    inline PyObject *ToPython(PyObject *value) {
        return value;
    }

    template <typename T>
    inline PyObject *ToPython(ObjectRef<T>*value) {
        return *value;
    }

    inline bool FromPython(bool *out, PyObject *obj) {
        if (!PyBool_Check(obj)) {
            PyErr_SetString(PyExc_TypeError, "Expected a boolean value");
            return false;
        }

        *out = (obj == Py_True);
        return true;
    }

    inline PyObject *ToPython(bool value) {
        return PyBool_FromLong(value);
    }

    template <std::signed_integral I>
    inline bool FromPython(I *out, PyObject *obj) {
        constexpr long long MinValue = std::numeric_limits<I>::min();
        constexpr long long MaxValue = std::numeric_limits<I>::max();

        int overflow;
        long long tmp = PyLong_AsLongLongAndOverflow(obj, &overflow);
        if (overflow != 0 || tmp < MinValue || tmp > MaxValue) {
            PyErr_SetString(PyExc_OverflowError, "Python int too large to convert to C int");
            return false;
        }

        *out = static_cast<I>(tmp);
        return true;
    }

    template <std::signed_integral I>
    inline PyObject *ToPython(I value) {
        return PyLong_FromLongLong(value);
    }

    template <std::unsigned_integral I>
    inline bool FromPython(I *out, PyObject *obj) {
        constexpr unsigned long long MaxValue = std::numeric_limits<I>::max();

        unsigned long long tmp = PyLong_AsUnsignedLongLong(obj);
        if (tmp > MaxValue) {
            if (PyErr_Occurred() == nullptr) {
                PyErr_SetString(PyExc_OverflowError, "Python int too large to convert to C int");
            }
            return false;
        }

        *out = static_cast<I>(tmp);
        return true;
    }

    template <std::unsigned_integral I>
    inline PyObject *ToPython(I value) {
        return PyLong_FromUnsignedLongLong(value);
    }

}  // namespace boros::python
