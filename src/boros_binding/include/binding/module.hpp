// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

namespace boros::python {

    // Utilities for working with Python modules.
    //
    // # Module Definitions
    //
    // Modules are defined as static PyModuleDef objects and instantiated
    // per interpreter in a process called "Multi-phase Initialization".
    // What this means is that every instance of a module gets its own
    // per-module state object.
    //
    // The ModuleDefinition helper implements this by default when a
    // compatible state type is provided.
    //
    // # Module State
    //
    // The purpose of the module state is to replace mutable state in
    // global static variables which are shared with subinterpreters.
    // The GetModuleState* family of functions provides convenient
    // access to the state.
    //
    // Do note that from instance methods, the module state can only
    // be accessed from the **defining class** of the method which
    // may be tricky to get right with inheritance in mind.

    /// A type suitable for use as per-instance module state.
    template <typename T>
    concept ModuleState = requires(T &state, visitproc visit, void *arg) {
        { state.Traverse(visit, arg) } -> std::same_as<int>;
        { state.Clear() } -> std::same_as<int>;
        { state.Free() } -> std::same_as<void>;
    };

    /// Gets the per-module state through a Python module object.
    template <ModuleState State>
    State &GetModuleState(PyObject *mod) {
        return *static_cast<State *>(PyModule_GetState(mod));
    }

    /// Gets the per-module state through a Python type object.
    template <ModuleState State>
    State &GetModuleStateFromType(PyTypeObject *tp) {
        return GetModuleState<State>(PyType_GetModule(tp));
    }

    /// Instantiates a new type from spec and adds it to a module.
    inline PyTypeObject *InstantiateType(PyObject *mod, PyType_Spec &spec, PyObject *base = nullptr, bool bind = true) {
        auto *tp = reinterpret_cast<PyTypeObject *>(PyType_FromModuleAndSpec(mod, &spec, base));
        if (tp == nullptr) {
            return nullptr;
        }

        if (bind && PyModule_AddType(mod, tp) < 0) {
            return nullptr;
        }

        return tp;
    }

    /// Represents a Python extension module definition. This uses
    /// a suitable setup for two-phase initialization by default.
    template <ModuleState State>
    class ModuleDefinition {
    private:
        PyModuleDef m_module;

    private:
        static int Traverse(PyObject *mod, visitproc visit, void *arg) {
            auto &state = GetModuleState<State>(mod);
            return state.Traverse(visit, arg);
        }

        static int Clear(PyObject *mod) {
            auto &state = GetModuleState<State>(mod);
            return state.Clear();
        }

        static void Free(void *mod) {
            auto *module = static_cast<PyObject *>(mod);
            auto &state  = GetModuleState<State>(module);
            state.Free();
            Clear(module);
        }

    public:
        constexpr ModuleDefinition(const char *name, const char *doc, PyMethodDef *methods, PyModuleDef_Slot *slots)
            : m_module{PyModuleDef_HEAD_INIT, name, doc, sizeof(State), methods, slots, &Traverse, &Clear, &Free} {}

        PyObject *CreateModule() { return PyModuleDef_Init(&m_module); }
    };

    /// Creates a Python module slot entry from the given data.
    template <typename Ptr>
        requires std::is_pointer_v<Ptr>
    PyModuleDef_Slot ModuleSlot(int slot, Ptr ptr) {
        return {slot, reinterpret_cast<void *>(ptr)};
    }

    /// Creates a table of module slots suitable for use with
    /// module object creation.
    template <typename... Ts>
        requires(std::is_same_v<Ts, PyModuleDef_Slot> && ...)
    constexpr std::array<PyModuleDef_Slot, sizeof...(Ts) + 1> ModuleSlotTable(Ts... slots) {
        return {{slots..., {0, nullptr}}};
    }

}  // namespace boros::python
