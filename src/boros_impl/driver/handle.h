/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "util/python.h"

#include "driver/proactor.h"
#include "driver/run_config.h"
#include "op/base.h"
#include "task.h"

/* Per-thread runtime context. */
typedef struct {
    Proactor proactor;
    TaskList run_queue;
} RuntimeHandle;

RuntimeHandle *runtime_enter(PyObject *mod, RunConfig *config);
void runtime_exit(PyObject *mod);

RuntimeHandle *runtime_get_local(PyObject *mod);

int runtime_schedule_io(RuntimeHandle *rt, Task *task, Operation *op);
