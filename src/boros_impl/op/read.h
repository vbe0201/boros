/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "op/base.h"

typedef struct {
  Operation base;
  int fd;
  void *buf;
  unsigned int nbytes;
  off_t offset;
} ReadOperation;

PyObject *read_operation_create();

PyTypeObject *read_operation_register(PyObject *mod);
