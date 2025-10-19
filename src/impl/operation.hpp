// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

#include <linux/io_uring.h>

namespace boros::impl {

    /// A Python structure describing a low-level I/O operation on the
    /// runtime and allowing to await the completion of that operation.
    struct OperationObj {
        PyObject_HEAD
        io_uring_sqe *sqe;

        /// Makes the Operation class accessible to Python code through
        /// the given module object.
        static auto Register(PyObject *module) noexcept -> PyObject*;
    };

}
