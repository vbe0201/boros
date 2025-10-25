// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "python_utils.hpp"

#include <linux/io_uring.h>
#include <type_traits>

namespace boros {

    /// A Python structure describing a low-level I/O operation on the
    /// runtime and allowing to await the completion of that operation.
    struct Operation {
        PyObject_HEAD
        io_uring_sqe *sqe;

        /// The implementation of the __new__ operator for this class.
        static auto New(PyTypeObject *tp, PyObject *args, PyObject *kwds) noexcept -> PyObject*;

        /// Exposes the Operation class to a given Python module.
        static auto Register(python::Module mod) noexcept -> PyObject*;
    };

    static_assert(python::PythonObject<Operation>);

}
