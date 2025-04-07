// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

#ifndef DOXYGEN

// @formatter:off

////////////////////////////////////////////////////////////////////////////////////

namespace THelpers
{
	template <typename T, bool IsTriviallyDestructible = __is_enum(T)>
	struct TIsTriviallyDestructibleImpl
	{
		enum { Value = true };
	};

	template <typename T>
	struct TIsTriviallyDestructibleImpl<T, false>
	{
#if defined(__clang__) && __clang_major__ >= 15
        enum { Value = __is_trivially_destructible(T) };
#else
        enum { Value = __has_trivial_destructor(T) };
#endif
	};
}

////////////////////////////////////////////////////////////////////////////////////

// Performs boolean AND operation.

template<typename... Types>
struct TAnd;

template<bool Left, typename... Right>
struct TAndValue
{
	enum { Value = TAnd<Right...>::Value };
};

template<typename... Right>
struct TAndValue<false, Right...>
{
	enum { Value = false };
};

template<typename Left, typename... Right>
struct TAnd<Left, Right...> : TAndValue<Left::Value, Right...>
{
};

template<>
struct TAnd<>
{
	enum { Value = true };
};

////////////////////////////////////////////////////////////////////////////////////

// Performs boolean OR operation.

template<typename... Types>
struct TOr;

template<bool Left, typename... Right>
struct TOrValue
{
	enum { Value = TOr<Right...>::Value };
};

template<typename... Right>
struct TOrValue<true, Right...>
{
	enum { Value = true };
};

template<typename Left, typename... Right>
struct TOr<Left, Right...> : TOrValue<Left::Value, Right...>
{
};

template<>
struct TOr<>
{
	enum { Value = false };
};

////////////////////////////////////////////////////////////////////////////////////

// Performs boolean NOT operation.

template<typename Type>
struct TNot
{
    enum { Value = !Type::Value };
};

////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename B> struct TIsTheSame        { enum { Value = false }; };
template<typename T>             struct TIsTheSame<T, T>  { enum { Value = true  }; };

////////////////////////////////////////////////////////////////////////////////////

template<typename T> struct TIsLValueReference     { enum { Value = false }; };
template<typename T> struct TIsLValueReference<T&> { enum { Value = true }; };

////////////////////////////////////////////////////////////////////////////////////

template<typename T> struct TIsRValueReferenceType      { enum { Value = false }; };
template<typename T> struct TIsRValueReferenceType<T&&> { enum { Value = true }; };

////////////////////////////////////////////////////////////////////////////////////

template<typename T> struct TIsReferenceType      { enum { Value = false }; };
template<typename T> struct TIsReferenceType<T&>  { enum { Value = true }; };
template<typename T> struct TIsReferenceType<T&&> { enum { Value = true }; };

////////////////////////////////////////////////////////////////////////////////////

template<typename T> struct TIsVoidType            { enum { Value = false }; };
template<> struct TIsVoidType<void>                { enum { Value = true }; };
template<> struct TIsVoidType<void const>          { enum { Value = true }; };
template<> struct TIsVoidType<void volatile>       { enum { Value = true }; };
template<> struct TIsVoidType<void const volatile> { enum { Value = true }; };

////////////////////////////////////////////////////////////////////////////////////

// Checks if a type is a pointer.

template<typename T> struct TIsPointer     { enum { Value = false }; };
template<typename T> struct TIsPointer<T*> { enum { Value = true }; };

////////////////////////////////////////////////////////////////////////////////////

// Checks if a type is an enum.

template<typename T>
struct TIsEnum
{
	enum { Value = __is_enum(T) };
};

////////////////////////////////////////////////////////////////////////////////////

// Checks if a type is POD (plain old data type).

template<typename T>
struct TIsPODType 
{ 
	enum { Value = TOrValue<__is_pod(T) || __is_enum(T), TIsPointer<T>>::Value };
};

////////////////////////////////////////////////////////////////////////////////////

template<class Base, class Derived>
struct TIsBaseOf
{
	enum { Value = __is_base_of(Base, Derived) };
};

////////////////////////////////////////////////////////////////////////////////////

