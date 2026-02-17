/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/listen.h"

#include "util/python.h"

#include "module.h"

static void listen_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    ListenOperation *op = (ListenOperation *)self;

    io_uring_prep_listen(sqe, op->base.scratch, op->backlog);
}

static void listen_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->outcome);
    } else {
        assert(cqe->res == 0);
        outcome_capture(&op->outcome, Py_None);
    }
}

static OperationVTable g_listen_operation_vtable = {
    .prepare  = listen_prepare,
    .complete = listen_complete,
};

PyObject *listen_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 arguments, got %zu instead", nargs);
        return NULL;
    }

    int fd;
    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    int backlog;
    if (!python_parse_int(&backlog, args[1])) {
        return NULL;
    }

    ListenOperation *op = (ListenOperation *)operation_alloc(state->ListenOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_listen_operation_vtable;
        op->base.scratch = fd;
        op->backlog      = backlog;
    }

    return (PyObject *)op;
}

static PyType_Slot g_listen_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_listen_operation_spec = {
    .name      = "_impl._ListenOperation",
    .basicsize = sizeof(ListenOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_listen_operation_slots,
};

PyTypeObject *listen_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_listen_operation_spec, (PyObject *)state->Operation_type);
}
