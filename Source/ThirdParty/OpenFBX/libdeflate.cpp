// ofbx changes : removed unused code, single .h and .c
/*
 * Copyright 2016 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * ---------------------------------------------------------------------------
 *
 * This is a highly optimized DEFLATE decompressor.  It is much faster than
 * vanilla zlib, typically well over twice as fast, though results vary by CPU.
 *
 * Why this is faster than vanilla zlib:
 *
 * - Word accesses rather than byte accesses when reading input
 * - Word accesses rather than byte accesses when copying matches
 * - Faster Huffman decoding combined with various DEFLATE-specific tricks
 * - Larger bitbuffer variable that doesn't need to be refilled as often
 * - Other optimizations to remove unnecessary branches
 * - Only full-buffer decompression is supported, so the code doesn't need to
 *   support stopping and resuming decompression.
 * - On x86_64, a version of the decompression routine is compiled with BMI2
 *   instructions enabled and is used automatically at runtime when supported.
 */

/*
 * lib_common.h - internal header included by all library code
 */

#ifndef LIB_LIB_COMMON_H
#define LIB_LIB_COMMON_H

#ifdef LIBDEFLATE_H
 /*
  * When building the library, LIBDEFLATEAPI needs to be defined properly before
  * including libdeflate.h.
  */
#  error "lib_common.h must always be included before libdeflate.h"
#endif

#if defined(LIBDEFLATE_DLL) && (defined(_WIN32) || defined(__CYGWIN__))
#  define LIBDEFLATE_EXPORT_SYM  __declspec(dllexport)
#elif defined(__GNUC__)
#  define LIBDEFLATE_EXPORT_SYM  __attribute__((visibility("default")))
#else
#  define LIBDEFLATE_EXPORT_SYM
#endif

/*
 * On i386, gcc assumes that the stack is 16-byte aligned at function entry.
 * However, some compilers (e.g. MSVC) and programming languages (e.g. Delphi)
 * only guarantee 4-byte alignment when calling functions.  This is mainly an
 * issue on Windows, but it has been seen on Linux too.  Work around this ABI
 * incompatibility by realigning the stack pointer when entering libdeflate.
 * This prevents crashes in SSE/AVX code.
 */
#if defined(__GNUC__) && defined(__i386__)
#  define LIBDEFLATE_ALIGN_STACK  __attribute__((force_align_arg_pointer))
#else
#  define LIBDEFLATE_ALIGN_STACK
#endif

#define LIBDEFLATEAPI	LIBDEFLATE_EXPORT_SYM LIBDEFLATE_ALIGN_STACK

/*
 * common_defs.h
 *
 * Copyright 2016 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include "libdeflate.h"

#include <stdbool.h>
#include <stddef.h>	/* for size_t */
#include <stdint.h>
#ifdef _MSC_VER
#  include <intrin.h>	/* for _BitScan*() and other intrinsics */
#  include <stdlib.h>	/* for _byteswap_*() */
   /* Disable MSVC warnings that are expected. */
   /* /W2 */
#  pragma warning(disable : 4146) /* unary minus on unsigned type */
   /* /W3 */
#  pragma warning(disable : 4018) /* signed/unsigned mismatch */
#  pragma warning(disable : 4244) /* possible loss of data */
#  pragma warning(disable : 4267) /* possible loss of precision */
#  pragma warning(disable : 4310) /* cast truncates constant value */
   /* /W4 */
#  pragma warning(disable : 4100) /* unreferenced formal parameter */
#  pragma warning(disable : 4127) /* conditional expression is constant */
#  pragma warning(disable : 4189) /* local variable initialized but not referenced */
#  pragma warning(disable : 4232) /* nonstandard extension used */
#  pragma warning(disable : 4245) /* conversion from 'int' to 'unsigned int' */
#  pragma warning(disable : 4295) /* array too small to include terminating null */
#endif
#ifndef FREESTANDING
#  include <string.h>	/* for memcpy() */
#endif

/* ========================================================================== */
/*                             Target architecture                            */
/* ========================================================================== */

/* If possible, define a compiler-independent ARCH_* macro. */
#undef ARCH_X86_64
#undef ARCH_X86_32
#undef ARCH_ARM64
#undef ARCH_ARM32
#ifdef _MSC_VER
#  if defined(_M_X64)
#    define ARCH_X86_64
#  elif defined(_M_IX86)
#    define ARCH_X86_32
#  elif defined(_M_ARM64)
#    define ARCH_ARM64
#  elif defined(_M_ARM)
#    define ARCH_ARM32
#  endif
#else
#  if defined(__x86_64__)
#    define ARCH_X86_64
#  elif defined(__i386__)
#    define ARCH_X86_32
#  elif defined(__aarch64__)
#    define ARCH_ARM64
#  elif defined(__arm__)
#    define ARCH_ARM32
#  endif
#endif

/* ========================================================================== */
/*                              Type definitions                              */
/* ========================================================================== */

/* Fixed-width integer types */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

/* ssize_t, if not available in <sys/types.h> */
#ifdef _MSC_VER
#  ifdef _WIN64
     typedef long long ssize_t;
#  else
     typedef long ssize_t;
#  endif
#endif

/*
 * Word type of the target architecture.  Use 'size_t' instead of
 * 'unsigned long' to account for platforms such as Windows that use 32-bit
 * 'unsigned long' on 64-bit architectures.
 */
typedef size_t machine_word_t;

/* Number of bytes in a word */
#define WORDBYTES	((int)sizeof(machine_word_t))

/* Number of bits in a word */
#define WORDBITS	(8 * WORDBYTES)

/* ========================================================================== */
/*                         Optional compiler features                         */
/* ========================================================================== */

/* Compiler version checks.  Only use when absolutely necessary. */
#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#  define GCC_PREREQ(major, minor)		\
	(__GNUC__ > (major) ||			\
	 (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#  define GCC_PREREQ(major, minor)	0
#endif
#ifdef __clang__
#  ifdef __apple_build_version__
#    define CLANG_PREREQ(major, minor, apple_version)	\
	(__apple_build_version__ >= (apple_version))
#  else
#    define CLANG_PREREQ(major, minor, apple_version)	\
	(__clang_major__ > (major) ||			\
	 (__clang_major__ == (major) && __clang_minor__ >= (minor)))
#  endif
#else
#  define CLANG_PREREQ(major, minor, apple_version)	0
#endif

/*
 * Macros to check for compiler support for attributes and builtins.  clang
 * implements these macros, but gcc doesn't, so generally any use of one of
 * these macros must also be combined with a gcc version check.
 */
#ifndef __has_attribute
#  define __has_attribute(attribute)	0
#endif
#ifndef __has_builtin
#  define __has_builtin(builtin)	0
#endif

/* inline - suggest that a function be inlined */
#ifdef _MSC_VER
#  define inline		__inline
#endif /* else assume 'inline' is usable as-is */

/* forceinline - force a function to be inlined, if possible */
#if defined(__GNUC__) || __has_attribute(always_inline)
#  define forceinline		inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#  define forceinline		__forceinline
#else
#  define forceinline		inline
#endif

/* MAYBE_UNUSED - mark a function or variable as maybe unused */
#if defined(__GNUC__) || __has_attribute(unused)
#  define MAYBE_UNUSED		__attribute__((unused))
#else
#  define MAYBE_UNUSED
#endif

/*
 * restrict - hint that writes only occur through the given pointer.
 *
 * Don't use MSVC's __restrict, since it has nonstandard behavior.
 * Standard restrict is okay, if it is supported.
 */
#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#  if defined(__GNUC__) || defined(__clang__)
#    define restrict		__restrict__
#  else
#    define restrict
#  endif
#endif /* else assume 'restrict' is usable as-is */

/* likely(expr) - hint that an expression is usually true */
#if defined(__GNUC__) || __has_builtin(__builtin_expect)
#  define likely(expr)		__builtin_expect(!!(expr), 1)
#else
#  define likely(expr)		(expr)
#endif

/* unlikely(expr) - hint that an expression is usually false */
#if defined(__GNUC__) || __has_builtin(__builtin_expect)
#  define unlikely(expr)	__builtin_expect(!!(expr), 0)
#else
#  define unlikely(expr)	(expr)
#endif

/* prefetchr(addr) - prefetch into L1 cache for read */
#undef prefetchr
#if defined(__GNUC__) || __has_builtin(__builtin_prefetch)
#  define prefetchr(addr)	__builtin_prefetch((addr), 0)
#elif defined(_MSC_VER)
#  if defined(ARCH_X86_32) || defined(ARCH_X86_64)
#    define prefetchr(addr)	_mm_prefetch((addr), _MM_HINT_T0)
#  elif defined(ARCH_ARM64)
#    define prefetchr(addr)	__prefetch2((addr), 0x00 /* prfop=PLDL1KEEP */)
#  elif defined(ARCH_ARM32)
#    define prefetchr(addr)	__prefetch(addr)
#  endif
#endif
#ifndef prefetchr
#  define prefetchr(addr)
#endif

/* prefetchw(addr) - prefetch into L1 cache for write */
#undef prefetchw
#if defined(__GNUC__) || __has_builtin(__builtin_prefetch)
#  define prefetchw(addr)	__builtin_prefetch((addr), 1)
#elif defined(_MSC_VER)
#  if defined(ARCH_X86_32) || defined(ARCH_X86_64)
#    define prefetchw(addr)	_m_prefetchw(addr)
#  elif defined(ARCH_ARM64)
#    define prefetchw(addr)	__prefetch2((addr), 0x10 /* prfop=PSTL1KEEP */)
#  elif defined(ARCH_ARM32)
#    define prefetchw(addr)	__prefetchw(addr)
#  endif
#endif
#ifndef prefetchw
#  define prefetchw(addr)
#endif

/*
 * _aligned_attribute(n) - declare that the annotated variable, or variables of
 * the annotated type, must be aligned on n-byte boundaries.
 */
#undef _aligned_attribute
#if defined(__GNUC__) || __has_attribute(aligned)
#  define _aligned_attribute(n)	__attribute__((aligned(n)))
#elif defined(_MSC_VER)
#  define _aligned_attribute(n)	__declspec(align(n))
#endif

/*
 * _target_attribute(attrs) - override the compilation target for a function.
 *
 * This accepts one or more comma-separated suffixes to the -m prefix jointly
 * forming the name of a machine-dependent option.  On gcc-like compilers, this
 * enables codegen for the given targets, including arbitrary compiler-generated
 * code as well as the corresponding intrinsics.  On other compilers this macro
 * expands to nothing, though MSVC allows intrinsics to be used anywhere anyway.
 */
#if GCC_PREREQ(4, 4) || __has_attribute(target)
#  define _target_attribute(attrs)	__attribute__((target(attrs)))
#  define COMPILER_SUPPORTS_TARGET_FUNCTION_ATTRIBUTE	1
#else
#  define _target_attribute(attrs)
#  define COMPILER_SUPPORTS_TARGET_FUNCTION_ATTRIBUTE	0
#endif

/* ========================================================================== */
/*                          Miscellaneous macros                              */
/* ========================================================================== */

#define ARRAY_LEN(A)		(sizeof(A) / sizeof((A)[0]))
#define MIN(a, b)		((a) <= (b) ? (a) : (b))
#define MAX(a, b)		((a) >= (b) ? (a) : (b))
#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))
#define STATIC_ASSERT(expr)	((void)sizeof(char[1 - 2 * !(expr)]))
#define ALIGN(n, a)		(((n) + (a) - 1) & ~((a) - 1))
#define ROUND_UP(n, d)		((d) * DIV_ROUND_UP((n), (d)))

/* ========================================================================== */
/*                           Endianness handling                              */
/* ========================================================================== */

/*
 * CPU_IS_LITTLE_ENDIAN() - 1 if the CPU is little endian, or 0 if it is big
 * endian.  When possible this is a compile-time macro that can be used in
 * preprocessor conditionals.  As a fallback, a generic method is used that
 * can't be used in preprocessor conditionals but should still be optimized out.
 */
#if defined(__BYTE_ORDER__) /* gcc v4.6+ and clang */
#  define CPU_IS_LITTLE_ENDIAN()  (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#elif defined(_MSC_VER)
#  define CPU_IS_LITTLE_ENDIAN()  true
#else
static forceinline bool CPU_IS_LITTLE_ENDIAN(void)
{
	union {
		u32 w;
		u8 b;
	} u;

	u.w = 1;
	return u.b;
}
#endif

/* bswap16(v) - swap the bytes of a 16-bit integer */
static forceinline u16 bswap16(u16 v)
{
#if GCC_PREREQ(4, 8) || __has_builtin(__builtin_bswap16)
	return __builtin_bswap16(v);
#elif defined(_MSC_VER)
	return _byteswap_ushort(v);
#else
	return (v << 8) | (v >> 8);
#endif
}

/* bswap32(v) - swap the bytes of a 32-bit integer */
static forceinline u32 bswap32(u32 v)
{
#if GCC_PREREQ(4, 3) || __has_builtin(__builtin_bswap32)
	return __builtin_bswap32(v);
#elif defined(_MSC_VER)
	return _byteswap_ulong(v);
#else
	return ((v & 0x000000FF) << 24) |
	       ((v & 0x0000FF00) << 8) |
	       ((v & 0x00FF0000) >> 8) |
	       ((v & 0xFF000000) >> 24);
#endif
}

/* bswap64(v) - swap the bytes of a 64-bit integer */
static forceinline u64 bswap64(u64 v)
{
#if GCC_PREREQ(4, 3) || __has_builtin(__builtin_bswap64)
	return __builtin_bswap64(v);
#elif defined(_MSC_VER)
	return _byteswap_uint64(v);
#else
	return ((v & 0x00000000000000FF) << 56) |
	       ((v & 0x000000000000FF00) << 40) |
	       ((v & 0x0000000000FF0000) << 24) |
	       ((v & 0x00000000FF000000) << 8) |
	       ((v & 0x000000FF00000000) >> 8) |
	       ((v & 0x0000FF0000000000) >> 24) |
	       ((v & 0x00FF000000000000) >> 40) |
	       ((v & 0xFF00000000000000) >> 56);
#endif
}

#define le16_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? (v) : bswap16(v))
#define le32_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? (v) : bswap32(v))
#define le64_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? (v) : bswap64(v))
#define be16_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? bswap16(v) : (v))
#define be32_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? bswap32(v) : (v))
#define be64_bswap(v) (CPU_IS_LITTLE_ENDIAN() ? bswap64(v) : (v))

/* ========================================================================== */
/*                          Unaligned memory accesses                         */
/* ========================================================================== */

/*
 * UNALIGNED_ACCESS_IS_FAST() - 1 if unaligned memory accesses can be performed
 * efficiently on the target platform, otherwise 0.
 */
#if (defined(__GNUC__) || defined(__clang__)) && \
	(defined(ARCH_X86_64) || defined(ARCH_X86_32) || \
	 defined(__ARM_FEATURE_UNALIGNED) || defined(__powerpc64__) || \
	 /*
	  * For all compilation purposes, WebAssembly behaves like any other CPU
	  * instruction set. Even though WebAssembly engine might be running on
	  * top of different actual CPU architectures, the WebAssembly spec
	  * itself permits unaligned access and it will be fast on most of those
	  * platforms, and simulated at the engine level on others, so it's
	  * worth treating it as a CPU architecture with fast unaligned access.
	  */ defined(__wasm__))
#  define UNALIGNED_ACCESS_IS_FAST	1
#elif defined(_MSC_VER)
#  define UNALIGNED_ACCESS_IS_FAST	1
#else
#  define UNALIGNED_ACCESS_IS_FAST	0
#endif

/*
 * Implementing unaligned memory accesses using memcpy() is portable, and it
 * usually gets optimized appropriately by modern compilers.  I.e., each
 * memcpy() of 1, 2, 4, or WORDBYTES bytes gets compiled to a load or store
 * instruction, not to an actual function call.
 *
 * We no longer use the "packed struct" approach to unaligned accesses, as that
 * is nonstandard, has unclear semantics, and doesn't receive enough testing
 * (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94994).
 *
 * arm32 with __ARM_FEATURE_UNALIGNED in gcc 5 and earlier is a known exception
 * where memcpy() generates inefficient code
 * (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67366).  However, we no longer
 * consider that one case important enough to maintain different code for.
 * If you run into it, please just use a newer version of gcc (or use clang).
 */

#ifdef FREESTANDING
#  define MEMCOPY	__builtin_memcpy
#else
#  define MEMCOPY	memcpy
#endif

/* Unaligned loads and stores without endianness conversion */

#define DEFINE_UNALIGNED_TYPE(type)				\
static forceinline type						\
load_##type##_unaligned(const void *p)				\
{								\
	type v;							\
								\
	MEMCOPY(&v, p, sizeof(v));				\
	return v;						\
}								\
								\
static forceinline void						\
store_##type##_unaligned(type v, void *p)			\
{								\
	MEMCOPY(p, &v, sizeof(v));				\
}

DEFINE_UNALIGNED_TYPE(u16)
DEFINE_UNALIGNED_TYPE(u32)
DEFINE_UNALIGNED_TYPE(u64)
DEFINE_UNALIGNED_TYPE(machine_word_t)

#undef MEMCOPY

#define load_word_unaligned	load_machine_word_t_unaligned
#define store_word_unaligned	store_machine_word_t_unaligned

/* Unaligned loads with endianness conversion */

static forceinline u16
get_unaligned_le16(const u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST)
		return le16_bswap(load_u16_unaligned(p));
	else
		return ((u16)p[1] << 8) | p[0];
}

static forceinline u16
get_unaligned_be16(const u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST)
		return be16_bswap(load_u16_unaligned(p));
	else
		return ((u16)p[0] << 8) | p[1];
}

static forceinline u32
get_unaligned_le32(const u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST)
		return le32_bswap(load_u32_unaligned(p));
	else
		return ((u32)p[3] << 24) | ((u32)p[2] << 16) |
			((u32)p[1] << 8) | p[0];
}

static forceinline u32
get_unaligned_be32(const u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST)
		return be32_bswap(load_u32_unaligned(p));
	else
		return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
			((u32)p[2] << 8) | p[3];
}

static forceinline u64
get_unaligned_le64(const u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST)
		return le64_bswap(load_u64_unaligned(p));
	else
		return ((u64)p[7] << 56) | ((u64)p[6] << 48) |
			((u64)p[5] << 40) | ((u64)p[4] << 32) |
			((u64)p[3] << 24) | ((u64)p[2] << 16) |
			((u64)p[1] << 8) | p[0];
}

static forceinline machine_word_t
get_unaligned_leword(const u8 *p)
{
	STATIC_ASSERT(WORDBITS == 32 || WORDBITS == 64);
	if (WORDBITS == 32)
		return get_unaligned_le32(p);
	else
		return get_unaligned_le64(p);
}

/* Unaligned stores with endianness conversion */

static forceinline void
put_unaligned_le16(u16 v, u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST) {
		store_u16_unaligned(le16_bswap(v), p);
	} else {
		p[0] = (u8)(v >> 0);
		p[1] = (u8)(v >> 8);
	}
}

static forceinline void
put_unaligned_be16(u16 v, u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST) {
		store_u16_unaligned(be16_bswap(v), p);
	} else {
		p[0] = (u8)(v >> 8);
		p[1] = (u8)(v >> 0);
	}
}

static forceinline void
put_unaligned_le32(u32 v, u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST) {
		store_u32_unaligned(le32_bswap(v), p);
	} else {
		p[0] = (u8)(v >> 0);
		p[1] = (u8)(v >> 8);
		p[2] = (u8)(v >> 16);
		p[3] = (u8)(v >> 24);
	}
}

