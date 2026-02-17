/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* Configuration for setting up the runtime context. */
typedef struct {
    PyObject_HEAD
    unsigned int sq_size;
    unsigned int cq_size;
    unsigned int ftable_size;
    int wqfd;
} RunConfig;

/* Registers RunConfig as a Python class onto the module. */
PyTypeObject *run_config_register(PyObject *mod);
