/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/read.h"

#include <liburing.h>

#include "util/python.h"

static void read_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    ReadOperation *op = (ReadOperation *)self;

    io_uring_prep_read(sqe, op->fd, op->buf, op->nbytes, op->offset);
}

static void read_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;
    outcome_capture(&op->outcome, PyLong_FromLong(cqe->res));
}

static OperationVTable g_read_operation_vtable = {
    .prepare  = read_prepare,
    .complete = read_complete,
};

PyObject *read_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    ImplState *state = PyModule_GetState(mod);

    if (nargs != 4) {
        PyErr_Format(PyExc_TypeError, "Expected 4 arguments, got %zu instead", nargs);
        return NULL;
    }

    int fd = 0;

    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    void *buf = PyLong_AsVoidPtr(args[1]);

    if (buf == NULL) {
        return NULL;
    }

    unsigned int nbytes = 0;

    if (!python_parse_unsigned_int(&nbytes, args[2])) {
        return NULL;
    }

    unsigned int offset = 0;

    if (!python_parse_unsigned_int(&offset, args[3])) {
        return NULL;
    }

    ReadOperation *op = (ReadOperation *)operation_alloc(state->ReadOperation_type);

    if (op != NULL) {
        op->base.vtable = &g_read_operation_vtable;
        op->fd          = fd;
        op->buf         = buf;
        op->nbytes      = nbytes;
        op->offset      = offset;
    }

    return (PyObject *)op;
}

static PyType_Slot g_read_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_read_operation_spec = {
    .name      = "_impl._ReadOperation",
    .basicsize = sizeof(ReadOperation),
    .itemsize  = 4,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_read_operation_slots,
};

PyTypeObject *read_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_read_operation_spec, (PyObject *)state->Operation_type);
}
