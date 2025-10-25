// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "python_utils.hpp"

#include "operation.hpp"
#include "runtime.hpp"

using namespace boros;

namespace {

    auto g_impl_methods = python::MethodTable();

    constinit PyModuleDef g_impl_module = {
        PyModuleDef_HEAD_INIT,
        "_impl",
        "Implementation details of boros._impl",
        -1,
        g_impl_methods.data(),
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

}

PyMODINIT_FUNC PyInit__impl() {
    auto module = python::Module::Create(g_impl_module);

    auto *operation = Operation::Register(module);
    if (operation == nullptr) [[unlikely]] {
        return nullptr;
    }

    auto *runtime_context = RuntimeContext::Register(module);
    if (runtime_context == nullptr) [[unlikely]] {
        return nullptr;
    }

    return module.raw;
}
