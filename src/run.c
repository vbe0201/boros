/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "run.h"

#include "driver/handle.h"
#include "module.h"

PyObject *boros_run(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);
    PyObject *coro   = NULL;
    RunConfig *conf  = NULL;
    Task *root_task  = NULL;
    PyObject *out    = NULL;

    /* Make sure we received the expected number of arguments. */
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        if (nargs != 0) {
            coro = args[0];
        }

        PyErr_Format(PyExc_TypeError, "Expected 2 arguments, got %zu instead", nargs);
        goto cleanup;
    }

    /* Parse the first argument into a coroutine object. */
    coro = args[0];
    if (!PyCoro_CheckExact(coro)) {
        PyErr_SetString(PyExc_TypeError, "Expected coroutine object");
        goto cleanup;
    }

    /* Parse the second argument into a RunConfig-or-subclass instance. */
    conf = (RunConfig *)args[1];
    if (PyObject_TypeCheck((PyObject *)conf, state->RunConfig_type) == 0) {
        PyErr_SetString(PyExc_TypeError, "Expected RunConfig instance");
        goto cleanup;
    }

    /* Allocate a Task for our entrypoint coroutine. */
    root_task = task_create(mod, NULL, coro);
    if (root_task == NULL) {
        PyErr_SetNone(PyExc_MemoryError);
        goto cleanup;
    }

    /* Set up the runtime state. */
    RuntimeHandle *rt = runtime_enter(mod, conf);
    if (rt == NULL) {
        goto cleanup;
    }

    // TODO: Run queue is unbounded because running a task could add more
    // tasks to the back of the queue. This could lead to starvation issues.
    task_list_push_back(&rt->run_queue, root_task);
    while (!task_list_empty(&rt->run_queue)) {
        Task *current = task_list_front(&rt->run_queue);
        task_list_pop_front(&rt->run_queue);

        switch (PyIter_Send(current->coro, Py_None, &out)) {
        case PYGEN_NEXT:
            // TODO: This code should probably set the next send value for PyIter_Send
            //  --> I don't think we even have PyIter_Send values. Operation just unwraps
            //      its outcome when it's resumed again
            if (PyObject_TypeCheck(out, state->Operation_type) != 0) {
                runtime_schedule_io(rt, current, (Operation *)out);
                // TODO: Error handling
            } else {
                PyErr_Format(PyExc_RuntimeError, "Task got bad yield value: %R", out);

                Py_DECREF(out);
                out = NULL;
                goto end;
            }

            break;

        case PYGEN_RETURN:
        case PYGEN_ERROR:
            // TODO: This is not strictly the main task exiting here. Unconditional
            // goto works for now but becomes a bug in the future.
            // Py_DECREF(current);
            goto end;
        }

        // TODO: Error handling
        proactor_run(&rt->proactor, &rt->run_queue, 0);
    }

end:
    runtime_exit(mod);

cleanup:
    /*
     * Close the coroutine so Python does not emit a "coroutine was never
     * awaited" RuntimeWarning when it is garbage-collected.  For a coroutine
     * that already ran to completion this is a harmless no-op.
     */
    if (coro != NULL) {
        PyObject *exc = PyErr_GetRaisedException();
        PyObject *r   = PyObject_CallMethod(coro, "close", NULL);
        if (r == NULL) {
            PyErr_WriteUnraisable(coro);
        } else {
            Py_DECREF(r);
        }
        PyErr_SetRaisedException(exc);
    }

    Py_XDECREF(root_task);
    return out;
}
