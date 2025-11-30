// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "extension.hpp"

#include "event_loop.hpp"
#include "task.hpp"

namespace boros {

    auto ModuleState::Traverse(visitproc visit, void *arg) -> int {
        Py_VISIT(TaskType);
        Py_VISIT(EventLoopPolicyType);
        Py_VISIT(EventLoopType);

        return 0;
    }

    auto ModuleState::Clear() -> int {
        Py_CLEAR(TaskType);
        Py_CLEAR(EventLoopPolicyType);
        Py_CLEAR(EventLoopType);

        return 0;
    }

    auto ModuleState::Free() -> void {
        PyThread_tss_free(local_event_loop);
    }

    namespace {

        auto ModuleExec(PyObject *mod) -> int {
            auto &state = python::GetModuleState<ModuleState>(mod);

            state.TaskType = Task::Register(mod);
            if (state.TaskType == nullptr) [[unlikely]] {
                return -1;
            }

            state.EventLoopPolicyType = EventLoopPolicy::Register(mod);
            if (state.EventLoopPolicyType == nullptr) [[unlikely]] {
                return -1;
            }

            state.EventLoopType = EventLoop::Register(mod);
            if (state.EventLoopType == nullptr) [[unlikely]] {
                return -1;
            }

            state.local_event_loop = PyThread_tss_alloc();
            if (state.local_event_loop == nullptr) [[unlikely]] {
                return -1;
            }

            if (PyThread_tss_create(state.local_event_loop) != 0) [[unlikely]] {
                return -1;
            }

            return 0;
        }

        auto g_low_level_methods = python::MethodTable(
            python::Method<EventLoop::Create>("create_event_loop"),
            python::Method<EventLoop::Destroy>("destroy_event_loop"),
            python::Method<EventLoop::Get>("get_event_loop"),
            python::Method<EventLoop::Nop>("nop")
        );

        auto g_low_level_module_slots = python::ModuleSlotTable(
            python::ModuleSlot(Py_mod_exec, &ModuleExec),
            python::ModuleSlot(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED)
        );

        constinit auto g_low_level_module = LowLevelModuleDef(
            "_low_level",
            "",
            g_low_level_methods.data(),
            g_low_level_module_slots.data()
        );

        PyMODINIT_FUNC PyInit__low_level() {
            return g_low_level_module.CreateModule();
        }

    }

}
