// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include <Python.h>

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
    return PyModule_Create(&g_impl_module);
}
