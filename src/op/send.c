/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/send.h"

#include "util/python.h"

#include "module.h"

static void send_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    SendOperation *op = (SendOperation *)self;

    char *buf     = PyBytes_AS_STRING(op->buf);
    size_t nbytes = PyBytes_Size(op->buf);
    io_uring_prep_send(sqe, op->base.scratch, buf, nbytes, op->flags);
}

static void send_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->outcome);
    } else {
        outcome_capture(&op->outcome, PyLong_FromLong(cqe->res));
    }
}

static OperationVTable g_send_operation_vtable = {
    .prepare  = send_prepare,
    .complete = send_complete,
};

PyObject *send_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "Expected 3 arguments, got %zu instead", nargs);
        return NULL;
    }

    int fd;
    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    PyObject *buf = args[1];
    if (!PyBytes_Check(buf)) {
        PyErr_Format(PyExc_TypeError, "Expected variable of type bytes");
        return NULL;
    }

    int flags;
    if (!python_parse_int(&flags, args[2])) {
        return NULL;
    }

    SendOperation *op = (SendOperation *)operation_alloc(state->SendOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_send_operation_vtable;
        op->base.scratch = fd;
        op->buf          = Py_NewRef(buf);
        op->flags        = flags;
    }

    return (PyObject *)op;
}

static int send_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    SendOperation *op = (SendOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->buf);
    return operation_traverse(&op->base, visit, arg);
}

static int send_clear_impl(PyObject *self) {
    SendOperation *op = (SendOperation *)self;

    Py_CLEAR(op->buf);
    return operation_clear(&op->base);
}

static PyType_Slot g_send_operation_slots[] = {
    {Py_tp_traverse, send_traverse_impl},
    {Py_tp_clear, send_clear_impl},
    {0, NULL},
};

static PyType_Spec g_send_operation_spec = {
    .name      = "_impl._SendOperation",
    .basicsize = sizeof(SendOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_send_operation_slots,
};

PyTypeObject *send_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_send_operation_spec, (PyObject *)state->Operation_type);
}
