/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "util/python.h"

#include <liburing.h>

#include "driver/run_config.h"
#include "task.h"

typedef struct {
    struct io_uring ring;
    size_t pending_events;
} Proactor;

int proactor_init(Proactor *proactor, RunConfig *config);
void proactor_exit(Proactor *proactor);
int proactor_enable(Proactor *proactor);

bool proactor_can_submit(Proactor *proactor, unsigned nentries);
struct io_uring_sqe *proactor_get_submission(Proactor *proactor);
int proactor_submit(Proactor *proactor);

int proactor_run(Proactor *proactor, TaskList *list, unsigned long timeout);
