/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "run.h"

#include "driver/handle.h"
#include "module.h"

PyObject *boros_run(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    PyObject *out;
    ImplState *state = PyModule_GetState(mod);

    /* Make sure we received the expected number of arguments. */
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 arguments, got %zu instead", nargs);
        return NULL;
    }

    /* Parse the first argument into a coroutine object. */
    PyObject *coro = args[0];
    //if (!PyCoro_CheckExact(coro)) {
    //    PyErr_SetString(PyExc_TypeError, "Expected coroutine object");
    //    return NULL;
    //}

    /* Parse the second argument into a RunConfig-or-subclass instance. */
    RunConfig *conf = (RunConfig *)args[1];
    if (PyObject_TypeCheck((PyObject *)conf, state->RunConfig_type) == 0) {
        PyErr_SetString(PyExc_TypeError, "Expected RunConfig instance");
        return NULL;
    }

    /* Allocate a Task for our entrypoint coroutine. */
    Task *root_task = task_create(mod, NULL, coro);
    if (root_task == NULL) {
        return PyErr_NoMemory();
    }

    /* Set up the runtime state. */
    RuntimeHandle *rt = runtime_enter(mod, conf);
    if (rt == NULL) {
        Py_DECREF(root_task);
        return NULL;
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
    Py_DECREF(root_task);
    runtime_exit(mod);
    return out;
}
