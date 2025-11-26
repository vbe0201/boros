// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <tuple>
#include <type_traits>

#include "macros.hpp"

namespace boros::python {

    /// Specifies a C++ type suitable for use as a Python object.
    template <typename T>
    concept PythonObject =
        std::is_same_v<T, PyObject> ||
        std::is_same_v<T, PyTypeObject> ||
        (
            std::is_standard_layout_v<T> &&
            requires (T &obj) {
                // The documentation states what PyObject_HEAD expands to.
                // It should be fine to hardcode the name here.
                { obj.ob_base } -> std::same_as<PyObject&>;
            }
        );

    /// A type suitable for use as per-instance module state.
    template <typename T>
    concept ModuleState = requires (T &state, visitproc visit, void *arg) {
        { state.Traverse(visit, arg) } -> std::same_as<int>;
        { state.Clear()              } -> std::same_as<int>;
        { state.Free()               } -> std::same_as<void>;
    };

    /// Gets the per-module state through a module object.
    template <ModuleState State>
    ALWAYS_INLINE auto GetModuleState(PyObject *module) -> State& {
        return *static_cast<State*>(PyModule_GetState(module));
    }

    /// Gets the per-module state through a Python type object.
    template <ModuleState State>
    ALWAYS_INLINE auto GetModuleStateFromType(PyTypeObject *tp) -> State& {
        return GetState<State>(PyType_GetModule(tp));
    }

    /// Gets the per-module state through a Python object.
    template <ModuleState State>
    ALWAYS_INLINE auto GetModuleStateFromObject(PyObject *obj) -> State& {
        return GetStateFromType<State>(Py_TYPE(obj));
    }

    /// Instantiates a new type from a spec and binds it to a module.
    ALWAYS_INLINE auto AddTypeToModule(PyObject *module, PyType_Spec &spec) -> PyTypeObject* {
        auto *tp = PyType_FromModuleAndSpec(module, &spec, nullptr);
        if (tp == nullptr) [[unlikely]] {
            return nullptr;
        }

        if (PyModule_AddObjectRef(module, spec.name, tp) < 0) [[unlikely]] {
            return nullptr;
        }

        return reinterpret_cast<PyTypeObject*>(tp);
    }

    /// Represents a Python extension module.
    template <ModuleState State>
    struct ModuleDefinition {
    private:
        PyModuleDef m_module;

    private:
        static auto Traverse(PyObject *module, visitproc visit, void *arg) -> int {
            auto &state = GetModuleState<State>(module);
            return state.Traverse(visit, arg);
        }

        static auto Clear(PyObject *module) -> int {
            auto &state = GetModuleState<State>(module);
            return state.Clear();
        }

        static auto Free(void *module) -> void {
            auto *mod = static_cast<PyObject*>(module);
            auto &state = GetModuleState<State>(mod);
            state.Free();
            Clear(mod);
        }

    public:
        constexpr ModuleDefinition(const char *name, const char *doc, PyMethodDef *methods, PyModuleDef_Slot *slots)
            : m_module{PyModuleDef_HEAD_INIT, name, doc, sizeof(State), methods, slots, &Traverse, &Clear, &Free} {}

        auto CreateModule() -> PyObject* {
            return PyModuleDef_Init(&m_module);
        }
    };

    /// A marker for Python extension methods.
    struct Module {
        PyObject *raw;
    };

    /// A marker for Python classmethods.
    struct Class {
        PyTypeObject *tp;
    };

    /// A marker for Python staticmethods.
    struct Static {};

    template <typename Ptr> requires PythonObject<std::remove_pointer_t<Ptr>>
    auto FromPython(Ptr &out, PyObject *obj) -> void {
        out = reinterpret_cast<Ptr>(obj);
    }

    template <typename Ptr> requires PythonObject<std::remove_pointer_t<Ptr>>
    auto ToPython(Ptr value) -> PyObject* {
        return reinterpret_cast<PyObject*>(value);
    }

    ALWAYS_INLINE auto FromPython(bool &out, PyObject *obj) -> void {
        if (!PyBool_Check(obj)) [[unlikely]] {
            PyErr_SetString(PyExc_TypeError, "Expected a boolean value");
            return;
        }
        out = (obj == Py_True);
    }

