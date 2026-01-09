/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "module.h"

#include <assert.h>

#include "context/run_config.h"
#include "op/base.h"
#include "op/cancel.h"
#include "op/close.h"
#include "op/connect.h"
#include "op/nop.h"
#include "op/open.h"
#include "op/read.h"
#include "op/socket.h"
#include "op/write.h"
#include "run.h"
#include "task.h"

static int module_traverse(PyObject *mod, visitproc visit, void *arg) {
    ImplState *state = PyModule_GetState(mod);
    Py_VISIT(state->RunConfig_type);
    Py_VISIT(state->Task_type);
    Py_VISIT(state->Operation_type);
    Py_VISIT(state->OperationWaiter_type);
    Py_VISIT(state->NopOperation_type);
    Py_VISIT(state->SocketOperation_type);
    Py_VISIT(state->OpenOperation_type);
    Py_VISIT(state->ReadOperation_type);
    Py_VISIT(state->WriteOperation_type);
    Py_VISIT(state->CloseOperation_type);
    Py_VISIT(state->CancelOperation_type);
    Py_VISIT(state->ConnectOperation_type);
    return 0;
}

static int module_clear(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    Py_CLEAR(state->RunConfig_type);
    Py_CLEAR(state->Task_type);
    Py_CLEAR(state->Operation_type);
    Py_CLEAR(state->OperationWaiter_type);
    Py_CLEAR(state->NopOperation_type);
    Py_CLEAR(state->SocketOperation_type);
    Py_CLEAR(state->OpenOperation_type);
    Py_CLEAR(state->ReadOperation_type);
    Py_CLEAR(state->WriteOperation_type);
    Py_CLEAR(state->CloseOperation_type);
    Py_CLEAR(state->CancelOperation_type);
    Py_CLEAR(state->ConnectOperation_type);
    return 0;
}

static void module_free(void *mod) {
    ImplState *state = PyModule_GetState(mod);

    PyThread_tss_free(state->local_context);

    module_clear(mod);
}

static int module_exec(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);

    state->RunConfig_type = run_config_register(mod);
    if (state->RunConfig_type == NULL) {
        return -1;
    }

    state->Task_type = task_register(mod);
    if (state->Task_type == NULL) {
        return -1;
    }

    state->Operation_type = operation_register(mod);
    if (state->Operation_type == NULL) {
        return -1;
    }

    state->OperationWaiter_type = operation_waiter_register(mod);
    if (state->OperationWaiter_type == NULL) {
        return -1;
    }

    state->NopOperation_type = nop_operation_register(mod);
    if (state->NopOperation_type == NULL) {
        return -1;
    }

    state->SocketOperation_type = socket_operation_register(mod);
    if (state->SocketOperation_type == NULL) {
        return -1;
    }

    state->OpenOperation_type = open_operation_register(mod);
    if (state->OpenOperation_type == NULL) {
        return -1;
    }

    state->ReadOperation_type = read_operation_register(mod);
    if (state->ReadOperation_type == NULL) {
        return -1;
    }

    state->WriteOperation_type = write_operation_register(mod);
    if (state->WriteOperation_type == NULL) {
        return -1;
    }

    state->CloseOperation_type = close_operation_register(mod);
    if (state->CloseOperation_type == NULL) {
        return -1;
    }

    state->CancelOperation_type = cancel_operation_register(mod);
    if (state->CancelOperation_type == NULL) {
        return -1;
    }

    state->ConnectOperation_type = connect_operation_register(mod);
    if (state->ConnectOperation_type == NULL) {
        return -1;
    }

    state->local_context = PyThread_tss_alloc();
    if (state->local_context == NULL) {
        return -1;
    }

    if (PyThread_tss_create(state->local_context) != 0) {
        return -1;
    }

    return 0;
}

PyDoc_STRVAR(g_nop_doc, "Asynchronous nop operation on the io_uring.");
PyDoc_STRVAR(g_socket_doc, "Asynchronous socket(2) operation on the io_uring.");
PyDoc_STRVAR(g_read_doc, "Asynchronous read(2) operation on the io_uring.");
PyDoc_STRVAR(g_write_doc, "Asynchronous write(2) operation on the io_uring.");
PyDoc_STRVAR(g_close_doc, "Asynchronous close(2) operation on the io_uring.");
PyDoc_STRVAR(g_open_doc, "Asynchronous open(2) operation on the io_uring.");
PyDoc_STRVAR(g_cancel_fd_doc, "Asynchronously cancels all operations on a fd.");
PyDoc_STRVAR(g_cancel_op_doc, "Asynchronously cancels a specific operation.");
PyDoc_STRVAR(g_connect_doc, "Asynchronous connect(2) operation on the io_uring.");

PyDoc_STRVAR(g_run_doc, "Drives a given coroutine to completion.\n\n"
                        "This is the entrypoint to the boros runtime.");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
static PyMethodDef g_module_methods[] = {
    {"nop", (PyCFunction)nop_operation_create, METH_O, g_nop_doc},
    {"socket", (PyCFunction)socket_operation_create, METH_FASTCALL, g_socket_doc},
    {"run", (PyCFunction)boros_run, METH_FASTCALL, g_run_doc},
    {"open", (PyCFunction)open_operation_create, METH_FASTCALL, g_open_doc},
    {"read", (PyCFunction)read_operation_create, METH_FASTCALL, g_read_doc},
    {"write", (PyCFunction)write_operation_create, METH_FASTCALL, g_write_doc},
    {"close", (PyCFunction)close_operation_create, METH_FASTCALL, g_close_doc},
    {"cancel_fd", (PyCFunction)cancel_operation_create_fd, METH_O, g_cancel_fd_doc},
    {"cancel_op", (PyCFunction)cancel_operation_create_op, METH_O, g_cancel_op_doc},
    {"connect", (PyCFunction)connect_operation_create, METH_FASTCALL, g_connect_doc},
    {NULL, NULL, 0, NULL},
};
#pragma GCC diagnostic pop

static PyModuleDef_Slot g_module_slots[] = {
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL},
};

static PyModuleDef g_implmodule = {
    .m_base     = PyModuleDef_HEAD_INIT,
    .m_name     = "_impl",
    .m_doc      = "Low-level implementation details of the boros module",
    .m_size     = sizeof(ImplState),
    .m_methods  = g_module_methods,
    .m_slots    = g_module_slots,
    .m_traverse = module_traverse,
    .m_clear    = module_clear,
    .m_free     = module_free,
};

PyMODINIT_FUNC PyInit__impl() {
    return PyModuleDef_Init(&g_implmodule);
}
