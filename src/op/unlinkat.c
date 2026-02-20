/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/unlinkat.h"

#include "util/python.h"

#include "module.h"

static void unlinkat_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    UnlinkAtOperation *op = (UnlinkAtOperation *)self;

    const char *pathname = PyBytes_AS_STRING(op->path);
    io_uring_prep_unlinkat(sqe, op->base.scratch, pathname, op->flags);
}

static void unlinkat_complete(PyObject *self, struct io_uring_cqe *cqe) {
    UnlinkAtOperation *op = (UnlinkAtOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        assert(cqe->res == 0);
        outcome_capture(&(op->base.outcome), Py_None);
    }
}

static OperationVTable g_unlinkat_operation_vtable = {
    .prepare  = unlinkat_prepare,
    .complete = unlinkat_complete,
};

PyObject *unlinkat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "Expected 3 arguments, got %zu instead", nargs);
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

    UnlinkAtOperation *op = (UnlinkAtOperation *)operation_alloc(state->UnlinkAtOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_unlinkat_operation_vtable;
        op->base.scratch = dfd;
        op->path         = path;
        op->flags        = flags;
    } else {
        Py_DECREF(path);
    }

    return (PyObject *)op;
}

static int unlinkat_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    UnlinkAtOperation *op = (UnlinkAtOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->path);
    return operation_traverse(&op->base, visit, arg);
}

static int unlinkat_clear_impl(PyObject *self) {
    UnlinkAtOperation *op = (UnlinkAtOperation *)self;

    Py_CLEAR(op->path);
    return operation_clear(&op->base);
}

static PyType_Slot g_unlinkat_operation_slots[] = {
    {Py_tp_traverse, unlinkat_traverse_impl},
    {Py_tp_clear, unlinkat_clear_impl},
    {0, NULL},
};

static PyType_Spec g_unlinkat_operation_spec = {
    .name      = "_impl._UnlinkAtOperation",
    .basicsize = sizeof(UnlinkAtOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_unlinkat_operation_slots,
};

PyTypeObject *unlinkat_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_unlinkat_operation_spec, (PyObject *)state->Operation_type);
}