    ALWAYS_INLINE auto ToPython(bool value) -> PyObject* {
        return PyBool_FromLong(value);
    }

    template <std::signed_integral I>
    ALWAYS_INLINE auto FromPython(I &out, PyObject *obj) -> void {
        constexpr long long MinValue = std::numeric_limits<I>::min();
        constexpr long long MaxValue = std::numeric_limits<I>::max();

        int overflow;
        long long tmp = PyLong_AsLongLongAndOverflow(obj, &overflow);
        if (overflow != 0 || tmp < MinValue || tmp > MaxValue) [[unlikely]] {
            PyErr_SetString(PyExc_OverflowError, "Python int too large to convert to C int");
            return;
        }

        out = static_cast<I>(tmp);
    }

    template <std::signed_integral I>
    ALWAYS_INLINE auto ToPython(I value) -> PyObject* {
        return PyLong_FromLongLong(value);
    }

    template <std::unsigned_integral I>
    ALWAYS_INLINE auto FromPython(I &out, PyObject *obj) -> void {
        constexpr unsigned long long MaxValue = std::numeric_limits<I>::max();

        unsigned long long tmp = PyLong_AsUnsignedLongLong(obj);
        if (tmp > MaxValue) [[unlikely]] {
            PyErr_SetString(PyExc_OverflowError, "Python int too large to convert to C int");
            return;
        }

        out = static_cast<I>(tmp);
    }

    template <std::unsigned_integral I>
    ALWAYS_INLINE auto ToPython(I value) -> PyObject* {
        return PyLong_FromUnsignedLongLong(value);
    }

    ALWAYS_INLINE auto FromPython(Module &out, PyObject *obj) -> void {
        assert(PyModule_Check(obj));
        out.raw = obj;
    }

    ALWAYS_INLINE auto ToPython(Module mod) -> PyObject* {
        return mod.raw;
    }

    ALWAYS_INLINE auto FromPython(Class &out, PyObject *obj) -> void {
        assert(PyType_Check(obj));
        out.tp = reinterpret_cast<PyTypeObject*>(obj);
    }

    ALWAYS_INLINE auto ToPython(Class cls) -> PyObject* {
        return reinterpret_cast<PyObject*>(cls.tp);
    }

    ALWAYS_INLINE auto FromPython(Static&, PyObject *obj) -> void {
        BOROS_UNUSED(obj);
        assert(obj == nullptr);
    }

    ALWAYS_INLINE auto ToPython(Static) -> PyObject* {
        return nullptr;
    }

    /// A C++ type that can be converted to and from a Python object.
    template <typename T>
    concept PythonConvertible = requires (PyObject *obj, T &out, T value) {
        { FromPython(out, obj) } -> std::same_as<void>;
        { ToPython(value)      } -> std::same_as<PyObject*>;
    };

    /// Represents an ABI compatible Python object with an inner C++
    /// type payload. This decouples the implementation details of a
    /// PyObject from the business logic of the extension.
    template <typename T>
    struct Object {
        // Common object header for all Python objects.
        PyObject_HEAD

        // The inner C++ object to wrap. This is inside a
        // union to control its entire lifetime manually.
        union {
            T inner;
        };

        /// Gets the inner payload of the object.
        ALWAYS_INLINE auto Get() -> T& {
            return inner;
        }
    };

    /// Allocates a new Python object from the corresponding type.
    template <typename T>
    auto Alloc(PyTypeObject *tp) -> Object<T>* {
        auto *tp_alloc = reinterpret_cast<allocfunc>(PyType_GetSlot(tp, Py_tp_alloc));

        auto *self = reinterpret_cast<Object<T>*>(tp_alloc(tp, 0));
        if (self != nullptr) [[likely]] {
            std::construct_at(&self->inner);
        }

        return self;
    }

