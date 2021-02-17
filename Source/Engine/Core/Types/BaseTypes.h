// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
#define MIN_int32 -((int32)2147483648)
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
struct Vector2;
struct Vector3;
struct Vector4;
struct Int2;
struct Int3;
struct Int4;
struct Quaternion;
struct Matrix;
struct Ray;
struct Rectangle;
struct Plane;
struct BoundingBox;
struct BoundingSphere;
struct BoundingFrustum;
struct Color;
struct Color32;
struct Variant;
class HeapAllocation;
template<int Capacity>
class FixedAllocation;
template<int Capacity, typename OtherAllocator>
class InlinedAllocation;
template<typename T, typename AllocationType>
class Array;
template<typename T, typename U>
class Pair;
template<typename KeyType, typename ValueType>
class Dictionary;

// Declares full set of operators for the enum type (using binary operation on integer values)
#define DECLARE_ENUM_OPERATORS(T) \
    inline T operator~ (T a) { return (T)~(int)a; } \
    inline T operator| (T a, T b) { return (T)((int)a | (int)b); } \
    inline int operator& (T a, T b) { return ((int)a & (int)b); } \
    inline T operator^ (T a, T b) { return (T)((int)a ^ (int)b); } \
    inline T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); } \
    inline T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); } \
    inline T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }

// Returns byte offset from the object pointer in vtable to the begin of the given inherited type implementation
#define VTABLE_OFFSET(type, baseType) (((intptr)static_cast<baseType*>((type*)1))-1)
