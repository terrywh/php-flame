//
// Project: C++ delegates
// File content:
//   - rome::delegate<Ret(Args...), Behavior>
//   - rome::fwd_delegate<void(Args...), Behavior>
//   - rome::event_delegate<void(Args...)>
//   - rome::command_delegate<void(Args...)>
// See the documentation in folder `doc` for more information.
//
// The rome::delegate implementation is based on the article of
// Sergey Ryazanov: "The Impossibly Fast C++ Delegates", 18 Jul 2005
// https://www.codeproject.com/articles/11015/the-impossibly-fast-c-delegates
//
// Copyright Roger Mettler 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ROME_DELEGATE_HPP
#define ROME_DELEGATE_HPP

#pragma once

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>


namespace rome {

// Used as template argument for delegates to declare that it invoking an empty delegate is valid
// behavior.
struct target_is_optional;
// Used as template argument for delegates to declare that an assigned target is expected when the
// delegate is invoked and leads to an exception otherwise.
struct target_is_expected;
// Used as template argument for delegates to declare that an assigned target is mandatory when the
// delegate is invoked and thus the design ensures that always a delegate is assigned.
struct target_is_mandatory;


class bad_delegate_call : public std::exception {
  public:
    auto what() const noexcept -> const char* override {
        return "rome::bad_delegate_call";
    }
};

namespace detail {
    namespace delegate {
        // The type used to store any small buffer optimizable data inside the delegate.
        using buffer_type = void*;

        constexpr std::size_t buffer_alignment =
            std::max(sizeof(buffer_type), alignof(buffer_type));

        // Returns whether size and alignment of type T are small enough so that it can be stored
        // within the delegate.
        template<typename T>
        constexpr bool is_small_buffer_optimizable =
            (sizeof(T) <= sizeof(buffer_type))
            && (alignof(T) <= buffer_alignment);  // NOLINT(misc-redundant-expression)

        // Type used to store the function that inkvokes the target when the delegate is called.
        template<typename Ret, typename... Args>
        using target_invoker_t = Ret (*)(buffer_type&, Args...);

        // Type used to store the function that deletes a possible assigned target.
        using target_deleter_t = void (*)(buffer_type&);


        // Used by a delegate when nothing needs to be done.
        template<typename Ret, typename... Args>
        auto do_nothing(buffer_type&, Args...) -> Ret {
        }


        // Used by an empty delegate when calling the delegate is invalid.
        template<typename Ret, typename... Args>
        [[noreturn]] auto throw_on_call(buffer_type&, Args...) -> Ret {
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND))
            throw rome::bad_delegate_call{};
#else
            std::terminate();
#endif
        }

        // Used by a delegate with an assigned functor that was small buffer optimized inside the
        // delegate.
        template<typename Functor, typename Ret, typename... Args>
        auto invoke_small_buffer_optimized_functor(buffer_type& buffer, Args... args) -> Ret {
            auto* pBuffer  = static_cast<void*>(&buffer);
            auto* pFunctor = static_cast<Functor*>(pBuffer);
            return pFunctor->operator()(static_cast<Args>(args)...);
        }

        // Used by a delegate with an assigned functor that was dynamically stored outside of the
        // delegate.
        template<typename Functor, typename Ret, typename... Args>
        auto invoke_dynamically_stored_functor(buffer_type& buffer, Args... args) -> Ret {
            auto* pFunctor = static_cast<Functor*>(buffer);
            return pFunctor->operator()(static_cast<Args>(args)...);
        }


        // Used by a delegate with an assigned functor that was small buffer optimized inside the
        // delegate.
        template<typename Functor>
        void destroy_buffer_optimized_functor(buffer_type& buffer) {
            auto* pBuffer  = static_cast<void*>(&buffer);
            auto* pFunctor = static_cast<Functor*>(pBuffer);
            pFunctor->~Functor();
        }

        // Used by a delegate with an assigned functor that was dynamically stored outside of the
        // delegate.
        template<typename Functor>
        void delete_dynamically_stored_functor(buffer_type& buffer) {
            auto* pFunctor = static_cast<Functor*>(buffer);
            delete pFunctor;
        }


        // The function that is called when a delegate has no target assigned, based on whether it
        // shall throw an exception or not.
        template<bool shallThrow, typename Ret, typename... Args>
        constexpr target_invoker_t<Ret, Args...> empty_invoker;

        template<typename Ret, typename... Args>
        constexpr target_invoker_t<Ret, Args...> empty_invoker<true, Ret, Args...> = throw_on_call;

