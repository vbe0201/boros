// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "extension.hpp"

namespace boros {

    auto ModuleState::Traverse(visitproc Py_UNUSED(visit), void *Py_UNUSED(arg)) -> int {
        return 0;
    }

    auto ModuleState::Clear() -> int {
        return 0;
    }

}

namespace {

    auto ModuleExec(PyObject *mod) -> int {
        return 0;
    }

    constinit auto g_low_level_methods = boros::python::MethodTable();

    auto g_low_level_module_slots = boros::python::ModuleSlotTable(
        boros::python::ModuleSlot(Py_mod_exec, &ModuleExec),
        boros::python::ModuleSlot(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED)
    );

    constinit auto g_low_level_module = boros::LowLevelModuleDef(
        "_low_level",
        "",
        g_low_level_methods.data(),
        g_low_level_module_slots.data()
    );

}

PyMODINIT_FUNC PyInit__low_level() {
    return g_low_level_module.CreateModule();
}
