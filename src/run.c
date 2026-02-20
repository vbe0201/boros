/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "run.h"

static const char g_bad_yield_value_fmt[] = "Event loop received unrecognized yield value: %R. In case "
                                            "you're trying to use a library written for a different "
                                            "framework like asyncio, this will not work directly.";

static void close_coro(PyObject *coro) {
    if (coro == NULL) {
        return;
    }

    /*
     * Close the coroutine while preserving the current exception state.
     * For coroutines that already completed, this is a no-op. But for
     * coroutines that haven't been polled yet, it supresses potential
     * "coroutine was never awaited" RuntimeWarnings on GC.
     */
    PyObject *exc = PyErr_GetRaisedException();
    PyObject *res = PyObject_CallMethod(coro, "close", NULL);
    if (exc != NULL) {
        if (res == NULL) {
            PyErr_WriteUnraisable(coro);
        }

        PyErr_SetRaisedException(exc);
    }
    Py_XDECREF(res);
}

static LoopStatus event_loop_handle_yield(RunState *rs, Task *task, PyObject *value) {
    if (PyObject_TypeCheck(value, rs->state->Operation_type) != 0) {
        /*
         * I/O operations are submitted to the kernel through io_uring.
         * When a completion arrives, the pending Task will be scheduled
         * to run again.
         */
        int rc = runtime_schedule_io(rs->rt, task, (Operation *)value);
        if (rc != 0) {
            Py_DECREF(value);
            return LOOP_ERROR;
        }

        return LOOP_CONTINUE;
    } else {
        PyErr_Format(PyExc_RuntimeError, g_bad_yield_value_fmt, value);
        Py_DECREF(value);
        return LOOP_ERROR;
    }
}

static LoopStatus event_loop_handle_return(RunState *rs, Task *task, PyObject *value) {
    if (task == rs->root) {
        rs->result = value;
        return LOOP_DONE;
    } else {
        Py_XDECREF(value);
        return LOOP_CONTINUE;
    }
}

static LoopStatus event_loop_handle_error(RunState *rs, Task *task) {
    if (task == rs->root) {
        return LOOP_ERROR;
    } else {
        PyErr_WriteUnraisable((PyObject *)task);
        return LOOP_CONTINUE;
    }
}

int event_loop_init(RunState *rs, PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    rs->state  = PyModule_GetState(mod);
    rs->rt     = NULL;
    rs->root   = NULL;
    rs->result = NULL;

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs > 0) {
        PyObject *coro = args[0];
        if (!PyCoro_CheckExact(coro)) {
            PyErr_SetString(PyExc_TypeError, "Expected coroutine object");
            return -1;
        }

        rs->root = task_create(mod, NULL, coro);
        if (rs->root == NULL) {
            PyErr_SetNone(PyExc_MemoryError);
            close_coro(coro);
            return -1;
        }
    }
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 arguments, got %zu instead", nargs);
        return -1;
    }

    if (PyObject_TypeCheck(args[1], rs->state->RunConfig_type) == 0) {
        PyErr_SetString(PyExc_TypeError, "Expected RunConfig instance");
        return -1;
    }

    rs->rt = runtime_enter(rs->state, (RunConfig *)args[1]);
    if (rs->rt == NULL) {
        return -1;
    }

    task_list_push_back(&rs->rt->run_queue, rs->root);
    return 0;
}

void event_loop_finish(RunState *rs) {
    if (rs->rt != NULL) {
        runtime_exit(rs->state);
        rs->rt = NULL;
    }

    if (rs->root != NULL) {
        close_coro(rs->root->coro);
        Py_CLEAR(rs->root);
    }
}

LoopStatus event_loop_run_step(RunState *rs) {
    RuntimeHandle *rt = rs->rt;
    TaskList ready;
    LoopStatus status;
    PyObject *out;

    /*
     * Process all runnable tasks which are ready in the current loop
     * step. Tasks that only become ready during this cycle as a result
     * of running another task will need to wait for the next round.
     */
    task_list_move(&ready, &rt->run_queue);
    while (!task_list_empty(&ready)) {
        Task *task = task_list_pop_front(&ready);

        switch (PyIter_Send(task->coro, Py_None, &out)) {
        case PYGEN_NEXT:
            status = event_loop_handle_yield(rs, task, out);
            break;
        case PYGEN_RETURN:
            status = event_loop_handle_return(rs, task, out);
            break;
        case PYGEN_ERROR:
            status = event_loop_handle_error(rs, task);
            break;

        default:
            Py_UNREACHABLE();
        }

        Py_DECREF(task);

        if (status != LOOP_CONTINUE) {
            task_list_clear(&ready);
            return status;
        }
    }

    if (rt->proactor.pending_events == 0 && task_list_empty(&rt->run_queue)) {
        PyErr_SetString(PyExc_RuntimeError, "Deadlock: no pending events and no ready tasks");
        return LOOP_ERROR;
    }

    if (rt->proactor.pending_events > 0) {
        if (proactor_run(&rt->proactor, &rt->run_queue, 0) != 0) {
            return LOOP_ERROR;
        }
    }

    return LOOP_CONTINUE;
}

LoopStatus event_loop_run_loop(RunState *rs) {
    while (true) {
        LoopStatus rc = event_loop_run_step(rs);
        if (rc != LOOP_CONTINUE) {
            return rc;
        }

        if (PyErr_CheckSignals() < 0) {
            return LOOP_ERROR;
        }
    }
}

PyObject *event_loop_run(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    RunState rs;

    if (event_loop_init(&rs, mod, args, nargsf) != 0) {
        event_loop_finish(&rs);
        return NULL;
    }

    LoopStatus s = event_loop_run_loop(&rs);
    event_loop_finish(&rs);

    if (s == LOOP_DONE) {
        return rs.result;
    }

    Py_XDECREF(rs.result);
    return NULL;
}