        template<typename Ret, typename... Args>
        constexpr target_invoker_t<Ret, Args...> empty_invoker<false, Ret, Args...> = do_nothing;
    }  // namespace delegate


    // Implements the actual behavior of all delegates
    template<typename Signature, bool shallThrowWhenEmpty>
    class delegate_core;

    template<typename Ret, typename... Args, bool shallThrowWhenEmpty>
    class delegate_core<Ret(Args...), shallThrowWhenEmpty> {
        using buffer_type = delegate::buffer_type;

        static constexpr auto emptyInvoker =
            delegate::empty_invoker<shallThrowWhenEmpty, Ret, Args...>;

        // buffer_ needs to be writable by `operator()(Args...) const` while small buffer
        // optimization is used
        alignas(delegate::buffer_alignment) mutable buffer_type buffer_ = nullptr;
        delegate::target_invoker_t<Ret, Args...> invokeTarget_          = emptyInvoker;
        delegate::target_deleter_t deleteTarget_                        = delegate::do_nothing;

      public:
        constexpr delegate_core() noexcept           = default;
        delegate_core(const delegate_core&) noexcept = delete;
        delegate_core(delegate_core&& orig) noexcept {
            orig.swap(*this);
        }

        ~delegate_core() {
            (*deleteTarget_)(buffer_);
        }

        auto operator=(const delegate_core&) noexcept -> delegate_core& = delete;
        auto operator=(delegate_core&& orig) noexcept -> delegate_core& {
            delegate_core{std::move(orig)}.swap(*this);
            return *this;
        }

        void drop_target() noexcept {
            delegate_core{}.swap(*this);
        }

        void swap(delegate_core& other) noexcept {
            using std::swap;
            swap(buffer_, other.buffer_);
            swap(invokeTarget_, other.invokeTarget_);
            swap(deleteTarget_, other.deleteTarget_);
        }

        constexpr explicit operator bool() const noexcept {
            return invokeTarget_ != emptyInvoker;
        }

        auto operator()(Args... args) const -> Ret {
            return (*invokeTarget_)(buffer_, static_cast<Args>(args)...);
        }

        // Stores the passed functor inside the local buffer of the delegate.
        template<typename T, typename Functor = std::decay_t<T>>
        static auto from_optimizable_functor(T&& functor) noexcept(
            noexcept(Functor(std::forward<T>(functor)))) -> delegate_core {
            delegate_core d;
            (void)::new (&d.buffer_) Functor(std::forward<T>(functor));
            d.invokeTarget_ =
                delegate::invoke_small_buffer_optimized_functor<Functor, Ret, Args...>;
            d.deleteTarget_ = delegate::destroy_buffer_optimized_functor<Functor>;
            return d;
        }

        // Stores the passed functor at a new location outside the local buffer of the delegate in a
        // dynamically allocated storage.
        template<typename T, typename Functor = std::decay_t<T>>
        static auto from_dynamic_allocated_functor(T&& functor) -> delegate_core {
            delegate_core d;
            d.buffer_       = new Functor(std::forward<T>(functor));
            d.invokeTarget_ = delegate::invoke_dynamically_stored_functor<Functor, Ret, Args...>;
            d.deleteTarget_ = delegate::delete_dynamically_stored_functor<Functor>;
            return d;
        }
    };


    namespace delegate {
        // Always false. Used to mark invalid parameters in static_assert.
        template<typename>
        constexpr bool invalid = false;

        // Whether `Behavior` is one of the valid types.
        template<typename Behavior>
        constexpr bool is_behavior = std::is_same<Behavior, target_is_expected>::value
                                     || std::is_same<Behavior, target_is_mandatory>::value
                                     || std::is_same<Behavior, target_is_optional>::value;

        // Whether `Behavior` is valid for return type `Ret`.
        template<typename Ret, typename Behavior>
        constexpr bool is_valid_behavior = !std::is_same<Behavior, target_is_optional>::value
                                           || (std::is_same<Behavior, target_is_optional>::value
                                               && std::is_same<Ret, void>::value);


        template<typename...>
        using void_t = void;

        template<typename T, typename Sig, typename = void>
        struct is_callable_by_impl : std::false_type {};

        template<typename T, typename Ret, typename... Args>
        struct is_callable_by_impl<T, Ret(Args...),
            void_t<decltype(std::declval<T>()(std::declval<Args>()...))>>
            : std::integral_constant<bool,
                  std::is_convertible<decltype(std::declval<T>()(std::declval<Args>()...)),
                      Ret>::value> {};

        // Returns whether an object of type `T` is callable by a function of signature `Sig` in
        // terms of the arguments are passable from `Sig` to `T` and the return value of `T` is
        // returnable by `Sig`.
        template<typename T, typename Sig>
        constexpr bool is_callable_by = is_callable_by_impl<T, Sig>::value;


        template<typename T>
        struct remove_member_pointer_impl;

        template<typename T, typename C>
        struct remove_member_pointer_impl<T C::*> {
            using type = T;
        };

        // Returns the type of the object the given member object pointer can point to. Compile
        // error if no member object pointer was given.
        template<typename T>
        using remove_member_pointer_t =
            typename remove_member_pointer_impl<std::remove_cv_t<T>>::type;


        // Returns whether given function is const qualified.
        template<typename T>
        struct is_const_function : std::false_type {};

        template<typename Ret, typename... Args>
        struct is_const_function<Ret(Args...) const> : std::true_type {};
        template<typename Ret, typename... Args>
        struct is_const_function<Ret(Args...) const&> : std::true_type {};
        template<typename Ret, typename... Args>
        struct is_const_function<Ret(Args...) const&&> : std::true_type {};
        template<typename Ret, typename... Args>
        struct is_const_function<Ret(Args...) const volatile> : std::true_type {};
        template<typename Ret, typename... Args>
        struct is_const_function<Ret(Args...) const volatile&> : std::true_type {};
        template<typename Ret, typename... Args>
        struct is_const_function<Ret(Args...) const volatile&&> : std::true_type {};


        // Returns whether given type allows that data can be changed, directly or inderectly
        // through some kind of pointer or C-array. It cannot detect if a class allows to mutate
        // data even if declared const (limitation of the C++ language).
        template<typename T, typename = std::true_type>
        struct is_immutable;

        template<typename T>
        struct is_immutable<T,
            std::integral_constant<bool, std::is_void<T>::value || std::is_null_pointer<T>::value
                                             || std::is_function<T>::value>> : std::true_type {};

        template<typename T>
        struct is_immutable<T, std::integral_constant<bool,
                                   std::is_integral<T>::value || std::is_floating_point<T>::value
                                       || std::is_union<T>::value || std::is_enum<T>::value
                                       || std::is_class<T>::value>> : std::is_const<T>::type {};

        template<typename T>
        struct is_immutable<T, std::integral_constant<bool, std::is_array<T>::value>>
            : is_immutable<std::remove_all_extents_t<T>>::type {};

        template<typename T>
        struct is_immutable<T, std::integral_constant<bool, std::is_pointer<T>::value>>
            : std::integral_constant<bool,
                  std::is_const<T>::value && is_immutable<std::remove_pointer_t<T>>::value> {};

        template<typename T>
        struct is_immutable<T,
            std::integral_constant<bool, std::is_member_object_pointer<T>::value>>
            : std::integral_constant<bool,
                  std::is_const<T>::value && is_immutable<remove_member_pointer_t<T>>::value> {};

        template<typename T>
        struct is_immutable<T,
            std::integral_constant<bool, std::is_member_function_pointer<T>::value>>
            : is_const_function<remove_member_pointer_t<T>>::type {};


        // Returns whether given function argument can be considered immutable. The type of the
        // argument does not allow that the callee can change data of the caller, directly or
        // inderectly through some kind references, pointers or C-array. It cannot detect if a class
        // allows to mutate data even if declared const (limitation of the C++ language).
        template<typename Arg>
        constexpr auto is_immutable_argument() -> bool {
            using NoRef = std::remove_reference_t<Arg>;
            if (std::is_lvalue_reference<Arg>::value && !std::is_const<NoRef>::value
                && !std::is_null_pointer<NoRef>::value && !std::is_function<NoRef>::value) {
                return false;
            }
            // Moved or copied arguments are immutable in a sense as it is not possible to change
            // the data at the caller's side at this level.
            return is_immutable<const std::decay_t<Arg>>::value;
        }

        // Returns whether the function arguments `Args...` can be considered immutable.
        template<typename... Args>
        constexpr bool are_immutable_arguments =
            std::is_same<std::integer_sequence<bool, true, is_immutable_argument<Args>()...>,
                std::integer_sequence<bool, is_immutable_argument<Args>()..., true>>::value;
    }  // namespace delegate


    // Provides common delegate behavior using the 'curiously recurring template pattern' so that
    // deriving delegates can reuse the functionality.
    template<typename DerivedDelegate>
    class base_delegate;

    template<template<typename, typename> class DerivedDelegate, typename Ret, typename... Args,
        typename Behavior>
    class base_delegate<DerivedDelegate<Ret(Args...), Behavior>> {
        using delegate_type = DerivedDelegate<Ret(Args...), Behavior>;
        using core_type =
            delegate_core<Ret(Args...), !std::is_same<Behavior, target_is_optional>::value>;
        core_type core_ = {};

      public:
        constexpr base_delegate() noexcept = default;
        base_delegate(core_type&& core) noexcept : core_{std::move(core)} {
        }

        void drop_target() noexcept {
            core_.drop_target();
        }

        void swap(delegate_type& other) noexcept {
            core_.swap(other.core_);
        }

        constexpr explicit operator bool() const noexcept {
            return core_.operator bool();
        }

        auto operator()(Args... args) const -> Ret {
            return core_.operator()(static_cast<Args>(args)...);
        }


        // Creates a new delegate targeting the passed function or static member function.
        template<Ret (*pFunction)(Args...)>
        static constexpr auto create() noexcept -> delegate_type {
            return create(
                [](Args... args) -> Ret { return (*pFunction)(static_cast<Args>(args)...); });
        }

        // Creates a new delegate targeting the non-static member function and related object.
        // Does NOT take ownership of the passed object `obj`.
        template<typename C, Ret (C::*pMethod)(Args...)>
        static auto create(C& obj) noexcept -> delegate_type {
            return create(
                [&obj](Args... args) -> Ret { return (obj.*pMethod)(static_cast<Args>(args)...); });
        }

        // Creates a new delegate targeting the passed non-static const member function and related
        // object. Does NOT take ownership of the passed object `obj`.
        template<typename C, Ret (C::*pMethod)(Args...) const>
        static auto create(const C& obj) noexcept -> delegate_type {
            return create(
                [&obj](Args... args) -> Ret { return (obj.*pMethod)(static_cast<Args>(args)...); });
        }

        // Dummy to capture passed values that are no functors.
        template<typename T, typename Functor = std::decay_t<T>,
            std::enable_if_t<!std::is_class<Functor>::value, int> = 0>
        static auto create(T&&) -> delegate_type {
            static_assert(std::is_class<Functor>::value,
                "Invalid object passed. Object needs to be a functor (a class type with a function "
                "call operator, e.g. a lambda).");
        }

        // Dummy to capture passed objects that cannot be called by the delegate.
        template<typename T, typename Functor = std::decay_t<T>,
            std::enable_if_t<std::is_class<Functor>::value
                                 && !delegate::is_callable_by<Functor, Ret(Args...)>,
                int> = 0>
        static auto create(T&&) -> delegate_type {
            static_assert(delegate::is_callable_by<Functor, Ret(Args...)>,
                "Passed functor has incompatible function call signature. The function call "
                "signature must be compatible with the signature of the delegate so that the "
                "delegate is able to invoke the functor.");
        }

        // Creates a new delegate targeting the passed functor and taking ownership of it.
        // The functor is stored in the local storage of the delegate (no dynamic allocation).
        template<typename T, typename Functor = std::decay_t<T>,
            std::enable_if_t<std::is_class<Functor>::value
                                 && delegate::is_callable_by<Functor, Ret(Args...)>
                                 && delegate::is_small_buffer_optimizable<Functor>,
                int> = 0>
        static auto create(T&& functor) noexcept(
            noexcept(core_type::template from_optimizable_functor(std::forward<T>(functor))))
            -> delegate_type {
            return {core_type::template from_optimizable_functor(std::forward<T>(functor))};
        }

        // Creates a new delegate targeting the passed functor and taking ownership of it.
        // The functor is stored in a dynamically allocated storage outside the delegate.
        template<typename T, typename Functor = std::decay_t<T>,
            std::enable_if_t<std::is_class<Functor>::value
                                 && delegate::is_callable_by<Functor, Ret(Args...)>
                                 && !delegate::is_small_buffer_optimizable<Functor>,
                int> = 0>
        static auto create(T&& functor) -> delegate_type {
            return {core_type::template from_dynamic_allocated_functor(std::forward<T>(functor))};
        }
    };
}  // namespace detail


