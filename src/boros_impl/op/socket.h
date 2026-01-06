/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
    /* domain is stored in base.scratch */
    Operation base;
    int type;
    int protocol;
} SocketOperation;

PyObject *socket_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *socket_operation_register(PyObject *mod);