    /// Implements the builtin __new__ for a type by allocating an
    /// instance and default-constructing the C++ type.
    template <typename T>
    auto DefaultNew(PyTypeObject *tp, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds)) -> PyObject* {
        return reinterpret_cast<PyObject*>(Alloc<T>(tp));
    }

    /// Deallocates a Python object by calling its C++ destructor,
    /// then freeing the allocation.
    template <typename T>
    auto DefaultDealloc(PyObject *self) -> void {
        PyTypeObject *tp = Py_TYPE(self);
        auto *tp_free = reinterpret_cast<freefunc>(PyType_GetSlot(tp, Py_tp_free));

        std::destroy_at(&reinterpret_cast<Object<T>*>(self)->inner);
        tp_free(self);
    }

    namespace impl {

        template <typename Cls, typename Ret, typename... Args>
            requires (PythonConvertible<Ret> || std::is_void_v<Ret>) && (PythonConvertible<Args> && ...)
        struct FunctionTraitsBase {
            using ClassType = Cls;
            using RetType   = Ret;
            using ArgsType  = std::tuple<Args...>;
        };

        template <typename>
        struct FunctionTraits;

        template <typename Ret, typename... Args>
        struct FunctionTraits<Ret(*)(Args...)> : public FunctionTraitsBase<void, Ret, Args...> {};

        template <typename Cls, typename Ret, typename... Args>
        struct FunctionTraits<Ret(Cls::*)(Args...)> : public FunctionTraitsBase<Cls, Ret, Args...> {};

        template <typename Cls, typename Ret, typename... Args>
        struct FunctionTraits<Ret(Cls::*)(Args...) const> : public FunctionTraitsBase<Cls, Ret, Args...> {};

        template <typename Cls, typename Tuple>
        struct ArgParser {
            static constexpr bool InstanceMethod = !std::is_void_v<Cls>;
            static constexpr bool ClassMethod    = false;
            static constexpr bool ModuleMethod   = false;
            static constexpr bool StaticMethod   = false;

            static constexpr std::size_t Arity = 0;

            static auto Parse(PyObject *Py_UNUSED(self), PyObject *const *Py_UNUSED(args), Py_ssize_t Py_UNUSED(nargs)) -> Tuple {
                static_assert(InstanceMethod, "Only instance methods can have no arguments");
                return Tuple{};
            }
        };

        template <typename Cls, typename First, typename... Rest>
        struct ArgParser<Cls, std::tuple<First, Rest...>> {
            static constexpr bool InstanceMethod = !std::is_void_v<Cls>;
            static constexpr bool ClassMethod    = !InstanceMethod && std::is_same_v<First, Class>;
            static constexpr bool ModuleMethod   = !InstanceMethod && std::is_same_v<First, Module>;
            static constexpr bool StaticMethod   = !InstanceMethod && std::is_same_v<First, Static>;

            // The amount of expected arguments differs based on whether we
            // are wrapping an instance method since self is the implicit
            // this instead of an actual function parameter.
            static constexpr std::size_t Arity = (1 + sizeof...(Rest)) - !InstanceMethod;

            static auto Parse(PyObject *self, PyObject *const *args, Py_ssize_t nargs) -> std::tuple<First, Rest...> {
                std::tuple<First, Rest...> res;
                if (nargs != Arity) [[unlikely]] {
                    PyErr_Format(PyExc_TypeError,
                        "Expected %zu arguments, got %zu instead", Arity, nargs);
                    return res;
                }

                if constexpr(Arity > 0) {
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) ALWAYS_INLINE_LAMBDA {
                        if constexpr (InstanceMethod) {
                            (FromPython(std::get<Is>(res), args[Is]), ...);
                        } else {
                            FromPython(std::get<0>(res), self);
                            (FromPython(std::get<Is + 1>(res), args[Is]), ...);
                        }
                    }(std::make_index_sequence<Arity>{});
                }

                return res;
            }
        };

        template <auto Fn>
        struct MethodHelper {
            using Traits = FunctionTraits<decltype(Fn)>;
            using Cls    = typename Traits::ClassType;
            using Args   = typename Traits::ArgsType;
            using Ret    = typename Traits::RetType;
            using Parser = ArgParser<Cls, Args>;

            static constexpr int Flags = [] {
                int flags = 0;

                if constexpr(Parser::Arity == 0) {
                    flags |= METH_NOARGS;
                } else if constexpr(Parser::Arity == 1) {
                    flags |= METH_O;
                } else {
                    flags |= METH_FASTCALL;
                }

                if constexpr(Parser::ClassMethod) {
                    flags |= METH_CLASS;
                } else if constexpr(Parser::StaticMethod) {
                    flags |= METH_STATIC;
                } else {
                    static_assert(Parser::InstanceMethod || Parser::ModuleMethod,
                        "Failed to determine the desired method type");
                }

                return flags;
            }();

        private:
            ALWAYS_INLINE static auto CallFreeFunctionImpl(auto &&args) -> decltype(auto) {
                return std::apply(Fn, std::forward<decltype(args)>(args));
            }

            ALWAYS_INLINE static auto CallMemberFunctionImpl(PyObject *self, auto &&args) -> decltype(auto) {
                auto Wrapper = [self] (auto &&...unpacked_args) ALWAYS_INLINE_LAMBDA -> decltype(auto) {
                    auto *obj = reinterpret_cast<Object<Cls>*>(self);
                    return (obj->inner.*Fn)(std::forward<decltype(unpacked_args)>(unpacked_args)...);
                };

                return std::apply(Wrapper, std::forward<decltype(args)>(args));
            }

            static auto CallImpl(PyObject *self, auto &&args) -> PyObject* {
                if constexpr(std::is_void_v<Ret>) {
                    if constexpr(Parser::InstanceMethod) {
                        CallMemberFunctionImpl(self, std::move(args));
                    } else {
                        CallFreeFunctionImpl(std::move(args));
                    }

                    if (PyErr_Occurred() != nullptr) [[unlikely]] {
                        return nullptr;
                    }

                    Py_RETURN_NONE;
                } else {
                    Ret ret;
                    if constexpr (Parser::InstanceMethod) {
                        ret = CallMemberFunctionImpl(self, std::move(args));
                    } else {
                        ret = CallFreeFunctionImpl(std::move(args));
                    }

                    if (PyErr_Occurred() != nullptr) [[unlikely]] {
                        return nullptr;
                    }

                    return ToPython<Ret>(ret);
                }
            }

            static auto Fastcall(PyObject *self, PyObject *const *args, Py_ssize_t nargs) -> PyObject* {
                auto cpp_args = Parser::Parse(self, args, PyVectorcall_NARGS(nargs));
                if (PyErr_Occurred() != nullptr) [[unlikely]] {
                    return nullptr;
                }

                return CallImpl(self, std::move(cpp_args));
            }

            static auto Fastcall1(PyObject *self, PyObject *arg) -> PyObject* {
                PyObject *args[1] = {arg};
                return Fastcall(self, args, 1);
            }

            static auto Fastcall0(PyObject *self, PyObject *Py_UNUSED(arg)) -> PyObject* {
                return Fastcall(self, nullptr, 0);
            }

        public:
            ALWAYS_INLINE static auto GetCallback() -> PyCFunction {
                if constexpr(Parser::Arity == 0) {
                    return reinterpret_cast<PyCFunction>(&Fastcall0);
                } else if constexpr(Parser::Arity == 1) {
                    return reinterpret_cast<PyCFunction>(&Fastcall1);
                } else {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wcast-function-type"
                    return reinterpret_cast<PyCFunction>(&Fastcall);
                    #pragma GCC diagnostic pop
                }
            }
        };

    }

    /// Creates a table of type slots suitable for use with type
    /// object creation.
    template <typename... Ts> requires (std::is_same_v<Ts, PyType_Slot> && ...)
    ALWAYS_INLINE auto TypeSlotTable(Ts... slots) -> std::array<PyType_Slot, sizeof...(Ts) + 1> {
        return {{slots..., {0, nullptr}}};
    }

    /// Creates a table of module slots suitable for use with
    /// module object creation.
    template <typename... Ts> requires (std::is_same_v<Ts, PyModuleDef_Slot> && ...)
    constexpr auto ModuleSlotTable(Ts... slots) -> std::array<PyModuleDef_Slot, sizeof...(Ts) + 1> {
        return {{slots..., {0, nullptr}}};
    }

    /// Creates a Python type slot entry from the given data.
    template <typename Ptr> requires std::is_pointer_v<Ptr>
    ALWAYS_INLINE auto TypeSlot(int slot, Ptr ptr) -> PyType_Slot {
        return {slot, reinterpret_cast<void*>(ptr)};
    }

    /// Creates a Python module slot entry from the given data.
    template <typename Ptr> requires std::is_pointer_v<Ptr>
    ALWAYS_INLINE auto ModuleSlot(int slot, Ptr ptr) -> PyModuleDef_Slot {
        return {slot, reinterpret_cast<void*>(ptr)};
    }

    /// Creates a table of methods suitable for use with module
    /// or type object creation.
    template <typename... Ms> requires (std::is_same_v<Ms, PyMethodDef> && ...)
    constexpr auto MethodTable(Ms... methods) -> std::array<PyMethodDef, sizeof...(Ms) + 1> {
        return {{methods..., {nullptr, nullptr, 0, nullptr}}};
    }

    template <typename... Ms> requires (std::is_same_v<Ms, PyMemberDef> && ...)
    constexpr auto MemberTable(Ms... members) -> std::array<PyMemberDef, sizeof...(Ms) + 1> {
        return {{members..., {nullptr, 0, 0, 0, nullptr}}};
    }

    namespace impl {

        template <typename>
        constexpr inline int MemberTypeTag = 0;

        template <>
        constexpr inline int MemberTypeTag<char> = Py_T_BYTE;

        template <>
        constexpr inline int MemberTypeTag<short> = Py_T_SHORT;

        template <>
        constexpr inline int MemberTypeTag<int> = Py_T_INT;

        template <>
        constexpr inline int MemberTypeTag<long> = Py_T_LONG;

        template <>
        constexpr inline int MemberTypeTag<long long> = Py_T_LONGLONG;

        template <>
        constexpr inline int MemberTypeTag<unsigned char> = Py_T_UBYTE;

        template <>
        constexpr inline int MemberTypeTag<unsigned int> = Py_T_UINT;

        template <>
        constexpr inline int MemberTypeTag<unsigned short> = Py_T_USHORT;

        template <>
        constexpr inline int MemberTypeTag<unsigned long> = Py_T_ULONG;

        template <>
        constexpr inline int MemberTypeTag<unsigned long long> = Py_T_ULONGLONG;

        template <>
        constexpr inline int MemberTypeTag<float> = Py_T_FLOAT;

        template <>
        constexpr inline int MemberTypeTag<double> = Py_T_DOUBLE;

        template <>
        constexpr inline int MemberTypeTag<bool> = Py_T_BOOL;

        template <>
        constexpr inline int MemberTypeTag<const char*> = Py_T_STRING;

        template <std::size_t N>
        constexpr inline int MemberTypeTag<const char[N]> = Py_T_STRING_INPLACE;

        template <>
        constexpr inline int MemberTypeTag<PyObject*> = Py_T_OBJECT_EX;

        template <typename T, std::size_t Offset>
        constexpr auto MemberImpl(const char *name, int flags = 0, const char *doc = nullptr) -> PyMemberDef {
            return {name, MemberTypeTag<T>, Offset, flags, doc};
        }

    }

    /// Maps a C++ struct member to a Python class member.
    #define PythonMember(type, name, ...)                                         \
        ::boros::python::impl::MemberImpl<                                        \
            decltype(type::name),                                                 \
            offsetof(::boros::python::Object<type>, inner) + offsetof(type, name) \
        >(BOROS_STR(name) __VA_OPT__(,) __VA_ARGS__)

    /// Creates a Python method by wrapping a given C++ function.
    /// Arguments and return types will be detected and converted
    /// between Python objects internally.
    template <auto Fn>
    ALWAYS_INLINE auto Method(const char *name, const char *doc = nullptr) -> PyMethodDef {
        using Helper = impl::MethodHelper<Fn>;
        return {name, Helper::GetCallback(), Helper::Flags, doc};
    }

    /// Creates a new Python type specification from the given
    /// metadata.
    template <typename T>
    constexpr auto TypeSpec(const char *name, PyType_Slot *slots, unsigned int flags = Py_TPFLAGS_DEFAULT) -> PyType_Spec {
        return {name, sizeof(Object<T>), 0, flags, slots};
    }

}
