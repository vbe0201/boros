// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "wrapper/python_header.hpp"

#include <optional>

namespace boros::python {

    /// Requirements for a type to be used as a per-instance module state.
    template <typename T>
    concept ModuleState = requires (T &state, visitproc visit, void *arg) {
        { state.Traverse(visit, arg) } noexcept -> std::same_as<int>;
        { state.Clear()              } noexcept -> std::same_as<int>;
    };

    template <ModuleState Extra>
    struct Module {
        PyObject *raw;

        /// Gets the extra state associated with this module.
        auto GetState() noexcept -> Extra&;

        /// Instantiates a new type and attaches it to this module.
        auto Add(PyType_Spec &spec) noexcept -> PyObject* {
            auto *tp = PyType_FromModuleAndSpec(raw, std::addressof(spec), nullptr);
            if (tp == nullptr) [[unlikely]] {
                return nullptr;
            }

            if (PyModule_AddObjectRef(raw, spec.name, tp) < 0) [[unlikely]] {
                return nullptr;
            }

            return tp;
        }
    };

    /// Defines a new Python extension module.
    template <ModuleState Extra>
    struct ModuleDefinition {
    private:
        PyModuleDef m_module;

    private:
        static auto Traverse(PyObject *module, visitproc visit, void *arg) noexcept -> int {
            auto *state = static_cast<Extra*>(PyModule_GetState(module));
            return state->Traverse(visit, arg);
        }

        static auto Clear(PyObject *module) noexcept -> int {
            auto *state = static_cast<Extra*>(PyModule_GetState(module));
            return state->Clear();
        }

        static auto Free(void *module) noexcept -> void {
            Clear(static_cast<PyObject*>(module));
        }

    public:
        constexpr ModuleDefinition(const char *name, const char *doc, PyMethodDef *methods, PyModuleDef_Slot *slots)
            : m_module{PyModuleDef_HEAD_INIT, name, doc, sizeof(Extra), methods, slots, &Traverse, &Clear, &Free} {}

        /// Creates a new module object instance.
        auto CreateModule() noexcept -> PyObject* {
            return PyModuleDef_Init(std::addressof(m_module));
        }
    };

}
