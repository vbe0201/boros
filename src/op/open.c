/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/open.h"

#include "util/python.h"

#include <fcntl.h>

#include "module.h"

static void openat_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    OpenAtOperation *op = (OpenAtOperation *)self;

    const char *pathname = PyBytes_AS_STRING(op->path);
    io_uring_prep_openat(sqe, op->dfd, pathname, op->base.scratch, op->mode);
}

static void openat_complete(PyObject *self, struct io_uring_cqe *cqe) {
    OpenAtOperation *op = (OpenAtOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        outcome_capture(&(op->base.outcome), PyLong_FromLong(cqe->res));
    }
}

static OperationVTable g_openat_operation_vtable = {
    .prepare  = openat_prepare,
    .complete = openat_complete,
};

PyObject *openat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 4) {
        PyErr_Format(PyExc_TypeError, "Expected 4 arguments, got %zu instead", nargs);
        return NULL;
    }

    int dfd;
    if (args[0] == Py_None) {
        dfd = AT_FDCWD;
    } else if (!python_parse_int(&dfd, args[0])) {
        return NULL;
    }

    PyObject *path;
    if (!PyUnicode_FSConverter(args[1], &path)) {
        return NULL;
    }

    int flags;
    if (!python_parse_int(&flags, args[2])) {
        return NULL;
    }

    unsigned int mode;
    if (!python_parse_unsigned_int(&mode, args[3])) {
        return NULL;
    }

    OpenAtOperation *op = (OpenAtOperation *)operation_alloc(state->OpenAtOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_openat_operation_vtable;
        op->base.scratch = flags;
        op->path         = path;
        op->dfd          = dfd;
        op->mode         = mode;
    } else {
        Py_DECREF(path);
    }

    return (PyObject *)op;
}

static int openat_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    OpenAtOperation *op = (OpenAtOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->path);
    return operation_traverse(&op->base, visit, arg);
}

static int openat_clear_impl(PyObject *self) {
    OpenAtOperation *op = (OpenAtOperation *)self;

    Py_CLEAR(op->path);
    return operation_clear(&op->base);
}

static PyType_Slot g_openat_operation_slots[] = {
    {Py_tp_traverse, openat_traverse_impl},
    {Py_tp_clear, openat_clear_impl},
    {0, NULL},
};

static PyType_Spec g_openat_operation_spec = {
    .name      = "_impl._OpenOperation",
    .basicsize = sizeof(OpenAtOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_openat_operation_slots,
};

PyTypeObject *openat_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_openat_operation_spec, (PyObject *)state->Operation_type);
}
