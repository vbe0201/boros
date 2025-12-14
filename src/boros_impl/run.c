/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "run.h"

#include "context/state.h"
#include "module.h"
#include "op/base.h"

static struct io_uring_sqe *get_submission(RuntimeState *rt) {
    struct io_uring_sqe *sqe;

    sqe = proactor_get_submission(&rt->proactor);
    if (sqe == NULL) {
        /*
         * When the submission queue is full, the best solution is to
         * just submit operations to the kernel to make space. We will
         * inform the user though because this usually only happens
         * with a chronically undersized submission queue ring.
         */
        PyErr_WarnEx(PyExc_UserWarning, "Submission Queue too small. Resize it.", 1);
        // TODO: More detailed warning message.

        proactor_submit(&rt->proactor);
        // TODO: Error handling for submit

        sqe = proactor_get_submission(&rt->proactor);
        assert(sqe != NULL);
    }

    return sqe;
}

static void schedule_io(RuntimeState *rt, Task *task, Operation *op) {
    struct io_uring_sqe *sqe = get_submission(rt);

    /*
     * Prepare the operation with its designated setup callback and
     * attach the Operation as a marker to the submission. After a
     * kernel roundtrip, we will get the same Operation reference
     * back in the completion and can unblock the waiting Task.
     */
    op->awaiter = (Task *)Py_NewRef((PyObject *)task);
    (op->vtable->prepare)((PyObject *)op, sqe);
    io_uring_sqe_set_data(sqe, op);
}

PyObject *boros_run(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 arguments, got %zu instead", nargs);
        return NULL;
    }

    PyObject *coro = args[0];
    // TODO: Can we validate coro with stable ABI somehow?

    RunConfig *conf = (RunConfig *)args[1];
    // TODO: isinstance check for RunConfig

    RuntimeState *rt = runtime_state_create(mod, conf);
    if (rt == NULL) {
        return NULL;
    }

    Task *root_task = task_create(mod, NULL, coro);
    // TODO: Allocation can fail, handle the error.
    task_list_push_back(&rt->run_queue, root_task);

    PyObject *out;
    // TODO: Run queue is unbounded because running a task could add more
    // tasks to the back of the queue. This could lead to starvation issues.
    // TODO: We might want better scheduling so the order appears random.
    while (!task_list_empty(&rt->run_queue)) {
        Task *current = task_list_front(&rt->run_queue);
        task_list_pop_front(&rt->run_queue);

        switch (PyIter_Send(current->coro, Py_None, &out)) {
        case PYGEN_NEXT:
            // TODO: isinstance can still fail so this needs error handling
            // TODO: This code should probably set the next send value for PyIter_Send
            if (PyObject_IsInstance(out, (PyObject *)state->Operation_type)) {
                schedule_io(rt, current, (Operation *)out);
            }

            break;

        case PYGEN_RETURN:
        case PYGEN_ERROR:
            // TODO: This is not strictly the main task exiting here. Unconditional
            // goto works for now but becomes a bug in the future.
            Py_DECREF(current);
            goto end;
        }

        // TODO: Error handling
        proactor_run(&rt->proactor, 0);
        // TODO: Do we have to decrement the reference count of operations we reap??
        proactor_reap_completions(&rt->proactor, &rt->run_queue);
    }

end:
    runtime_state_destroy(mod);
    return out;
}
