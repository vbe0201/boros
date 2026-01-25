/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/linkat.h"

#include <liburing.h>

#include "module.h"
#include "object.h"
#include "util/python.h"

static void linkat_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    LinkAtOperation *op = (LinkAtOperation *)self;

    const char *oldpath = PyBytes_AS_STRING(op->oldpath);
    const char *newpath = PyBytes_AS_STRING(op->newpath);
    io_uring_prep_linkat(sqe, op->olddirfd, oldpath, op->newdirfd, newpath, op->base.scratch);
}

static void linkat_complete(PyObject *self, struct io_uring_cqe *cqe) {
    LinkAtOperation *op = (LinkAtOperation *)self;
    
    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        assert(cqe->res == 0);
        outcome_capture(&(op->base.outcome), Py_None);
    }
}

static OperationVTable g_linkat_operation_vtable = {
    .prepare  = linkat_prepare,
    .complete = linkat_complete,
};

PyObject *linkat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 5) {
        PyErr_Format(PyExc_TypeError, "Expected 5 argument, got %zu instead", nargs);
        return NULL;
    }

    int olddirfd;
    if (args[0] == Py_None) {
        olddirfd = AT_FDCWD;
    } else if (!python_parse_int(&olddirfd, args[0])) {
        return NULL;
    }

    PyObject *oldpath;
    if (!PyUnicode_FSConverter(args[1], &oldpath)) {
        return NULL;
    }

    int newdirfd;
    if (args[0] == Py_None) {
        newdirfd = AT_FDCWD;
    } else if (!python_parse_int(&newdirfd, args[2])) {
        return NULL;
    }

    PyObject *newpath;
    if (!PyUnicode_FSConverter(args[3], &newpath)) {
        return NULL;
    }

    int flags;
    if (!python_parse_int(&flags, args[4])) {
        return NULL;
    }


    LinkAtOperation *op = (LinkAtOperation *)operation_alloc(state->LinkAtOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_linkat_operation_vtable;
        op->base.scratch = flags;
        op->olddirfd = olddirfd;
        op->newdirfd = newdirfd;
        op->oldpath = oldpath;
        op->newpath = newpath;
    } else {
        Py_DECREF(oldpath);
        Py_DECREF(newpath);
    }

    return (PyObject *)op;
}

static int linkat_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    LinkAtOperation *op = (LinkAtOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->oldpath);
    Py_VISIT(op->newpath);
    return operation_traverse(&op->base, visit, arg);
}

static int linkat_clear_impl(PyObject *self) {
    LinkAtOperation *op = (LinkAtOperation *)self;

    Py_CLEAR(op->oldpath);
    Py_CLEAR(op->newpath);
    return operation_clear(&op->base);
}

static PyType_Slot g_linkat_operation_slots[] = {
    {Py_tp_traverse, linkat_traverse_impl},
    {Py_tp_clear, linkat_clear_impl},
    {0, NULL},
};

static PyType_Spec g_linkat_operation_spec = {
    .name      = "_impl._LinkAtOperation",
    .basicsize = sizeof(LinkAtOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC,
    .slots     = g_linkat_operation_slots,
};

PyTypeObject *linkat_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_linkat_operation_spec, (PyObject *)state->Operation_type);
}
