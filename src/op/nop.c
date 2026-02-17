/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/nop.h"

#include "util/python.h"

#include "module.h"

static void nop_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    Operation *op = (Operation *)self;

    io_uring_prep_nop(sqe);
    sqe->nop_flags |= IORING_NOP_INJECT_RESULT;
    sqe->len = op->scratch;
}

static void nop_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;
    outcome_capture(&op->outcome, PyLong_FromLong(cqe->res));
}

static OperationVTable g_nop_operation_vtable = {
    .prepare  = nop_prepare,
    .complete = nop_complete,
};

PyObject *nop_operation_create(PyObject *mod, PyObject *res_) {
    ImplState *state = PyModule_GetState(mod);

    int res;
    if (!python_parse_int(&res, res_)) {
        return NULL;
    }

    NopOperation *op = (NopOperation *)operation_alloc(state->NopOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_nop_operation_vtable;
        op->base.scratch = res;
    }

    return (PyObject *)op;
}

static PyType_Slot g_nop_operation_slots[] = {
    {0, NULL},
};

static PyType_Spec g_nop_operation_spec = {
    .name      = "_impl._NopOperation",
    .basicsize = sizeof(NopOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_nop_operation_slots,
};

PyTypeObject *nop_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_nop_operation_spec, (PyObject *)state->Operation_type);
}