static forceinline void
put_unaligned_be32(u32 v, u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST) {
		store_u32_unaligned(be32_bswap(v), p);
	} else {
		p[0] = (u8)(v >> 24);
		p[1] = (u8)(v >> 16);
		p[2] = (u8)(v >> 8);
		p[3] = (u8)(v >> 0);
	}
}

static forceinline void
put_unaligned_le64(u64 v, u8 *p)
{
	if (UNALIGNED_ACCESS_IS_FAST) {
		store_u64_unaligned(le64_bswap(v), p);
	} else {
		p[0] = (u8)(v >> 0);
		p[1] = (u8)(v >> 8);
		p[2] = (u8)(v >> 16);
		p[3] = (u8)(v >> 24);
		p[4] = (u8)(v >> 32);
		p[5] = (u8)(v >> 40);
		p[6] = (u8)(v >> 48);
		p[7] = (u8)(v >> 56);
	}
}

static forceinline void
put_unaligned_leword(machine_word_t v, u8 *p)
{
	STATIC_ASSERT(WORDBITS == 32 || WORDBITS == 64);
	if (WORDBITS == 32)
		put_unaligned_le32(v, p);
	else
		put_unaligned_le64(v, p);
}

/* ========================================================================== */
/*                         Bit manipulation functions                         */
/* ========================================================================== */

/*
 * Bit Scan Reverse (BSR) - find the 0-based index (relative to the least
 * significant end) of the *most* significant 1 bit in the input value.  The
 * input value must be nonzero!
 */

static forceinline unsigned
bsr32(u32 v)
{
#if defined(__GNUC__) || __has_builtin(__builtin_clz)
	return 31 - __builtin_clz(v);
#elif defined(_MSC_VER)
	unsigned long i;

	_BitScanReverse(&i, v);
	return i;
#else
	unsigned i = 0;

	while ((v >>= 1) != 0)
		i++;
	return i;
#endif
}

static forceinline unsigned
bsr64(u64 v)
{
#if defined(__GNUC__) || __has_builtin(__builtin_clzll)
	return 63 - __builtin_clzll(v);
#elif defined(_MSC_VER) && defined(_WIN64)
	unsigned long i;

	_BitScanReverse64(&i, v);
	return i;
#else
	unsigned i = 0;

	while ((v >>= 1) != 0)
		i++;
	return i;
#endif
}

static forceinline unsigned
bsrw(machine_word_t v)
{
	STATIC_ASSERT(WORDBITS == 32 || WORDBITS == 64);
	if (WORDBITS == 32)
		return bsr32(v);
	else
		return bsr64(v);
}

/*
 * Bit Scan Forward (BSF) - find the 0-based index (relative to the least
 * significant end) of the *least* significant 1 bit in the input value.  The
 * input value must be nonzero!
 */

static forceinline unsigned
bsf32(u32 v)
{
#if defined(__GNUC__) || __has_builtin(__builtin_ctz)
	return __builtin_ctz(v);
#elif defined(_MSC_VER)
	unsigned long i;

	_BitScanForward(&i, v);
	return i;
#else
	unsigned i = 0;

	for (; (v & 1) == 0; v >>= 1)
		i++;
	return i;
#endif
}

static forceinline unsigned
bsf64(u64 v)
{
#if defined(__GNUC__) || __has_builtin(__builtin_ctzll)
	return __builtin_ctzll(v);
#elif defined(_MSC_VER) && defined(_WIN64)
	unsigned long i;

	_BitScanForward64(&i, v);
	return i;
#else
	unsigned i = 0;

	for (; (v & 1) == 0; v >>= 1)
		i++;
	return i;
#endif
}

static forceinline unsigned
bsfw(machine_word_t v)
{
	STATIC_ASSERT(WORDBITS == 32 || WORDBITS == 64);
	if (WORDBITS == 32)
		return bsf32(v);
	else
		return bsf64(v);
}

/*
 * rbit32(v): reverse the bits in a 32-bit integer.  This doesn't have a
 * fallback implementation; use '#ifdef rbit32' to check if this is available.
 */
#undef rbit32
#if (defined(__GNUC__) || defined(__clang__)) && defined(ARCH_ARM32) && \
	(__ARM_ARCH >= 7 || (__ARM_ARCH == 6 && defined(__ARM_ARCH_6T2__)))
static forceinline u32
rbit32(u32 v)
{
	__asm__("rbit %0, %1" : "=r" (v) : "r" (v));
	return v;
}
#define rbit32 rbit32
#elif (defined(__GNUC__) || defined(__clang__)) && defined(ARCH_ARM64)
static forceinline u32
rbit32(u32 v)
{
	__asm__("rbit %w0, %w1" : "=r" (v) : "r" (v));
	return v;
}
#define rbit32 rbit32
#endif

#endif /* COMMON_DEFS_H */


typedef void *(*malloc_func_t)(size_t);
typedef void (*free_func_t)(void *);

extern malloc_func_t libdeflate_default_malloc_func;
extern free_func_t libdeflate_default_free_func;

void *libdeflate_aligned_malloc(malloc_func_t malloc_func,
				size_t alignment, size_t size);
void libdeflate_aligned_free(free_func_t free_func, void *ptr);

#ifdef FREESTANDING
/*
 * With -ffreestanding, <string.h> may be missing, and we must provide
 * implementations of memset(), memcpy(), memmove(), and memcmp().
 * See https://gcc.gnu.org/onlinedocs/gcc/Standards.html
 *
 * Also, -ffreestanding disables interpreting calls to these functions as
 * built-ins.  E.g., calling memcpy(&v, p, WORDBYTES) will make a function call,
 * not be optimized to a single load instruction.  For performance reasons we
 * don't want that.  So, declare these functions as macros that expand to the
 * corresponding built-ins.  This approach is recommended in the gcc man page.
 * We still need the actual function definitions in case gcc calls them.
 */
void *memset(void *s, int c, size_t n);
#define memset(s, c, n)		__builtin_memset((s), (c), (n))

void *memcpy(void *dest, const void *src, size_t n);
#define memcpy(dest, src, n)	__builtin_memcpy((dest), (src), (n))

void *memmove(void *dest, const void *src, size_t n);
#define memmove(dest, src, n)	__builtin_memmove((dest), (src), (n))

int memcmp(const void *s1, const void *s2, size_t n);
#define memcmp(s1, s2, n)	__builtin_memcmp((s1), (s2), (n))

#undef LIBDEFLATE_ENABLE_ASSERTIONS
#else
#include <string.h>
#endif

/*
 * Runtime assertion support.  Don't enable this in production builds; it may
 * hurt performance significantly.
 */
#ifdef LIBDEFLATE_ENABLE_ASSERTIONS
void libdeflate_assertion_failed(const char *expr, const char *file, int line);
#define ASSERT(expr) { if (unlikely(!(expr))) \
	libdeflate_assertion_failed(#expr, __FILE__, __LINE__); }
#else
#define ASSERT(expr) (void)(expr)
#endif

#define CONCAT_IMPL(a, b)	a##b
#define CONCAT(a, b)		CONCAT_IMPL(a, b)
#define ADD_SUFFIX(name)	CONCAT(name, SUFFIX)

#endif /* LIB_LIB_COMMON_H */

/*
 * deflate_constants.h - constants for the DEFLATE compression format
 */

#ifndef LIB_DEFLATE_CONSTANTS_H
#define LIB_DEFLATE_CONSTANTS_H

/* Valid block types  */
#define DEFLATE_BLOCKTYPE_UNCOMPRESSED		0
#define DEFLATE_BLOCKTYPE_STATIC_HUFFMAN	1
#define DEFLATE_BLOCKTYPE_DYNAMIC_HUFFMAN	2

/* Minimum and maximum supported match lengths (in bytes)  */
#define DEFLATE_MIN_MATCH_LEN			3
#define DEFLATE_MAX_MATCH_LEN			258

/* Maximum supported match offset (in bytes) */
#define DEFLATE_MAX_MATCH_OFFSET		32768

/* log2 of DEFLATE_MAX_MATCH_OFFSET */
#define DEFLATE_WINDOW_ORDER			15

/* Number of symbols in each Huffman code.  Note: for the literal/length
 * and offset codes, these are actually the maximum values; a given block
 * might use fewer symbols.  */
#define DEFLATE_NUM_PRECODE_SYMS		19
#define DEFLATE_NUM_LITLEN_SYMS			288
#define DEFLATE_NUM_OFFSET_SYMS			32

/* The maximum number of symbols across all codes  */
#define DEFLATE_MAX_NUM_SYMS			288

/* Division of symbols in the literal/length code  */
#define DEFLATE_NUM_LITERALS			256
#define DEFLATE_END_OF_BLOCK			256
#define DEFLATE_FIRST_LEN_SYM			257

/* Maximum codeword length, in bits, within each Huffman code  */
#define DEFLATE_MAX_PRE_CODEWORD_LEN		7
#define DEFLATE_MAX_LITLEN_CODEWORD_LEN		15
#define DEFLATE_MAX_OFFSET_CODEWORD_LEN		15

/* The maximum codeword length across all codes  */
#define DEFLATE_MAX_CODEWORD_LEN		15

/* Maximum possible overrun when decoding codeword lengths  */
#define DEFLATE_MAX_LENS_OVERRUN		137

/*
 * Maximum number of extra bits that may be required to represent a match
 * length or offset.
 */
#define DEFLATE_MAX_EXTRA_LENGTH_BITS		5
#define DEFLATE_MAX_EXTRA_OFFSET_BITS		13

#endif /* LIB_DEFLATE_CONSTANTS_H */

/*
 * cpu_features_common.h - code shared by all lib/$arch/cpu_features.c
 *
 * Copyright 2020 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LIB_CPU_FEATURES_COMMON_H
#define LIB_CPU_FEATURES_COMMON_H

#if defined(TEST_SUPPORT__DO_NOT_USE) && !defined(FREESTANDING)
   /* for strdup() and strtok_r() */
#  undef _ANSI_SOURCE
#  ifndef __APPLE__
#    undef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#endif

struct cpu_feature {
	u32 bit;
	const char *name;
};

#if defined(TEST_SUPPORT__DO_NOT_USE) && !defined(FREESTANDING)
/* Disable any features that are listed in $LIBDEFLATE_DISABLE_CPU_FEATURES. */
static inline void
disable_cpu_features_for_testing(u32 *features,
				 const struct cpu_feature *feature_table,
				 size_t feature_table_length)
{
	char *env_value, *strbuf, *p, *saveptr = NULL;
	size_t i;

	env_value = getenv("LIBDEFLATE_DISABLE_CPU_FEATURES");
	if (!env_value)
		return;
	strbuf = strdup(env_value);
	if (!strbuf)
		abort();
	p = strtok_r(strbuf, ",", &saveptr);
	while (p) {
		for (i = 0; i < feature_table_length; i++) {
			if (strcmp(p, feature_table[i].name) == 0) {
				*features &= ~feature_table[i].bit;
				break;
			}
		}
		if (i == feature_table_length) {
			fprintf(stderr,
				"unrecognized feature in LIBDEFLATE_DISABLE_CPU_FEATURES: \"%s\"\n",
				p);
			abort();
		}
		p = strtok_r(NULL, ",", &saveptr);
	}
	free(strbuf);
}
#else /* TEST_SUPPORT__DO_NOT_USE */
static inline void
disable_cpu_features_for_testing(u32 *features,
				 const struct cpu_feature *feature_table,
				 size_t feature_table_length)
{
}
#endif /* !TEST_SUPPORT__DO_NOT_USE */

#endif /* LIB_CPU_FEATURES_COMMON_H */

/*
 * x86/cpu_features.h - feature detection for x86 CPUs
 *
 * Copyright 2016 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LIB_X86_CPU_FEATURES_H
#define LIB_X86_CPU_FEATURES_H

#define HAVE_DYNAMIC_X86_CPU_FEATURES	0

#if defined(ARCH_X86_32) || defined(ARCH_X86_64)

#if COMPILER_SUPPORTS_TARGET_FUNCTION_ATTRIBUTE || defined(_MSC_VER)
#  undef HAVE_DYNAMIC_X86_CPU_FEATURES
#  define HAVE_DYNAMIC_X86_CPU_FEATURES	1
#endif

#define X86_CPU_FEATURE_SSE2		0x00000001
#define X86_CPU_FEATURE_PCLMUL		0x00000002
#define X86_CPU_FEATURE_AVX		0x00000004
#define X86_CPU_FEATURE_AVX2		0x00000008
#define X86_CPU_FEATURE_BMI2		0x00000010

#define HAVE_SSE2(features)	(HAVE_SSE2_NATIVE     || ((features) & X86_CPU_FEATURE_SSE2))
#define HAVE_PCLMUL(features)	(HAVE_PCLMUL_NATIVE   || ((features) & X86_CPU_FEATURE_PCLMUL))
#define HAVE_AVX(features)	(HAVE_AVX_NATIVE      || ((features) & X86_CPU_FEATURE_AVX))
#define HAVE_AVX2(features)	(HAVE_AVX2_NATIVE     || ((features) & X86_CPU_FEATURE_AVX2))
#define HAVE_BMI2(features)	(HAVE_BMI2_NATIVE     || ((features) & X86_CPU_FEATURE_BMI2))

#if HAVE_DYNAMIC_X86_CPU_FEATURES
#define X86_CPU_FEATURES_KNOWN		0x80000000
extern volatile u32 libdeflate_x86_cpu_features;

void libdeflate_init_x86_cpu_features(void);

static inline u32 get_x86_cpu_features(void)
{
	if (libdeflate_x86_cpu_features == 0)
		libdeflate_init_x86_cpu_features();
	return libdeflate_x86_cpu_features;
}
#else /* HAVE_DYNAMIC_X86_CPU_FEATURES */
static inline u32 get_x86_cpu_features(void) { return 0; }
#endif /* !HAVE_DYNAMIC_X86_CPU_FEATURES */

/*
 * Prior to gcc 4.9 (r200349) and clang 3.8 (r239883), x86 intrinsics not
 * available in the main target couldn't be used in 'target' attribute
 * functions.  Unfortunately clang has no feature test macro for this, so we
 * have to check its version.
 */
#if HAVE_DYNAMIC_X86_CPU_FEATURES && \
	(GCC_PREREQ(4, 9) || CLANG_PREREQ(3, 8, 7030000) || defined(_MSC_VER))
#  define HAVE_TARGET_INTRINSICS	1
#else
#  define HAVE_TARGET_INTRINSICS	0
#endif

/* SSE2 */
#if defined(__SSE2__) || \
	(defined(_MSC_VER) && \
	 (defined(ARCH_X86_64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)))
#  define HAVE_SSE2_NATIVE	1
#else
#  define HAVE_SSE2_NATIVE	0
#endif
#define HAVE_SSE2_INTRIN	(HAVE_SSE2_NATIVE || HAVE_TARGET_INTRINSICS)

/* PCLMUL */
#if defined(__PCLMUL__) || (defined(_MSC_VER) && defined(__AVX2__))
#  define HAVE_PCLMUL_NATIVE	1
#else
#  define HAVE_PCLMUL_NATIVE	0
#endif
#if HAVE_PCLMUL_NATIVE || (HAVE_TARGET_INTRINSICS && \
			   (GCC_PREREQ(4, 4) || CLANG_PREREQ(3, 2, 0) || \
			    defined(_MSC_VER)))
#  define HAVE_PCLMUL_INTRIN	1
#else
#  define HAVE_PCLMUL_INTRIN	0
#endif

/* AVX */
#ifdef __AVX__
#  define HAVE_AVX_NATIVE	1
#else
#  define HAVE_AVX_NATIVE	0
#endif
#if HAVE_AVX_NATIVE || (HAVE_TARGET_INTRINSICS && \
			(GCC_PREREQ(4, 6) || CLANG_PREREQ(3, 0, 0) || \
			 defined(_MSC_VER)))
#  define HAVE_AVX_INTRIN	1
#else
#  define HAVE_AVX_INTRIN	0
#endif

/* AVX2 */
#ifdef __AVX2__
#  define HAVE_AVX2_NATIVE	1
#else
#  define HAVE_AVX2_NATIVE	0
#endif
#if HAVE_AVX2_NATIVE || (HAVE_TARGET_INTRINSICS && \
			 (GCC_PREREQ(4, 7) || CLANG_PREREQ(3, 1, 0) || \
			  defined(_MSC_VER)))
#  define HAVE_AVX2_INTRIN	1
#else
#  define HAVE_AVX2_INTRIN	0
#endif

/* BMI2 */
#if defined(__BMI2__) || (defined(_MSC_VER) && defined(__AVX2__))
#  define HAVE_BMI2_NATIVE	1
#else
#  define HAVE_BMI2_NATIVE	0
#endif
#if HAVE_BMI2_NATIVE || (HAVE_TARGET_INTRINSICS && \
			 (GCC_PREREQ(4, 7) || CLANG_PREREQ(3, 1, 0) || \
			  defined(_MSC_VER)))
#  define HAVE_BMI2_INTRIN	1
#else
#  define HAVE_BMI2_INTRIN	0
#endif

#endif /* ARCH_X86_32 || ARCH_X86_64 */

#endif /* LIB_X86_CPU_FEATURES_H */


/*
 * If the expression passed to SAFETY_CHECK() evaluates to false, then the
 * decompression routine immediately returns LIBDEFLATE_BAD_DATA, indicating the
 * compressed data is invalid.
 *
 * Theoretically, these checks could be disabled for specialized applications
 * where all input to the decompressor will be trusted.
 */
#if 0
#  pragma message("UNSAFE DECOMPRESSION IS ENABLED. THIS MUST ONLY BE USED IF THE DECOMPRESSOR INPUT WILL ALWAYS BE TRUSTED!")
#  define SAFETY_CHECK(expr)	(void)(expr)
#else
#  define SAFETY_CHECK(expr)	if (unlikely(!(expr))) return LIBDEFLATE_BAD_DATA
#endif

/*****************************************************************************
 *				Input bitstream                              *
 *****************************************************************************/

/*
 * The state of the "input bitstream" consists of the following variables:
 *
 *	- in_next: a pointer to the next unread byte in the input buffer
 *
 *	- in_end: a pointer to just past the end of the input buffer
 *
 *	- bitbuf: a word-sized variable containing bits that have been read from
 *		  the input buffer or from the implicit appended zero bytes
 *
 *	- bitsleft: the number of bits in 'bitbuf' available to be consumed.
 *		    After REFILL_BITS_BRANCHLESS(), 'bitbuf' can actually
 *		    contain more bits than this.  However, only the bits counted
 *		    by 'bitsleft' can actually be consumed; the rest can only be
 *		    used for preloading.
 *
 *		    As a micro-optimization, we allow bits 8 and higher of
 *		    'bitsleft' to contain garbage.  When consuming the bits
 *		    associated with a decode table entry, this allows us to do
 *		    'bitsleft -= entry' instead of 'bitsleft -= (u8)entry'.
 *		    On some CPUs, this helps reduce instruction dependencies.
 *		    This does have the disadvantage that 'bitsleft' sometimes
 *		    needs to be cast to 'u8', such as when it's used as a shift
 *		    amount in REFILL_BITS_BRANCHLESS().  But that one happens
 *		    for free since most CPUs ignore high bits in shift amounts.
 *
 *	- overread_count: the total number of implicit appended zero bytes that
 *			  have been loaded into the bitbuffer, including any
 *			  counted by 'bitsleft' and any already consumed
 */

