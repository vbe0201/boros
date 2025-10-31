// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "wrapper/python.hpp"

namespace boros {

    /// State that is managed for every module instantiation.
    struct ModuleState {
        PyObject *RuntimeContextType;
        PyObject *TaskType;

        /// Implements the Python module traversal step.
        auto Traverse(visitproc visit, void *arg) noexcept -> int;

        /// Implements the Python module cleanup step.
        auto Clear() noexcept -> int;
    };

    /// The concrete module definition type this extension is using.
    using ImplModuleDef = python::ModuleDefinition<ModuleState>;

    /// The concrete module type this extension is using.
    using ImplModule = python::Module<ModuleState>;

}
