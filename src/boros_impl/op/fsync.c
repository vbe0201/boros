/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/fsync.h"

#include <liburing.h>

#include "module.h"
#include "util/python.h"

static void fsync_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    FsyncOperation *op = (FsyncOperation *)self;

    io_uring_prep_fsync(sqe, op->base.scratch, op->fsync_flags);
}

static void fsync_complete(PyObject *self, struct io_uring_cqe *cqe) {
    FsyncOperation *op = (FsyncOperation *)self;
    outcome_capture(&op->base.outcome, PyLong_FromLong(cqe->res));
}

static OperationVTable g_fsync_operation_vtable = {
    .prepare  = fsync_prepare,
    .complete = fsync_complete,
};

PyObject *fsync_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "Expected 2 argument, got %zu instead", nargs);
        return NULL;
    }

    int fd;
    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    int fsync_flags;
    if (!python_parse_int(&fsync_flags, args[1])) {
        return NULL;
    }

    FsyncOperation *op = (FsyncOperation *)operation_alloc(state->FsyncOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_fsync_operation_vtable;
        op->base.scratch = fd;
        op->fsync_flags = fsync_flags;
    }

    return (PyObject *)op;
}

static PyType_Slot g_fsync_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_nop_operation_spec = {
    .name      = "_impl._FsyncOperation",
    .basicsize = sizeof(FsyncOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_fsync_operation_slots,
};

PyTypeObject *fsync_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_fsync_operation_spec, (PyObject *)state->Operation_type);
}
