/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "io/proactor.h"

#include <assert.h>
#include <string.h>

#include "op/base.h"

int proactor_init(Proactor *proactor, RunConfig *config) {
    int res;
    struct io_uring_params p;

    memset(&p, 0, sizeof(p));

    /*
     * Clamp submission queue size at the maximum number of entries.
     * Reduces configuration errors with invalid parameters.
     */
    p.flags |= IORING_SETUP_CLAMP;
    /*
     * Create the ring in disabled state by default. Allows for some
     * additional setup before submissions are allowed.
     */
    p.flags |= IORING_SETUP_R_DISABLED;
    /*
     * Submit all requests to the kernel even when one of them fails
     * inline. We do not care about that since we still receive a
     * completion event with the error and can then handle it.
     */
    p.flags |= IORING_SETUP_SUBMIT_ALL;
    /*
     * Decouple async event reaping and retrying from regular system
     * calls. If this isn't set, then io_uring uses normal task_work
     * for this and we could end up running that way too often. This
     * flag defers task_work to when the event loop enters the kernel
     * anyway to wait for new events.
     */
    p.flags |= IORING_SETUP_DEFER_TASKRUN;
    /*
     * Inform the kernel that only a single thread submits to the ring.
     * This enables internal performance optimizations since our ring
     * is only designed for single-threaded usage anyway.
     */
    p.flags |= IORING_SETUP_SINGLE_ISSUER;

    /* Configure a completion queue size, if given. */
    if (config->cq_size != 0 && config->sq_size != config->cq_size) {
        p.flags |= IORING_SETUP_CQSIZE;
        p.cq_entries = config->cq_size;
    }

    /* Share another ring's work queue, if specified. */
    if (config->wqfd != -1) {
        p.flags |= IORING_SETUP_ATTACH_WQ;
        p.wq_fd = config->wqfd;
    }

    /* Initialize the ring instance. */
    res = io_uring_queue_init_params(config->sq_size, &proactor->ring, &p);
    if (res != 0) {
        return res;
    }

    /*
     * Register the file descriptor of the ring as a direct descriptor
     * as an optimization. This may fail if the global limit of direct
     * ring fds is already exhausted, so we just ignore errors.
     */
    (void)io_uring_register_ring_fd(&proactor->ring);

    return 0;
}

void proactor_exit(Proactor *proactor) {
    io_uring_queue_exit(&proactor->ring);
}

int proactor_ready(Proactor *proactor) {
    return io_uring_enable_rings(&proactor->ring);
}

struct io_uring_sqe *proactor_get_submission(Proactor *proactor) {
    return io_uring_get_sqe(&proactor->ring);
}

int proactor_submit(Proactor *proactor) {
    return io_uring_submit(&proactor->ring);
}

int proactor_run(Proactor *proactor, unsigned long timeout) {
    if (timeout == 0) {
        return io_uring_submit_and_wait(&proactor->ring, 1);
    } else {
        struct io_uring_cqe *tmp;
        struct __kernel_timespec ts = {
            .tv_sec  = timeout / 1000,
            .tv_nsec = (timeout % 1000) * 1000000,
        };

        return io_uring_submit_and_wait_timeout(&proactor->ring, &tmp, 1, &ts, NULL);
    }
}

void proactor_reap_completions(Proactor *proactor, TaskList *queue) {
    unsigned int count = 0;
    unsigned head;
    struct io_uring_cqe *cqe;

    io_uring_for_each_cqe(&proactor->ring, head, cqe) {
        ++count;

        /* Extract the Operation attachment from the completion. */
        Operation *op = (Operation *)io_uring_cqe_get_data(cqe);
        assert(op != NULL);

        /* Run the completion hook to store the result. */
        (op->vtable->complete)((PyObject *)op, cqe);
        op->state = State_Ready;

        /* Append the waiting Task to the back of the run queue. */
        task_list_push_back(queue, op->awaiter);

        Py_DECREF(op);
    }

    io_uring_cq_advance(&proactor->ring, count);
}
