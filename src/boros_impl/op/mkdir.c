/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/mkdir.h"

#include <liburing.h>

#include "module.h"
#include "util/python.h"

static void mkdir_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    MkdirOperation *op = (MkdirOperation *)self;

    const char *pathname = PyBytes_AS_STRING(op->path);
    io_uring_prep_mkdir(sqe, pathname, op->base.scratch);
}

static void mkdir_complete(PyObject *self, struct io_uring_cqe *cqe) {
    MkdirOperation *op = (MkdirOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        outcome_capture(&(op->base.outcome), PyLong_FromLong(cqe->res));
    }
}

static OperationVTable g_mkdir_operation_vtable = {
    .prepare  = mkdir_prepare,
    .complete = mkdir_complete,
};

PyObject *mkdir_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 argument, got %zu instead", nargs);
        return NULL;
    }

    PyObject *path;
    if (!PyUnicode_FSConverter(args[0], &path)) {
        return NULL;
    }

    unsigned int mode = 0;
    if (!python_parse_unsigned_int(&mode, args[1])) {
        return NULL;
    }

    MkdirOperation *op = (MkdirOperation *)operation_alloc(state->MkdirOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_mkdir_operation_vtable;
        op->path         = path;
        op->base.scratch = mode;
    } else {
        Py_DECREF(path);
    }

    return (PyObject *)op;
}

static int mkdir_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    MkdirOperation *op = (MkdirOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->path);
    return operation_traverse(&op->base, visit, arg);
}

static int mkdir_clear_impl(PyObject *self) {
    MkdirOperation *op = (MkdirOperation *)self;

    Py_CLEAR(op->path);
    return operation_clear(&op->base);
}

static PyType_Slot g_mkdir_operation_slots[] = {
    {Py_tp_traverse, mkdir_traverse_impl},
    {Py_tp_clear, mkdir_clear_impl},
    {0, NULL},
};

static PyType_Spec g_mkdir_operation_spec = {
    .name      = "_impl._MkdirOperation",
    .basicsize = sizeof(MkdirOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_mkdir_operation_slots,
};

PyTypeObject *mkdir_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_mkdir_operation_spec, (PyObject *)state->Operation_type);
}
