/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/symlinkat.h"

#include <liburing.h>

#include "module.h"
#include "object.h"
#include "util/python.h"

static void symlinkat_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    SymlinkAtOperation *op = (SymlinkAtOperation *)self;

    const char *target = PyBytes_AS_STRING(op->target);
    const char *linkpath = PyBytes_AS_STRING(op->linkpath);

    io_uring_prep_symlinkat(sqe, target, op->base.scratch, linkpath);
}

static void symlinkat_complete(PyObject *self, struct io_uring_cqe *cqe) {
    SymlinkAtOperation *op = (SymlinkAtOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        assert(cqe->res == 0);
        outcome_capture(&(op->base.outcome), Py_None);
    }
}

static OperationVTable g_symlink_operation_vtable = {
    .prepare  = symlinkat_prepare,
    .complete = symlinkat_complete,
};

PyObject *symlinkat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "Expected 3 argument, got %zu instead", nargs);
        return NULL;
    }

    PyObject *target;
    if (!PyUnicode_FSConverter(args[0], &target)) {
        return NULL;
    }

    int newdirfd;
    if (args[1] == Py_None) {
        newdirfd = AT_FDCWD;
    } else if (!python_parse_int(&newdirfd, args[1])) {
        return NULL;
    }

    PyObject *linkpath;
    if (!PyUnicode_FSConverter(args[2], &linkpath)) {
        return NULL;
    }

    SymlinkAtOperation *op = (SymlinkAtOperation *)operation_alloc(state->SymlinkAtOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_symlink_operation_vtable;
        op->base.scratch = newdirfd;
        op->target = target;
        op->linkpath = linkpath;
    } else {
        Py_DECREF(target);
        Py_DECREF(linkpath);
    }

    return (PyObject *)op;
}

static int symlinkat_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    SymlinkAtOperation *op = (SymlinkAtOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->target);
    Py_VISIT(op->linkpath);
    return operation_traverse(&op->base, visit, arg);
}

static int symlinkat_clear_impl(PyObject *self) {
    SymlinkAtOperation *op = (SymlinkAtOperation *)self;

    Py_CLEAR(op->target);
    Py_CLEAR(op->linkpath);
    return operation_clear(&op->base);
}

static PyType_Slot g_symlinkat_operation_slots[] = {
    {Py_tp_traverse, symlinkat_traverse_impl},
    {Py_tp_clear, symlinkat_clear_impl},
    {0, NULL},
};

static PyType_Spec g_symlinkat_operation_spec = {
    .name      = "_impl._SymlinkAtOperation",
    .basicsize = sizeof(SymlinkAtOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC,
    .slots     = g_symlinkat_operation_slots,
};

PyTypeObject *symlinkat_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_symlinkat_operation_spec, (PyObject *)state->Operation_type);
}
