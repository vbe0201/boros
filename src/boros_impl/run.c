/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "run.h"

#include <stdio.h>

#include "context/state.h"
#include "module.h"
#include "op/base.h"

/*PyObject *boros_run(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);
    PyObject *out;
    PyObject *send = Py_None;

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 arguments, got %zu instead", nargs);
        return NULL;
    }

    PyObject *coro = args[0];
    // TODO: Can we validate this somehow?

    int res = PyObject_IsInstance(args[1], (PyObject *)state->RunConfig_type);
    if (res != 1) {
        if (res == 0) {
            PyErr_SetString(PyExc_ValueError, "Expected a RunConfig instance");
        }
        return NULL;
    }

    RunConfig *conf = (RunConfig *)args[1];

    RuntimeState *rt = runtime_state_create(mod, conf);
    if (rt == NULL) {
        return NULL;
    }

    Task *root_task = task_alloc(mod, Py_None, coro);
    if (root_task == NULL) {
        out = NULL;
        goto end;
    }
    task_list_push_back(&rt->run_queue, root_task);

    while (!task_list_empty(&rt->run_queue)) {
        Task *task = task_list_front(&rt->run_queue);
        task_list_pop_front(&rt->run_queue);

        fprintf(stderr, "Running loop\n");
        fprintf(stderr, "got task %p\n", task);
        PySendResult sres = PyIter_Send(task->coro, Py_NewRef(send), &out);
        fprintf(stderr, "sent\n");
        switch (sres) {
        case PYGEN_NEXT:
            printf("task %p yielded\n", task);
            if (PyObject_IsInstance(out, (PyObject *)state->Operation_type)) {
                struct io_uring_sqe *sqe = proactor_get_submission(&rt->proactor);
                if (sqe == NULL) {
                    PyErr_WarnEx(PyExc_UserWarning, "Submission Queue too small. Resize it.", 1);
                    proactor_submit(&rt->proactor);
                    sqe = proactor_get_submission(&rt->proactor);
                    assert(sqe != NULL);
                }

                Operation *op = (Operation *)out;
                op->awaiter   = task;
                (op->vtable->prepare)(out, sqe);
                io_uring_sqe_set_data(sqe, op);
            } else {
                PyErr_SetString(PyExc_RuntimeError, "Unrecognized yield value");
                out = NULL;
                goto end;
            }

            break;

        case PYGEN_RETURN:
        case PYGEN_ERROR:
            goto end;
        }

        proactor_run(&rt->proactor, 0);
        proactor_reap_completions(&rt->proactor, &rt->run_queue);
    }

end:
    Py_CLEAR(send);
    runtime_state_destroy(mod);
    return out;
}*/

PyObject *boros_run(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    PyObject *coro = args[0];

    TaskList list;
    task_list_init(&list);

    Py_INCREF(coro);
    Task *task = task_alloc(mod, NULL, coro);
    task_list_push_back(&list, task);

    PyObject *out;
    while (!task_list_empty(&list)) {
        fprintf(stderr, "iter\n");
        Task *current = task_list_front(&list);
        task_list_pop_front(&list);
        fprintf(stderr, "task: %p, coro: %p\n", current, current->coro);

        switch (PyIter_Send(current->coro, Py_NewRef(Py_None), &out)) {
        case PYGEN_NEXT:
            fprintf(stderr, "coro yielded\n");

            Operation *op = (Operation *)out;
            fprintf(stderr, "op: %p\n", op);
            struct io_uring_cqe cqe = {.res = 1};
            (op->vtable->complete)(out, &cqe);
            fprintf(stderr, "just to be sure, it's not the vtable right?\n");

            break;

        case PYGEN_RETURN:
            fprintf(stderr, "success\n");
            return out;

        case PYGEN_ERROR:
            fprintf(stderr, "error?\n");
            //Py_DECREF(task);
            fprintf(stderr, "%i\n", out == NULL);
            PyErr_Clear();
            Py_RETURN_NONE;
        }

        task_list_push_back(&list, current);
    }

    Py_RETURN_NONE;
}