// Removes any const or volatile qualifiers from a type.

template<typename T> struct TRemoveCV                   { typedef T Type; };
template<typename T> struct TRemoveCV<const T>          { typedef T Type; };
template<typename T> struct TRemoveCV<volatile T>       { typedef T Type; };
template<typename T> struct TRemoveCV<const volatile T> { typedef T Type; };

////////////////////////////////////////////////////////////////////////////////////

// Removes any reference qualifiers from a type.

template<typename T> struct TRemoveReference      { typedef T Type; };
template<typename T> struct TRemoveReference<T&>  { typedef T Type; };
template<typename T> struct TRemoveReference<T&&> { typedef T Type; };

////////////////////////////////////////////////////////////////////////////////////

// Removes any const qualifiers from a type.

template<typename T> struct TRemoveConst          { typedef T Type; };
template<typename T> struct TRemoveConst<const T> { typedef T Type; };

////////////////////////////////////////////////////////////////////////////////////

// Adds qualifiers to a type.

template<typename T> struct TAddCV    { typedef const volatile T Type; };
template<typename T> struct TAddConst { typedef const T Type; };

////////////////////////////////////////////////////////////////////////////////////

// Creates a lvalue or rvalue reference type.

namespace THelpers
{
    template<typename T>
    struct TTtypeIdentity { using Type = T; };

    template<typename T>
    auto TTryAddLValueReference(int) -> TTtypeIdentity<T&>;
    template <typename T>
    auto TTryAddLValueReference(...) -> TTtypeIdentity<T>;

    template<typename T>
    auto TTryAddRValueReference(int) -> TTtypeIdentity<T&&>;
    template<typename T>
    auto TTryAddRValueReference(...) -> TTtypeIdentity<T>;
}
 
template<typename T>
struct TAddLValueReference : decltype(THelpers::TTryAddLValueReference<T>(0))
{
};
 
template<typename T>
struct TAddRValueReference : decltype(THelpers::TTryAddRValueReference<T>(0))
{
};

////////////////////////////////////////////////////////////////////////////////////

// Checks if a type has a copy constructor.

template<typename T>
struct TIsCopyConstructible
{
	enum { Value = __is_constructible(T, typename TAddLValueReference<typename TAddConst<T>::Type>::Type) };
};

// Checks if a type has a move constructor.

template<typename T>
struct TIsMoveConstructible
{
    enum { Value = __is_constructible(T, typename TAddRValueReference<T>::Type) };
};

////////////////////////////////////////////////////////////////////////////////////

// Checks if a type has a trivial copy constructor.

template<typename T>
struct TIsTriviallyCopyConstructible
{
#if defined(__clang__) && __clang_major__ >= 15
    enum { Value = TOrValue<__is_trivially_copyable(T), TIsPODType<T>>::Value };
#else
    enum { Value = TOrValue<__has_trivial_copy(T), TIsPODType<T>>::Value };
#endif
};

////////////////////////////////////////////////////////////////////////////////////

template<typename T> 
struct TIsTriviallyConstructible
{ 
	enum { Value = TIsPODType<T>::Value };
};

////////////////////////////////////////////////////////////////////////////////////

// Check if a type has a trivial destructor.

template <typename T>
struct TIsTriviallyDestructible
{
	enum { Value = THelpers::TIsTriviallyDestructibleImpl<T>::Value };
};

////////////////////////////////////////////////////////////////////////////////////

// Checks if a type has a trivial copy assignment operator.

template<typename T>
struct TIsTriviallyCopyAssignable
{
#if defined(__clang__) && __clang_major__ >= 15
    enum { Value = TOrValue<__is_trivially_assignable(T, const T), TIsPODType<T>>::Value };
#else
    enum { Value = TOrValue<__has_trivial_assign(T), TIsPODType<T>>::Value };
#endif
};

////////////////////////////////////////////////////////////////////////////////////

template<typename T>                           struct TIsFunction                     { enum { Value = false }; };
template<typename RetType, typename... Params> struct TIsFunction<RetType(Params...)> { enum { Value = true }; };

////////////////////////////////////////////////////////////////////////////////////

