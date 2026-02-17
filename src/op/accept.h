/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* fd is stored in base.scratch */
    Operation base;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    int flags;
} AcceptOperation;

PyObject *accept_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *accept_operation_register(PyObject *mod);
