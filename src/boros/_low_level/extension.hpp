// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "python.hpp"

namespace boros {

    /// State that is managed for every instantiation of this extension.
    struct ModuleState {
        PyTypeObject *TaskType;
        PyTypeObject *EventLoopPolicyType;
        PyTypeObject *EventLoopType;

        Py_tss_t *local_event_loop;

        /// Implements the Python module traversal step.
        auto Traverse(visitproc visit, void *arg) -> int;

        /// Implements the Python module cleanup step.
        auto Clear() -> int;

        /// Implements the Python module deallocation step.
        auto Free() -> void;
    };

    /// The module definition type this extension type is exposing.
    using LowLevelModuleDef = boros::python::ModuleDefinition<ModuleState>;

}
