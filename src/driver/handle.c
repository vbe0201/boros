/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "driver/handle.h"

#include <assert.h>

static inline void runtime_destroy(RuntimeHandle *handle);

static inline RuntimeHandle *runtime_create(RunConfig *config) {
    RuntimeHandle *handle = PyMem_Malloc(sizeof(RuntimeHandle));
    if (handle == NULL) {
        PyErr_SetNone(PyExc_MemoryError);
        return NULL;
    }

    if (proactor_init(&handle->proactor, config) != 0) {
        PyMem_Free(handle);
        return NULL;
    }
    task_list_init(&handle->run_queue);

    if (proactor_enable(&handle->proactor) != 0) {
        runtime_destroy(handle);
        return NULL;
    }

    return handle;
}

static inline void runtime_destroy(RuntimeHandle *handle) {
    task_list_clear(&handle->run_queue);
    proactor_exit(&handle->proactor);

    PyMem_Free(handle);
}

RuntimeHandle *runtime_enter(ImplState *state, RunConfig *config) {
    RuntimeHandle *handle;

    assert(PyThread_tss_is_created(state->local_handle));

    handle = PyThread_tss_get(state->local_handle);
    if (handle != NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Runtime is already active on the current thread");
        return NULL;
    }

    handle = runtime_create(config);
    if (handle == NULL) {
        return NULL;
    }

    PyThread_tss_set(state->local_handle, handle);
    return handle;
}

void runtime_exit(ImplState *state) {
    RuntimeHandle *handle = PyThread_tss_get(state->local_handle);
    if (handle == NULL) {
        return;
    }

    runtime_destroy(handle);
    PyThread_tss_set(state->local_handle, NULL);
}

RuntimeHandle *runtime_get_local(ImplState *state) {
    RuntimeHandle *handle = PyThread_tss_get(state->local_handle);
    if (handle == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "No runtime active on the current thread");
        return NULL;
    }

    return handle;
}

int runtime_schedule_io(RuntimeHandle *rt, Task *task, Operation *op) {
    struct io_uring_sqe *sqe;

    sqe = proactor_get_submission(&rt->proactor);
    if (sqe == NULL) {
        return -1;
    }

    /*
     * Associate the kernel submission with the corresponding Operation.
     * This allows us to retrieve the Operation (and its awaiter) back
     * when the completion for this operation arrives.
     */
    op->awaiter = (Task *)Py_NewRef((PyObject *)task);
    (op->vtable->prepare)((PyObject *)op, sqe);
    io_uring_sqe_set_data(sqe, op);

    return 0;
}
