/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "context/run_config.h"
#include "io/proactor.h"
#include "task.h"

/* Per-thread runtime state. */
typedef struct {
    TaskList run_queue;
    Proactor proactor;
} RuntimeState;

/* Creates the thread-local state from the given config. */
RuntimeState *runtime_state_create(PyObject *mod, RunConfig *config);

/* Destroys the thread-local state. */
void runtime_state_destroy(PyObject *mod);

/* Gets the active runtime state on the current thread. */
RuntimeState *runtime_state_get(PyObject *mod);