/*
 * The type for the bitbuffer variable ('bitbuf' described above).  For best
 * performance, this should have size equal to a machine word.
 *
 * 64-bit platforms have a significant advantage: they get a bigger bitbuffer
 * which they don't have to refill as often.
 */
typedef machine_word_t bitbuf_t;
#define BITBUF_NBITS	(8 * (int)sizeof(bitbuf_t))

/* BITMASK(n) returns a bitmask of length 'n'. */
#define BITMASK(n)	(((bitbuf_t)1 << (n)) - 1)

/*
 * MAX_BITSLEFT is the maximum number of consumable bits, i.e. the maximum value
 * of '(u8)bitsleft'.  This is the size of the bitbuffer variable, minus 1 if
 * the branchless refill method is being used (see REFILL_BITS_BRANCHLESS()).
 */
#define MAX_BITSLEFT	\
	(UNALIGNED_ACCESS_IS_FAST ? BITBUF_NBITS - 1 : BITBUF_NBITS)

/*
 * CONSUMABLE_NBITS is the minimum number of bits that are guaranteed to be
 * consumable (counted in 'bitsleft') immediately after refilling the bitbuffer.
 * Since only whole bytes can be added to 'bitsleft', the worst case is
 * 'MAX_BITSLEFT - 7': the smallest amount where another byte doesn't fit.
 */
#define CONSUMABLE_NBITS	(MAX_BITSLEFT - 7)

/*
 * FASTLOOP_PRELOADABLE_NBITS is the minimum number of bits that are guaranteed
 * to be preloadable immediately after REFILL_BITS_IN_FASTLOOP().  (It is *not*
 * guaranteed after REFILL_BITS(), since REFILL_BITS() falls back to a
 * byte-at-a-time refill method near the end of input.)  This may exceed the
 * number of consumable bits (counted by 'bitsleft').  Any bits not counted in
 * 'bitsleft' can only be used for precomputation and cannot be consumed.
 */
#define FASTLOOP_PRELOADABLE_NBITS	\
	(UNALIGNED_ACCESS_IS_FAST ? BITBUF_NBITS : CONSUMABLE_NBITS)

/*
 * PRELOAD_SLACK is the minimum number of bits that are guaranteed to be
 * preloadable but not consumable, following REFILL_BITS_IN_FASTLOOP() and any
 * subsequent consumptions.  This is 1 bit if the branchless refill method is
 * being used, and 0 bits otherwise.
 */
#define PRELOAD_SLACK	MAX(0, FASTLOOP_PRELOADABLE_NBITS - MAX_BITSLEFT)

/*
 * CAN_CONSUME(n) is true if it's guaranteed that if the bitbuffer has just been
 * refilled, then it's always possible to consume 'n' bits from it.  'n' should
 * be a compile-time constant, to enable compile-time evaluation.
 */
#define CAN_CONSUME(n)	(CONSUMABLE_NBITS >= (n))

/*
 * CAN_CONSUME_AND_THEN_PRELOAD(consume_nbits, preload_nbits) is true if it's
 * guaranteed that after REFILL_BITS_IN_FASTLOOP(), it's always possible to
 * consume 'consume_nbits' bits, then preload 'preload_nbits' bits.  The
 * arguments should be compile-time constants to enable compile-time evaluation.
 */
#define CAN_CONSUME_AND_THEN_PRELOAD(consume_nbits, preload_nbits)	\
	(CONSUMABLE_NBITS >= (consume_nbits) &&				\
	 FASTLOOP_PRELOADABLE_NBITS >= (consume_nbits) + (preload_nbits))

/*
 * REFILL_BITS_BRANCHLESS() branchlessly refills the bitbuffer variable by
 * reading the next word from the input buffer and updating 'in_next' and
 * 'bitsleft' based on how many bits were refilled -- counting whole bytes only.
 * This is much faster than reading a byte at a time, at least if the CPU is
 * little endian and supports fast unaligned memory accesses.
 *
 * The simplest way of branchlessly updating 'bitsleft' would be:
 *
 *	bitsleft += (MAX_BITSLEFT - bitsleft) & ~7;
 *
 * To make it faster, we define MAX_BITSLEFT to be 'WORDBITS - 1' rather than
 * WORDBITS, so that in binary it looks like 111111 or 11111.  Then, we update
 * 'bitsleft' by just setting the bits above the low 3 bits:
 *
 *	bitsleft |= MAX_BITSLEFT & ~7;
 *
 * That compiles down to a single instruction like 'or $0x38, %rbp'.  Using
 * 'MAX_BITSLEFT == WORDBITS - 1' also has the advantage that refills can be
 * done when 'bitsleft == MAX_BITSLEFT' without invoking undefined behavior.
 *
 * The simplest way of branchlessly updating 'in_next' would be:
 *
 *	in_next += (MAX_BITSLEFT - bitsleft) >> 3;
 *
 * With 'MAX_BITSLEFT == WORDBITS - 1' we could use an XOR instead, though this
 * isn't really better:
 *
 *	in_next += (MAX_BITSLEFT ^ bitsleft) >> 3;
 *
 * An alternative which can be marginally better is the following:
 *
 *	in_next += sizeof(bitbuf_t) - 1;
 *	in_next -= (bitsleft >> 3) & 0x7;
 *
 * It seems this would increase the number of CPU instructions from 3 (sub, shr,
 * add) to 4 (add, shr, and, sub).  However, if the CPU has a bitfield
 * extraction instruction (e.g. arm's ubfx), it stays at 3, and is potentially
 * more efficient because the length of the longest dependency chain decreases
 * from 3 to 2.  This alternative also has the advantage that it ignores the
 * high bits in 'bitsleft', so it is compatible with the micro-optimization we
 * use where we let the high bits of 'bitsleft' contain garbage.
 */
#define REFILL_BITS_BRANCHLESS()					\
do {									\
	bitbuf |= get_unaligned_leword(in_next) << (u8)bitsleft;	\
	in_next += sizeof(bitbuf_t) - 1;				\
	in_next -= (bitsleft >> 3) & 0x7;				\
	bitsleft |= MAX_BITSLEFT & ~7;					\
} while (0)

/*
 * REFILL_BITS() loads bits from the input buffer until the bitbuffer variable
 * contains at least CONSUMABLE_NBITS consumable bits.
 *
 * This checks for the end of input, and it doesn't guarantee
 * FASTLOOP_PRELOADABLE_NBITS, so it can't be used in the fastloop.
 *
 * If we would overread the input buffer, we just don't read anything, leaving
 * the bits zeroed but marking them filled.  This simplifies the decompressor
 * because it removes the need to always be able to distinguish between real
 * overreads and overreads caused only by the decompressor's own lookahead.
 *
 * We do still keep track of the number of bytes that have been overread, for
 * two reasons.  First, it allows us to determine the exact number of bytes that
 * were consumed once the stream ends or an uncompressed block is reached.
 * Second, it allows us to stop early if the overread amount gets so large (more
 * than sizeof bitbuf) that it can only be caused by a real overread.  (The
 * second part is arguably unneeded, since libdeflate is buffer-based; given
 * infinite zeroes, it will eventually either completely fill the output buffer
 * or return an error.  However, we do it to be slightly more friendly to the
 * not-recommended use case of decompressing with an unknown output size.)
 */
#define REFILL_BITS()							\
do {									\
	if (UNALIGNED_ACCESS_IS_FAST &&					\
	    likely(in_end - in_next >= sizeof(bitbuf_t))) {		\
		REFILL_BITS_BRANCHLESS();				\
	} else {							\
		while ((u8)bitsleft < CONSUMABLE_NBITS) {		\
			if (likely(in_next != in_end)) {		\
				bitbuf |= (bitbuf_t)*in_next++ <<	\
					  (u8)bitsleft;			\
			} else {					\
				overread_count++;			\
				SAFETY_CHECK(overread_count <=		\
					     sizeof(bitbuf_t));		\
			}						\
			bitsleft += 8;					\
		}							\
	}								\
} while (0)

/*
 * REFILL_BITS_IN_FASTLOOP() is like REFILL_BITS(), but it doesn't check for the
 * end of the input.  It can only be used in the fastloop.
 */
#define REFILL_BITS_IN_FASTLOOP()					\
do {									\
	STATIC_ASSERT(UNALIGNED_ACCESS_IS_FAST ||			\
		      FASTLOOP_PRELOADABLE_NBITS == CONSUMABLE_NBITS);	\
	if (UNALIGNED_ACCESS_IS_FAST) {					\
		REFILL_BITS_BRANCHLESS();				\
	} else {							\
		while ((u8)bitsleft < CONSUMABLE_NBITS) {		\
			bitbuf |= (bitbuf_t)*in_next++ << (u8)bitsleft;	\
			bitsleft += 8;					\
		}							\
	}								\
} while (0)

/*
 * This is the worst-case maximum number of output bytes that are written to
 * during each iteration of the fastloop.  The worst case is 2 literals, then a
 * match of length DEFLATE_MAX_MATCH_LEN.  Additionally, some slack space must
 * be included for the intentional overrun in the match copy implementation.
 */
#define FASTLOOP_MAX_BYTES_WRITTEN	\
	(2 + DEFLATE_MAX_MATCH_LEN + (5 * WORDBYTES) - 1)

/*
 * This is the worst-case maximum number of input bytes that are read during
 * each iteration of the fastloop.  To get this value, we first compute the
 * greatest number of bits that can be refilled during a loop iteration.  The
 * refill at the beginning can add at most MAX_BITSLEFT, and the amount that can
 * be refilled later is no more than the maximum amount that can be consumed by
 * 2 literals that don't need a subtable, then a match.  We convert this value
 * to bytes, rounding up; this gives the maximum number of bytes that 'in_next'
 * can be advanced.  Finally, we add sizeof(bitbuf_t) to account for
 * REFILL_BITS_BRANCHLESS() reading a word past 'in_next'.
 */
#define FASTLOOP_MAX_BYTES_READ					\
	(DIV_ROUND_UP(MAX_BITSLEFT + (2 * LITLEN_TABLEBITS) +	\
		      LENGTH_MAXBITS + OFFSET_MAXBITS, 8) +	\
	 sizeof(bitbuf_t))

/*****************************************************************************
 *                              Huffman decoding                             *
 *****************************************************************************/

/*
 * The fastest way to decode Huffman-encoded data is basically to use a decode
 * table that maps the next TABLEBITS bits of data to their symbol.  Each entry
 * decode_table[i] maps to the symbol whose codeword is a prefix of 'i'.  A
 * symbol with codeword length 'n' has '2**(TABLEBITS-n)' entries in the table.
 *
 * Ideally, TABLEBITS and the maximum codeword length would be the same; some
 * compression formats are designed with this goal in mind.  Unfortunately, in
 * DEFLATE, the maximum litlen and offset codeword lengths are 15 bits, which is
 * too large for a practical TABLEBITS.  It's not *that* much larger, though, so
 * the workaround is to use a single level of subtables.  In the main table,
 * entries for prefixes of codewords longer than TABLEBITS contain a "pointer"
 * to the appropriate subtable along with the number of bits it is indexed with.
 *
 * The most efficient way to allocate subtables is to allocate them dynamically
 * after the main table.  The worst-case number of table entries needed,
 * including subtables, is precomputable; see the ENOUGH constants below.
 *
 * A useful optimization is to store the codeword lengths in the decode table so
 * that they don't have to be looked up by indexing a separate table that maps
 * symbols to their codeword lengths.  We basically do this; however, for the
 * litlen and offset codes we also implement some DEFLATE-specific optimizations
 * that build in the consideration of the "extra bits" and the
 * literal/length/end-of-block division.  For the exact decode table entry
 * format we use, see the definitions of the *_decode_results[] arrays below.
 */


/*
 * These are the TABLEBITS values we use for each of the DEFLATE Huffman codes,
 * along with their corresponding ENOUGH values.
 *
 * For the precode, we use PRECODE_TABLEBITS == 7 since this is the maximum
 * precode codeword length.  This avoids ever needing subtables.
 *
 * For the litlen and offset codes, we cannot realistically avoid ever needing
 * subtables, since litlen and offset codewords can be up to 15 bits.  A higher
 * TABLEBITS reduces the number of lookups that need a subtable, which increases
 * performance; however, it increases memory usage and makes building the table
 * take longer, which decreases performance.  We choose values that work well in
 * practice, making subtables rarely needed without making the tables too large.
 *
 * Our choice of OFFSET_TABLEBITS == 8 is a bit low; without any special
 * considerations, 9 would fit the trade-off curve better.  However, there is a
 * performance benefit to using exactly 8 bits when it is a compile-time
 * constant, as many CPUs can take the low byte more easily than the low 9 bits.
 *
 * zlib treats its equivalents of TABLEBITS as maximum values; whenever it
 * builds a table, it caps the actual table_bits to the longest codeword.  This
 * makes sense in theory, as there's no need for the table to be any larger than
 * needed to support the longest codeword.  However, having the table bits be a
 * compile-time constant is beneficial to the performance of the decode loop, so
 * there is a trade-off.  libdeflate currently uses the dynamic table_bits
 * strategy for the litlen table only, due to its larger maximum size.
 * PRECODE_TABLEBITS and OFFSET_TABLEBITS are smaller, so going dynamic there
 * isn't as useful, and OFFSET_TABLEBITS=8 is useful as mentioned above.
 *
 * Each TABLEBITS value has a corresponding ENOUGH value that gives the
 * worst-case maximum number of decode table entries, including the main table
 * and all subtables.  The ENOUGH value depends on three parameters:
 *
 *	(1) the maximum number of symbols in the code (DEFLATE_NUM_*_SYMS)
 *	(2) the maximum number of main table bits (*_TABLEBITS)
 *	(3) the maximum allowed codeword length (DEFLATE_MAX_*_CODEWORD_LEN)
 *
 * The ENOUGH values were computed using the utility program 'enough' from zlib.
 */
#define PRECODE_TABLEBITS	7
#define PRECODE_ENOUGH		128	/* enough 19 7 7	*/
#define LITLEN_TABLEBITS	11
#define LITLEN_ENOUGH		2342	/* enough 288 11 15	*/
#define OFFSET_TABLEBITS	8
#define OFFSET_ENOUGH		402	/* enough 32 8 15	*/

/*
 * make_decode_table_entry() creates a decode table entry for the given symbol
 * by combining the static part 'decode_results[sym]' with the dynamic part
 * 'len', which is the remaining codeword length (the codeword length for main
 * table entries, or the codeword length minus TABLEBITS for subtable entries).
 *
 * In all cases, we add 'len' to each of the two low-order bytes to create the
 * appropriately-formatted decode table entry.  See the definitions of the
 * *_decode_results[] arrays below, where the entry format is described.
 */
static forceinline u32
make_decode_table_entry(const u32 decode_results[], u32 sym, u32 len)
{
	return decode_results[sym] + (len << 8) + len;
}

/*
 * Here is the format of our precode decode table entries.  Bits not explicitly
 * described contain zeroes:
 *
 *	Bit 20-16:  presym
 *	Bit 10-8:   codeword length [not used]
 *	Bit 2-0:    codeword length
 *
 * The precode decode table never has subtables, since we use
 * PRECODE_TABLEBITS == DEFLATE_MAX_PRE_CODEWORD_LEN.
 *
 * precode_decode_results[] contains the static part of the entry for each
 * symbol.  make_decode_table_entry() produces the final entries.
 */
static const u32 precode_decode_results[] = {
#define ENTRY(presym)	((u32)presym << 16)
	ENTRY(0)   , ENTRY(1)   , ENTRY(2)   , ENTRY(3)   ,
	ENTRY(4)   , ENTRY(5)   , ENTRY(6)   , ENTRY(7)   ,
	ENTRY(8)   , ENTRY(9)   , ENTRY(10)  , ENTRY(11)  ,
	ENTRY(12)  , ENTRY(13)  , ENTRY(14)  , ENTRY(15)  ,
	ENTRY(16)  , ENTRY(17)  , ENTRY(18)  ,
#undef ENTRY
};

/* Litlen and offset decode table entry flags */

/* Indicates a literal entry in the litlen decode table */
#define HUFFDEC_LITERAL			0x80000000

/* Indicates that HUFFDEC_SUBTABLE_POINTER or HUFFDEC_END_OF_BLOCK is set */
#define HUFFDEC_EXCEPTIONAL		0x00008000

/* Indicates a subtable pointer entry in the litlen or offset decode table */
#define HUFFDEC_SUBTABLE_POINTER	0x00004000

/* Indicates an end-of-block entry in the litlen decode table */
#define HUFFDEC_END_OF_BLOCK		0x00002000

/* Maximum number of bits that can be consumed by decoding a match length */
#define LENGTH_MAXBITS		(DEFLATE_MAX_LITLEN_CODEWORD_LEN + \
				 DEFLATE_MAX_EXTRA_LENGTH_BITS)
#define LENGTH_MAXFASTBITS	(LITLEN_TABLEBITS /* no subtable needed */ + \
				 DEFLATE_MAX_EXTRA_LENGTH_BITS)

/*
 * Here is the format of our litlen decode table entries.  Bits not explicitly
 * described contain zeroes:
 *
 *	Literals:
 *		Bit 31:     1 (HUFFDEC_LITERAL)
 *		Bit 23-16:  literal value
 *		Bit 15:     0 (!HUFFDEC_EXCEPTIONAL)
 *		Bit 14:     0 (!HUFFDEC_SUBTABLE_POINTER)
 *		Bit 13:     0 (!HUFFDEC_END_OF_BLOCK)
 *		Bit 11-8:   remaining codeword length [not used]
 *		Bit 3-0:    remaining codeword length
 *	Lengths:
 *		Bit 31:     0 (!HUFFDEC_LITERAL)
 *		Bit 24-16:  length base value
 *		Bit 15:     0 (!HUFFDEC_EXCEPTIONAL)
 *		Bit 14:     0 (!HUFFDEC_SUBTABLE_POINTER)
 *		Bit 13:     0 (!HUFFDEC_END_OF_BLOCK)
 *		Bit 11-8:   remaining codeword length
 *		Bit 4-0:    remaining codeword length + number of extra bits
 *	End of block:
 *		Bit 31:     0 (!HUFFDEC_LITERAL)
 *		Bit 15:     1 (HUFFDEC_EXCEPTIONAL)
 *		Bit 14:     0 (!HUFFDEC_SUBTABLE_POINTER)
 *		Bit 13:     1 (HUFFDEC_END_OF_BLOCK)
 *		Bit 11-8:   remaining codeword length [not used]
 *		Bit 3-0:    remaining codeword length
 *	Subtable pointer:
 *		Bit 31:     0 (!HUFFDEC_LITERAL)
 *		Bit 30-16:  index of start of subtable
 *		Bit 15:     1 (HUFFDEC_EXCEPTIONAL)
 *		Bit 14:     1 (HUFFDEC_SUBTABLE_POINTER)
 *		Bit 13:     0 (!HUFFDEC_END_OF_BLOCK)
 *		Bit 11-8:   number of subtable bits
 *		Bit 3-0:    number of main table bits
 *
 * This format has several desirable properties:
 *
 *	- The codeword length, length slot base, and number of extra length bits
 *	  are all built in.  This eliminates the need to separately look up this
 *	  information by indexing separate arrays by symbol or length slot.
 *
 *	- The HUFFDEC_* flags enable easily distinguishing between the different
 *	  types of entries.  The HUFFDEC_LITERAL flag enables a fast path for
 *	  literals; the high bit is used for this, as some CPUs can test the
 *	  high bit more easily than other bits.  The HUFFDEC_EXCEPTIONAL flag
 *	  makes it possible to detect the two unlikely cases (subtable pointer
 *	  and end of block) in a single bit flag test.
 *
 *	- The low byte is the number of bits that need to be removed from the
 *	  bitstream; this makes this value easily accessible, and it enables the
 *	  micro-optimization of doing 'bitsleft -= entry' instead of
 *	  'bitsleft -= (u8)entry'.  It also includes the number of extra bits,
 *	  so they don't need to be removed separately.
 *
 *	- The flags in bits 15-13 are arranged to be 0 when the
 *	  "remaining codeword length" in bits 11-8 is needed, making this value
 *	  fairly easily accessible as well via a shift and downcast.
 *
 *	- Similarly, bits 13-12 are 0 when the "subtable bits" in bits 11-8 are
 *	  needed, making it possible to extract this value with '& 0x3F' rather
 *	  than '& 0xF'.  This value is only used as a shift amount, so this can
 *	  save an 'and' instruction as the masking by 0x3F happens implicitly.
 *
 * litlen_decode_results[] contains the static part of the entry for each
 * symbol.  make_decode_table_entry() produces the final entries.
 */
