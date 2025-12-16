/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

/* Representation of the io_uring socket operation. */
typedef struct {
    /* domain is stored in base.scratch */
    Operation base;
    int type;
    int protocol;
} SocketOperation;

/* Creates and returns a socket operation. */
PyObject *socket_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);

/* Registers SocketOperation as a Python class onto the module. */
PyTypeObject *socket_operation_register(PyObject *mod);
