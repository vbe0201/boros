/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <liburing.h>

#include "context/run_config.h"
#include "task.h"

/* The proactor for handling I/O in the runtime. */
typedef struct {
    struct io_uring ring;
} Proactor;

/* Initializes the proactor with the given configuration settings. */
int proactor_init(Proactor *proactor, RunConfig *config);

/* Tears down the proactor instance. */
void proactor_exit(Proactor *proactor);

/* Marks the proactor as ready for use from the calling thread. */
int proactor_ready(Proactor *proactor);

/* Gets a submission queue entry from the proactor. */
struct io_uring_sqe *proactor_get_submission(Proactor *proactor);

/* Passes submissions to the kernel without waiting. */
int proactor_submit(Proactor *proactor);

/* Passes submissions to the kernel and waits for completions. */
int proactor_run(Proactor *proactor, unsigned long timeout);

/* Reaps completions from the proactor into the run queue. */
void proactor_reap_completions(Proactor *proactor, TaskList *queue);
