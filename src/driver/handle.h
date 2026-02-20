/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "util/python.h"

#include "driver/proactor.h"
#include "driver/run_config.h"
#include "module.h"
#include "op/base.h"
#include "task.h"

/* Per-thread runtime context. */
typedef struct {
    Proactor proactor;
    TaskList run_queue;
} RuntimeHandle;

RuntimeHandle *runtime_enter(ImplState *state, RunConfig *config);
void runtime_exit(ImplState *state);

RuntimeHandle *runtime_get_local(ImplState *state);

int runtime_schedule_io(RuntimeHandle *rt, Task *task, Operation *op);