static const u32 litlen_decode_results[] = {

	/* Literals */
#define ENTRY(literal)	(HUFFDEC_LITERAL | ((u32)literal << 16))
	ENTRY(0)   , ENTRY(1)   , ENTRY(2)   , ENTRY(3)   ,
	ENTRY(4)   , ENTRY(5)   , ENTRY(6)   , ENTRY(7)   ,
	ENTRY(8)   , ENTRY(9)   , ENTRY(10)  , ENTRY(11)  ,
	ENTRY(12)  , ENTRY(13)  , ENTRY(14)  , ENTRY(15)  ,
	ENTRY(16)  , ENTRY(17)  , ENTRY(18)  , ENTRY(19)  ,
	ENTRY(20)  , ENTRY(21)  , ENTRY(22)  , ENTRY(23)  ,
	ENTRY(24)  , ENTRY(25)  , ENTRY(26)  , ENTRY(27)  ,
	ENTRY(28)  , ENTRY(29)  , ENTRY(30)  , ENTRY(31)  ,
	ENTRY(32)  , ENTRY(33)  , ENTRY(34)  , ENTRY(35)  ,
	ENTRY(36)  , ENTRY(37)  , ENTRY(38)  , ENTRY(39)  ,
	ENTRY(40)  , ENTRY(41)  , ENTRY(42)  , ENTRY(43)  ,
	ENTRY(44)  , ENTRY(45)  , ENTRY(46)  , ENTRY(47)  ,
	ENTRY(48)  , ENTRY(49)  , ENTRY(50)  , ENTRY(51)  ,
	ENTRY(52)  , ENTRY(53)  , ENTRY(54)  , ENTRY(55)  ,
	ENTRY(56)  , ENTRY(57)  , ENTRY(58)  , ENTRY(59)  ,
	ENTRY(60)  , ENTRY(61)  , ENTRY(62)  , ENTRY(63)  ,
	ENTRY(64)  , ENTRY(65)  , ENTRY(66)  , ENTRY(67)  ,
	ENTRY(68)  , ENTRY(69)  , ENTRY(70)  , ENTRY(71)  ,
	ENTRY(72)  , ENTRY(73)  , ENTRY(74)  , ENTRY(75)  ,
	ENTRY(76)  , ENTRY(77)  , ENTRY(78)  , ENTRY(79)  ,
	ENTRY(80)  , ENTRY(81)  , ENTRY(82)  , ENTRY(83)  ,
	ENTRY(84)  , ENTRY(85)  , ENTRY(86)  , ENTRY(87)  ,
	ENTRY(88)  , ENTRY(89)  , ENTRY(90)  , ENTRY(91)  ,
	ENTRY(92)  , ENTRY(93)  , ENTRY(94)  , ENTRY(95)  ,
	ENTRY(96)  , ENTRY(97)  , ENTRY(98)  , ENTRY(99)  ,
	ENTRY(100) , ENTRY(101) , ENTRY(102) , ENTRY(103) ,
	ENTRY(104) , ENTRY(105) , ENTRY(106) , ENTRY(107) ,
	ENTRY(108) , ENTRY(109) , ENTRY(110) , ENTRY(111) ,
	ENTRY(112) , ENTRY(113) , ENTRY(114) , ENTRY(115) ,
	ENTRY(116) , ENTRY(117) , ENTRY(118) , ENTRY(119) ,
	ENTRY(120) , ENTRY(121) , ENTRY(122) , ENTRY(123) ,
	ENTRY(124) , ENTRY(125) , ENTRY(126) , ENTRY(127) ,
	ENTRY(128) , ENTRY(129) , ENTRY(130) , ENTRY(131) ,
	ENTRY(132) , ENTRY(133) , ENTRY(134) , ENTRY(135) ,
	ENTRY(136) , ENTRY(137) , ENTRY(138) , ENTRY(139) ,
	ENTRY(140) , ENTRY(141) , ENTRY(142) , ENTRY(143) ,
	ENTRY(144) , ENTRY(145) , ENTRY(146) , ENTRY(147) ,
	ENTRY(148) , ENTRY(149) , ENTRY(150) , ENTRY(151) ,
	ENTRY(152) , ENTRY(153) , ENTRY(154) , ENTRY(155) ,
	ENTRY(156) , ENTRY(157) , ENTRY(158) , ENTRY(159) ,
	ENTRY(160) , ENTRY(161) , ENTRY(162) , ENTRY(163) ,
	ENTRY(164) , ENTRY(165) , ENTRY(166) , ENTRY(167) ,
	ENTRY(168) , ENTRY(169) , ENTRY(170) , ENTRY(171) ,
	ENTRY(172) , ENTRY(173) , ENTRY(174) , ENTRY(175) ,
	ENTRY(176) , ENTRY(177) , ENTRY(178) , ENTRY(179) ,
	ENTRY(180) , ENTRY(181) , ENTRY(182) , ENTRY(183) ,
	ENTRY(184) , ENTRY(185) , ENTRY(186) , ENTRY(187) ,
	ENTRY(188) , ENTRY(189) , ENTRY(190) , ENTRY(191) ,
	ENTRY(192) , ENTRY(193) , ENTRY(194) , ENTRY(195) ,
	ENTRY(196) , ENTRY(197) , ENTRY(198) , ENTRY(199) ,
	ENTRY(200) , ENTRY(201) , ENTRY(202) , ENTRY(203) ,
	ENTRY(204) , ENTRY(205) , ENTRY(206) , ENTRY(207) ,
	ENTRY(208) , ENTRY(209) , ENTRY(210) , ENTRY(211) ,
	ENTRY(212) , ENTRY(213) , ENTRY(214) , ENTRY(215) ,
	ENTRY(216) , ENTRY(217) , ENTRY(218) , ENTRY(219) ,
	ENTRY(220) , ENTRY(221) , ENTRY(222) , ENTRY(223) ,
	ENTRY(224) , ENTRY(225) , ENTRY(226) , ENTRY(227) ,
	ENTRY(228) , ENTRY(229) , ENTRY(230) , ENTRY(231) ,
	ENTRY(232) , ENTRY(233) , ENTRY(234) , ENTRY(235) ,
	ENTRY(236) , ENTRY(237) , ENTRY(238) , ENTRY(239) ,
	ENTRY(240) , ENTRY(241) , ENTRY(242) , ENTRY(243) ,
	ENTRY(244) , ENTRY(245) , ENTRY(246) , ENTRY(247) ,
	ENTRY(248) , ENTRY(249) , ENTRY(250) , ENTRY(251) ,
	ENTRY(252) , ENTRY(253) , ENTRY(254) , ENTRY(255) ,
#undef ENTRY

	/* End of block */
	HUFFDEC_EXCEPTIONAL | HUFFDEC_END_OF_BLOCK,

	/* Lengths */
#define ENTRY(length_base, num_extra_bits)	\
	(((u32)(length_base) << 16) | (num_extra_bits))
	ENTRY(3  , 0) , ENTRY(4  , 0) , ENTRY(5  , 0) , ENTRY(6  , 0),
	ENTRY(7  , 0) , ENTRY(8  , 0) , ENTRY(9  , 0) , ENTRY(10 , 0),
	ENTRY(11 , 1) , ENTRY(13 , 1) , ENTRY(15 , 1) , ENTRY(17 , 1),
	ENTRY(19 , 2) , ENTRY(23 , 2) , ENTRY(27 , 2) , ENTRY(31 , 2),
	ENTRY(35 , 3) , ENTRY(43 , 3) , ENTRY(51 , 3) , ENTRY(59 , 3),
	ENTRY(67 , 4) , ENTRY(83 , 4) , ENTRY(99 , 4) , ENTRY(115, 4),
	ENTRY(131, 5) , ENTRY(163, 5) , ENTRY(195, 5) , ENTRY(227, 5),
	ENTRY(258, 0) , ENTRY(258, 0) , ENTRY(258, 0) ,
#undef ENTRY
};

/* Maximum number of bits that can be consumed by decoding a match offset */
#define OFFSET_MAXBITS		(DEFLATE_MAX_OFFSET_CODEWORD_LEN + \
				 DEFLATE_MAX_EXTRA_OFFSET_BITS)
#define OFFSET_MAXFASTBITS	(OFFSET_TABLEBITS /* no subtable needed */ + \
				 DEFLATE_MAX_EXTRA_OFFSET_BITS)

/*
 * Here is the format of our offset decode table entries.  Bits not explicitly
 * described contain zeroes:
 *
 *	Offsets:
 *		Bit 31-16:  offset base value
 *		Bit 15:     0 (!HUFFDEC_EXCEPTIONAL)
 *		Bit 14:     0 (!HUFFDEC_SUBTABLE_POINTER)
 *		Bit 11-8:   remaining codeword length
 *		Bit 4-0:    remaining codeword length + number of extra bits
 *	Subtable pointer:
 *		Bit 31-16:  index of start of subtable
 *		Bit 15:     1 (HUFFDEC_EXCEPTIONAL)
 *		Bit 14:     1 (HUFFDEC_SUBTABLE_POINTER)
 *		Bit 11-8:   number of subtable bits
 *		Bit 3-0:    number of main table bits
 *
 * These work the same way as the length entries and subtable pointer entries in
 * the litlen decode table; see litlen_decode_results[] above.
 */
static const u32 offset_decode_results[] = {
#define ENTRY(offset_base, num_extra_bits)	\
	(((u32)(offset_base) << 16) | (num_extra_bits))
	ENTRY(1     , 0)  , ENTRY(2     , 0)  , ENTRY(3     , 0)  , ENTRY(4     , 0)  ,
	ENTRY(5     , 1)  , ENTRY(7     , 1)  , ENTRY(9     , 2)  , ENTRY(13    , 2) ,
	ENTRY(17    , 3)  , ENTRY(25    , 3)  , ENTRY(33    , 4)  , ENTRY(49    , 4)  ,
	ENTRY(65    , 5)  , ENTRY(97    , 5)  , ENTRY(129   , 6)  , ENTRY(193   , 6)  ,
	ENTRY(257   , 7)  , ENTRY(385   , 7)  , ENTRY(513   , 8)  , ENTRY(769   , 8)  ,
	ENTRY(1025  , 9)  , ENTRY(1537  , 9)  , ENTRY(2049  , 10) , ENTRY(3073  , 10) ,
	ENTRY(4097  , 11) , ENTRY(6145  , 11) , ENTRY(8193  , 12) , ENTRY(12289 , 12) ,
	ENTRY(16385 , 13) , ENTRY(24577 , 13) , ENTRY(24577 , 13) , ENTRY(24577 , 13) ,
#undef ENTRY
};

/*
 * The main DEFLATE decompressor structure.  Since libdeflate only supports
 * full-buffer decompression, this structure doesn't store the entire
 * decompression state, most of which is in stack variables.  Instead, this
 * struct just contains the decode tables and some temporary arrays used for
 * building them, as these are too large to comfortably allocate on the stack.
 *
 * Storing the decode tables in the decompressor struct also allows the decode
 * tables for the static codes to be reused whenever two static Huffman blocks
 * are decoded without an intervening dynamic block, even across streams.
 */
struct libdeflate_decompressor {

	/*
	 * The arrays aren't all needed at the same time.  'precode_lens' and
	 * 'precode_decode_table' are unneeded after 'lens' has been filled.
	 * Furthermore, 'lens' need not be retained after building the litlen
	 * and offset decode tables.  In fact, 'lens' can be in union with
	 * 'litlen_decode_table' provided that 'offset_decode_table' is separate
	 * and is built first.
	 */

	union {
		u8 precode_lens[DEFLATE_NUM_PRECODE_SYMS];

		struct {
			u8 lens[DEFLATE_NUM_LITLEN_SYMS +
				DEFLATE_NUM_OFFSET_SYMS +
				DEFLATE_MAX_LENS_OVERRUN];

			u32 precode_decode_table[PRECODE_ENOUGH];
		} l;

		u32 litlen_decode_table[LITLEN_ENOUGH];
	} u;

	u32 offset_decode_table[OFFSET_ENOUGH];

	/* used only during build_decode_table() */
	u16 sorted_syms[DEFLATE_MAX_NUM_SYMS];

	bool static_codes_loaded;
	unsigned litlen_tablebits;

	/* The free() function for this struct, chosen at allocation time */
	free_func_t free_func;
};

/*
 * Build a table for fast decoding of symbols from a Huffman code.  As input,
 * this function takes the codeword length of each symbol which may be used in
 * the code.  As output, it produces a decode table for the canonical Huffman
 * code described by the codeword lengths.  The decode table is built with the
 * assumption that it will be indexed with "bit-reversed" codewords, where the
 * low-order bit is the first bit of the codeword.  This format is used for all
 * Huffman codes in DEFLATE.
 *
 * @decode_table
 *	The array in which the decode table will be generated.  This array must
 *	have sufficient length; see the definition of the ENOUGH numbers.
 * @lens
 *	An array which provides, for each symbol, the length of the
 *	corresponding codeword in bits, or 0 if the symbol is unused.  This may
 *	alias @decode_table, since nothing is written to @decode_table until all
 *	@lens have been consumed.  All codeword lengths are assumed to be <=
 *	@max_codeword_len but are otherwise considered untrusted.  If they do
 *	not form a valid Huffman code, then the decode table is not built and
 *	%false is returned.
 * @num_syms
 *	The number of symbols in the code, including all unused symbols.
 * @decode_results
 *	An array which gives the incomplete decode result for each symbol.  The
 *	needed values in this array will be combined with codeword lengths to
 *	make the final decode table entries using make_decode_table_entry().
 * @table_bits
 *	The log base-2 of the number of main table entries to use.
 *	If @table_bits_ret != NULL, then @table_bits is treated as a maximum
 *	value and it will be decreased if a smaller table would be sufficient.
 * @max_codeword_len
 *	The maximum allowed codeword length for this Huffman code.
 *	Must be <= DEFLATE_MAX_CODEWORD_LEN.
 * @sorted_syms
 *	A temporary array of length @num_syms.
 * @table_bits_ret
 *	If non-NULL, then the dynamic table_bits is enabled, and the actual
 *	table_bits value will be returned here.
 *
 * Returns %true if successful; %false if the codeword lengths do not form a
 * valid Huffman code.
 */