// Can store and invoke any callable target. See the documentation in `doc/delegate.md`.
template<typename Signature, typename Behavior = target_is_expected>
class delegate {
    static_assert(detail::delegate::invalid<Signature>,
        "Invalid parameter 'Signature'. The template parameter "
        "'Signature' must be a valid function signature.");
};

template<typename Ret, typename... Args, typename Behavior>
class delegate<Ret(Args...), Behavior>
    : private detail::base_delegate<delegate<Ret(Args...), Behavior>> {
    static_assert(detail::delegate::is_behavior<Behavior>,
        "Invalid parameter 'Behavior'. The template parameter 'Behavior' must either be empty or "
        "contain one of the types 'rome::target_is_optional', 'rome::target_is_expected' or "
        "'rome::target_is_mandatory'.");
    static_assert(detail::delegate::is_valid_behavior<Ret, Behavior>,
        "Return type coflicts with parameter 'Behavior'. The parameter 'Behavior' is only "
        "allowed to be 'rome::target_is_optional' if the return type is 'void'.");

    using base_type = detail::base_delegate<delegate<Ret(Args...), Behavior>>;
    friend base_type;  // give base_type access to private constructor `delegate(base_type&&)`

    delegate(base_type&& base) noexcept : base_type{std::move(base)} {
    }

  public:
    constexpr delegate() noexcept      = default;
    delegate(const delegate&) noexcept = delete;
    delegate(delegate&&) noexcept      = default;
    ~delegate()                        = default;

    auto operator=(const delegate&) noexcept -> delegate& = delete;
    auto operator=(delegate&&) noexcept -> delegate&      = default;

    constexpr delegate(std::nullptr_t) noexcept : delegate{} {
    }
    constexpr auto operator=(std::nullptr_t) noexcept -> delegate& {
        base_type::drop_target();
        return *this;
    }

    using base_type::swap;
    using base_type::operator bool;
    using base_type::operator();
    using base_type::create;
};

