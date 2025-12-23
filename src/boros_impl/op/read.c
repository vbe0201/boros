/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/read.h"

#include <liburing.h>

#include "bytesobject.h"
#include "pyerrors.h"
#include "util/python.h"
#include "module.h"

static void read_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    ReadOperation *op = (ReadOperation *)self;

    char *buf;

    buf = PyBytes_AsString(op->buf);

    io_uring_prep_read(sqe, op->base.scratch, buf, op->nbytes, op->offset);
}

static void read_complete(PyObject *self, struct io_uring_cqe *cqe) {
    ReadOperation *op = (ReadOperation *)self;
    
    if (cqe->res < 0) {
        outcome_capture_errno(-cqe->res);
    } else {
        _PyBytes_Resize(&op->buf, cqe->res);
        outcome_capture(&op->outcome, op->buf);
    }
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

    unsigned int nbytes = 0;

    if (!python_parse_unsigned_int(&nbytes, args[2])) {
        return NULL;
    }

    unsigned long long offset = 0;

    if (!python_parse_unsigned_long_long(&offset, args[3])) {
        return NULL;
    }

    PyObject *buf;
    buf = PyBytes_FromStringAndSize(NULL, nbytes);
    
    if (buf != NULL) {
        PyErr_Format(PyExc_OSError, "Couldn't create buffer with size %zu", nbytes);
        return NULL;
    }

    ReadOperation *op = (ReadOperation *)operation_alloc(state->ReadOperation_type);

    if (op != NULL) {
        op->base.vtable  = &g_read_operation_vtable;
        op->base.scratch = fd;
        op->buf          = buf;
        op->nbytes       = nbytes;
        op->offset       = offset;
    }

    return (PyObject *)op;
}

static int read_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(Py_TYPE(self));

    Py_VISIT(((ReadOperation*)self)->buf);

    return operation_traverse((ReadOperation *)self, visit, arg);
}
static int read_clear_impl(PyObject *self) {
    Py_CLEAR(self->buf);
    
    return operation_clear((ReadOperation *)self);
}

static PyType_Slot g_read_operation_slots[] = {
    {Py_tp_traverse, read_traverse_impl},
    {Py_tp_clear, read_clear_impl},
    {0, NULL},
};

static PyType_Spec g_read_operation_spec = {
    .name      = "_impl._ReadOperation",
    .basicsize = sizeof(ReadOperation),
    .itemsize  = 4,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_read_operation_slots,
};

PyTypeObject *read_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_read_operation_spec, (PyObject *)state->Operation_type);
}
