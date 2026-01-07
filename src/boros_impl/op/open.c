/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/open.h"

#include <fcntl.h>
#include <liburing.h>

#include "module.h"
#include "util/python.h"

static void open_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    OpenOperation *op = (OpenOperation *)self;

    const char *pathname = PyBytes_AS_STRING(op->path);
    io_uring_prep_openat(sqe, AT_FDCWD, pathname, op->base.scratch, op->mode);
}

static void open_complete(PyObject *self, struct io_uring_cqe *cqe) {
    OpenOperation *op = (OpenOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        outcome_capture(&(op->base.outcome), PyLong_FromLong(cqe->res));
    }
}

static OperationVTable g_open_operation_vtable = {
    .prepare  = open_prepare,
    .complete = open_complete,
};

PyObject *open_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "Expected 3 arguments, got %zu instead", nargs);
        return NULL;
    }

    int flags = 0;
    if (!python_parse_int(&flags, args[1])) {
        return NULL;
    }

    unsigned int mode = 0;
    if (!python_parse_unsigned_int(&mode, args[2])) {
        return NULL;
    }

    PyObject *path;
    if (!PyUnicode_FSConverter(args[0], &path)) {
        return NULL;
    }

    OpenOperation *op = (OpenOperation *)operation_alloc(state->OpenOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_open_operation_vtable;
        op->base.scratch = flags;
        op->path         = path;
        op->mode         = mode;
    } else {
        Py_DECREF(path);
    }

    return (PyObject *)op;
}

static int open_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    OpenOperation *op = (OpenOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->path);
    return operation_traverse(&op->base, visit, arg);
}

static int open_clear_impl(PyObject *self) {
    OpenOperation *op = (OpenOperation *)self;

    Py_CLEAR(op->path);
    return operation_clear(&op->base);
}

static PyType_Slot g_open_operation_slots[] = {
    {Py_tp_traverse, open_traverse_impl},
    {Py_tp_clear, open_clear_impl},
    {0, NULL},
};

static PyType_Spec g_open_operation_spec = {
    .name      = "_impl._OpenOperation",
    .basicsize = sizeof(OpenOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_open_operation_slots,
};

PyTypeObject *open_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_open_operation_spec, (PyObject *)state->Operation_type);
}
