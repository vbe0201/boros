/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/write.h"

#include <liburing.h>

#include "module.h"
#include "util/python.h"

static void write_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    WriteOperation *op = (WriteOperation *)self;

    char *buf     = PyBytes_AS_STRING(op->buf);
    size_t nbytes = PyBytes_Size(op->buf);
    io_uring_prep_write(sqe, op->base.scratch, buf, nbytes, op->offset);
}

static void write_complete(PyObject *self, struct io_uring_cqe *cqe) {
    WriteOperation *op = (WriteOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        outcome_capture(&(op->base.outcome), PyLong_FromLong(cqe->res));
    }
}

static OperationVTable g_write_operation_vtable = {
    .prepare  = write_prepare,
    .complete = write_complete,
};

PyObject *write_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
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

    unsigned long long offset;
    if (!python_parse_unsigned_long_long(&offset, args[2])) {
        return NULL;
    }

    WriteOperation *op = (WriteOperation *)operation_alloc(state->WriteOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_write_operation_vtable;
        op->base.scratch = fd;
        op->buf          = Py_NewRef(buf);
        op->offset       = offset;
    }

    return (PyObject *)op;
}

static int write_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    WriteOperation *op = (WriteOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->buf);
    return operation_traverse(&op->base, visit, arg);
}

static int write_clear_impl(PyObject *self) {
    WriteOperation *op = (WriteOperation *)self;

    Py_CLEAR(op->buf);
    return operation_clear(&op->base);
}

static PyType_Slot g_write_operation_slots[] = {
    {Py_tp_traverse, write_traverse_impl},
    {Py_tp_clear, write_clear_impl},
    {0, NULL},
};

static PyType_Spec g_write_operation_spec = {
    .name      = "_impl._WriteOperation",
    .basicsize = sizeof(WriteOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_write_operation_slots,
};

PyTypeObject *write_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_write_operation_spec, (PyObject *)state->Operation_type);
}
