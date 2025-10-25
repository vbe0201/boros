// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

// Must be the first included header since Python may override
// functionality in some of the standard headers.
#include <Python.h>

#include <array>
#include <tuple>

#include "macros.hpp"
#include "pyerrors.h"

namespace boros::python {

    /// Wraps a raw Python module for convenience features.
    struct Module {
        PyObject *raw;

        /// Creates a new module instance from its definition.
        static auto Create(PyModuleDef &def) noexcept -> Module;

        /// Instantiates a new type object and attaches it to this module.
        auto Add(PyType_Spec &spec) noexcept -> PyObject*;
    };

    /// Specifies if a C++ type matches the requirements for Python objects.
    template <typename T>
    concept PythonObject =
        std::is_same_v<T, PyObject> ||
        (
            std::is_standard_layout_v<T> &&
            requires (T &obj) {
                // The documentation states what the PyObject_HEAD macro
                // expands to, so it should be okay to hardcode the name.
                { obj.ob_base } -> std::same_as<PyObject&>;
            }
        );

    /// Allocates a new Python object instance from the given type object.
    /// This is suitable as the implementation of __new__ for a type T.
    template <typename T> requires PythonObject<T>
    auto New(PyTypeObject *tp) noexcept -> T* {
        auto *tp_alloc = reinterpret_cast<allocfunc>(PyType_GetSlot(tp, Py_tp_alloc));
        return reinterpret_cast<T*>(tp_alloc(tp, 0));
    }

    template <typename T> requires PythonObject<T>
    auto FromPython(T* &out, PyObject *obj) noexcept -> void {
        out = reinterpret_cast<T*>(obj);
    }

    template <typename T> requires PythonObject<T>
    auto ToPython(T *value) noexcept -> PyObject* {
        return reinterpret_cast<PyObject*>(value);
    }

    auto FromPython(PyTypeObject* &out, PyObject *obj) noexcept -> void;
    auto ToPython(PyTypeObject *value) noexcept -> PyObject*;

    auto FromPython(Module &out, PyObject *obj) noexcept -> void;
    auto ToPython(Module value) noexcept -> PyObject*;

    auto FromPython(std::nullptr_t &out, PyObject *obj) noexcept -> void;
    auto ToPython(std::nullptr_t value) noexcept -> PyObject*;

    auto FromPython(bool &out, PyObject *obj) noexcept -> void;
    auto ToPython(bool value) noexcept -> PyObject*;

    auto FromPython(long &out, PyObject *obj) noexcept -> void;
    auto ToPython(long value) noexcept -> PyObject*;

    auto FromPython(unsigned long &out, PyObject *obj) noexcept -> void;
    auto ToPython(unsigned long value) noexcept -> PyObject*;

    /// Specifies a C++ type T is convertible to and from a Python object.
    template <typename T>
    concept PythonConvertible = requires (PyObject *obj, T &out, T value) {
        { FromPython(out, obj) } noexcept -> std::same_as<void>;
        { ToPython(value)      } noexcept -> std::same_as<PyObject*>;
    };

    /// Specifies that a sequence of C++ types is convertible to and from
    /// a Python object.
    template <typename... Ts>
    concept AllPythonConvertible = (PythonConvertible<Ts> && ...);

    namespace impl {

        template <typename T>
        constexpr inline std::size_t SizeOf = sizeof(T);

        template <>
        constexpr inline std::size_t SizeOf<void> = 0;

        template <typename Cls, typename... Args>
        struct ArgParser {
            static constexpr bool ForInstanceMethod = !std::is_same_v<Cls, void>;
            static constexpr bool ForClassMethod    = false;
            static constexpr bool ForStaticMethod   = false;
            static constexpr bool ForFreeFunction   = false;

            using Type = std::tuple<>;
            static constexpr std::size_t Arity = 0;

            static_assert(ForInstanceMethod, "Only instance methods are allowed to have no parameters");

