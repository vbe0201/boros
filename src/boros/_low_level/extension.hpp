// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "python.hpp"

namespace boros {

    /// State that is managed for every instantiation of this extension.
    struct ModuleState {
        /// Implements the Python module traversal step.
        auto Traverse(visitproc visit, void *arg) -> int;

        /// Implements the Python module cleanup step.
        auto Clear() -> int;
    };

    /// The module definition type this extension type is exposing.
    using LowLevelModuleDef = boros::python::ModuleDefinition<ModuleState>;

}