template<typename X, typename Y> struct TAreTypesEqual { enum { Value = false }; };
template<typename T> struct TAreTypesEqual<T, T>       { enum { Value = true }; };

////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline typename TRemoveReference<T>::Type&& MoveTemp(T&& obj)
{
    return (typename TRemoveReference<T>::Type&&)obj;
}

////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline void Swap(T& a, T& b) noexcept
{
    T tmp = MoveTemp(a);
    a = MoveTemp(b);
    b = MoveTemp(tmp);
}

////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline T&& Forward(typename TRemoveReference<T>::Type& t) noexcept
{
    return static_cast<T&&>(t);
}

template<typename T>
inline T&& Forward(typename TRemoveReference<T>::Type&& t) noexcept
{
    return static_cast<T&&>(t);
}

////////////////////////////////////////////////////////////////////////////////////

template<bool Condition, typename TrueResult, typename FalseResult>
struct TStaticIf;

template<typename TrueResult, typename FalseResult>
struct TStaticIf<true, TrueResult, FalseResult>
{
    typedef TrueResult Value;
};

template<typename TrueResult, typename FalseResult>
struct TStaticIf<false, TrueResult, FalseResult>
{
    typedef FalseResult Value;
};

////////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TRemovePointer
{
    typedef T Type;
};

template<typename T>
struct TRemovePointer<T*>
{
    typedef typename TRemovePointer<T>::Type Type;
};

////////////////////////////////////////////////////////////////////////////////////

// Includes a function in an overload set if the predicate is true.

template<bool Predicate, typename Result = void>
struct TEnableIf;

template<typename Result>
struct TEnableIf<true, Result>
{
    typedef Result Type;
};

template<typename Result>
struct TEnableIf<false, Result>
{
};

////////////////////////////////////////////////////////////////////////////////////

// Reverses the order of the bits of a value.
template<typename T>
inline typename TEnableIf<TAreTypesEqual<T, unsigned int>::Value, T>::Type ReverseBits(T bits)
{
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
    bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
    return bits;
}

////////////////////////////////////////////////////////////////////////////////////

// Checks if a type T is bitwise-constructible from a given argument type U. Can be used to perform a fast memory copy instead of slower constructor invocations.

template<typename T, typename Arg>
struct TIsBitwiseConstructible
{
    static_assert(!TIsReferenceType<T>::Value && !TIsReferenceType<Arg>::Value,"TIsBitwiseConstructible cannot use reference types");
    static_assert(TAreTypesEqual<T, typename TRemoveCV<T>::Type>::Value && TAreTypesEqual<Arg, typename TRemoveCV<Arg>::Type>::Value, "TIsBitwiseConstructible cannot use qualified types");
    enum { Value = false };
};

template<typename T>
struct TIsBitwiseConstructible<T, T>
{
    enum { Value = TIsTriviallyCopyConstructible<T>::Value };
};

template<typename T, typename U>
struct TIsBitwiseConstructible<const T, U> : TIsBitwiseConstructible<T, U>
{
};

template<typename T>
struct TIsBitwiseConstructible<const T*, T*>
{
    enum { Value = true };
};

template<> struct TIsBitwiseConstructible<uint8, int8>   { enum { Value = true }; };
template<> struct TIsBitwiseConstructible<int8, uint8>   { enum { Value = true }; };
template<> struct TIsBitwiseConstructible<uint16, int16> { enum { Value = true }; };
template<> struct TIsBitwiseConstructible<int16, uint16> { enum { Value = true }; };
template<> struct TIsBitwiseConstructible<uint32, int32> { enum { Value = true }; };
template<> struct TIsBitwiseConstructible<int32, uint32> { enum { Value = true }; };
template<> struct TIsBitwiseConstructible<uint64, int64> { enum { Value = true }; };
template<> struct TIsBitwiseConstructible<int64, uint64> { enum { Value = true }; };

// @formatter:on

////////////////////////////////////////////////////////////////////////////////////

// Utility to select double for float type or float otherwise

template<typename T>
struct TOtherFloat { typedef float Type; };
template<>
struct TOtherFloat<float> { typedef double Type; };

#endif