static bool
build_decode_table(u32 decode_table[],
		   const u8 lens[],
		   const unsigned num_syms,
		   const u32 decode_results[],
		   unsigned table_bits,
		   unsigned max_codeword_len,
		   u16 *sorted_syms,
		   unsigned *table_bits_ret)
{
	unsigned len_counts[DEFLATE_MAX_CODEWORD_LEN + 1];
	unsigned offsets[DEFLATE_MAX_CODEWORD_LEN + 1];
	unsigned sym;		/* current symbol */
	unsigned codeword;	/* current codeword, bit-reversed */
	unsigned len;		/* current codeword length in bits */
	unsigned count;		/* num codewords remaining with this length */
	u32 codespace_used;	/* codespace used out of '2^max_codeword_len' */
	unsigned cur_table_end; /* end index of current table */
	unsigned subtable_prefix; /* codeword prefix of current subtable */
	unsigned subtable_start;  /* start index of current subtable */
	unsigned subtable_bits;   /* log2 of current subtable length */

	/* Count how many codewords have each length, including 0. */
	for (len = 0; len <= max_codeword_len; len++)
		len_counts[len] = 0;
	for (sym = 0; sym < num_syms; sym++)
		len_counts[lens[sym]]++;

	/*
	 * Determine the actual maximum codeword length that was used, and
	 * decrease table_bits to it if allowed.
	 */
	while (max_codeword_len > 1 && len_counts[max_codeword_len] == 0)
		max_codeword_len--;
	if (table_bits_ret != NULL) {
		table_bits = MIN(table_bits, max_codeword_len);
		*table_bits_ret = table_bits;
	}

	/*
	 * Sort the symbols primarily by increasing codeword length and
	 * secondarily by increasing symbol value; or equivalently by their
	 * codewords in lexicographic order, since a canonical code is assumed.
	 *
	 * For efficiency, also compute 'codespace_used' in the same pass over
	 * 'len_counts[]' used to build 'offsets[]' for sorting.
	 */

	/* Ensure that 'codespace_used' cannot overflow. */
	STATIC_ASSERT(sizeof(codespace_used) == 4);
	STATIC_ASSERT(UINT32_MAX / (1U << (DEFLATE_MAX_CODEWORD_LEN - 1)) >=
		      DEFLATE_MAX_NUM_SYMS);

	offsets[0] = 0;
	offsets[1] = len_counts[0];
	codespace_used = 0;
	for (len = 1; len < max_codeword_len; len++) {
		offsets[len + 1] = offsets[len] + len_counts[len];
		codespace_used = (codespace_used << 1) + len_counts[len];
	}
	codespace_used = (codespace_used << 1) + len_counts[len];

	for (sym = 0; sym < num_syms; sym++)
		sorted_syms[offsets[lens[sym]]++] = sym;

	sorted_syms += offsets[0]; /* Skip unused symbols */

	/* lens[] is done being used, so we can write to decode_table[] now. */

	/*
	 * Check whether the lengths form a complete code (exactly fills the
	 * codespace), an incomplete code (doesn't fill the codespace), or an
	 * overfull code (overflows the codespace).  A codeword of length 'n'
	 * uses proportion '1/(2^n)' of the codespace.  An overfull code is
	 * nonsensical, so is considered invalid.  An incomplete code is
	 * considered valid only in two specific cases; see below.
	 */

	/* overfull code? */
	if (unlikely(codespace_used > (1U << max_codeword_len)))
		return false;

	/* incomplete code? */
	if (unlikely(codespace_used < (1U << max_codeword_len))) {
		u32 entry;
		unsigned i;

		if (codespace_used == 0) {
			/*
			 * An empty code is allowed.  This can happen for the
			 * offset code in DEFLATE, since a dynamic Huffman block
			 * need not contain any matches.
			 */

			/* sym=0, len=1 (arbitrary) */
			entry = make_decode_table_entry(decode_results, 0, 1);
		} else {
			/*
			 * Allow codes with a single used symbol, with codeword
			 * length 1.  The DEFLATE RFC is unclear regarding this
			 * case.  What zlib's decompressor does is permit this
			 * for the litlen and offset codes and assume the
			 * codeword is '0' rather than '1'.  We do the same
			 * except we allow this for precodes too, since there's
			 * no convincing reason to treat the codes differently.
			 * We also assign both codewords '0' and '1' to the
			 * symbol to avoid having to handle '1' specially.
			 */
			if (codespace_used != (1U << (max_codeword_len - 1)) ||
			    len_counts[1] != 1)
				return false;
			entry = make_decode_table_entry(decode_results,
							*sorted_syms, 1);
		}
		/*
		 * Note: the decode table still must be fully initialized, in
		 * case the stream is malformed and contains bits from the part
		 * of the codespace the incomplete code doesn't use.
		 */
		for (i = 0; i < (1U << table_bits); i++)
			decode_table[i] = entry;
		return true;
	}

	/*
	 * The lengths form a complete code.  Now, enumerate the codewords in
	 * lexicographic order and fill the decode table entries for each one.
	 *
	 * First, process all codewords with len <= table_bits.  Each one gets
	 * '2^(table_bits-len)' direct entries in the table.
	 *
	 * Since DEFLATE uses bit-reversed codewords, these entries aren't
	 * consecutive but rather are spaced '2^len' entries apart.  This makes
	 * filling them naively somewhat awkward and inefficient, since strided
	 * stores are less cache-friendly and preclude the use of word or
	 * vector-at-a-time stores to fill multiple entries per instruction.
	 *
	 * To optimize this, we incrementally double the table size.  When
	 * processing codewords with length 'len', the table is treated as
	 * having only '2^len' entries, so each codeword uses just one entry.
	 * Then, each time 'len' is incremented, the table size is doubled and
	 * the first half is copied to the second half.  This significantly
	 * improves performance over naively doing strided stores.
	 *
	 * Note that some entries copied for each table doubling may not have
	 * been initialized yet, but it doesn't matter since they're guaranteed
	 * to be initialized later (because the Huffman code is complete).
	 */
	codeword = 0;
	len = 1;
	while ((count = len_counts[len]) == 0)
		len++;
	cur_table_end = 1U << len;
	while (len <= table_bits) {
		/* Process all 'count' codewords with length 'len' bits. */
		do {
			unsigned bit;

			/* Fill the first entry for the current codeword. */
			decode_table[codeword] =
				make_decode_table_entry(decode_results,
							*sorted_syms++, len);

			if (codeword == cur_table_end - 1) {
				/* Last codeword (all 1's) */
				for (; len < table_bits; len++) {
					memcpy(&decode_table[cur_table_end],
					       decode_table,
					       cur_table_end *
						sizeof(decode_table[0]));
					cur_table_end <<= 1;
				}
				return true;
			}
			/*
			 * To advance to the lexicographically next codeword in
			 * the canonical code, the codeword must be incremented,
			 * then 0's must be appended to the codeword as needed
			 * to match the next codeword's length.
			 *
			 * Since the codeword is bit-reversed, appending 0's is
			 * a no-op.  However, incrementing it is nontrivial.  To
			 * do so efficiently, use the 'bsr' instruction to find
			 * the last (highest order) 0 bit in the codeword, set
			 * it, and clear any later (higher order) 1 bits.  But
			 * 'bsr' actually finds the highest order 1 bit, so to
			 * use it first flip all bits in the codeword by XOR'ing
			 * it with (1U << len) - 1 == cur_table_end - 1.
			 */
			bit = 1U << bsr32(codeword ^ (cur_table_end - 1));
			codeword &= bit - 1;
			codeword |= bit;
		} while (--count);

		/* Advance to the next codeword length. */
		do {
			if (++len <= table_bits) {
				memcpy(&decode_table[cur_table_end],
				       decode_table,
				       cur_table_end * sizeof(decode_table[0]));
				cur_table_end <<= 1;
			}
		} while ((count = len_counts[len]) == 0);
	}

	/* Process codewords with len > table_bits.  These require subtables. */
	cur_table_end = 1U << table_bits;
	subtable_prefix = -1;
	subtable_start = 0;
	for (;;) {
		u32 entry;
		unsigned i;
		unsigned stride;
		unsigned bit;

		/*
		 * Start a new subtable if the first 'table_bits' bits of the
		 * codeword don't match the prefix of the current subtable.
		 */
		if ((codeword & ((1U << table_bits) - 1)) != subtable_prefix) {
			subtable_prefix = (codeword & ((1U << table_bits) - 1));
			subtable_start = cur_table_end;
			/*
			 * Calculate the subtable length.  If the codeword has
			 * length 'table_bits + n', then the subtable needs
			 * '2^n' entries.  But it may need more; if fewer than
			 * '2^n' codewords of length 'table_bits + n' remain,
			 * then the length will need to be incremented to bring
			 * in longer codewords until the subtable can be
			 * completely filled.  Note that because the Huffman
			 * code is complete, it will always be possible to fill
			 * the subtable eventually.
			 */
			subtable_bits = len - table_bits;
			codespace_used = count;
			while (codespace_used < (1U << subtable_bits)) {
				subtable_bits++;
				codespace_used = (codespace_used << 1) +
					len_counts[table_bits + subtable_bits];
			}
			cur_table_end = subtable_start + (1U << subtable_bits);

			/*
			 * Create the entry that points from the main table to
			 * the subtable.
			 */
			decode_table[subtable_prefix] =
				((u32)subtable_start << 16) |
				HUFFDEC_EXCEPTIONAL |
				HUFFDEC_SUBTABLE_POINTER |
				(subtable_bits << 8) | table_bits;
		}

		/* Fill the subtable entries for the current codeword. */
		entry = make_decode_table_entry(decode_results, *sorted_syms++,
						len - table_bits);
		i = subtable_start + (codeword >> table_bits);
		stride = 1U << (len - table_bits);
		do {
			decode_table[i] = entry;
			i += stride;
		} while (i < cur_table_end);

		/* Advance to the next codeword. */
		if (codeword == (1U << len) - 1) /* last codeword (all 1's)? */
			return true;
		bit = 1U << bsr32(codeword ^ ((1U << len) - 1));
		codeword &= bit - 1;
		codeword |= bit;
		count--;
		while (count == 0)
			count = len_counts[++len];
	}
}

/* Build the decode table for the precode.  */
static bool
build_precode_decode_table(struct libdeflate_decompressor *d)
{
	/* When you change TABLEBITS, you must change ENOUGH, and vice versa! */
	STATIC_ASSERT(PRECODE_TABLEBITS == 7 && PRECODE_ENOUGH == 128);

	STATIC_ASSERT(ARRAY_LEN(precode_decode_results) ==
		      DEFLATE_NUM_PRECODE_SYMS);

	return build_decode_table(d->u.l.precode_decode_table,
				  d->u.precode_lens,
				  DEFLATE_NUM_PRECODE_SYMS,
				  precode_decode_results,
				  PRECODE_TABLEBITS,
				  DEFLATE_MAX_PRE_CODEWORD_LEN,
				  d->sorted_syms,
				  NULL);
}

/* Build the decode table for the literal/length code.  */
static bool
build_litlen_decode_table(struct libdeflate_decompressor *d,
			  unsigned num_litlen_syms, unsigned num_offset_syms)
{
	/* When you change TABLEBITS, you must change ENOUGH, and vice versa! */
	STATIC_ASSERT(LITLEN_TABLEBITS == 11 && LITLEN_ENOUGH == 2342);

	STATIC_ASSERT(ARRAY_LEN(litlen_decode_results) ==
		      DEFLATE_NUM_LITLEN_SYMS);

	return build_decode_table(d->u.litlen_decode_table,
				  d->u.l.lens,
				  num_litlen_syms,
				  litlen_decode_results,
				  LITLEN_TABLEBITS,
				  DEFLATE_MAX_LITLEN_CODEWORD_LEN,
				  d->sorted_syms,
				  &d->litlen_tablebits);
}

/* Build the decode table for the offset code.  */
static bool
build_offset_decode_table(struct libdeflate_decompressor *d,
			  unsigned num_litlen_syms, unsigned num_offset_syms)
{
	/* When you change TABLEBITS, you must change ENOUGH, and vice versa! */
	STATIC_ASSERT(OFFSET_TABLEBITS == 8 && OFFSET_ENOUGH == 402);

	STATIC_ASSERT(ARRAY_LEN(offset_decode_results) ==
		      DEFLATE_NUM_OFFSET_SYMS);

	return build_decode_table(d->offset_decode_table,
				  d->u.l.lens + num_litlen_syms,
				  num_offset_syms,
				  offset_decode_results,
				  OFFSET_TABLEBITS,
				  DEFLATE_MAX_OFFSET_CODEWORD_LEN,
				  d->sorted_syms,
				  NULL);
}

/*****************************************************************************
 *                         Main decompression routine
 *****************************************************************************/

typedef enum libdeflate_result (*decompress_func_t)
	(struct libdeflate_decompressor * restrict d,
	 const void * restrict in, size_t in_nbytes,
	 void * restrict out, size_t out_nbytes_avail,
	 size_t *actual_in_nbytes_ret, size_t *actual_out_nbytes_ret);

#define FUNCNAME deflate_decompress_default
#undef ATTRIBUTES
#undef EXTRACT_VARBITS
#undef EXTRACT_VARBITS8
/*
 * decompress_template.h
 *
 * Copyright 2016 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This is the actual DEFLATE decompression routine, lifted out of
 * deflate_decompress.c so that it can be compiled multiple times with different
 * target instruction sets.
 */

#ifndef ATTRIBUTES
#  define ATTRIBUTES
#endif
#ifndef EXTRACT_VARBITS
#  define EXTRACT_VARBITS(word, count)	((word) & BITMASK(count))
#endif
#ifndef EXTRACT_VARBITS8
#  define EXTRACT_VARBITS8(word, count)	((word) & BITMASK((u8)(count)))
#endif

