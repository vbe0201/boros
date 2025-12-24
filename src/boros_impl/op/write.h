/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
  Operation base;
  PyObject *buf;
  Py_ssize_t nbytes;
  unsigned long long offset;
} WriteOperation;

PyObject *write_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargs);

PyTypeObject *write_operation_register(PyObject *mod);
