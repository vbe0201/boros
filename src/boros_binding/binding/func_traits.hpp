// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <tuple>

namespace boros::python::impl {

    template <typename Cls, typename Ret, typename... Args>
    struct FunctionTraitsBase {
        using ClassType = Cls;
        using RetType   = Ret;
        using ArgsType  = std::tuple<Args...>;
    };

    template <typename>
    struct FunctionTraits;

    template <typename Ret, typename... Args>
    struct FunctionTraits<Ret (*)(Args...)> : public FunctionTraitsBase<void, Ret, Args...> {};

    template <typename Cls, typename Ret, typename... Args>
    struct FunctionTraits<Ret (Cls::*)(Args...)> : public FunctionTraitsBase<Cls, Ret, Args...> {};

    template <typename Cls, typename Ret, typename... Args>
    struct FunctionTraits<Ret (Cls::*)(Args...) const> : public FunctionTraitsBase<Cls, Ret, Args...> {};

}  // namespace boros::python::impl