            ALWAYS_INLINE static auto Parse(PyObject *self, PyObject *const *args, Py_ssize_t nargs) noexcept -> Type {
                BOROS_UNUSED(self, args, nargs);
                return Type{};
            }
        };

        template <typename Cls, typename First, typename... Rest>
        struct ArgParser<Cls, First, Rest...> {
            static constexpr bool ForInstanceMethod = !std::is_same_v<Cls, void>;
            static constexpr bool ForClassMethod    = std::is_same_v<First, PyTypeObject*>;
            static constexpr bool ForStaticMethod   = std::is_same_v<First, std::nullptr_t>;
            static constexpr bool ForFreeFunction   = std::is_same_v<First, Module>;

            using Type = std::tuple<First, Rest...>;
            static constexpr std::size_t Arity = 1 + sizeof...(Rest);

        private:
            template <std::size_t... Is>
            ALWAYS_INLINE static auto ParseImpl(PyObject *self, PyObject *const *args, std::index_sequence<Is...>) noexcept -> Type {
                Type res;

                // Parse the function arguments. An instance method handles self
                // separately, all other function types must respect self also.
                if constexpr (ForInstanceMethod) {
                    (FromPython(std::get<Is>(res), args[Is]), ...);
                } else {
                    FromPython(std::get<0>(res), self);
                    (FromPython(std::get<Is + 1>(res), args[Is]), ...);
                }

                return res;
            }

        public:
            static auto Parse(PyObject *self, PyObject *const *args, Py_ssize_t nargs) noexcept -> Type {
                // For instance methods, the amount of expected arguments is equal
                // to the arity because self has special handling. In all other
                // cases, we require one less argument than we have parameters on
                // the function because the first argument will be self.
                constexpr auto ExpectedArgs = Arity - !ForInstanceMethod;

                Type res;

                if (nargs != ExpectedArgs) [[unlikely]] {
                    PyErr_Format(PyExc_TypeError,
                        "Expected %zu arguments, got %zu instead", ExpectedArgs, nargs);
                    return res;
                }

                return ParseImpl(self, args, std::make_index_sequence<ExpectedArgs>{});
            }
        };

        template <typename Cls, typename Ret, typename... Args>
        struct FunctionTraitsBase {
            using ClassType = Cls;
            using RetType   = Ret;
            using Parser    = ArgParser<Cls, Args...>;
        };

        template <typename>
        struct FunctionTraits;

        template <typename Ret, typename... Args>
        struct FunctionTraits<Ret(*)(Args...) noexcept> : public FunctionTraitsBase<void, Ret, Args...> {};

        template <typename Cls, typename Ret, typename... Args>
        struct FunctionTraits<Ret(Cls::*)(Args...) noexcept> : public FunctionTraitsBase<Cls, Ret, Args...> {};

        template <typename Cls, typename Ret, typename... Args>
        struct FunctionTraits<Ret(Cls::*)(Args...) const noexcept> : public FunctionTraitsBase<Cls, Ret, Args...> {};

        template <typename Fn>
        struct FunctionTraits : public FunctionTraits<decltype(&Fn::operator())> {};

        template <auto Fn>
        ALWAYS_INLINE auto CallFreeFunctionImpl(auto &&args) noexcept -> decltype(auto) {
            return std::apply(Fn, std::forward<decltype(args)>(args));
        }

        template <typename Obj, auto Fn>
        ALWAYS_INLINE auto CallMemberFunctionImpl(PyObject *self, auto &&args) noexcept -> decltype(auto) {
            auto Wrapper = [self] (auto &&...unpacked_args) noexcept -> decltype(auto) {
                auto *obj = reinterpret_cast<Obj*>(self);
                return (obj->*Fn)(std::forward<decltype(unpacked_args)>(unpacked_args)...);
            };

            return std::apply(Wrapper, std::forward<decltype(args)>(args));
        }

