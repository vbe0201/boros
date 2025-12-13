/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "context/state.h"

#include <assert.h>
#include <errno.h>

#include "module.h"

RuntimeState *runtime_state_create(PyObject *mod, RunConfig *config) {
    ImplState *state = PyModule_GetState(mod);

    RuntimeState *rt = PyMem_Malloc(sizeof(RuntimeState));
    if (rt == NULL) {
        PyErr_SetNone(PyExc_MemoryError);
        return NULL;
    }

    task_list_init(&rt->run_queue);

    int res = proactor_init(&rt->proactor, config);
    if (res != 0) {
        errno = -res;
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    proactor_ready(&rt->proactor);

    assert(PyThread_tss_is_created(state->local_context));
    PyThread_tss_set(state->local_context, rt);

    return rt;
}

void runtime_state_destroy(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    assert(PyThread_tss_is_created(state->local_context));

    RuntimeState *rt = PyThread_tss_get(state->local_context);
    if (rt == NULL) {
        return;
    }

    proactor_exit(&rt->proactor);

    PyMem_Free(rt);
    PyThread_tss_set(state->local_context, NULL);
}

RuntimeState *runtime_state_get(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    assert(PyThread_tss_is_created(state->local_context));

    RuntimeState *rt = PyThread_tss_get(state->local_context);
    if (rt == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "No runtime active on the current thread");
        return NULL;
    }

    return rt;
}
