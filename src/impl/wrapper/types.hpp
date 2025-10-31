// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "wrapper/python_header.hpp"

#include <array>
#include <tuple>

#include "macros.hpp"
#include "wrapper/conversion.hpp"
#include "wrapper/module.hpp"

namespace boros::python {

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

            static_assert(ForInstanceMethod, "Only instance methods can have no arguments");

            ALWAYS_INLINE static auto Parse(PyObject *self, PyObject *const *args, Py_ssize_t nargs) noexcept -> Type {
                BOROS_UNUSED(self, args, nargs);
                return Type{};
            }
        };

        template <typename Cls, typename First, typename... Rest>
        struct ArgParser<Cls, First, Rest...> {
        private:
            template <typename T>
            static constexpr bool ForFreeFunctionImpl = false;

            template <typename T>
            static constexpr bool ForFreeFunctionImpl<Module<T>> = true;

        public:
            static constexpr bool ForInstanceMethod = !std::is_void_v<Cls>;
            static constexpr bool ForClassMethod    = std::is_same_v<First, PyTypeObject*>;
            static constexpr bool ForStaticMethod   = std::is_same_v<First, std::nullptr_t>;
            static constexpr bool ForFreeFunction   = ForFreeFunctionImpl<First>;

            using Type = std::tuple<First, Rest...>;
            static constexpr std::size_t Arity = 1 + sizeof...(Rest);

        private:
            template <std::size_t... Is>
            ALWAYS_INLINE static auto ParseImpl(Type &res, PyObject *self, PyObject *const *args, std::index_sequence<Is...>) noexcept -> void {
                // Parse the function arguments. An instance method handles self
                // separately, all other function types must respect self also.
                if constexpr (ForInstanceMethod) {
                    (FromPython(std::get<Is>(res), args[Is]), ...);
                } else {
                    FromPython(std::get<0>(res), self);
                    (FromPython(std::get<Is + 1>(res), args[Is]), ...);
                }
            }

        public:
            static auto Parse(PyObject *self, PyObject *const *args, Py_ssize_t nargs) noexcept -> Type {
                // The amount of expected arguments differs based on whether we
                // are invoking an instance method or any other function type
                // since self is not a parameter for instance methods.
                constexpr auto ExpectedArgs = Arity - !ForInstanceMethod;

                Type res;
                if (nargs != ExpectedArgs) [[unlikely]] {
                    PyErr_Format(PyExc_TypeError,
                        "Expected %zu arguments, got %zu instead", ExpectedArgs, nargs);
                    return res;
                }
                ParseImpl(res, self, args, std::make_index_sequence<ExpectedArgs>{});
                return res;
            }
        };

        template <typename Cls, typename Ret, typename... Args> requires (PythonObject<Cls> || std::is_void_v<Cls>) && (PythonConvertible<Ret> || std::is_void_v<Ret>) && (PythonConvertible<Args> && ...)
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

        template <auto Fn>
        ALWAYS_INLINE auto CallFreeFunctionImpl(auto &&args) noexcept -> decltype(auto) {
            return std::apply(Fn, std::forward<decltype(args)>(args));
        }

        template <typename Cls, auto Fn>
        ALWAYS_INLINE auto CallMemberFunctionImpl(PyObject *self, auto &&args) noexcept -> decltype(auto) {
            auto Wrapper = [self] (auto &&...unpacked_args) noexcept -> decltype(auto) {
                auto *obj = reinterpret_cast<Cls*>(self);
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

            // Call the wrapped C++ function and detect Python exceptions.
            if constexpr(std::is_void_v<Ret>) {
                if constexpr(Parser::ForInstanceMethod) {
                    CallMemberFunctionImpl<Cls, Fn>(self, std::move(cpp_args));
                } else {
                    CallFreeFunctionImpl<Fn>(std::move(cpp_args));
                }

                if (PyErr_Occurred() != nullptr) [[unlikely]] {
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

                if (PyErr_Occurred() != nullptr) [[unlikely]] {
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

    /// Creates a table of module slots suitable for use with
    /// module object creation.
    template <typename... Ts> requires (std::is_same_v<Ts, PyModuleDef_Slot> && ...)
    ALWAYS_INLINE constexpr auto ModuleSlotTable(Ts... slots) noexcept -> std::array<PyModuleDef_Slot, sizeof...(Ts) + 1> {
        return {{slots..., {0, nullptr}}};
    }

    /// Creates a Python type slot entry from the given data.
    template <typename Ptr> requires std::is_pointer_v<Ptr>
    ALWAYS_INLINE auto TypeSlot(int slot, Ptr ptr) noexcept -> PyType_Slot {
        return {slot, reinterpret_cast<void*>(ptr)};
    }

    /// Creates a Python module slot entry from the given data.
    template <typename Ptr> requires std::is_pointer_v<Ptr>
    ALWAYS_INLINE auto ModuleSlot(int slot, Ptr ptr) noexcept -> PyModuleDef_Slot {
        return {slot, reinterpret_cast<void*>(ptr)};
    }

    /// Creates a table of methods suitable for use with module
    /// or type object creation.
    template <typename... Ms> requires (std::is_same_v<Ms, PyMethodDef> && ...)
    ALWAYS_INLINE constexpr auto MethodTable(Ms... methods) noexcept -> std::array<PyMethodDef, sizeof...(Ms) + 1> {
        return {{methods..., {nullptr, nullptr, 0, nullptr}}};
    }

    /// Creates a Python method by wrapping a given C++ function.
    /// Arguments and return type will be detected and converted
    /// between Python objects internally.
    template <auto Fn>
    ALWAYS_INLINE auto Method(const char *name, const char *doc = nullptr) noexcept -> PyMethodDef {
        using Traits = impl::FunctionTraits<decltype(Fn)>;
        using Parser = typename Traits::Parser;

        int flags = METH_FASTCALL;

        // Determine what type of function we're dealing with.
        if constexpr(!Parser::ForInstanceMethod) {
            if constexpr(Parser::ForClassMethod) {
                flags |= METH_CLASS;
            } else if constexpr(Parser::ForStaticMethod) {
                flags |= METH_STATIC;
            } else {
                static_assert(Parser::ForFreeFunction, "Failed to determine the desired method type");
            }
        }

        // Return a PyMethodDef with the given function wrapped.
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wcast-function-type"
        return {name, reinterpret_cast<PyCFunction>(&impl::MethodWrapper<Fn>), flags, doc};
        #pragma GCC diagnostic pop
    }

    /// Creates a new Python type specification from the given metadata.
    template <typename T, typename Extra = void> requires PythonObject<T>
    ALWAYS_INLINE auto TypeSpec(const char *name, PyType_Slot *slots, unsigned int flags = Py_TPFLAGS_DEFAULT) noexcept -> PyType_Spec {
        return {name, sizeof(T), impl::SizeOf<Extra>, flags, slots};
    }

}
