/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/recv.h"

#include <liburing.h>

#include "module.h"
#include "util/python.h"

static void recv_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    RecvOperation *op = (RecvOperation *)self;

    char *buf = PyBytes_AS_STRING(op->buf);
    io_uring_prep_recv(sqe, op->base.scratch, buf, op->nbytes, op->flags);
}

static void recv_complete(PyObject *self, struct io_uring_cqe *cqe) {
    RecvOperation *op = (RecvOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->base.outcome);
    } else {
        _PyBytes_Resize(&op->buf, cqe->res);
        outcome_capture(&op->base.outcome, Py_NewRef(op->buf));
    }
}

static OperationVTable g_recv_operation_vtable = {
    .prepare  = recv_prepare,
    .complete = recv_complete,
};

PyObject *recv_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
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

    unsigned int nbytes;
    if (!python_parse_unsigned_int(&nbytes, args[1])) {
        return NULL;
    }

    int flags;
    if (!python_parse_int(&flags, args[2])) {
        return NULL;
    }

    PyObject *buf = PyBytes_FromStringAndSize(NULL, nbytes);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }

    RecvOperation *op = (RecvOperation *)operation_alloc(state->RecvOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_recv_operation_vtable;
        op->base.scratch = fd;
        op->buf          = buf;
        op->nbytes       = nbytes;
        op->flags        = flags;
    }

    return (PyObject *)op;
}

static int recv_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    RecvOperation *op = (RecvOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->buf);
    return operation_traverse(&op->base, visit, arg);
}

static int recv_clear_impl(PyObject *self) {
    RecvOperation *op = (RecvOperation *)self;

    Py_CLEAR(op->buf);
    return operation_clear(&op->base);
}

static PyType_Slot g_recv_operation_slots[] = {
    {Py_tp_traverse, recv_traverse_impl},
    {Py_tp_clear, recv_clear_impl},
    {0, NULL},
};

static PyType_Spec g_recv_operation_spec = {
    .name      = "_impl._RecvOperation",
    .basicsize = sizeof(RecvOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_recv_operation_slots,
};

PyTypeObject *recv_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_recv_operation_spec, (PyObject *)state->Operation_type);
}