template<typename Ret, typename... Args>
class delegate<Ret(Args...), target_is_mandatory>
    : private detail::base_delegate<delegate<Ret(Args...), target_is_mandatory>> {
    using base_type = detail::base_delegate<delegate<Ret(Args...), target_is_mandatory>>;
    friend base_type;  // give base_type access to private constructor `delegate(base_type&&)`

    delegate(base_type&& base) noexcept : base_type{std::move(base)} {
    }

  public:
    constexpr delegate() noexcept      = delete;
    delegate(const delegate&) noexcept = delete;
    delegate(delegate&&) noexcept      = default;
    ~delegate()                        = default;

    auto operator=(const delegate&) noexcept -> delegate& = delete;
    auto operator=(delegate&&) noexcept -> delegate&      = default;

    using base_type::swap;
    using base_type::operator bool;
    using base_type::operator();
    using base_type::create;
};

template<typename Sig, typename Behavior>
constexpr auto operator==(const delegate<Sig, Behavior>& lhs, std::nullptr_t) -> bool {
    return !lhs;
}
template<typename Sig, typename Behavior>
constexpr auto operator==(std::nullptr_t, const delegate<Sig, Behavior>& rhs) -> bool {
    return !rhs;
}
template<typename Sig, typename Behavior>
constexpr auto operator!=(const delegate<Sig, Behavior>& lhs, std::nullptr_t) -> bool {
    return static_cast<bool>(lhs);
}
template<typename Sig, typename Behavior>
constexpr auto operator!=(std::nullptr_t, const delegate<Sig, Behavior>& rhs) -> bool {
    return static_cast<bool>(rhs);
}