static enum libdeflate_result ATTRIBUTES MAYBE_UNUSED
FUNCNAME(struct libdeflate_decompressor * restrict d,
	 const void * restrict in, size_t in_nbytes,
	 void * restrict out, size_t out_nbytes_avail,
	 size_t *actual_in_nbytes_ret, size_t *actual_out_nbytes_ret)
{
	u8 *out_next = out;
	u8 * const out_end = out_next + out_nbytes_avail;
	u8 * const out_fastloop_end =
		out_end - MIN(out_nbytes_avail, FASTLOOP_MAX_BYTES_WRITTEN);

	/* Input bitstream state; see deflate_decompress.c for documentation */
	const u8 *in_next = in;
	const u8 * const in_end = in_next + in_nbytes;
	const u8 * const in_fastloop_end =
		in_end - MIN(in_nbytes, FASTLOOP_MAX_BYTES_READ);
	bitbuf_t bitbuf = 0;
	bitbuf_t saved_bitbuf;
	u32 bitsleft = 0;
	size_t overread_count = 0;

	bool is_final_block;
	unsigned block_type;
	unsigned num_litlen_syms;
	unsigned num_offset_syms;
	bitbuf_t litlen_tablemask;
	u32 entry;

next_block:
	/* Starting to read the next block */
	;

	STATIC_ASSERT(CAN_CONSUME(1 + 2 + 5 + 5 + 4 + 3));
	REFILL_BITS();

	/* BFINAL: 1 bit */
	is_final_block = bitbuf & BITMASK(1);

	/* BTYPE: 2 bits */
	block_type = (bitbuf >> 1) & BITMASK(2);

	if (block_type == DEFLATE_BLOCKTYPE_DYNAMIC_HUFFMAN) {

		/* Dynamic Huffman block */

		/* The order in which precode lengths are stored */
		static const u8 deflate_precode_lens_permutation[DEFLATE_NUM_PRECODE_SYMS] = {
			16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
		};

		unsigned num_explicit_precode_lens;
		unsigned i;

		/* Read the codeword length counts. */

		STATIC_ASSERT(DEFLATE_NUM_LITLEN_SYMS == 257 + BITMASK(5));
		num_litlen_syms = 257 + ((bitbuf >> 3) & BITMASK(5));

		STATIC_ASSERT(DEFLATE_NUM_OFFSET_SYMS == 1 + BITMASK(5));
		num_offset_syms = 1 + ((bitbuf >> 8) & BITMASK(5));

		STATIC_ASSERT(DEFLATE_NUM_PRECODE_SYMS == 4 + BITMASK(4));
		num_explicit_precode_lens = 4 + ((bitbuf >> 13) & BITMASK(4));

		d->static_codes_loaded = false;

		/*
		 * Read the precode codeword lengths.
		 *
		 * A 64-bit bitbuffer is just one bit too small to hold the
		 * maximum number of precode lens, so to minimize branches we
		 * merge one len with the previous fields.
		 */
		STATIC_ASSERT(DEFLATE_MAX_PRE_CODEWORD_LEN == (1 << 3) - 1);
		if (CAN_CONSUME(3 * (DEFLATE_NUM_PRECODE_SYMS - 1))) {
			d->u.precode_lens[deflate_precode_lens_permutation[0]] =
				(bitbuf >> 17) & BITMASK(3);
			bitbuf >>= 20;
			bitsleft -= 20;
			REFILL_BITS();
			i = 1;
			do {
				d->u.precode_lens[deflate_precode_lens_permutation[i]] =
					bitbuf & BITMASK(3);
				bitbuf >>= 3;
				bitsleft -= 3;
			} while (++i < num_explicit_precode_lens);
		} else {
			bitbuf >>= 17;
			bitsleft -= 17;
			i = 0;
			do {
				if ((u8)bitsleft < 3)
					REFILL_BITS();
				d->u.precode_lens[deflate_precode_lens_permutation[i]] =
					bitbuf & BITMASK(3);
				bitbuf >>= 3;
				bitsleft -= 3;
			} while (++i < num_explicit_precode_lens);
		}
		for (; i < DEFLATE_NUM_PRECODE_SYMS; i++)
			d->u.precode_lens[deflate_precode_lens_permutation[i]] = 0;

		/* Build the decode table for the precode. */
		SAFETY_CHECK(build_precode_decode_table(d));

		/* Decode the litlen and offset codeword lengths. */
		i = 0;
		do {
			unsigned presym;
			u8 rep_val;
			unsigned rep_count;

			if ((u8)bitsleft < DEFLATE_MAX_PRE_CODEWORD_LEN + 7)
				REFILL_BITS();

			/*
			 * The code below assumes that the precode decode table
			 * doesn't have any subtables.
			 */
			STATIC_ASSERT(PRECODE_TABLEBITS == DEFLATE_MAX_PRE_CODEWORD_LEN);

			/* Decode the next precode symbol. */
			entry = d->u.l.precode_decode_table[
				bitbuf & BITMASK(DEFLATE_MAX_PRE_CODEWORD_LEN)];
			bitbuf >>= (u8)entry;
			bitsleft -= entry; /* optimization: subtract full entry */
			presym = entry >> 16;

			if (presym < 16) {
				/* Explicit codeword length */
				d->u.l.lens[i++] = presym;
				continue;
			}

			/* Run-length encoded codeword lengths */

			/*
			 * Note: we don't need to immediately verify that the
			 * repeat count doesn't overflow the number of elements,
			 * since we've sized the lens array to have enough extra
			 * space to allow for the worst-case overrun (138 zeroes
			 * when only 1 length was remaining).
			 *
			 * In the case of the small repeat counts (presyms 16
			 * and 17), it is fastest to always write the maximum
			 * number of entries.  That gets rid of branches that
			 * would otherwise be required.
			 *
			 * It is not just because of the numerical order that
			 * our checks go in the order 'presym < 16', 'presym ==
			 * 16', and 'presym == 17'.  For typical data this is
			 * ordered from most frequent to least frequent case.
			 */
			STATIC_ASSERT(DEFLATE_MAX_LENS_OVERRUN == 138 - 1);

			if (presym == 16) {
				/* Repeat the previous length 3 - 6 times. */
				SAFETY_CHECK(i != 0);
				rep_val = d->u.l.lens[i - 1];
				STATIC_ASSERT(3 + BITMASK(2) == 6);
				rep_count = 3 + (bitbuf & BITMASK(2));
				bitbuf >>= 2;
				bitsleft -= 2;
				d->u.l.lens[i + 0] = rep_val;
				d->u.l.lens[i + 1] = rep_val;
				d->u.l.lens[i + 2] = rep_val;
				d->u.l.lens[i + 3] = rep_val;
				d->u.l.lens[i + 4] = rep_val;
				d->u.l.lens[i + 5] = rep_val;
				i += rep_count;
			} else if (presym == 17) {
				/* Repeat zero 3 - 10 times. */
				STATIC_ASSERT(3 + BITMASK(3) == 10);
				rep_count = 3 + (bitbuf & BITMASK(3));
				bitbuf >>= 3;
				bitsleft -= 3;
				d->u.l.lens[i + 0] = 0;
				d->u.l.lens[i + 1] = 0;
				d->u.l.lens[i + 2] = 0;
				d->u.l.lens[i + 3] = 0;
				d->u.l.lens[i + 4] = 0;
				d->u.l.lens[i + 5] = 0;
				d->u.l.lens[i + 6] = 0;
				d->u.l.lens[i + 7] = 0;
				d->u.l.lens[i + 8] = 0;
				d->u.l.lens[i + 9] = 0;
				i += rep_count;
			} else {
				/* Repeat zero 11 - 138 times. */
				STATIC_ASSERT(11 + BITMASK(7) == 138);
				rep_count = 11 + (bitbuf & BITMASK(7));
				bitbuf >>= 7;
				bitsleft -= 7;
				memset(&d->u.l.lens[i], 0,
				       rep_count * sizeof(d->u.l.lens[i]));
				i += rep_count;
			}
		} while (i < num_litlen_syms + num_offset_syms);

		/* Unnecessary, but check this for consistency with zlib. */
		SAFETY_CHECK(i == num_litlen_syms + num_offset_syms);

	} else if (block_type == DEFLATE_BLOCKTYPE_UNCOMPRESSED) {
		u16 len, nlen;

		/*
		 * Uncompressed block: copy 'len' bytes literally from the input
		 * buffer to the output buffer.
		 */

		bitsleft -= 3; /* for BTYPE and BFINAL */

		/*
		 * Align the bitstream to the next byte boundary.  This means
		 * the next byte boundary as if we were reading a byte at a
		 * time.  Therefore, we have to rewind 'in_next' by any bytes
		 * that have been refilled but not actually consumed yet (not
		 * counting overread bytes, which don't increment 'in_next').
		 */
		bitsleft = (u8)bitsleft;
		SAFETY_CHECK(overread_count <= (bitsleft >> 3));
		in_next -= (bitsleft >> 3) - overread_count;
		overread_count = 0;
		bitbuf = 0;
		bitsleft = 0;

		SAFETY_CHECK(in_end - in_next >= 4);
		len = get_unaligned_le16(in_next);
		nlen = get_unaligned_le16(in_next + 2);
		in_next += 4;

		SAFETY_CHECK(len == (u16)~nlen);
		if (unlikely(len > out_end - out_next))
			return LIBDEFLATE_INSUFFICIENT_SPACE;
		SAFETY_CHECK(len <= in_end - in_next);

		memcpy(out_next, in_next, len);
		in_next += len;
		out_next += len;

		goto block_done;

	} else {
		unsigned i;

		SAFETY_CHECK(block_type == DEFLATE_BLOCKTYPE_STATIC_HUFFMAN);

		/*
		 * Static Huffman block: build the decode tables for the static
		 * codes.  Skip doing so if the tables are already set up from
		 * an earlier static block; this speeds up decompression of
		 * degenerate input of many empty or very short static blocks.
		 *
		 * Afterwards, the remainder is the same as decompressing a
		 * dynamic Huffman block.
		 */

		bitbuf >>= 3; /* for BTYPE and BFINAL */
		bitsleft -= 3;

		if (d->static_codes_loaded)
			goto have_decode_tables;

		d->static_codes_loaded = true;

		STATIC_ASSERT(DEFLATE_NUM_LITLEN_SYMS == 288);
		STATIC_ASSERT(DEFLATE_NUM_OFFSET_SYMS == 32);

		for (i = 0; i < 144; i++)
			d->u.l.lens[i] = 8;
		for (; i < 256; i++)
			d->u.l.lens[i] = 9;
		for (; i < 280; i++)
			d->u.l.lens[i] = 7;
		for (; i < 288; i++)
			d->u.l.lens[i] = 8;

		for (; i < 288 + 32; i++)
			d->u.l.lens[i] = 5;

		num_litlen_syms = 288;
		num_offset_syms = 32;
	}

	/* Decompressing a Huffman block (either dynamic or static) */

	SAFETY_CHECK(build_offset_decode_table(d, num_litlen_syms, num_offset_syms));
	SAFETY_CHECK(build_litlen_decode_table(d, num_litlen_syms, num_offset_syms));
have_decode_tables:
	litlen_tablemask = BITMASK(d->litlen_tablebits);

	/*
	 * This is the "fastloop" for decoding literals and matches.  It does
	 * bounds checks on in_next and out_next in the loop conditions so that
	 * additional bounds checks aren't needed inside the loop body.
	 *
	 * To reduce latency, the bitbuffer is refilled and the next litlen
	 * decode table entry is preloaded before each loop iteration.
	 */
	if (in_next >= in_fastloop_end || out_next >= out_fastloop_end)
		goto generic_loop;
	REFILL_BITS_IN_FASTLOOP();
	entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
	do {
		u32 length, offset, lit;
		const u8 *src;
		u8 *dst;

		/*
		 * Consume the bits for the litlen decode table entry.  Save the
		 * original bitbuf for later, in case the extra match length
		 * bits need to be extracted from it.
		 */
		saved_bitbuf = bitbuf;
		bitbuf >>= (u8)entry;
		bitsleft -= entry; /* optimization: subtract full entry */

		/*
		 * Begin by checking for a "fast" literal, i.e. a literal that
		 * doesn't need a subtable.
		 */
		if (entry & HUFFDEC_LITERAL) {
			/*
			 * On 64-bit platforms, we decode up to 2 extra fast
			 * literals in addition to the primary item, as this
			 * increases performance and still leaves enough bits
			 * remaining for what follows.  We could actually do 3,
			 * assuming LITLEN_TABLEBITS=11, but that actually
			 * decreases performance slightly (perhaps by messing
			 * with the branch prediction of the conditional refill
			 * that happens later while decoding the match offset).
			 *
			 * Note: the definitions of FASTLOOP_MAX_BYTES_WRITTEN
			 * and FASTLOOP_MAX_BYTES_READ need to be updated if the
			 * number of extra literals decoded here is changed.
			 */
			if (/* enough bits for 2 fast literals + length + offset preload? */
			    CAN_CONSUME_AND_THEN_PRELOAD(2 * LITLEN_TABLEBITS +
							 LENGTH_MAXBITS,
							 OFFSET_TABLEBITS) &&
			    /* enough bits for 2 fast literals + slow literal + litlen preload? */
			    CAN_CONSUME_AND_THEN_PRELOAD(2 * LITLEN_TABLEBITS +
							 DEFLATE_MAX_LITLEN_CODEWORD_LEN,
							 LITLEN_TABLEBITS)) {
				/* 1st extra fast literal */
				lit = entry >> 16;
				entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
				saved_bitbuf = bitbuf;
				bitbuf >>= (u8)entry;
				bitsleft -= entry;
				*out_next++ = lit;
				if (entry & HUFFDEC_LITERAL) {
					/* 2nd extra fast literal */
					lit = entry >> 16;
					entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
					saved_bitbuf = bitbuf;
					bitbuf >>= (u8)entry;
					bitsleft -= entry;
					*out_next++ = lit;
					if (entry & HUFFDEC_LITERAL) {
						/*
						 * Another fast literal, but
						 * this one is in lieu of the
						 * primary item, so it doesn't
						 * count as one of the extras.
						 */
						lit = entry >> 16;
						entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
						REFILL_BITS_IN_FASTLOOP();
						*out_next++ = lit;
						continue;
					}
				}
			} else {
				/*
				 * Decode a literal.  While doing so, preload
				 * the next litlen decode table entry and refill
				 * the bitbuffer.  To reduce latency, we've
				 * arranged for there to be enough "preloadable"
				 * bits remaining to do the table preload
				 * independently of the refill.
				 */
				STATIC_ASSERT(CAN_CONSUME_AND_THEN_PRELOAD(
						LITLEN_TABLEBITS, LITLEN_TABLEBITS));
				lit = entry >> 16;
				entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
				REFILL_BITS_IN_FASTLOOP();
				*out_next++ = lit;
				continue;
			}
		}

		/*
		 * It's not a literal entry, so it can be a length entry, a
		 * subtable pointer entry, or an end-of-block entry.  Detect the
		 * two unlikely cases by testing the HUFFDEC_EXCEPTIONAL flag.
		 */
		if (unlikely(entry & HUFFDEC_EXCEPTIONAL)) {
			/* Subtable pointer or end-of-block entry */

			if (unlikely(entry & HUFFDEC_END_OF_BLOCK))
				goto block_done;

			/*
			 * A subtable is required.  Load and consume the
			 * subtable entry.  The subtable entry can be of any
			 * type: literal, length, or end-of-block.
			 */
			entry = d->u.litlen_decode_table[(entry >> 16) +
				EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
			saved_bitbuf = bitbuf;
			bitbuf >>= (u8)entry;
			bitsleft -= entry;

			/*
			 * 32-bit platforms that use the byte-at-a-time refill
			 * method have to do a refill here for there to always
			 * be enough bits to decode a literal that requires a
			 * subtable, then preload the next litlen decode table
			 * entry; or to decode a match length that requires a
			 * subtable, then preload the offset decode table entry.
			 */
			if (!CAN_CONSUME_AND_THEN_PRELOAD(DEFLATE_MAX_LITLEN_CODEWORD_LEN,
							  LITLEN_TABLEBITS) ||
			    !CAN_CONSUME_AND_THEN_PRELOAD(LENGTH_MAXBITS,
							  OFFSET_TABLEBITS))
				REFILL_BITS_IN_FASTLOOP();
			if (entry & HUFFDEC_LITERAL) {
				/* Decode a literal that required a subtable. */
				lit = entry >> 16;
				entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
				REFILL_BITS_IN_FASTLOOP();
				*out_next++ = lit;
				continue;
			}
			if (unlikely(entry & HUFFDEC_END_OF_BLOCK))
				goto block_done;
			/* Else, it's a length that required a subtable. */
		}

		/*
		 * Decode the match length: the length base value associated
		 * with the litlen symbol (which we extract from the decode
		 * table entry), plus the extra length bits.  We don't need to
		 * consume the extra length bits here, as they were included in
		 * the bits consumed by the entry earlier.  We also don't need
		 * to check for too-long matches here, as this is inside the
		 * fastloop where it's already been verified that the output
		 * buffer has enough space remaining to copy a max-length match.
		 */
		length = entry >> 16;
		length += EXTRACT_VARBITS8(saved_bitbuf, entry) >> (u8)(entry >> 8);

		/*
		 * Decode the match offset.  There are enough "preloadable" bits
		 * remaining to preload the offset decode table entry, but a
		 * refill might be needed before consuming it.
		 */
		STATIC_ASSERT(CAN_CONSUME_AND_THEN_PRELOAD(LENGTH_MAXFASTBITS,
							   OFFSET_TABLEBITS));
		entry = d->offset_decode_table[bitbuf & BITMASK(OFFSET_TABLEBITS)];
		if (CAN_CONSUME_AND_THEN_PRELOAD(OFFSET_MAXBITS,
						 LITLEN_TABLEBITS)) {
			/*
			 * Decoding a match offset on a 64-bit platform.  We may
			 * need to refill once, but then we can decode the whole
			 * offset and preload the next litlen table entry.
			 */
			if (unlikely(entry & HUFFDEC_EXCEPTIONAL)) {
				/* Offset codeword requires a subtable */
				if (unlikely((u8)bitsleft < OFFSET_MAXBITS +
					     LITLEN_TABLEBITS - PRELOAD_SLACK))
					REFILL_BITS_IN_FASTLOOP();
				bitbuf >>= OFFSET_TABLEBITS;
				bitsleft -= OFFSET_TABLEBITS;
				entry = d->offset_decode_table[(entry >> 16) +
					EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
			} else if (unlikely((u8)bitsleft < OFFSET_MAXFASTBITS +
					    LITLEN_TABLEBITS - PRELOAD_SLACK))
				REFILL_BITS_IN_FASTLOOP();
		} else {
			/* Decoding a match offset on a 32-bit platform */
			REFILL_BITS_IN_FASTLOOP();
			if (unlikely(entry & HUFFDEC_EXCEPTIONAL)) {
				/* Offset codeword requires a subtable */
				bitbuf >>= OFFSET_TABLEBITS;
				bitsleft -= OFFSET_TABLEBITS;
				entry = d->offset_decode_table[(entry >> 16) +
					EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
				REFILL_BITS_IN_FASTLOOP();
				/* No further refill needed before extra bits */
				STATIC_ASSERT(CAN_CONSUME(
					OFFSET_MAXBITS - OFFSET_TABLEBITS));
			} else {
				/* No refill needed before extra bits */
				STATIC_ASSERT(CAN_CONSUME(OFFSET_MAXFASTBITS));
			}
		}
		saved_bitbuf = bitbuf;
		bitbuf >>= (u8)entry;
		bitsleft -= entry; /* optimization: subtract full entry */
		offset = entry >> 16;
		offset += EXTRACT_VARBITS8(saved_bitbuf, entry) >> (u8)(entry >> 8);

		/* Validate the match offset; needed even in the fastloop. */
		SAFETY_CHECK(offset <= out_next - (const u8 *)out);
		src = out_next - offset;
		dst = out_next;
		out_next += length;

		/*
		 * Before starting to issue the instructions to copy the match,
		 * refill the bitbuffer and preload the litlen decode table
		 * entry for the next loop iteration.  This can increase
		 * performance by allowing the latency of the match copy to
		 * overlap with these other operations.  To further reduce
		 * latency, we've arranged for there to be enough bits remaining
		 * to do the table preload independently of the refill, except
		 * on 32-bit platforms using the byte-at-a-time refill method.
		 */
		if (!CAN_CONSUME_AND_THEN_PRELOAD(
			MAX(OFFSET_MAXBITS - OFFSET_TABLEBITS,
			    OFFSET_MAXFASTBITS),
			LITLEN_TABLEBITS) &&
		    unlikely((u8)bitsleft < LITLEN_TABLEBITS - PRELOAD_SLACK))
			REFILL_BITS_IN_FASTLOOP();
		entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
		REFILL_BITS_IN_FASTLOOP();

		/*
		 * Copy the match.  On most CPUs the fastest method is a
		 * word-at-a-time copy, unconditionally copying about 5 words
		 * since this is enough for most matches without being too much.
		 *
		 * The normal word-at-a-time copy works for offset >= WORDBYTES,
		 * which is most cases.  The case of offset == 1 is also common
		 * and is worth optimizing for, since it is just RLE encoding of
		 * the previous byte, which is the result of compressing long
		 * runs of the same byte.
		 *
		 * Writing past the match 'length' is allowed here, since it's
		 * been ensured there is enough output space left for a slight
		 * overrun.  FASTLOOP_MAX_BYTES_WRITTEN needs to be updated if
		 * the maximum possible overrun here is changed.
		 */
		if (UNALIGNED_ACCESS_IS_FAST && offset >= WORDBYTES) {
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			while (dst < out_next) {
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
			}
		} else if (UNALIGNED_ACCESS_IS_FAST && offset == 1) {
			machine_word_t v;

			/*
			 * This part tends to get auto-vectorized, so keep it
			 * copying a multiple of 16 bytes at a time.
			 */
			v = (machine_word_t)0x0101010101010101 * src[0];
			store_word_unaligned(v, dst);
			dst += WORDBYTES;
			store_word_unaligned(v, dst);
			dst += WORDBYTES;
			store_word_unaligned(v, dst);
			dst += WORDBYTES;
			store_word_unaligned(v, dst);
			dst += WORDBYTES;
			while (dst < out_next) {
				store_word_unaligned(v, dst);
				dst += WORDBYTES;
				store_word_unaligned(v, dst);
				dst += WORDBYTES;
				store_word_unaligned(v, dst);
				dst += WORDBYTES;
				store_word_unaligned(v, dst);
				dst += WORDBYTES;
			}
		} else if (UNALIGNED_ACCESS_IS_FAST) {
			store_word_unaligned(load_word_unaligned(src), dst);
			src += offset;
			dst += offset;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += offset;
			dst += offset;
			do {
				store_word_unaligned(load_word_unaligned(src), dst);
				src += offset;
				dst += offset;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += offset;
				dst += offset;
			} while (dst < out_next);
		} else {
			*dst++ = *src++;
			*dst++ = *src++;
			do {
				*dst++ = *src++;
			} while (dst < out_next);
		}
	} while (in_next < in_fastloop_end && out_next < out_fastloop_end);

	/*
	 * This is the generic loop for decoding literals and matches.  This
	 * handles cases where in_next and out_next are close to the end of
	 * their respective buffers.  Usually this loop isn't performance-
	 * critical, as most time is spent in the fastloop above instead.  We
	 * therefore omit some optimizations here in favor of smaller code.
	 */
generic_loop:
	for (;;) {
		u32 length, offset;
		const u8 *src;
		u8 *dst;

		REFILL_BITS();
		entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
		saved_bitbuf = bitbuf;
		bitbuf >>= (u8)entry;
		bitsleft -= entry;
		if (unlikely(entry & HUFFDEC_SUBTABLE_POINTER)) {
			entry = d->u.litlen_decode_table[(entry >> 16) +
					EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
			saved_bitbuf = bitbuf;
			bitbuf >>= (u8)entry;
			bitsleft -= entry;
		}
		length = entry >> 16;
		if (entry & HUFFDEC_LITERAL) {
			if (unlikely(out_next == out_end))
				return LIBDEFLATE_INSUFFICIENT_SPACE;
			*out_next++ = length;
			continue;
		}
		if (unlikely(entry & HUFFDEC_END_OF_BLOCK))
			goto block_done;
		length += EXTRACT_VARBITS8(saved_bitbuf, entry) >> (u8)(entry >> 8);
		if (unlikely(length > out_end - out_next))
			return LIBDEFLATE_INSUFFICIENT_SPACE;

		if (!CAN_CONSUME(LENGTH_MAXBITS + OFFSET_MAXBITS))
			REFILL_BITS();
		entry = d->offset_decode_table[bitbuf & BITMASK(OFFSET_TABLEBITS)];
		if (unlikely(entry & HUFFDEC_EXCEPTIONAL)) {
			bitbuf >>= OFFSET_TABLEBITS;
			bitsleft -= OFFSET_TABLEBITS;
			entry = d->offset_decode_table[(entry >> 16) +
					EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
			if (!CAN_CONSUME(OFFSET_MAXBITS))
				REFILL_BITS();
		}
		offset = entry >> 16;
		offset += EXTRACT_VARBITS8(bitbuf, entry) >> (u8)(entry >> 8);
		bitbuf >>= (u8)entry;
		bitsleft -= entry;

		SAFETY_CHECK(offset <= out_next - (const u8 *)out);
		src = out_next - offset;
		dst = out_next;
		out_next += length;

		STATIC_ASSERT(DEFLATE_MIN_MATCH_LEN == 3);
		*dst++ = *src++;
		*dst++ = *src++;
		do {
			*dst++ = *src++;
		} while (dst < out_next);
	}

block_done:
	/* Finished decoding a block */

	if (!is_final_block)
		goto next_block;

	/* That was the last block. */

	bitsleft = (u8)bitsleft;

	/*
	 * If any of the implicit appended zero bytes were consumed (not just
	 * refilled) before hitting end of stream, then the data is bad.
	 */
	SAFETY_CHECK(overread_count <= (bitsleft >> 3));

	/* Optionally return the actual number of bytes consumed. */
	if (actual_in_nbytes_ret) {
		/* Don't count bytes that were refilled but not consumed. */
		in_next -= (bitsleft >> 3) - overread_count;

		*actual_in_nbytes_ret = in_next - (u8 *)in;
	}

	/* Optionally return the actual number of bytes written. */
	if (actual_out_nbytes_ret) {
		*actual_out_nbytes_ret = out_next - (u8 *)out;
	} else {
		if (out_next != out_end)
			return LIBDEFLATE_SHORT_OUTPUT;
	}
	return LIBDEFLATE_SUCCESS;
}

#undef FUNCNAME
#undef ATTRIBUTES
#undef EXTRACT_VARBITS
#undef EXTRACT_VARBITS8


/* Include architecture-specific implementation(s) if available. */
#undef DEFAULT_IMPL
#undef arch_select_decompress_func
#if defined(ARCH_X86_32) || defined(ARCH_X86_64)
#ifndef LIB_X86_DECOMPRESS_IMPL_H
#define LIB_X86_DECOMPRESS_IMPL_H

/*
 * BMI2 optimized version
 *
 * FIXME: with MSVC, this isn't actually compiled with BMI2 code generation
 * enabled yet.  That would require that this be moved to its own .c file.
 */
#if HAVE_BMI2_INTRIN
#  define deflate_decompress_bmi2	deflate_decompress_bmi2
#  define FUNCNAME			deflate_decompress_bmi2
#  if !HAVE_BMI2_NATIVE
#    define ATTRIBUTES			_target_attribute("bmi2")
#  endif
   /*
    * Even with __attribute__((target("bmi2"))), gcc doesn't reliably use the
    * bzhi instruction for 'word & BITMASK(count)'.  So use the bzhi intrinsic
    * explicitly.  EXTRACT_VARBITS() is equivalent to 'word & BITMASK(count)';
    * EXTRACT_VARBITS8() is equivalent to 'word & BITMASK((u8)count)'.
    * Nevertheless, their implementation using the bzhi intrinsic is identical,
    * as the bzhi instruction truncates the count to 8 bits implicitly.
    */
#  ifndef __clang__
#    include <immintrin.h>
#    ifdef ARCH_X86_64
#      define EXTRACT_VARBITS(word, count)  _bzhi_u64((word), (count))
#      define EXTRACT_VARBITS8(word, count) _bzhi_u64((word), (count))
#    else
#      define EXTRACT_VARBITS(word, count)  _bzhi_u32((word), (count))
#      define EXTRACT_VARBITS8(word, count) _bzhi_u32((word), (count))
#    endif
#  endif
/*
 * decompress_template.h
 *
 * Copyright 2016 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This is the actual DEFLATE decompression routine, lifted out of
 * deflate_decompress.c so that it can be compiled multiple times with different
 * target instruction sets.
 */

#ifndef ATTRIBUTES
#  define ATTRIBUTES
#endif
#ifndef EXTRACT_VARBITS
#  define EXTRACT_VARBITS(word, count)	((word) & BITMASK(count))
#endif
#ifndef EXTRACT_VARBITS8
#  define EXTRACT_VARBITS8(word, count)	((word) & BITMASK((u8)(count)))
#endif

static enum libdeflate_result ATTRIBUTES MAYBE_UNUSED
FUNCNAME(struct libdeflate_decompressor * restrict d,
	 const void * restrict in, size_t in_nbytes,
	 void * restrict out, size_t out_nbytes_avail,
	 size_t *actual_in_nbytes_ret, size_t *actual_out_nbytes_ret)
{
	u8 *out_next = out;
	u8 * const out_end = out_next + out_nbytes_avail;
	u8 * const out_fastloop_end =
		out_end - MIN(out_nbytes_avail, FASTLOOP_MAX_BYTES_WRITTEN);

	/* Input bitstream state; see deflate_decompress.c for documentation */
	const u8 *in_next = in;
	const u8 * const in_end = in_next + in_nbytes;
	const u8 * const in_fastloop_end =
		in_end - MIN(in_nbytes, FASTLOOP_MAX_BYTES_READ);
	bitbuf_t bitbuf = 0;
	bitbuf_t saved_bitbuf;
	u32 bitsleft = 0;
	size_t overread_count = 0;

	bool is_final_block;
	unsigned block_type;
	unsigned num_litlen_syms;
	unsigned num_offset_syms;
	bitbuf_t litlen_tablemask;
	u32 entry;

next_block:
	/* Starting to read the next block */
	;

	STATIC_ASSERT(CAN_CONSUME(1 + 2 + 5 + 5 + 4 + 3));
	REFILL_BITS();

	/* BFINAL: 1 bit */
	is_final_block = bitbuf & BITMASK(1);

	/* BTYPE: 2 bits */
	block_type = (bitbuf >> 1) & BITMASK(2);

	if (block_type == DEFLATE_BLOCKTYPE_DYNAMIC_HUFFMAN) {

		/* Dynamic Huffman block */

		/* The order in which precode lengths are stored */
		static const u8 deflate_precode_lens_permutation[DEFLATE_NUM_PRECODE_SYMS] = {
			16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
		};

		unsigned num_explicit_precode_lens;
		unsigned i;

		/* Read the codeword length counts. */

		STATIC_ASSERT(DEFLATE_NUM_LITLEN_SYMS == 257 + BITMASK(5));
		num_litlen_syms = 257 + ((bitbuf >> 3) & BITMASK(5));

		STATIC_ASSERT(DEFLATE_NUM_OFFSET_SYMS == 1 + BITMASK(5));
		num_offset_syms = 1 + ((bitbuf >> 8) & BITMASK(5));

		STATIC_ASSERT(DEFLATE_NUM_PRECODE_SYMS == 4 + BITMASK(4));
		num_explicit_precode_lens = 4 + ((bitbuf >> 13) & BITMASK(4));

		d->static_codes_loaded = false;

		/*
		 * Read the precode codeword lengths.
		 *
		 * A 64-bit bitbuffer is just one bit too small to hold the
		 * maximum number of precode lens, so to minimize branches we
		 * merge one len with the previous fields.
		 */
		STATIC_ASSERT(DEFLATE_MAX_PRE_CODEWORD_LEN == (1 << 3) - 1);
		if (CAN_CONSUME(3 * (DEFLATE_NUM_PRECODE_SYMS - 1))) {
			d->u.precode_lens[deflate_precode_lens_permutation[0]] =
				(bitbuf >> 17) & BITMASK(3);
			bitbuf >>= 20;
			bitsleft -= 20;
			REFILL_BITS();
			i = 1;
			do {
				d->u.precode_lens[deflate_precode_lens_permutation[i]] =
					bitbuf & BITMASK(3);
				bitbuf >>= 3;
				bitsleft -= 3;
			} while (++i < num_explicit_precode_lens);
		} else {
			bitbuf >>= 17;
			bitsleft -= 17;
			i = 0;
			do {
				if ((u8)bitsleft < 3)
					REFILL_BITS();
				d->u.precode_lens[deflate_precode_lens_permutation[i]] =
					bitbuf & BITMASK(3);
				bitbuf >>= 3;
				bitsleft -= 3;
			} while (++i < num_explicit_precode_lens);
		}
		for (; i < DEFLATE_NUM_PRECODE_SYMS; i++)
			d->u.precode_lens[deflate_precode_lens_permutation[i]] = 0;

		/* Build the decode table for the precode. */
		SAFETY_CHECK(build_precode_decode_table(d));

		/* Decode the litlen and offset codeword lengths. */
		i = 0;
		do {
			unsigned presym;
			u8 rep_val;
			unsigned rep_count;

			if ((u8)bitsleft < DEFLATE_MAX_PRE_CODEWORD_LEN + 7)
				REFILL_BITS();

			/*
			 * The code below assumes that the precode decode table
			 * doesn't have any subtables.
			 */
			STATIC_ASSERT(PRECODE_TABLEBITS == DEFLATE_MAX_PRE_CODEWORD_LEN);

			/* Decode the next precode symbol. */
			entry = d->u.l.precode_decode_table[
				bitbuf & BITMASK(DEFLATE_MAX_PRE_CODEWORD_LEN)];
			bitbuf >>= (u8)entry;
			bitsleft -= entry; /* optimization: subtract full entry */
			presym = entry >> 16;

			if (presym < 16) {
				/* Explicit codeword length */
				d->u.l.lens[i++] = presym;
				continue;
			}

			/* Run-length encoded codeword lengths */

			/*
			 * Note: we don't need to immediately verify that the
			 * repeat count doesn't overflow the number of elements,
			 * since we've sized the lens array to have enough extra
			 * space to allow for the worst-case overrun (138 zeroes
			 * when only 1 length was remaining).
			 *
			 * In the case of the small repeat counts (presyms 16
			 * and 17), it is fastest to always write the maximum
			 * number of entries.  That gets rid of branches that
			 * would otherwise be required.
			 *
			 * It is not just because of the numerical order that
			 * our checks go in the order 'presym < 16', 'presym ==
			 * 16', and 'presym == 17'.  For typical data this is
			 * ordered from most frequent to least frequent case.
			 */
			STATIC_ASSERT(DEFLATE_MAX_LENS_OVERRUN == 138 - 1);

			if (presym == 16) {
				/* Repeat the previous length 3 - 6 times. */
				SAFETY_CHECK(i != 0);
				rep_val = d->u.l.lens[i - 1];
				STATIC_ASSERT(3 + BITMASK(2) == 6);
				rep_count = 3 + (bitbuf & BITMASK(2));
				bitbuf >>= 2;
				bitsleft -= 2;
				d->u.l.lens[i + 0] = rep_val;
				d->u.l.lens[i + 1] = rep_val;
				d->u.l.lens[i + 2] = rep_val;
				d->u.l.lens[i + 3] = rep_val;
				d->u.l.lens[i + 4] = rep_val;
				d->u.l.lens[i + 5] = rep_val;
				i += rep_count;
			} else if (presym == 17) {
				/* Repeat zero 3 - 10 times. */
				STATIC_ASSERT(3 + BITMASK(3) == 10);
				rep_count = 3 + (bitbuf & BITMASK(3));
				bitbuf >>= 3;
				bitsleft -= 3;
				d->u.l.lens[i + 0] = 0;
				d->u.l.lens[i + 1] = 0;
				d->u.l.lens[i + 2] = 0;
				d->u.l.lens[i + 3] = 0;
				d->u.l.lens[i + 4] = 0;
				d->u.l.lens[i + 5] = 0;
				d->u.l.lens[i + 6] = 0;
				d->u.l.lens[i + 7] = 0;
				d->u.l.lens[i + 8] = 0;
				d->u.l.lens[i + 9] = 0;
				i += rep_count;
			} else {
				/* Repeat zero 11 - 138 times. */
				STATIC_ASSERT(11 + BITMASK(7) == 138);
				rep_count = 11 + (bitbuf & BITMASK(7));
				bitbuf >>= 7;
				bitsleft -= 7;
				memset(&d->u.l.lens[i], 0,
				       rep_count * sizeof(d->u.l.lens[i]));
				i += rep_count;
			}
		} while (i < num_litlen_syms + num_offset_syms);

		/* Unnecessary, but check this for consistency with zlib. */
		SAFETY_CHECK(i == num_litlen_syms + num_offset_syms);

	} else if (block_type == DEFLATE_BLOCKTYPE_UNCOMPRESSED) {
		u16 len, nlen;

		/*
		 * Uncompressed block: copy 'len' bytes literally from the input
		 * buffer to the output buffer.
		 */

		bitsleft -= 3; /* for BTYPE and BFINAL */

		/*
		 * Align the bitstream to the next byte boundary.  This means
		 * the next byte boundary as if we were reading a byte at a
		 * time.  Therefore, we have to rewind 'in_next' by any bytes
		 * that have been refilled but not actually consumed yet (not
		 * counting overread bytes, which don't increment 'in_next').
		 */
		bitsleft = (u8)bitsleft;
		SAFETY_CHECK(overread_count <= (bitsleft >> 3));
		in_next -= (bitsleft >> 3) - overread_count;
		overread_count = 0;
		bitbuf = 0;
		bitsleft = 0;

		SAFETY_CHECK(in_end - in_next >= 4);
		len = get_unaligned_le16(in_next);
		nlen = get_unaligned_le16(in_next + 2);
		in_next += 4;

		SAFETY_CHECK(len == (u16)~nlen);
		if (unlikely(len > out_end - out_next))
			return LIBDEFLATE_INSUFFICIENT_SPACE;
		SAFETY_CHECK(len <= in_end - in_next);

		memcpy(out_next, in_next, len);
		in_next += len;
		out_next += len;

		goto block_done;

	} else {
		unsigned i;

		SAFETY_CHECK(block_type == DEFLATE_BLOCKTYPE_STATIC_HUFFMAN);

		/*
		 * Static Huffman block: build the decode tables for the static
		 * codes.  Skip doing so if the tables are already set up from
		 * an earlier static block; this speeds up decompression of
		 * degenerate input of many empty or very short static blocks.
		 *
		 * Afterwards, the remainder is the same as decompressing a
		 * dynamic Huffman block.
		 */

		bitbuf >>= 3; /* for BTYPE and BFINAL */
		bitsleft -= 3;

		if (d->static_codes_loaded)
			goto have_decode_tables;

		d->static_codes_loaded = true;

		STATIC_ASSERT(DEFLATE_NUM_LITLEN_SYMS == 288);
		STATIC_ASSERT(DEFLATE_NUM_OFFSET_SYMS == 32);

		for (i = 0; i < 144; i++)
			d->u.l.lens[i] = 8;
		for (; i < 256; i++)
			d->u.l.lens[i] = 9;
		for (; i < 280; i++)
			d->u.l.lens[i] = 7;
		for (; i < 288; i++)
			d->u.l.lens[i] = 8;

		for (; i < 288 + 32; i++)
			d->u.l.lens[i] = 5;

		num_litlen_syms = 288;
		num_offset_syms = 32;
	}

	/* Decompressing a Huffman block (either dynamic or static) */

	SAFETY_CHECK(build_offset_decode_table(d, num_litlen_syms, num_offset_syms));
	SAFETY_CHECK(build_litlen_decode_table(d, num_litlen_syms, num_offset_syms));
have_decode_tables:
	litlen_tablemask = BITMASK(d->litlen_tablebits);

	/*
	 * This is the "fastloop" for decoding literals and matches.  It does
	 * bounds checks on in_next and out_next in the loop conditions so that
	 * additional bounds checks aren't needed inside the loop body.
	 *
	 * To reduce latency, the bitbuffer is refilled and the next litlen
	 * decode table entry is preloaded before each loop iteration.
	 */
	if (in_next >= in_fastloop_end || out_next >= out_fastloop_end)
		goto generic_loop;
	REFILL_BITS_IN_FASTLOOP();
	entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
	do {
		u32 length, offset, lit;
		const u8 *src;
		u8 *dst;

		/*
		 * Consume the bits for the litlen decode table entry.  Save the
		 * original bitbuf for later, in case the extra match length
		 * bits need to be extracted from it.
		 */
		saved_bitbuf = bitbuf;
		bitbuf >>= (u8)entry;
		bitsleft -= entry; /* optimization: subtract full entry */

		/*
		 * Begin by checking for a "fast" literal, i.e. a literal that
		 * doesn't need a subtable.
		 */
		if (entry & HUFFDEC_LITERAL) {
			/*
			 * On 64-bit platforms, we decode up to 2 extra fast
			 * literals in addition to the primary item, as this
			 * increases performance and still leaves enough bits
			 * remaining for what follows.  We could actually do 3,
			 * assuming LITLEN_TABLEBITS=11, but that actually
			 * decreases performance slightly (perhaps by messing
			 * with the branch prediction of the conditional refill
			 * that happens later while decoding the match offset).
			 *
			 * Note: the definitions of FASTLOOP_MAX_BYTES_WRITTEN
			 * and FASTLOOP_MAX_BYTES_READ need to be updated if the
			 * number of extra literals decoded here is changed.
			 */
			if (/* enough bits for 2 fast literals + length + offset preload? */
			    CAN_CONSUME_AND_THEN_PRELOAD(2 * LITLEN_TABLEBITS +
							 LENGTH_MAXBITS,
							 OFFSET_TABLEBITS) &&
			    /* enough bits for 2 fast literals + slow literal + litlen preload? */
			    CAN_CONSUME_AND_THEN_PRELOAD(2 * LITLEN_TABLEBITS +
							 DEFLATE_MAX_LITLEN_CODEWORD_LEN,
							 LITLEN_TABLEBITS)) {
				/* 1st extra fast literal */
				lit = entry >> 16;
				entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
				saved_bitbuf = bitbuf;
				bitbuf >>= (u8)entry;
				bitsleft -= entry;
				*out_next++ = lit;
				if (entry & HUFFDEC_LITERAL) {
					/* 2nd extra fast literal */
					lit = entry >> 16;
					entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
					saved_bitbuf = bitbuf;
					bitbuf >>= (u8)entry;
					bitsleft -= entry;
					*out_next++ = lit;
					if (entry & HUFFDEC_LITERAL) {
						/*
						 * Another fast literal, but
						 * this one is in lieu of the
						 * primary item, so it doesn't
						 * count as one of the extras.
						 */
						lit = entry >> 16;
						entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
						REFILL_BITS_IN_FASTLOOP();
						*out_next++ = lit;
						continue;
					}
				}
			} else {
				/*
				 * Decode a literal.  While doing so, preload
				 * the next litlen decode table entry and refill
				 * the bitbuffer.  To reduce latency, we've
				 * arranged for there to be enough "preloadable"
				 * bits remaining to do the table preload
				 * independently of the refill.
				 */
				STATIC_ASSERT(CAN_CONSUME_AND_THEN_PRELOAD(
						LITLEN_TABLEBITS, LITLEN_TABLEBITS));
				lit = entry >> 16;
				entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
				REFILL_BITS_IN_FASTLOOP();
				*out_next++ = lit;
				continue;
			}
		}

		/*
		 * It's not a literal entry, so it can be a length entry, a
		 * subtable pointer entry, or an end-of-block entry.  Detect the
		 * two unlikely cases by testing the HUFFDEC_EXCEPTIONAL flag.
		 */
		if (unlikely(entry & HUFFDEC_EXCEPTIONAL)) {
			/* Subtable pointer or end-of-block entry */

			if (unlikely(entry & HUFFDEC_END_OF_BLOCK))
				goto block_done;

			/*
			 * A subtable is required.  Load and consume the
			 * subtable entry.  The subtable entry can be of any
			 * type: literal, length, or end-of-block.
			 */
			entry = d->u.litlen_decode_table[(entry >> 16) +
				EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
			saved_bitbuf = bitbuf;
			bitbuf >>= (u8)entry;
			bitsleft -= entry;

			/*
			 * 32-bit platforms that use the byte-at-a-time refill
			 * method have to do a refill here for there to always
			 * be enough bits to decode a literal that requires a
			 * subtable, then preload the next litlen decode table
			 * entry; or to decode a match length that requires a
			 * subtable, then preload the offset decode table entry.
			 */
			if (!CAN_CONSUME_AND_THEN_PRELOAD(DEFLATE_MAX_LITLEN_CODEWORD_LEN,
							  LITLEN_TABLEBITS) ||
			    !CAN_CONSUME_AND_THEN_PRELOAD(LENGTH_MAXBITS,
							  OFFSET_TABLEBITS))
				REFILL_BITS_IN_FASTLOOP();
			if (entry & HUFFDEC_LITERAL) {
				/* Decode a literal that required a subtable. */
				lit = entry >> 16;
				entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
				REFILL_BITS_IN_FASTLOOP();
				*out_next++ = lit;
				continue;
			}
			if (unlikely(entry & HUFFDEC_END_OF_BLOCK))
				goto block_done;
			/* Else, it's a length that required a subtable. */
		}

		/*
		 * Decode the match length: the length base value associated
		 * with the litlen symbol (which we extract from the decode
		 * table entry), plus the extra length bits.  We don't need to
		 * consume the extra length bits here, as they were included in
		 * the bits consumed by the entry earlier.  We also don't need
		 * to check for too-long matches here, as this is inside the
		 * fastloop where it's already been verified that the output
		 * buffer has enough space remaining to copy a max-length match.
		 */
		length = entry >> 16;
		length += EXTRACT_VARBITS8(saved_bitbuf, entry) >> (u8)(entry >> 8);

		/*
		 * Decode the match offset.  There are enough "preloadable" bits
		 * remaining to preload the offset decode table entry, but a
		 * refill might be needed before consuming it.
		 */
		STATIC_ASSERT(CAN_CONSUME_AND_THEN_PRELOAD(LENGTH_MAXFASTBITS,
							   OFFSET_TABLEBITS));
		entry = d->offset_decode_table[bitbuf & BITMASK(OFFSET_TABLEBITS)];
		if (CAN_CONSUME_AND_THEN_PRELOAD(OFFSET_MAXBITS,
						 LITLEN_TABLEBITS)) {
			/*
			 * Decoding a match offset on a 64-bit platform.  We may
			 * need to refill once, but then we can decode the whole
			 * offset and preload the next litlen table entry.
			 */
			if (unlikely(entry & HUFFDEC_EXCEPTIONAL)) {
				/* Offset codeword requires a subtable */
				if (unlikely((u8)bitsleft < OFFSET_MAXBITS +
					     LITLEN_TABLEBITS - PRELOAD_SLACK))
					REFILL_BITS_IN_FASTLOOP();
				bitbuf >>= OFFSET_TABLEBITS;
				bitsleft -= OFFSET_TABLEBITS;
				entry = d->offset_decode_table[(entry >> 16) +
					EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
			} else if (unlikely((u8)bitsleft < OFFSET_MAXFASTBITS +
					    LITLEN_TABLEBITS - PRELOAD_SLACK))
				REFILL_BITS_IN_FASTLOOP();
		} else {
			/* Decoding a match offset on a 32-bit platform */
			REFILL_BITS_IN_FASTLOOP();
			if (unlikely(entry & HUFFDEC_EXCEPTIONAL)) {
				/* Offset codeword requires a subtable */
				bitbuf >>= OFFSET_TABLEBITS;
				bitsleft -= OFFSET_TABLEBITS;
				entry = d->offset_decode_table[(entry >> 16) +
					EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
				REFILL_BITS_IN_FASTLOOP();
				/* No further refill needed before extra bits */
				STATIC_ASSERT(CAN_CONSUME(
					OFFSET_MAXBITS - OFFSET_TABLEBITS));
			} else {
				/* No refill needed before extra bits */
				STATIC_ASSERT(CAN_CONSUME(OFFSET_MAXFASTBITS));
			}
		}
		saved_bitbuf = bitbuf;
		bitbuf >>= (u8)entry;
		bitsleft -= entry; /* optimization: subtract full entry */
		offset = entry >> 16;
		offset += EXTRACT_VARBITS8(saved_bitbuf, entry) >> (u8)(entry >> 8);

		/* Validate the match offset; needed even in the fastloop. */
		SAFETY_CHECK(offset <= out_next - (const u8 *)out);
		src = out_next - offset;
		dst = out_next;
		out_next += length;

		/*
		 * Before starting to issue the instructions to copy the match,
		 * refill the bitbuffer and preload the litlen decode table
		 * entry for the next loop iteration.  This can increase
		 * performance by allowing the latency of the match copy to
		 * overlap with these other operations.  To further reduce
		 * latency, we've arranged for there to be enough bits remaining
		 * to do the table preload independently of the refill, except
		 * on 32-bit platforms using the byte-at-a-time refill method.
		 */
		if (!CAN_CONSUME_AND_THEN_PRELOAD(
			MAX(OFFSET_MAXBITS - OFFSET_TABLEBITS,
			    OFFSET_MAXFASTBITS),
			LITLEN_TABLEBITS) &&
		    unlikely((u8)bitsleft < LITLEN_TABLEBITS - PRELOAD_SLACK))
			REFILL_BITS_IN_FASTLOOP();
		entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
		REFILL_BITS_IN_FASTLOOP();

		/*
		 * Copy the match.  On most CPUs the fastest method is a
		 * word-at-a-time copy, unconditionally copying about 5 words
		 * since this is enough for most matches without being too much.
		 *
		 * The normal word-at-a-time copy works for offset >= WORDBYTES,
		 * which is most cases.  The case of offset == 1 is also common
		 * and is worth optimizing for, since it is just RLE encoding of
		 * the previous byte, which is the result of compressing long
		 * runs of the same byte.
		 *
		 * Writing past the match 'length' is allowed here, since it's
		 * been ensured there is enough output space left for a slight
		 * overrun.  FASTLOOP_MAX_BYTES_WRITTEN needs to be updated if
		 * the maximum possible overrun here is changed.
		 */
		if (UNALIGNED_ACCESS_IS_FAST && offset >= WORDBYTES) {
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += WORDBYTES;
			dst += WORDBYTES;
			while (dst < out_next) {
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += WORDBYTES;
				dst += WORDBYTES;
			}
		} else if (UNALIGNED_ACCESS_IS_FAST && offset == 1) {
			machine_word_t v;

			/*
			 * This part tends to get auto-vectorized, so keep it
			 * copying a multiple of 16 bytes at a time.
			 */
			v = (machine_word_t)0x0101010101010101 * src[0];
			store_word_unaligned(v, dst);
			dst += WORDBYTES;
			store_word_unaligned(v, dst);
			dst += WORDBYTES;
			store_word_unaligned(v, dst);
			dst += WORDBYTES;
			store_word_unaligned(v, dst);
			dst += WORDBYTES;
			while (dst < out_next) {
				store_word_unaligned(v, dst);
				dst += WORDBYTES;
				store_word_unaligned(v, dst);
				dst += WORDBYTES;
				store_word_unaligned(v, dst);
				dst += WORDBYTES;
				store_word_unaligned(v, dst);
				dst += WORDBYTES;
			}
		} else if (UNALIGNED_ACCESS_IS_FAST) {
			store_word_unaligned(load_word_unaligned(src), dst);
			src += offset;
			dst += offset;
			store_word_unaligned(load_word_unaligned(src), dst);
			src += offset;
			dst += offset;
			do {
				store_word_unaligned(load_word_unaligned(src), dst);
				src += offset;
				dst += offset;
				store_word_unaligned(load_word_unaligned(src), dst);
				src += offset;
				dst += offset;
			} while (dst < out_next);
		} else {
			*dst++ = *src++;
			*dst++ = *src++;
			do {
				*dst++ = *src++;
			} while (dst < out_next);
		}
	} while (in_next < in_fastloop_end && out_next < out_fastloop_end);

	/*
	 * This is the generic loop for decoding literals and matches.  This
	 * handles cases where in_next and out_next are close to the end of
	 * their respective buffers.  Usually this loop isn't performance-
	 * critical, as most time is spent in the fastloop above instead.  We
	 * therefore omit some optimizations here in favor of smaller code.
	 */
generic_loop:
	for (;;) {
		u32 length, offset;
		const u8 *src;
		u8 *dst;

		REFILL_BITS();
		entry = d->u.litlen_decode_table[bitbuf & litlen_tablemask];
		saved_bitbuf = bitbuf;
		bitbuf >>= (u8)entry;
		bitsleft -= entry;
		if (unlikely(entry & HUFFDEC_SUBTABLE_POINTER)) {
			entry = d->u.litlen_decode_table[(entry >> 16) +
					EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
			saved_bitbuf = bitbuf;
			bitbuf >>= (u8)entry;
			bitsleft -= entry;
		}
		length = entry >> 16;
		if (entry & HUFFDEC_LITERAL) {
			if (unlikely(out_next == out_end))
				return LIBDEFLATE_INSUFFICIENT_SPACE;
			*out_next++ = length;
			continue;
		}
		if (unlikely(entry & HUFFDEC_END_OF_BLOCK))
			goto block_done;
		length += EXTRACT_VARBITS8(saved_bitbuf, entry) >> (u8)(entry >> 8);
		if (unlikely(length > out_end - out_next))
			return LIBDEFLATE_INSUFFICIENT_SPACE;

		if (!CAN_CONSUME(LENGTH_MAXBITS + OFFSET_MAXBITS))
			REFILL_BITS();
		entry = d->offset_decode_table[bitbuf & BITMASK(OFFSET_TABLEBITS)];
		if (unlikely(entry & HUFFDEC_EXCEPTIONAL)) {
			bitbuf >>= OFFSET_TABLEBITS;
			bitsleft -= OFFSET_TABLEBITS;
			entry = d->offset_decode_table[(entry >> 16) +
					EXTRACT_VARBITS(bitbuf, (entry >> 8) & 0x3F)];
			if (!CAN_CONSUME(OFFSET_MAXBITS))
				REFILL_BITS();
		}
		offset = entry >> 16;
		offset += EXTRACT_VARBITS8(bitbuf, entry) >> (u8)(entry >> 8);
		bitbuf >>= (u8)entry;
		bitsleft -= entry;

		SAFETY_CHECK(offset <= out_next - (const u8 *)out);
		src = out_next - offset;
		dst = out_next;
		out_next += length;

		STATIC_ASSERT(DEFLATE_MIN_MATCH_LEN == 3);
		*dst++ = *src++;
		*dst++ = *src++;
		do {
			*dst++ = *src++;
		} while (dst < out_next);
	}

block_done:
	/* Finished decoding a block */

	if (!is_final_block)
		goto next_block;

	/* That was the last block. */

	bitsleft = (u8)bitsleft;

	/*
	 * If any of the implicit appended zero bytes were consumed (not just
	 * refilled) before hitting end of stream, then the data is bad.
	 */
	SAFETY_CHECK(overread_count <= (bitsleft >> 3));

	/* Optionally return the actual number of bytes consumed. */
	if (actual_in_nbytes_ret) {
		/* Don't count bytes that were refilled but not consumed. */
		in_next -= (bitsleft >> 3) - overread_count;

		*actual_in_nbytes_ret = in_next - (u8 *)in;
	}

	/* Optionally return the actual number of bytes written. */
	if (actual_out_nbytes_ret) {
		*actual_out_nbytes_ret = out_next - (u8 *)out;
	} else {
		if (out_next != out_end)
			return LIBDEFLATE_SHORT_OUTPUT;
	}
	return LIBDEFLATE_SUCCESS;
}

#undef FUNCNAME
#undef ATTRIBUTES
#undef EXTRACT_VARBITS
#undef EXTRACT_VARBITS8

#endif /* HAVE_BMI2_INTRIN */

#if defined(deflate_decompress_bmi2) && HAVE_BMI2_NATIVE
#define DEFAULT_IMPL	deflate_decompress_bmi2
#else
static inline decompress_func_t
arch_select_decompress_func(void)
{
#ifdef deflate_decompress_bmi2
	if (HAVE_BMI2(get_x86_cpu_features()))
		return deflate_decompress_bmi2;
#endif
	return NULL;
}
#define arch_select_decompress_func	arch_select_decompress_func
#endif

#endif /* LIB_X86_DECOMPRESS_IMPL_H */

#endif

#ifndef DEFAULT_IMPL
#  define DEFAULT_IMPL deflate_decompress_default
#endif

#ifdef arch_select_decompress_func
static enum libdeflate_result
dispatch_decomp(struct libdeflate_decompressor *d,
		const void *in, size_t in_nbytes,
		void *out, size_t out_nbytes_avail,
		size_t *actual_in_nbytes_ret, size_t *actual_out_nbytes_ret);

static volatile decompress_func_t decompress_impl = dispatch_decomp;

/* Choose the best implementation at runtime. */
static enum libdeflate_result
dispatch_decomp(struct libdeflate_decompressor *d,
		const void *in, size_t in_nbytes,
		void *out, size_t out_nbytes_avail,
		size_t *actual_in_nbytes_ret, size_t *actual_out_nbytes_ret)
{
	decompress_func_t f = arch_select_decompress_func();

	if (f == NULL)
		f = DEFAULT_IMPL;

	decompress_impl = f;
	return f(d, in, in_nbytes, out, out_nbytes_avail,
		 actual_in_nbytes_ret, actual_out_nbytes_ret);
}
#else
/* The best implementation is statically known, so call it directly. */
#  define decompress_impl DEFAULT_IMPL
#endif

/*
 * This is the main DEFLATE decompression routine.  See libdeflate.h for the
 * documentation.
 *
 * Note that the real code is in decompress_template.h.  The part here just
 * handles calling the appropriate implementation depending on the CPU features
 * at runtime.
 */
LIBDEFLATEAPI enum libdeflate_result
libdeflate_deflate_decompress_ex(struct libdeflate_decompressor *d,
				 const void *in, size_t in_nbytes,
				 void *out, size_t out_nbytes_avail,
				 size_t *actual_in_nbytes_ret,
				 size_t *actual_out_nbytes_ret)
{
	return decompress_impl(d, in, in_nbytes, out, out_nbytes_avail,
			       actual_in_nbytes_ret, actual_out_nbytes_ret);
}

LIBDEFLATEAPI enum libdeflate_result
libdeflate_deflate_decompress(struct libdeflate_decompressor *d,
			      const void *in, size_t in_nbytes,
			      void *out, size_t out_nbytes_avail,
			      size_t *actual_out_nbytes_ret)
{
	return libdeflate_deflate_decompress_ex(d, in, in_nbytes,
						out, out_nbytes_avail,
						NULL, actual_out_nbytes_ret);
}

LIBDEFLATEAPI struct libdeflate_decompressor *
libdeflate_alloc_decompressor_ex(const struct libdeflate_options *options)
{
	struct libdeflate_decompressor *d;

	/*
	 * Note: if more fields are added to libdeflate_options, this code will
	 * need to be updated to support both the old and new structs.
	 */
	if (options->sizeof_options != sizeof(*options))
		return NULL;

	d = (options->malloc_func ? options->malloc_func :
	     libdeflate_default_malloc_func)(sizeof(*d));
	if (d == NULL)
		return NULL;
	/*
	 * Note that only certain parts of the decompressor actually must be
	 * initialized here:
	 *
	 * - 'static_codes_loaded' must be initialized to false.
	 *
	 * - The first half of the main portion of each decode table must be
	 *   initialized to any value, to avoid reading from uninitialized
	 *   memory during table expansion in build_decode_table().  (Although,
	 *   this is really just to avoid warnings with dynamic tools like
	 *   valgrind, since build_decode_table() is guaranteed to initialize
	 *   all entries eventually anyway.)
	 *
	 * - 'free_func' must be set.
	 *
	 * But for simplicity, we currently just zero the whole decompressor.
	 */
	memset(d, 0, sizeof(*d));
	d->free_func = options->free_func ?
		       options->free_func : libdeflate_default_free_func;
	return d;
}

LIBDEFLATEAPI struct libdeflate_decompressor *
libdeflate_alloc_decompressor(void)
{
	static const struct libdeflate_options defaults = {
		.sizeof_options = sizeof(defaults),
	};
	return libdeflate_alloc_decompressor_ex(&defaults);
}

LIBDEFLATEAPI void
libdeflate_free_decompressor(struct libdeflate_decompressor *d)
{
	if (d)
		d->free_func(d);
}


/*
 * utils.c - utility functions for libdeflate
 *
 * Copyright 2016 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef FREESTANDING
#  define malloc NULL
#  define free NULL
#else
#  include <stdlib.h>
#endif

malloc_func_t libdeflate_default_malloc_func = malloc;
free_func_t libdeflate_default_free_func = free;

void *
libdeflate_aligned_malloc(malloc_func_t malloc_func,
			  size_t alignment, size_t size)
{
	void *ptr = (*malloc_func)(sizeof(void *) + alignment - 1 + size);

	if (ptr) {
		void *orig_ptr = ptr;

		ptr = (void *)ALIGN((uintptr_t)ptr + sizeof(void *), alignment);
		((void **)ptr)[-1] = orig_ptr;
	}
	return ptr;
}

void
libdeflate_aligned_free(free_func_t free_func, void *ptr)
{
	(*free_func)(((void **)ptr)[-1]);
}

LIBDEFLATEAPI void
libdeflate_set_memory_allocator(malloc_func_t malloc_func,
				free_func_t free_func)
{
	libdeflate_default_malloc_func = malloc_func;
	libdeflate_default_free_func = free_func;
}

/*
 * Implementations of libc functions for freestanding library builds.
 * Normal library builds don't use these.  Not optimized yet; usually the
 * compiler expands these functions and doesn't actually call them anyway.
 */
#ifdef FREESTANDING
#undef memset
void * __attribute__((weak))
memset(void *s, int c, size_t n)
{
	u8 *p = s;
	size_t i;

	for (i = 0; i < n; i++)
		p[i] = c;
	return s;
}

#undef memcpy
void * __attribute__((weak))
memcpy(void *dest, const void *src, size_t n)
{
	u8 *d = dest;
	const u8 *s = src;
	size_t i;

	for (i = 0; i < n; i++)
		d[i] = s[i];
	return dest;
}

#undef memmove
void * __attribute__((weak))
memmove(void *dest, const void *src, size_t n)
{
	u8 *d = dest;
	const u8 *s = src;
	size_t i;

	if (d <= s)
		return memcpy(d, s, n);

	for (i = n; i > 0; i--)
		d[i - 1] = s[i - 1];
	return dest;
}

#undef memcmp
int __attribute__((weak))
memcmp(const void *s1, const void *s2, size_t n)
{
	const u8 *p1 = s1;
	const u8 *p2 = s2;
	size_t i;

	for (i = 0; i < n; i++) {
		if (p1[i] != p2[i])
			return (int)p1[i] - (int)p2[i];
	}
	return 0;
}
#endif /* FREESTANDING */

#ifdef LIBDEFLATE_ENABLE_ASSERTIONS
#include <stdio.h>
#include <stdlib.h>
void
libdeflate_assertion_failed(const char *expr, const char *file, int line)
{
	fprintf(stderr, "Assertion failed: %s at %s:%d\n", expr, file, line);
	abort();
}
#endif /* LIBDEFLATE_ENABLE_ASSERTIONS */

/*
 * x86/cpu_features.c - feature detection for x86 CPUs
 *
 * Copyright 2016 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#if HAVE_DYNAMIC_X86_CPU_FEATURES

/*
 * With old GCC versions we have to manually save and restore the x86_32 PIC
 * register (ebx).  See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47602
 */
#if defined(ARCH_X86_32) && defined(__PIC__)
#  define EBX_CONSTRAINT "=&r"
#else
#  define EBX_CONSTRAINT "=b"
#endif

/* Execute the CPUID instruction. */
static inline void
cpuid(u32 leaf, u32 subleaf, u32 *a, u32 *b, u32 *c, u32 *d)
{
#ifdef _MSC_VER
	int result[4];

	__cpuidex(result, leaf, subleaf);
	*a = result[0];
	*b = result[1];
	*c = result[2];
	*d = result[3];
#else
	__asm__ volatile(".ifnc %%ebx, %1; mov  %%ebx, %1; .endif\n"
			 "cpuid                                  \n"
			 ".ifnc %%ebx, %1; xchg %%ebx, %1; .endif\n"
			 : "=a" (*a), EBX_CONSTRAINT (*b), "=c" (*c), "=d" (*d)
			 : "a" (leaf), "c" (subleaf));
#endif
}

/* Read an extended control register. */
static inline u64
read_xcr(u32 index)
{
#ifdef _MSC_VER
	return _xgetbv(index);
#else
	u32 d, a;

	/*
	 * Execute the "xgetbv" instruction.  Old versions of binutils do not
	 * recognize this instruction, so list the raw bytes instead.
	 *
	 * This must be 'volatile' to prevent this code from being moved out
	 * from under the check for OSXSAVE.
	 */
	__asm__ volatile(".byte 0x0f, 0x01, 0xd0" :
			 "=d" (d), "=a" (a) : "c" (index));

	return ((u64)d << 32) | a;
#endif
}

static const struct cpu_feature x86_cpu_feature_table[] = {
	{X86_CPU_FEATURE_SSE2,		"sse2"},
	{X86_CPU_FEATURE_PCLMUL,	"pclmul"},
	{X86_CPU_FEATURE_AVX,		"avx"},
	{X86_CPU_FEATURE_AVX2,		"avx2"},
	{X86_CPU_FEATURE_BMI2,		"bmi2"},
};

volatile u32 libdeflate_x86_cpu_features = 0;

/* Initialize libdeflate_x86_cpu_features. */
void libdeflate_init_x86_cpu_features(void)
{
	u32 max_leaf, a, b, c, d;
	u64 xcr0 = 0;
	u32 features = 0;

	/* EAX=0: Highest Function Parameter and Manufacturer ID */
	cpuid(0, 0, &max_leaf, &b, &c, &d);
	if (max_leaf < 1)
		goto out;

	/* EAX=1: Processor Info and Feature Bits */
	cpuid(1, 0, &a, &b, &c, &d);
	if (d & (1 << 26))
		features |= X86_CPU_FEATURE_SSE2;
	if (c & (1 << 1))
		features |= X86_CPU_FEATURE_PCLMUL;
	if (c & (1 << 27))
		xcr0 = read_xcr(0);
	if ((c & (1 << 28)) && ((xcr0 & 0x6) == 0x6))
		features |= X86_CPU_FEATURE_AVX;

	if (max_leaf < 7)
		goto out;

	/* EAX=7, ECX=0: Extended Features */
	cpuid(7, 0, &a, &b, &c, &d);
	if ((b & (1 << 5)) && ((xcr0 & 0x6) == 0x6))
		features |= X86_CPU_FEATURE_AVX2;
	if (b & (1 << 8))
		features |= X86_CPU_FEATURE_BMI2;

out:
	disable_cpu_features_for_testing(&features, x86_cpu_feature_table,
					 ARRAY_LEN(x86_cpu_feature_table));

	libdeflate_x86_cpu_features = features | X86_CPU_FEATURES_KNOWN;
}

#endif /* HAVE_DYNAMIC_X86_CPU_FEATURES */