        template <auto Fn>
        auto MethodWrapper(PyObject *self, PyObject *const *args, Py_ssize_t nargs) noexcept -> PyObject* {
            using Traits = FunctionTraits<decltype(Fn)>;
            using Parser = typename Traits::Parser;
            using Cls    = typename Traits::ClassType;
            using Ret    = typename Traits::RetType;

            // Parse function arguments into C++ values.
            auto cpp_args = Parser::Parse(self, args, nargs);
            if (PyErr_Occurred() != nullptr) [[unlikely]] {
                return nullptr;
            }

            // Call the wrapped function and detect exceptions raised from it.
            if constexpr(std::is_same_v<Ret, void>) {
                if constexpr(Parser::ForInstanceMethod) {
                    CallMemberFunctionImpl<Cls, Fn>(self, std::move(cpp_args));
                } else {
                    CallFreeFunctionImpl<Fn>(std::move(cpp_args));
                }

                if (PyErr_Occurred()) [[unlikely]] {
                    return nullptr;
                }

                Py_RETURN_NONE;
            } else {
                Ret res;
                if constexpr(Parser::ForInstanceMethod) {
                    res = CallMemberFunctionImpl<Cls, Fn>(self, std::move(cpp_args));
                } else {
                    res = CallFreeFunctionImpl<Fn>(std::move(cpp_args));
                }

                if (PyErr_Occurred()) [[unlikely]] {
                    return nullptr;
                }

                return ToPython(res);
            }
        }

    }

    /// Creates a table of type slots suitable for use with type
    /// object creation.
    template <typename... Ts> requires (std::is_same_v<Ts, PyType_Slot> && ...)
    ALWAYS_INLINE constexpr auto TypeSlotTable(Ts... slots) noexcept -> std::array<PyType_Slot, sizeof...(Ts) + 1> {
        return {{slots..., {0, nullptr}}};
    }

    template <typename T> requires std::is_pointer_v<T>
    ALWAYS_INLINE auto TypeSlot(int slot, T ptr) noexcept -> PyType_Slot {
        return {slot, reinterpret_cast<void*>(ptr)};
    }

    /// Creates a table of method objects suitable for use with module
    /// or type object creation.
    template <typename... Ms> requires (std::is_same_v<Ms, PyMethodDef> && ...)
    ALWAYS_INLINE constexpr auto MethodTable(Ms... methods) noexcept -> std::array<PyMethodDef, sizeof...(Ms) + 1> {
        return {{methods..., {nullptr, nullptr, 0, nullptr}}};
    }

    /// Creates a Python method entry by wrapping a given C++ function.
    /// Argument and return type are determined and converted to Python
    /// values automatically.
    template <auto Fn>
    ALWAYS_INLINE auto Method(const char *name, const char *doc = nullptr) noexcept -> PyMethodDef {
        using Traits = impl::FunctionTraits<decltype(Fn)>;
        using Parser = typename Traits::Parser;

        // This enables a more performant Python calling convention where
        // function arguments don't need to be packed into tuple objects.
        int flags = METH_FASTCALL;

        // Determine if we're dealing with a classmethod, a staticmethod
        // or a free function and set appropriate flags.
        if constexpr(!Parser::ForInstanceMethod) {
            if constexpr(Parser::ForClassMethod) {
                flags |= METH_CLASS;
            } else if constexpr(Parser::ForStaticMethod) {
                flags |= METH_STATIC;
            } else {
                static_assert(Parser::ForFreeFunction,
                    "Failed to determine the desired method type");
            }
        }

        // Return a PyMethodDef with the given function wrapped.
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wcast-function-type"
        return {name, reinterpret_cast<PyCFunction>(&impl::MethodWrapper<Fn>), flags, doc};
        #pragma GCC diagnostic pop
    }

    template <typename T, typename Extra = void> requires PythonObject<T>
    auto TypeSpec(const char *name, PyType_Slot *slots, unsigned int flags = Py_TPFLAGS_DEFAULT) -> PyType_Spec {
        return {name, sizeof(T), impl::SizeOf<Extra>, flags, slots};
    }

}
