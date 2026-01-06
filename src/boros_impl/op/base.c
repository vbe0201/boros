/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/base.h"

#include "module.h"
#include "util/python.h"

/* Operation implementation */

Operation *operation_alloc(PyTypeObject *tp, ImplState *state) {
    Operation *op = (Operation *)python_alloc(tp);
    if (op != NULL) {
        op->module_state = state;
        op->state        = State_Pending;
        op->awaiter      = NULL;
        outcome_init(&op->outcome);
    }

    return op;
}

int operation_traverse(Operation *self, visitproc visit, void *arg) {
    Py_VISIT(self->awaiter);

    int res = outcome_traverse(&self->outcome, visit, arg);
    if (res != 0) {
        return res;
    }

    return 0;
}

int operation_clear(Operation *self) {
    Py_CLEAR(self->awaiter);
    outcome_clear(&self->outcome);
    return 0;
}

static int operation_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(Py_TYPE(self));
    return operation_traverse((Operation *)self, visit, arg);
}

static int operation_clear_impl(PyObject *self) {
    return operation_clear((Operation *)self);
}

static PyObject *operation_await(PyObject *self) {
    Operation *op = (Operation *)self;

    /* Guard against attempts to build subclasses without a vtable. */
    if (op->vtable == NULL) {
        PyErr_SetString(PyExc_TypeError, "Operation subclasses outside of boros._impl are unsupported");
        return NULL;
    }

    OperationWaiter *waiter = (OperationWaiter *)python_alloc(op->module_state->OperationWaiter_type);
    if (waiter != NULL) {
        waiter->op = Py_NewRef(self);
    }

    return (PyObject *)waiter;
}

// clang-format off
static PyType_Slot g_operation_slots[] = {
    {Py_tp_dealloc, python_tp_dealloc},
    {Py_tp_traverse, operation_traverse_impl},
    {Py_tp_clear, operation_clear_impl},
    {Py_am_await, operation_await},
    {0, NULL},
};
// clang-format on

static PyType_Spec g_operation_spec = {
    .name      = "_impl._Operation",
    .basicsize = sizeof(Operation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_BASETYPE,
    .slots     = g_operation_slots,
};

PyTypeObject *operation_register(PyObject *mod) {
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_operation_spec, NULL);
}

/* OperationWaiter implementation */

static PyObject *operation_waiter_iternext(PyObject *self) {
    OperationWaiter *waiter = (OperationWaiter *)self;
    Operation *op           = (Operation *)waiter->op;

    switch (op->state) {
    case State_Pending:
        /*
         * This is the initial state of an Operation. On await, it
         * transitions into Blocked state and suspends the running
         * coroutine by yielding the Operation to the event loop.
         *
         * The event loop then adds the operation to the io_uring
         * submission queue and associates the submission entry
         * with the waiting Task. This allows us to add the Task
         * back to the run queue again when it becomes ready.
         */
        assert(op->awaiter == NULL);
        op->state = State_Blocked;
        return Py_NewRef(waiter->op);

    case State_Blocked:
        /*
         * In Blocked state, the Task doing the `await operation`
         * should be suspended and not executing any code while
         * it waits for io_uring to post a completion.
         *
         * If this code still runs while in this state, it means
         * the user is manually fiddling with the object and not
         * actually awaiting it. This is unsupported behavior.
         */
        PyErr_SetString(PyExc_RuntimeError, "Operation was not properly awaited");
        return NULL;

    case State_Ready:
        /*
         * In Ready state, the operation has completed and the
         * Task is woken again. This state transition is done
         * by the event loop.
         *
         * Here we have unwrap our Outcome object which stores
         * either a return value or an exception.
         */
        PyObject *res = outcome_unwrap(&op->outcome);
        if (res != NULL) {
            PyObject *args[2] = {NULL, res};
            size_t nargsf     = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;

            PyObject *exc = PyObject_Vectorcall(PyExc_StopIteration, args + 1, nargsf, NULL);
            if (exc != NULL) {
                PyErr_SetRaisedException(exc);
            }
        }

        return NULL;

    default:
        PyErr_SetString(PyExc_RuntimeError, "Corrupt operation state");
        return NULL;
    }
}

static int operation_waiter_traverse(PyObject *self, visitproc visit, void *arg) {
    OperationWaiter *waiter = (OperationWaiter *)self;
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(waiter->op);
    return 0;
}

static int operation_waiter_clear(PyObject *self) {
    OperationWaiter *waiter = (OperationWaiter *)self;
    Py_CLEAR(waiter->op);
    return 0;
}

// clang-format off
static PyType_Slot g_operation_waiter_slots[] = {
    {Py_tp_dealloc, python_tp_dealloc},
    {Py_tp_traverse, operation_waiter_traverse},
    {Py_tp_clear, operation_waiter_clear},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, operation_waiter_iternext},
    {0, NULL},
};
// clang-format on

static PyType_Spec g_operation_waiter_spec = {
    .name      = "_impl._OperationWaiter",
    .basicsize = sizeof(OperationWaiter),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_operation_waiter_slots,
};

PyTypeObject *operation_waiter_register(PyObject *mod) {
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_operation_waiter_spec, NULL);
}
