/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/rename.h"

#include "util/python.h"

#include "module.h"

static void renameat_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    RenameAtOperation *op = (RenameAtOperation *)self;
    const char *oldpath   = PyBytes_AS_STRING(op->oldpath);
    const char *newpath   = PyBytes_AS_STRING(op->newpath);
    io_uring_prep_renameat(sqe, op->base.scratch, oldpath, op->newdfd, newpath, op->flags);
}

static void renameat_complete(PyObject *self, struct io_uring_cqe *cqe) {
    RenameAtOperation *op = (RenameAtOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        assert(cqe->res == 0);
        outcome_capture(&(op->base.outcome), Py_None);
    }
}

static OperationVTable g_renameat_operation_vtable = {
    .prepare  = renameat_prepare,
    .complete = renameat_complete,
};

PyObject *renameat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "Expected 3 argument, got %zu instead", nargs);
        return NULL;
    }

    int olddfd;
    if (args[0] == Py_None) {
        olddfd = AT_FDCWD;
    } else if (!python_parse_int(&olddfd, args[0])) {
        return NULL;
    }

    PyObject *oldpath;
    if (!PyUnicode_FSConverter(args[1], &oldpath)) {
        return NULL;
    }

    int newdfd;
    if (args[0] == Py_None) {
        newdfd = AT_FDCWD;
    } else if (!python_parse_int(&newdfd, args[2])) {
        return NULL;
    }

    PyObject *newpath;
    if (!PyUnicode_FSConverter(args[3], &newpath)) {
        return NULL;
    }

    unsigned int flags;
    if (args[4] == Py_None) {
        flags = 0;
    } else if (!python_parse_unsigned_int(&flags, args[4])) {
        return NULL;
    }

    RenameAtOperation *op = (RenameAtOperation *)operation_alloc(state->RenameAtOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_renameat_operation_vtable;
        op->oldpath      = oldpath;
        op->newpath      = newpath;
        op->base.scratch = olddfd;
        op->newdfd       = newdfd;
        op->flags        = flags;
    } else {
        Py_DECREF(oldpath);
        Py_DECREF(newpath);
    }

    return (PyObject *)op;
}

static int renameat_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    RenameAtOperation *op = (RenameAtOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->oldpath);
    Py_VISIT(op->newpath);
    return operation_traverse(&op->base, visit, arg);
}

static int renameat_clear_impl(PyObject *self) {
    RenameAtOperation *op = (RenameAtOperation *)self;

    Py_CLEAR(op->oldpath);
    Py_CLEAR(op->newpath);
    return operation_clear(&op->base);
}

static PyType_Slot g_renameat_operation_slots[] = {
    {Py_tp_traverse, renameat_traverse_impl},
    {Py_tp_clear, renameat_clear_impl},
    {0, NULL},
};

static PyType_Spec g_renameat_operation_spec = {
    .name      = "_impl._RenameOperation",
    .basicsize = sizeof(RenameAtOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_renameat_operation_slots,
};

PyTypeObject *renameat_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_renameat_operation_spec, (PyObject *)state->Operation_type);
}
