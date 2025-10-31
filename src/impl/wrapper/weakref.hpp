// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "wrapper/python_header.hpp"

#include "macros.hpp"

namespace boros::python {

    /// A RAII wrapper around a weak reference to a Python object.
    struct WeakReference {
    private:
        PyObject *m_ref;

    public:
        ALWAYS_INLINE WeakReference() noexcept : m_ref(nullptr) {}

        ALWAYS_INLINE ~WeakReference() noexcept {
            this->Clear();
        }

        WeakReference(const WeakReference&) = delete;
        auto operator=(const WeakReference&) -> WeakReference& = delete;

        ALWAYS_INLINE WeakReference(WeakReference &&rhs) noexcept : m_ref(rhs.m_ref) {
            rhs.m_ref = nullptr;
        }

        ALWAYS_INLINE auto operator=(WeakReference &&rhs) noexcept -> WeakReference& {
            if (this != &rhs) {
                Py_XDECREF(m_ref);
                m_ref = rhs.m_ref;
                rhs.m_ref = nullptr;
            }
            return *this;
        }

        /// Gets a strong reference to the object, if possible.
        auto Get() const noexcept -> PyObject*;

        /// Resets the reference to a new referenced object.
        auto Reset(PyObject *obj) noexcept -> void;

        /// Releases the held reference.
        auto Clear() noexcept -> void;
    };

}
