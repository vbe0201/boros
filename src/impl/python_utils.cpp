// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "python_utils.hpp"

namespace boros::python {

    auto Module::Create(PyModuleDef &def) noexcept -> Module {
        return {PyModule_Create(&def)};
    }

    auto Module::Add(PyType_Spec &spec) noexcept -> PyObject* {
        auto *tp = PyType_FromModuleAndSpec(raw, &spec, nullptr);
        if (tp == nullptr) [[unlikely]] {
            return nullptr;
        }

        if (PyModule_AddObjectRef(raw, spec.name, tp) < 0) [[unlikely]] {
            return nullptr;
        }

        return tp;
    }

    auto WeakRef::Get() const noexcept -> PyObject* {
        if (m_ref == nullptr) {
            return nullptr;
        }

        // FIXME: This function is deprecated. We should replace it
        // with PyWeakref_GetRef but only from version 3.13 onwards.
        auto *obj = PyWeakref_GetObject(m_ref);
        if (obj != nullptr && obj != Py_None) {
            Py_INCREF(obj);
        }

        return obj;
    }

    auto WeakRef::Reset(PyObject *obj) noexcept -> void {
        this->Clear();
        if (obj != nullptr) {
            m_ref = PyWeakref_NewRef(obj, nullptr);
        }
    }

    auto WeakRef::Clear() noexcept -> void {
        Py_CLEAR(m_ref);
    }

    auto FromPython(PyTypeObject* &out, PyObject *obj) noexcept -> void {
        if (!PyType_Check(obj)) [[unlikely]] {
            PyErr_SetString(PyExc_TypeError, "Expected a type object");
            return;
        }
        out = reinterpret_cast<PyTypeObject*>(obj);
    }

    auto ToPython(PyTypeObject *value) noexcept -> PyObject* {
        return reinterpret_cast<PyObject*>(value);
    }

    auto FromPython(Module &out, PyObject *obj) noexcept -> void {
        if (!PyModule_Check(obj)) [[unlikely]] {
            PyErr_SetString(PyExc_TypeError, "Expected a module object");
            return;
        }
        out = Module{obj};
    }

    auto ToPython(Module value) noexcept -> PyObject* {
        return value.raw;
    }

    auto FromPython(std::nullptr_t &out, PyObject *obj) noexcept -> void {
        if (obj != nullptr) [[unlikely]] {
            PyErr_SetString(PyExc_ValueError, "Expected a null pointer");
            return;
        }
        out = nullptr;
    }

    auto ToPython(std::nullptr_t value) noexcept -> PyObject* {
        return value;
    }

    auto FromPython(bool &out, PyObject *obj) noexcept -> void {
        if (!PyBool_Check(obj)) [[unlikely]] {
            PyErr_SetString(PyExc_TypeError, "Expected a boolean value");
            return;
        }
        out = (obj == Py_True);
    }

    auto ToPython(bool value) noexcept -> PyObject* {
        return PyBool_FromLong(value);
    }

    auto FromPython(long &out, PyObject *obj) noexcept -> void {
        out = PyLong_AsLong(obj);
    }

    auto ToPython(long value) noexcept -> PyObject* {
        return PyLong_FromLong(value);
    }

    auto FromPython(unsigned long &out, PyObject *obj) noexcept -> void {
        out = PyLong_AsUnsignedLong(obj);
    }

    auto ToPython(unsigned long value) noexcept -> PyObject* {
        return PyLong_FromUnsignedLong(value);
    }

}