// Can store and invoke targets as `rome::delegate` does, but with the restriction that data can
// only be forwarded. Thus the return type is restricted to `void` and the arguments are enforced to
// be of an immutable type. See the documentation in `doc/fwd_delegate.md`.
template<typename Signature, typename Behavior = target_is_expected>
class fwd_delegate {
    static_assert(detail::delegate::invalid<Signature>,
        "Invalid parameter 'Signature'. The template parameter 'Signature' must be a valid "
        "function signature with return type 'void'. Consider using 'rome::delegate' if a non-void "
        "return type is needed.");
};

template<typename... Args, typename Behavior>
class fwd_delegate<void(Args...), Behavior>
    : private detail::base_delegate<fwd_delegate<void(Args...), Behavior>> {
    static_assert(detail::delegate::is_behavior<Behavior>,
        "Invalid parameter 'Behavior'. The template parameter 'Behavior' must either be empty or "
        "contain one of the types 'rome::target_is_optional', 'rome::target_is_expected' or "
        "'rome::target_is_mandatory'.");
    static_assert(detail::delegate::are_immutable_arguments<Args...>,
        "Invalid mutable function argument in 'void(Args...)'. All function arguments of a "
        "'rome::fwd_delegate' must be immutable. The argument types shall prevent that the callee "
        "is able to modify passed data still owned by the caller. E.g. 'int&' is not allowed. "
        "'const int&' is allowed (readonly). 'int' and 'int&&' are also allowed (data owned by "
        "callee). Consider using 'rome::delegate' if mutable arguments are needed.");

    using base_type = detail::base_delegate<fwd_delegate<void(Args...), Behavior>>;
    friend base_type;  // give base_type access to private constructor `fwd_delegate(base_type&&)`

    fwd_delegate(base_type&& base) noexcept : base_type{std::move(base)} {
    }

  public:
    constexpr fwd_delegate() noexcept          = default;
    fwd_delegate(const fwd_delegate&) noexcept = delete;
    fwd_delegate(fwd_delegate&&) noexcept      = default;
    ~fwd_delegate()                            = default;

    auto operator=(const fwd_delegate&) noexcept -> fwd_delegate& = delete;
    auto operator=(fwd_delegate&&) noexcept -> fwd_delegate&      = default;

    constexpr fwd_delegate(std::nullptr_t) noexcept : fwd_delegate{} {
    }
    constexpr auto operator=(std::nullptr_t) noexcept -> fwd_delegate& {
        base_type::drop_target();
        return *this;
    }

    using base_type::swap;
    using base_type::operator bool;
    using base_type::operator();
    using base_type::create;
};

