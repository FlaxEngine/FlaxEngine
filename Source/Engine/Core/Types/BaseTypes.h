// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Compiler.h"
#include "Engine/Platform/Defines.h"

typedef unsigned char byte; // Unsigned 8 bits

// Unsigned base types
typedef unsigned char uint8; // 8-bit  unsigned
typedef unsigned short uint16; // 16-bit unsigned
typedef unsigned int uint32; // 32-bit unsigned
typedef unsigned long long uint64; // 64-bit unsigned

// Signed base types
typedef signed char int8; // 8-bit  signed
typedef signed short int16; // 16-bit signed
typedef signed int int32; // 32-bit signed
typedef long long int64; // 64-bit signed

// Pointer as integer and pointer size
#ifdef PLATFORM_64BITS
typedef uint64 uintptr;
typedef int64 intptr;
#define POINTER_SIZE 8
#else
typedef uint32 uintptr;
typedef int32 intptr;
#define POINTER_SIZE 4
#endif

/// <summary>
/// Defines 16-bit Unicode character for UTF-16 Little Endian.
/// </summary>
#if PLATFORM_TEXT_IS_CHAR16
typedef char16_t Char;
#else
typedef wchar_t Char;
#endif

// Limits
#define MIN_uint8 ((uint8)0x00)
#define	MIN_uint16 ((uint16)0x0000)
#define	MIN_uint32 ((uint32)0x00000000)
#define MIN_uint64 ((uint64)0x0000000000000000)
#define MIN_int8 ((int8)-128)
#define MIN_int16 ((int16)-32768)
#define MIN_int32 ((int32)-2147483648ll)
#define MIN_int64 -((int64)9223372036854775808)
#define MIN_float -(3.402823466e+38f)
#define MIN_double -(1.7976931348623158e+308)
#define MAX_uint8 ((uint8)0xff)
#define MAX_uint16 ((uint16)0xffff)
#define MAX_uint32 ((uint32)0xffffffff)
#define MAX_uint64 ((uint64)0xffffffffffffffff)
#define MAX_int8 ((int8)127)
#define MAX_int16 ((int16)32767)
#define MAX_int32 ((int32)2147483647)
#define MAX_int64 ((int64)9223372036854775807)
#define MAX_float (3.402823466e+38f)
#define MAX_double (1.7976931348623158e+308)

// Forward declarations
class String;
class StringView;
class StringAnsi;
class StringAnsiView;
struct Guid;
struct DateTime;
struct TimeSpan;
struct Variant;
template<typename T>
class Span;
class HeapAllocation;
template<int Capacity>
class FixedAllocation;
template<int Capacity, typename OtherAllocator>
class InlinedAllocation;
template<typename T, typename AllocationType>
class Array;
template<typename T, typename U>
class Pair;
template<typename KeyType, typename ValueType, typename AllocationType>
class Dictionary;
template<typename>
class Function;
template<typename... Params>
class Delegate;

// @formatter:off

#if USE_LARGE_WORLDS
// 64-bit precision for world coordinates
API_TYPEDEF(Alias) typedef double Real;
#define MIN_Real MIN_double
#define MAX_Real MAX_double
#else
// 32-bit precision for world coordinates
API_TYPEDEF(Alias) typedef float Real;
#define MIN_Real MIN_float
#define MAX_Real MAX_float
#endif

// Vector2
template<typename T> struct Vector2Base;
API_TYPEDEF() typedef Vector2Base<float> Float2;
API_TYPEDEF() typedef Vector2Base<double> Double2;
API_TYPEDEF() typedef Vector2Base<int32> Int2;
API_TYPEDEF(Alias) typedef Vector2Base<Real> Vector2;

// Vector3
template<typename T> struct Vector3Base;
API_TYPEDEF() typedef Vector3Base<float> Float3;
API_TYPEDEF() typedef Vector3Base<double> Double3;
API_TYPEDEF() typedef Vector3Base<int32> Int3;
API_TYPEDEF(Alias) typedef Vector3Base<Real> Vector3;

// Vector4
template<typename T> struct Vector4Base;
API_TYPEDEF() typedef Vector4Base<float> Float4;
API_TYPEDEF() typedef Vector4Base<double> Double4;
API_TYPEDEF() typedef Vector4Base<int32> Int4;
API_TYPEDEF(Alias) typedef Vector4Base<Real> Vector4;

struct BoundingBox;
struct Matrix;
struct Matrix3x3;
struct Double4x4;
struct Ray;
struct Plane;
struct Rectangle;
struct Quaternion;
struct BoundingSphere;
struct BoundingFrustum;
struct OrientedBoundingBox;
struct Transform;
struct Color;
struct Color32;
#if USE_LARGE_WORLDS
typedef Double4x4 Real4x4;
#else
typedef Matrix Real4x4;
#endif

// @formatter:on

// Declares full set of operators for the enum type (using binary operation on integer values)
#define DECLARE_ENUM_OPERATORS(T) \
    inline constexpr bool operator!(T a) { return !(__underlying_type(T))a; } \
    inline constexpr T operator~(T a) { return (T)~(__underlying_type(T))a; } \
    inline constexpr T operator|(T a, T b) { return (T)((__underlying_type(T))a | (__underlying_type(T))b); } \
    inline constexpr T operator&(T a, T b) { return (T)((__underlying_type(T))a & (__underlying_type(T))b); } \
    inline constexpr T operator^(T a, T b) { return (T)((__underlying_type(T))a ^ (__underlying_type(T))b); } \
    inline T& operator|=(T& a, T b) { return a = (T)((__underlying_type(T))a | (__underlying_type(T))b); } \
    inline T& operator&=(T& a, T b) { return a = (T)((__underlying_type(T))a & (__underlying_type(T))b); } \
    inline T& operator^=(T& a, T b) { return a = (T)((__underlying_type(T))a ^ (__underlying_type(T))b); }

// Returns true if given enum value has one or more enum flags set
template<typename T>
constexpr bool EnumHasAnyFlags(T value, T flags)
{
	return ((__underlying_type(T))value & (__underlying_type(T))flags) != 0;
}

// Returns true if given enum value has all of the enum flags set
template<typename T>
constexpr bool EnumHasAllFlags(T value, T flags)
{
	return ((__underlying_type(T))value & (__underlying_type(T))flags) == (__underlying_type(T))flags;
}

// Returns true if given enum value has none of enum flags set
template<typename T>
constexpr bool EnumHasNoneFlags(T value, T flags)
{
	return ((__underlying_type(T))value & (__underlying_type(T))flags) == 0;
}

// Returns enum value with additional enum flags set
template<typename T>
constexpr T EnumAddFlags(T value, T flags)
{
	return (T)((__underlying_type(T))value | (__underlying_type(T))flags);
}

// Returns byte offset from the object pointer in vtable to the begin of the given inherited type implementation
#define VTABLE_OFFSET(type, baseType) (((intptr)static_cast<baseType*>((type*)1))-1)
