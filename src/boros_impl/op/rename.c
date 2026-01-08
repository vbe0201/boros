/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/rename.h"

#include <liburing.h>

#include "module.h"
#include "util/python.h"

static void rename_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    RenameOperation *op = (RenameOperation *)self;
    const char *oldpath = PyBytes_AS_STRING(op->oldpath);
    const char *newpath = PyBytes_AS_STRING(op->newpath);
    io_uring_prep_rename(sqe, oldpath, newpath);
}

static void rename_complete(PyObject *self, struct io_uring_cqe *cqe) {
    RenameOperation *op = (RenameOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        outcome_capture(&(op->base.outcome), PyLong_FromLong(cqe->res));
    }
}

static OperationVTable g_rename_operation_vtable = {
    .prepare  = rename_prepare,
    .complete = rename_complete,
};

PyObject *rename_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 argument, got %zu instead", nargs);
        return NULL;
    }

    PyObject *oldpath;
    if (!PyUnicode_FSConverter(args[0], &oldpath)) {
        return NULL;
    }

    PyObject *newpath;
    if (!PyUnicode_FSConverter(args[1], &newpath)) {
        return NULL;
    }

    RenameOperation *op = (RenameOperation *)operation_alloc(state->RenameOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_rename_operation_vtable;
        op->oldpath = oldpath;
        op->newpath = newpath;
    } else {
        Py_DECREF(oldpath);
        Py_DECREF(newpath);
    }

    return (PyObject *)op;
}

static int rename_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    RenameOperation *op = (RenameOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->oldpath);
    Py_VISIT(op->newpath);
    return operation_traverse(&op->base, visit, arg);
}

static int rename_clear_impl(PyObject *self) {
    RenameOperation *op = (RenameOperation *)self;

    Py_CLEAR(op->oldpath);
    Py_CLEAR(op->newpath);
    return operation_clear(&op->base);
}

static PyType_Slot g_rename_operation_slots[] = {
    {Py_tp_traverse, rename_traverse_impl},
    {Py_tp_clear, rename_clear_impl},
    {0, NULL},
};

static PyType_Spec g_rename_operation_spec = {
    .name      = "_impl._RenameOperation",
    .basicsize = sizeof(RenameOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_rename_operation_slots,
};

PyTypeObject *rename_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_rename_operation_spec, (PyObject *)state->Operation_type);
}