template<typename... Args>
class fwd_delegate<void(Args...), target_is_mandatory>
    : private detail::base_delegate<fwd_delegate<void(Args...), target_is_mandatory>> {
    static_assert(detail::delegate::are_immutable_arguments<Args...>,
        "Invalid mutable function argument in 'void(Args...)'. All function arguments of a "
        "'rome::fwd_delegate' must be immutable. The argument types shall prevent that the callee "
        "is able to modify passed data still owned by the caller. E.g. 'int&' is not allowed. "
        "'const int&' is allowed (readonly). 'int' and 'int&&' are also allowed (data owned by "
        "callee). Consider using 'rome::delegate' if mutable arguments are needed.");

    using base_type = detail::base_delegate<fwd_delegate<void(Args...), target_is_mandatory>>;
    friend base_type;  // give base_type access to private constructor `fwd_delegate(base_type&&)`

    fwd_delegate(base_type&& base) noexcept : base_type{std::move(base)} {
    }

  public:
    constexpr fwd_delegate() noexcept          = delete;
    fwd_delegate(const fwd_delegate&) noexcept = delete;
    fwd_delegate(fwd_delegate&&) noexcept      = default;
    ~fwd_delegate()                            = default;

    auto operator=(const fwd_delegate&) noexcept -> fwd_delegate& = delete;
    auto operator=(fwd_delegate&&) noexcept -> fwd_delegate&      = default;

    using base_type::swap;
    using base_type::operator bool;
    using base_type::operator();
    using base_type::create;
};

template<typename Sig, typename Behavior>
constexpr auto operator==(const fwd_delegate<Sig, Behavior>& lhs, std::nullptr_t) -> bool {
    return !lhs;
}
template<typename Sig, typename Behavior>
constexpr auto operator==(std::nullptr_t, const fwd_delegate<Sig, Behavior>& rhs) -> bool {
    return !rhs;
}
template<typename Sig, typename Behavior>
constexpr auto operator!=(const fwd_delegate<Sig, Behavior>& lhs, std::nullptr_t) -> bool {
    return static_cast<bool>(lhs);
}
template<typename Sig, typename Behavior>
constexpr auto operator!=(std::nullptr_t, const fwd_delegate<Sig, Behavior>& rhs) -> bool {
    return static_cast<bool>(rhs);
}


// A `rome::fwd_delegate` with the `Behavior` set to `rome::target_is_mandatory`. Can be used where
// some part in a system requires that a command is handled by another part in the system. See the
// documentation in `doc/fwd_delegate.md`.
template<typename Signature>
using command_delegate = fwd_delegate<Signature, target_is_mandatory>;


// A `rome::fwd_delegate` with the `Behavior` set to `rome::target_is_optional`. Can be used where
// some part in a system provides an event that another part in the system might want to listen to.
// See the documentation in `doc/fwd_delegate.md`.
template<typename Signature>
using event_delegate = fwd_delegate<Signature, target_is_optional>;

}  // namespace rome

#endif  // ROME_DELEGATE_HPP
