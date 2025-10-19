// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include <Python.h>

#include "operation.hpp"
#include "runtime.hpp"

using namespace boros::impl;

namespace {

    constinit PyMethodDef g_impl_methods[] = {
        {nullptr, nullptr, 0, nullptr}
    };

    constinit PyModuleDef g_impl_module = {
        PyModuleDef_HEAD_INIT,
        "_impl",
        "Implementation details of boros._impl",
        -1,
        g_impl_methods,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

}

PyMODINIT_FUNC PyInit__impl() {
    auto *module = PyModule_Create(&g_impl_module);

    auto *operation = OperationObj::Register(module);
    if (operation == nullptr) [[unlikely]] {
        return nullptr;
    }

    auto *runtime_context = RuntimeContextObj::Register(module);
    if (runtime_context == nullptr) [[unlikely]] {
        return nullptr;
    }

    return module;
}
