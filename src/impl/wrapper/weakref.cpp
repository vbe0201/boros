// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "wrapper/weakref.hpp"

namespace boros::python {

    auto WeakReference::Get() const noexcept -> PyObject* {
        if (m_ref == nullptr) {
            return nullptr;
        }

        PyObject *obj = PyWeakref_GetObject(m_ref);
        if (obj != nullptr && obj != Py_None) {
            Py_INCREF(obj);
        }

        return obj;
    }

    auto WeakReference::Reset(PyObject *obj) noexcept -> void {
        this->Clear();
        if (obj != nullptr) {
            m_ref = PyWeakref_NewRef(obj, nullptr);
        }
    }

    auto WeakReference::Clear() noexcept -> void {
        Py_CLEAR(m_ref);
    }

}
