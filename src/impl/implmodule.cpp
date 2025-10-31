// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "implmodule.hpp"

#include "runtime.hpp"
#include "task.hpp"
#include "wrapper/types.hpp"

namespace boros {

    auto ModuleState::Traverse(visitproc visit, void *arg) noexcept -> int {
        Py_VISIT(RuntimeContextType);
        Py_VISIT(TaskType);

        return 0;
    }

    auto ModuleState::Clear() noexcept -> int {
        Py_CLEAR(RuntimeContextType);
        Py_CLEAR(TaskType);

        return 0;
    }

}

namespace {

    auto ModuleExec(PyObject *mod_) noexcept -> int {
        boros::ImplModule mod{mod_};

        auto &state = mod.GetState();

        state.RuntimeContextType = boros::RuntimeContext::Register(mod);
        if (state.RuntimeContextType == nullptr) [[unlikely]] {
            return -1;
        }

        state.TaskType = boros::Task::Register(mod);
        if (state.TaskType == nullptr) [[unlikely]] {
            return -1;
        }

        return 0;
    }

    constinit auto g_impl_methods = boros::python::MethodTable();

    auto g_impl_module_slots = boros::python::ModuleSlotTable(
        boros::python::ModuleSlot(Py_mod_exec, &ModuleExec),
        boros::python::ModuleSlot(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED)
    );

    constinit auto g_impl_module = boros::ImplModuleDef("_impl", "", g_impl_methods.data(), g_impl_module_slots.data());

}

PyMODINIT_FUNC PyInit__impl() {
    return g_impl_module.CreateModule();
}
