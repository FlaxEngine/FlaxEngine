// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if defined(__clang__)

#define DLLEXPORT __attribute__ ((__visibility__ ("default")))
#define DLLIMPORT
#define THREADLOCAL __thread
#define STDCALL __attribute__((stdcall))
#define CDECL __attribute__((cdecl))
#define RESTRICT __restrict__
#define INLINE inline
#define FORCE_INLINE inline
#define FORCE_NOINLINE __attribute__((noinline))
#define NO_RETURN __attribute__((noreturn))
#define NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#define NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#define PACK_BEGIN()
#define PACK_END() __attribute__((__packed__))
#define ALIGN_BEGIN(_align)
#define ALIGN_END(_align) __attribute__( (aligned(_align) ) )
#define OFFSET_OF(X, Y) __builtin_offsetof(X, Y)
#define PRAGMA_DISABLE_DEPRECATION_WARNINGS \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#define PRAGMA_ENABLE_DEPRECATION_WARNINGS \
    _Pragma("clang diagnostic pop")
#define PRAGMA_DISABLE_OPTIMIZATION
#define PRAGMA_ENABLE_OPTIMIZATION

#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wmacro-redefined"
#pragma clang diagnostic ignored "-Waddress-of-packed-member"
#pragma clang diagnostic ignored "-Wnull-dereference"
#pragma clang diagnostic ignored "-Winvalid-noreturn"

#elif defined(__GNUC__)

#define DLLEXPORT __attribute__ ((__visibility__ ("default")))
#define DLLIMPORT
#define THREADLOCAL __thread
#define STDCALL __attribute__((stdcall))
#define CDECL __attribute__((cdecl))
#define RESTRICT __restrict__
#define INLINE inline
#define FORCE_INLINE inline
#define FORCE_NOINLINE __attribute__((noinline))
#define NO_RETURN __attribute__((noreturn))
#define NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#define NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#define PACK_BEGIN()
#define PACK_END() __attribute__((__packed__))
#define ALIGN_BEGIN(_align)
#define ALIGN_END(_align) __attribute__( (aligned(_align) ) )
#define OFFSET_OF(X, Y) __builtin_offsetof(X, Y)
#define PRAGMA_DISABLE_DEPRECATION_WARNINGS
#define PRAGMA_ENABLE_DEPRECATION_WARNINGS
#define PRAGMA_DISABLE_OPTIMIZATION
#define PRAGMA_ENABLE_OPTIMIZATION

#elif defined(_MSC_VER)

#if _MSC_VER < 1900
#error "Required Visual Studio 2015 or newer."
#endif

#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#define THREADLOCAL __declspec(thread)
#define STDCALL __stdcall
#define CDECL __cdecl
#define RESTRICT __restrict
#define INLINE __inline
#define FORCE_INLINE __forceinline
#define FORCE_NOINLINE __declspec(noinline)
#define NO_RETURN __declspec(noreturn)
#define NO_SANITIZE_ADDRESS
#define NO_SANITIZE_THREAD
#define PACK_BEGIN() __pragma(pack(push, 1))
#define PACK_END() ; __pragma(pack(pop))
#define ALIGN_BEGIN(_align) __declspec(align(_align))
#define ALIGN_END(_align)
#define OFFSET_OF(X, Y) offsetof(X, Y)
#undef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__
#define PRAGMA_DISABLE_DEPRECATION_WARNINGS \
			__pragma(warning(push)) \
			__pragma(warning(disable: 4995)) \
			__pragma(warning(disable: 4996))
#define PRAGMA_ENABLE_DEPRECATION_WARNINGS \
			__pragma (warning(pop))
#define PRAGMA_DISABLE_OPTIMIZATION __pragma(optimize("", off))
#define PRAGMA_ENABLE_OPTIMIZATION __pragma(optimize("", on))

#pragma warning(disable: 4251)

#else

#pragma error "Unknown compiler."

#endif

#define PACK_STRUCT(_declaration) PACK_BEGIN() _declaration PACK_END()

#define _DEPRECATED_0() [[deprecated]]
#define _DEPRECATED_1(msg) [[deprecated(msg)]]
#define _DEPRECATED(_0, _1, LASTARG, ...) LASTARG
#define DEPRECATED(...) _DEPRECATED(, ##__VA_ARGS__, _DEPRECATED_1(__VA_ARGS__), _DEPRECATED_0())

// C++ 17
#if __cplusplus >= 201703L
#define IF_CONSTEXPR constexpr
#else
#define IF_CONSTEXPR
#endif
