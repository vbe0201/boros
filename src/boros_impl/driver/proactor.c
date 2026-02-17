/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "driver/proactor.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "op/base.h"

static inline void reap_completion(TaskList *list, struct io_uring_cqe *cqe) {
    assert(cqe != NULL);

    /*
     * Extract the Operation from the completion entry and run its
     * finalizer to make the result available to the Python side.
     */
    Operation *op = (Operation *)io_uring_cqe_get_data(cqe);
    (op->vtable->complete)((PyObject *)op, cqe);
    op->state = State_Ready;

    /* Append the unblocked task to the end of the run queue. */
    task_list_push_back(list, op->awaiter);

    /*
     * The proactor holds a reference on Operation for the duration
     * of its trip through the kernel to ensure it stays alive while
     * still in use. Now we don't need it anymore.
     */
    Py_DECREF(op);
}

static inline void reap_completions(Proactor *proactor, TaskList *list) {
    unsigned int count = 0;
    unsigned head;
    struct io_uring_cqe *cqe;

    io_uring_for_each_cqe(&proactor->ring, head, cqe) {
        ++count;
        reap_completion(list, cqe);
    }

    io_uring_cq_advance(&proactor->ring, count);
    proactor->pending_events -= count;
}

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
        errno = -res;
        PyErr_SetFromErrno(PyExc_OSError);
        return res;
    }

    /* Allocate the table of direct file descriptors. */
    if (config->ftable_size > 0) {
        res = io_uring_register_files_sparse(&proactor->ring, config->ftable_size);
        if (res != 0) {
            errno = -res;
            PyErr_SetFromErrno(PyExc_OSError);
            return res;
        }

        /*
         * Register the file descriptor of the ring as a direct descriptor
         * as an optimization. This may fail if the global limit of direct
         * ring fds is already exhausted, so we just ignore errors.
         */
        if (config->ftable_size > 0) {
            (void)io_uring_register_ring_fd(&proactor->ring);
        }
    }

    proactor->pending_events = 0;
    return 0;
}

void proactor_exit(Proactor *proactor) {
    io_uring_queue_exit(&proactor->ring);
    assert(proactor->pending_events == 0);
}

int proactor_enable(Proactor *proactor) {
    int res = io_uring_enable_rings(&proactor->ring);
    if (res < 0) {
        errno = -res;
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    return 0;
}

struct io_uring_sqe *proactor_get_submission(Proactor *proactor) {
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(&proactor->ring);
    if (sqe == NULL) {
        /*
         * When the submission queue is full, the best solution is to
         * just submit operations to the kernel immediately. We will
         * inform the user though because this is usually a symptom
         * of a chronically undersized submission queue ring.
         */
        PyErr_WarnEx(PyExc_UserWarning, "Submission Queue too small. Resize it.", 1);
        // TODO: Better message.

        if (proactor_submit(proactor) != 0) {
            return NULL;
        }

        sqe = io_uring_get_sqe(&proactor->ring);
        assert(sqe != NULL);
    }

    ++proactor->pending_events;
    return sqe;
}

bool proactor_can_submit(Proactor *proactor, unsigned nentries) {
    return io_uring_sq_space_left(&proactor->ring) >= nentries;
}

int proactor_submit(Proactor *proactor) {
    while (true) {
        int res = io_uring_submit(&proactor->ring);
        if (res < 0) {
            if (res == -EINTR) {
                /*
                 * Rationale for ignoring EINTR is that there is never an
                 * easy way to recover from the interrupt in other parts
                 * of the application logic. So keep retrying and just
                 * postpone the signal handling to a later time.
                 */
                continue;
            }

            errno = -res;
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }

        return res;
    }
}

int proactor_run(Proactor *proactor, TaskList *list, unsigned long timeout) {
    int res;

    if (timeout == 0) {
        res = io_uring_submit_and_wait(&proactor->ring, 1);
    } else {
        struct io_uring_cqe *tmp;
        struct __kernel_timespec ts = {
            .tv_sec  = timeout / 1000,
            .tv_nsec = (timeout % 1000) * 1000000,
        };

        res = io_uring_submit_and_wait_timeout(&proactor->ring, &tmp, 1, &ts, NULL);
    }

    if (res < 0) {
        if (res == -ETIME || res == -EINTR) {
            return 0;
        }

        errno = res;
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    reap_completions(proactor, list);
    return 0;
}
