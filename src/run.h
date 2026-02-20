/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "util/python.h"

#include "driver/handle.h"
#include "module.h"
#include "task.h"

typedef struct {
    ImplState *state;
    RuntimeHandle *rt;
    Task *root;
    PyObject *result;
} RunState;

typedef enum {
    LOOP_ERROR    = -1,
    LOOP_CONTINUE = 0,
    LOOP_DONE     = 1,
} LoopStatus;

int event_loop_init(RunState *rs, PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
void event_loop_finish(RunState *rs);

LoopStatus event_loop_run_step(RunState *rs);
LoopStatus event_loop_run_loop(RunState *rs);

PyObject *event_loop_run(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
