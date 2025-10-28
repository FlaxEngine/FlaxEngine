/*

Copyright (c) 2015 Harm Hanemaaijer <fgenfb@yahoo.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#ifndef __DETEX_H__
#define __DETEX_H__

#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS /* empty */
#define __END_DECLS /* empty */
#endif

/* Generic helper definitions for shared library support. */
#if defined _WIN32 || defined __CYGWIN__
  #define DETEX_HELPER_SHARED_IMPORT __declspec(dllimport)
  #define DETEX_HELPER_SHARED_EXPORT __declspec(dllexport)
  #define DETEX_HELPER_SHARED_LOCAL
#else
  #if __GNUC__ >= 4
    #define DETEX_HELPER_SHARED_IMPORT __attribute__ ((visibility ("default")))
    #define DETEX_HELPER_SHARED_EXPORT __attribute__ ((visibility ("default")))
    #define DETEX_HELPER_SHARED_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DETEX_HELPER_SHARED_IMPORT
    #define DETEX_HELPER_SHARED_EXPORT
    #define DETEX_HELPER_SHARED_LOCAL
  #endif
#endif

/* Now we use the generic helper definitions above to define DETEX_API and DETEX_LOCAL. */
/* DETEX_API is used for the public API symbols. It either imports or exports the symbol */
/* for shared/DLL libraries (or does nothing for static build). DETEX_LOCAL is used for */
/* non-API symbols. */

#ifdef DETEX_SHARED
  /* Defined if DETEX is compiled as a shared library. */
  #ifdef DETEX_SHARED_EXPORTS
    /* Defined if we are building the detex shared library (instead of using it). */
    #define DETEX_API DETEX_HELPER_SHARED_EXPORT
  #else
    #define DETEX_API DETEX_HELPER_SHARED_IMPORT
  #endif /* DETEX_SHARED_EXPORTS */
  #define DETEX_LOCAL DETEX_HELPER_SHARED_LOCAL
#else
  /* DETEX_SHARED is not defined: this means detex is a static lib. */
  #define DETEX_API
  #define DETEX_LOCAL
#endif /* DETEX_SHARED */

__BEGIN_DECLS

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_MSC_VER)
#define DETEX_INLINE_ONLY __forceinline
#else
#define DETEX_INLINE_ONLY __attribute__((always_inline)) inline
#endif
#define DETEX_RESTRICT __restrict

/* Maximum uncompressed block size in bytes. */
#define DETEX_MAX_BLOCK_SIZE 256

/* Detex library pixel formats. */

enum {
	/* The format has 16-bit components. */
	DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT = 0x1,
	/* The format has 32-bit components. */
	DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT = 0x2,
	/* The format has an alpha component. */
	DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT = 0x4,
	/* The sequential component order is RGB. */
	DETEX_PIXEL_FORMAT_RGB_COMPONENT_ORDER_BIT = 0x0,
	/* The sequential component order is BGR. */
	DETEX_PIXEL_FORMAT_BGR_COMPONENT_ORDER_BIT = 0x8,
	/* The format has one component. */
	DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS = 0x0,
	/* The format has two components. */
	DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS = 0x10,
	/* The format has three components. */
	DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS = 0x20,
	/* The format has four components. */
	DETEX_PIXEL_FORMAT_FOUR_COMPONENTS_BITS = 0x30,
	/* The format is stored as 8-bit pixels. */
	DETEX_PIXEL_FORMAT_8BIT_PIXEL_BITS = 0x000,
	/* The format is stored as 16-bit pixels. */
	DETEX_PIXEL_FORMAT_16BIT_PIXEL_BITS = 0x100,
	/* The format is stored as 24-bit pixels. */
	DETEX_PIXEL_FORMAT_24BIT_PIXEL_BITS = 0x200,
	/* The format is stored as 32-bit pixels. */
	DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS = 0x300,
	/* The format is stored as 48-bit pixels. */
	DETEX_PIXEL_FORMAT_48BIT_PIXEL_BITS = 0x500,
	/* The format is stored as 64-bit pixels. */
	DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS = 0x700,
	/* The format is stored as 96-bit pixels. */
	DETEX_PIXEL_FORMAT_96BIT_PIXEL_BITS = 0xB00,
	/* The format is stored as 128-bit pixels. */
	DETEX_PIXEL_FORMAT_128BIT_PIXEL_BITS = 0xF00,
	/* The format has signed integer components. */
	DETEX_PIXEL_FORMAT_SIGNED_BIT = 0x1000,
	/* The format has (half-)float components. */
	DETEX_PIXEL_FORMAT_FLOAT_BIT = 0x2000,
	/* The fomat is HDR (high dynamic range). */
	DETEX_PIXEL_FORMAT_HDR_BIT = 0x4000,

	DETEX_PIXEL_FORMAT_RGBA8 = (
		DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_FOUR_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_BGRA8 = (
		DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_BGR_COMPONENT_ORDER_BIT |
		DETEX_PIXEL_FORMAT_FOUR_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_RGBX8 = (
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_BGRX8 = (
		DETEX_PIXEL_FORMAT_BGR_COMPONENT_ORDER_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_RGB8 = (
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_24BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_BGR8 = (
		DETEX_PIXEL_FORMAT_BGR_COMPONENT_ORDER_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_24BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_R8 = (
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_8BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_SIGNED_R8 = (
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_8BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_SIGNED_BIT
		),
	DETEX_PIXEL_FORMAT_RG8 = (
		DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_16BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_SIGNED_RG8 = (
		DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_16BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_SIGNED_BIT
		),
	DETEX_PIXEL_FORMAT_R16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_16BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_SIGNED_R16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_16BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_SIGNED_BIT
		),
	DETEX_PIXEL_FORMAT_RG16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_SIGNED_RG16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_SIGNED_BIT
		),
	DETEX_PIXEL_FORMAT_RGB16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_48BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_RGBX16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_RGBA16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_FOUR_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS
		),
	DETEX_PIXEL_FORMAT_FLOAT_R16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_16BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_R16_HDR = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_16BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RG16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RG16_HDR = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGBX16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGBX16_HDR = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGBA16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_FOUR_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGBA16_HDR = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_FOUR_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGB16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_48BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGB16_HDR = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_48BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_BGRX16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_BGR_COMPONENT_ORDER_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_BGRX16_HDR = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_BGR_COMPONENT_ORDER_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_SIGNED_FLOAT_RGBX16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_SIGNED_BIT |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_SIGNED_FLOAT_BGRX16 = (
		DETEX_PIXEL_FORMAT_16BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_BGR_COMPONENT_ORDER_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_SIGNED_BIT |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_R32 = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_R32_HDR = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_32BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RG32 = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RG32_HDR = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_TWO_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_64BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGB32 = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_96BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGB32_HDR = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_96BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGBX32 = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_128BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGBX32_HDR = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_THREE_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_128BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGBA32 = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_FOUR_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_128BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT
		),
	DETEX_PIXEL_FORMAT_FLOAT_RGBA32_HDR = (
		DETEX_PIXEL_FORMAT_32BIT_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_FOUR_COMPONENTS_BITS |
		DETEX_PIXEL_FORMAT_128BIT_PIXEL_BITS |
		DETEX_PIXEL_FORMAT_FLOAT_BIT |
		DETEX_PIXEL_FORMAT_HDR_BIT
		),
	DETEX_PIXEL_FORMAT_A8 = (
		DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT |
		DETEX_PIXEL_FORMAT_ONE_COMPONENT_BITS |
		DETEX_PIXEL_FORMAT_8BIT_PIXEL_BITS
		),
};

/* Mode mask flags. */

enum {
	DETEX_MODE_MASK_ETC_INDIVIDUAL = 0x1,
	DETEX_MODE_MASK_ETC_DIFFERENTIAL = 0x2,
	DETEX_MODE_MASK_ETC_T = 0x4,
	DETEX_MODE_MASK_ETC_H = 0x8,
	DETEX_MODE_MASK_ETC_PLANAR = 0x10,
	DETEX_MODE_MASK_ALL_MODES_ETC1 = 0x3,
	DETEX_MODE_MASK_ALL_MODES_ETC2 = 0x1F,
	DETEX_MODE_MASK_ALL_MODES_ETC2_PUNCHTHROUGH = 0X1E,
	DETEX_MODE_MASK_ALL_MODES_BPTC = 0xFF,
	DETEX_MODE_MASK_ALL_MODES_BPTC_FLOAT = 0x3FFF,
	DETEX_MODE_MASK_ALL = 0XFFFFFFFF,
};

/* Decompression function flags. */

enum {
	/* Function returns false (invalid block) when the compressed block */
	/* is in a format not allowed to be generated by an encoder. */
	DETEX_DECOMPRESS_FLAG_ENCODE = 0x1,
	/* For compression formats that have opaque and non-opaque modes, */
	/* return false (invalid block) when the compressed block is encoded */
	/* using a non-opaque mode. */
	DETEX_DECOMPRESS_FLAG_OPAQUE_ONLY = 0x2,
	/* For compression formats that have opaque and non-opaque modes, */
	/* return false (invalid block) when the compressed block is encoded */
	/* using an opaque mode. */
	DETEX_DECOMPRESS_FLAG_NON_OPAQUE_ONLY = 0x4,
};

/* Set mode function flags. */

enum {
	/* The block is opaque (alpha is always 0xFF). */
	DETEX_SET_MODE_FLAG_OPAQUE = 0x2,
	/* The block is non-opaque (alpha is not always 0xFF). */
	DETEX_SET_MODE_FLAG_NON_OPAQUE = 0x4,
	/* The block has punchthrough alpha (alpha is either 0x00 or 0xFF). */
	DETEX_SET_MODE_FLAG_PUNCHTHROUGH = 0x8,
	/* The block only consists of one or two different pixel colors. */
	DETEX_SET_MODE_FLAG_MAX_TWO_COLORS = 0x10,
};

/*
 * Decompression functions for 8-bit RGB8/RGBA8 formats. The output pixel format
 * is DETEX_PIXEL_FORMAT_RGBA8 or DETEX_PIXEL_FORMAT_RGBX8 (32-bit pixels with
 * optional alpha component, red component in lowest-order byte. When the
 * texture format does not have alpha, alpha is set to 0xFF.
 */

/* Decompress a 64-bit 4x4 pixel texture block compressed using the ETC1 */
/* format. */
DETEX_API bool detexDecompressBlockETC1(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 64-bit 4x4 pixel texture block compressed using the ETC2 */
/* format. */
DETEX_API bool detexDecompressBlockETC2(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* ETC2_PUNCHTROUGH format. */
DETEX_API bool detexDecompressBlockETC2_PUNCHTHROUGH(const uint8_t *bitstring,
	uint32_t mode_mask, uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 128-bit 4x4 pixel texture block compressed using the ETC2_EAC */
/* format. */
DETEX_API bool detexDecompressBlockETC2_EAC(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);


/* Decompress a 64-bit 4x4 pixel texture block compressed using the BC1 */
/* format. */
DETEX_API bool detexDecompressBlockBC1(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 64-bit 4x4 pixel texture block compressed using the BC1A */
/* format. */
DETEX_API bool detexDecompressBlockBC1A(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 64-bit 4x4 pixel texture block compressed using the BC2 */
/* format. */
DETEX_API bool detexDecompressBlockBC2(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 64-bit 4x4 pixel texture block compressed using the BC3 */
/* format. */
DETEX_API bool detexDecompressBlockBC3(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 128-bit 4x4 pixel texture block compressed using the BPTC */
/* (BC7) format. */
DETEX_API bool detexDecompressBlockBPTC(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);

/*
 * Decompression functions for 8-bit unsigned R and RG formats. The
 * output format is DETEX_PIXEL_FORMAT_R8 or DETEX_PIXEL_FORMAT_RG8.
 */

/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* unsigned RGTC1 (BC4) format. */
DETEX_API bool detexDecompressBlockRGTC1(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* unsigned RGTC2 (BC5) format. */
DETEX_API bool detexDecompressBlockRGTC2(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);

/*
 * Decompression functions for 16-bit unsigned/signed R and RG formats. The
 * output format is DETEX_PIXEL_FORMAT_R16, DETEX_PIXEL_FORMAT_SIGNED_R16,
 * DETEX_PIXEL_FORMAT_RG16, or DETEX_PIXEL_FORMAT_SIGNED_RG16.
 */

/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* signed RGTC1 (signed BC4) format. */
DETEX_API bool detexDecompressBlockSIGNED_RGTC1(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* signed RGTC2 (signed BC5) format. */
DETEX_API bool detexDecompressBlockSIGNED_RGTC2(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* ETC2_R11_EAC format. */
DETEX_API bool detexDecompressBlockEAC_R11(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* ETC2_SIGNED_R11_EAC format. */
DETEX_API bool detexDecompressBlockEAC_SIGNED_R11(const uint8_t *bitstring,
	uint32_t mode_mask, uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* ETC2_RG11_EAC format. */
DETEX_API bool detexDecompressBlockEAC_RG11(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* ETC2_SIGNED_RG11_EAC format. */
DETEX_API bool detexDecompressBlockEAC_SIGNED_RG11(const uint8_t *bitstring,
	uint32_t mode_mask, uint32_t flags, uint8_t *pixel_buffer);

/*
 * Decompression functions for 16-bit half-float formats. The output format is
 * DETEX_PIXEL_FORMAT_FLOAT_RGBX16 or DETEX_PIXEL_FORMAT_SIGNED_FLOAT_RGBX16.
 */

/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* BPTC_FLOAT (BC6H) format. The output format is */
/* DETEX_PIXEL_FORMAT_FLOAT_RGBX16. */
DETEX_API bool detexDecompressBlockBPTC_FLOAT(const uint8_t *bitstring, uint32_t mode_mask,
	uint32_t flags, uint8_t *pixel_buffer);
/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* BPTC_FLOAT (BC6H_FLOAT) format. The output format is */
/* DETEX_PIXEL_FORMAT_SIGNED_FLOAT_RGBX16. */
DETEX_API bool detexDecompressBlockBPTC_SIGNED_FLOAT(const uint8_t *bitstring,
	uint32_t mode_mask, uint32_t flags, uint8_t *pixel_buffer);


/*
 * Get mode functions. They return the internal compression format mode used
 * inside the compressed block. For compressed formats that do not use a mode,
 * there is no GetMode function.
 */

DETEX_API uint32_t detexGetModeBC1(const uint8_t *bitstring);
DETEX_API uint32_t detexGetModeETC1(const uint8_t *bitstring);
DETEX_API uint32_t detexGetModeETC2(const uint8_t *bitstring);
DETEX_API uint32_t detexGetModeETC2_PUNCHTHROUGH(const uint8_t *bitstring);
DETEX_API uint32_t detexGetModeETC2_EAC(const uint8_t *bitstring);
DETEX_API uint32_t detexGetModeBPTC(const uint8_t *bitstring);
DETEX_API uint32_t detexGetModeBPTC_FLOAT(const uint8_t *bitstring);
DETEX_API uint32_t detexGetModeBPTC_SIGNED_FLOAT(const uint8_t *bitstring);

/*
 * Set mode functions. The set mode function modifies a compressed texture block
 * so that the specified mode is set, making use of information about the block
 * (whether it is opaque, non-opaque or punchthrough for formats with alpha,
 * whether at most two different colors are used). For compressed formats
 * that do not use a mode, there is no SetMode function.
 */

DETEX_API void detexSetModeBC1(uint8_t *bitstring, uint32_t mode, uint32_t flags,
	uint32_t *colors);
DETEX_API void detexSetModeETC1(uint8_t *bitstring, uint32_t mode, uint32_t flags,
	uint32_t *colors);
DETEX_API void detexSetModeETC2(uint8_t *bitstring, uint32_t mode, uint32_t flags,
	uint32_t *colors);
DETEX_API void detexSetModeETC2_PUNCHTHROUGH(uint8_t *bitstring, uint32_t mode, uint32_t flags,
	uint32_t *colors);
DETEX_API void detexSetModeETC2_EAC(uint8_t *bitstring, uint32_t mode, uint32_t flags,
	uint32_t *colors);
DETEX_API void detexSetModeBPTC(uint8_t *bitstring, uint32_t mode, uint32_t flags,
	uint32_t *colors);
DETEX_API void detexSetModeBPTC_FLOAT(uint8_t *bitstring, uint32_t mode, uint32_t flags,
	uint32_t *colors);

/* Compressed texture format definitions for general texture decompression */
/* functions. */

#define DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(n) ((uint32_t)n << 24)

enum {
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_UNCOMPRESSED = 0,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC1 = 1,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_DXT1 = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC1,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_S3TC = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC1,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC1A,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_DXT1A = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC1A,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC2,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_DXT3 = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC2,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC3,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_DXT5 = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC3,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_RGTC1,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC4_UNORM = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_RGTC1,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_SIGNED_RGTC1,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC4_SNORM = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_SIGNED_RGTC1,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_RGTC2,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC5_UNORM = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_RGTC2,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_SIGNED_RGTC2,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC5_SNORM = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_SIGNED_RGTC2,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC_FLOAT,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC6H_UF16 = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC_FLOAT,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC_SIGNED_FLOAT,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC6H_SF16 = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC_SIGNED_FLOAT,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC7 = DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ETC1,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ETC2,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ETC2_PUNCHTHROUGH,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ETC2_EAC,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_EAC_R11,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_EAC_SIGNED_R11,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_EAC_RG11,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_EAC_SIGNED_RG11,
	DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ASTC_4X4,
};

enum {
	DETEX_TEXTURE_FORMAT_PIXEL_FORMAT_MASK = 0x0000FFFF,
	DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT = 0x00800000,
	DETEX_TEXTURE_FORMAT_BC1 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC1) |
		DETEX_PIXEL_FORMAT_RGBX8
		),
	DETEX_TEXTURE_FORMAT_BC1A = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC1A) |
		DETEX_PIXEL_FORMAT_RGBA8
		),
	DETEX_TEXTURE_FORMAT_BC2 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC2) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_RGBA8
		),
	DETEX_TEXTURE_FORMAT_BC3 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BC3) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_RGBA8
		),
	DETEX_TEXTURE_FORMAT_RGTC1 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_RGTC1) |
		DETEX_PIXEL_FORMAT_R8
		),
	DETEX_TEXTURE_FORMAT_SIGNED_RGTC1 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_SIGNED_RGTC1) |
		DETEX_PIXEL_FORMAT_SIGNED_R16
		),
	DETEX_TEXTURE_FORMAT_RGTC2 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_RGTC2) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_RG8
		),
	DETEX_TEXTURE_FORMAT_SIGNED_RGTC2 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_SIGNED_RGTC2) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_SIGNED_RG16
		),
	DETEX_TEXTURE_FORMAT_BPTC_FLOAT = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC_FLOAT) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_FLOAT_RGBX16
		),
	DETEX_TEXTURE_FORMAT_BPTC_SIGNED_FLOAT = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC_SIGNED_FLOAT) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_SIGNED_FLOAT_RGBX16
		),
	DETEX_TEXTURE_FORMAT_BPTC = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_BPTC) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_RGBA8
		),
	DETEX_TEXTURE_FORMAT_ETC1 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ETC1) |
		DETEX_PIXEL_FORMAT_RGBX8
		),
	DETEX_TEXTURE_FORMAT_ETC2 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ETC2) |
		DETEX_PIXEL_FORMAT_RGBX8
		),
	DETEX_TEXTURE_FORMAT_ETC2_PUNCHTHROUGH = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(	
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ETC2_PUNCHTHROUGH) |
		DETEX_PIXEL_FORMAT_RGBA8
		),
	DETEX_TEXTURE_FORMAT_ETC2_EAC = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ETC2_EAC) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_RGBA8
		),
	DETEX_TEXTURE_FORMAT_EAC_R11 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_EAC_R11) |
		DETEX_PIXEL_FORMAT_R16
		),
	DETEX_TEXTURE_FORMAT_EAC_SIGNED_R11 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_EAC_SIGNED_R11) |
		DETEX_PIXEL_FORMAT_SIGNED_R16
		),
	DETEX_TEXTURE_FORMAT_EAC_RG11 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_EAC_RG11) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_RG16
		),
	DETEX_TEXTURE_FORMAT_EAC_SIGNED_RG11 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_EAC_SIGNED_RG11) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_SIGNED_RG16
		),
	DETEX_TEXTURE_FORMAT_ASTC_4X4 = (
		DETEX_TEXTURE_FORMAT_COMPRESSED_FORMAT_BITS(
			DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_ASTC_4X4 ) |
		DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT |
		DETEX_PIXEL_FORMAT_RGBA8
		),
};

typedef struct {
	uint32_t format;
	uint8_t *data;
	int width;
	int height;
	int width_in_blocks;
	int height_in_blocks;
} detexTexture;

/*
 * General texture decompression functions (tiled or linear) with specified
 * compression format.
 */

/*
 * General block decompression function. Block is decompressed using the given
 * compressed format, and stored in the given pixel format.
 */
DETEX_API bool detexDecompressBlock(const uint8_t *bitstring, uint32_t texture_format,
	uint32_t mode_mask, uint32_t flags, uint8_t *pixel_buffer,
	uint32_t pixel_format);

/*
 * Decode texture function (tiled). Decode an entire compressed texture into an
 * array of image buffer tiles (corresponding to compressed blocks), converting
 * into the given pixel format.
 */
DETEX_API bool detexDecompressTextureTiled(const detexTexture *texture, uint8_t *pixel_buffer,
	uint32_t pixel_format);

/*
 * Decode texture function (linear). Decode an entire texture into a single
 * image buffer, with pixels stored row-by-row, converting into the given pixel
 * format.
 */
DETEX_API bool detexDecompressTextureLinear(const detexTexture *texture, uint8_t *pixel_buffer,
	uint32_t pixel_format);


/*
 * Miscellaneous functions.
 */

/*
 * Convert pixels between different formats. The target pixel buffer must
 * be allocated with sufficient size to the hold the result. Returns true if
 * succesful.
 */
DETEX_API bool detexConvertPixels(uint8_t *source_pixel_buffer, uint32_t nu_pixels,
	uint32_t source_pixel_format, uint8_t *target_pixel_buffer,
	uint32_t target_pixel_format);

/* Convert in-place, modifying the source pixel buffer only. If any conversion step changes the */
/* pixel size, the function will not be succesful and return false. */
DETEX_API bool detexConvertPixelsInPlace(uint8_t * DETEX_RESTRICT source_pixel_buffer,
	uint32_t nu_pixels, uint32_t source_pixel_format, uint32_t target_pixel_format);

/* Return the component bitfield masks for a pixel format (pixel size must be at most 64 bits). */
/* Return true if succesful. */
DETEX_API bool detexGetComponentMasks(uint32_t texture_format, uint64_t *red_mask, uint64_t *green_mask,
	uint64_t *blue_mask, uint64_t *alpha_mask);

/* Return a text description/identifier of the texture type. */
DETEX_API const char *detexGetTextureFormatText(uint32_t texture_format);

/* Return a alternative text description of the texture type. Returns empty string */
/* when there is no alternative description. */
DETEX_API const char *detexGetAlternativeTextureFormatText(uint32_t texture_format);

/* Return OpenGL TexImage2D/KTX file parameters for a texture format. */
DETEX_API bool detexGetOpenGLParameters(uint32_t texture_format, int *gl_internal_format,
	uint32_t *gl_format, uint32_t *gl_type);

/* Return DirectX 10 format for a texture format. */
DETEX_API bool detexGetDX10Parameters(uint32_t texture_format, uint32_t *dx10_format);

/* Return the error message for the last encountered error. */
DETEX_API const char *detexGetErrorMessage();


/*
 * HDR-related functions.
 */

/* Set HDR gamma curve parameters. */
DETEX_API void detexSetHDRParameters(float gamma, float range_min, float range_max);

/* Calculate the dynamic range of a pixel buffer. Valid for float and half-float formats. */
/* Returns true if successful. */
DETEX_API bool detexCalculateDynamicRange(uint8_t *pixel_buffer, int nu_pixels, uint32_t pixel_format,
	float *range_min_out, float *range_max_out);


/*
 * Texture file loading.
 */

/* Load texture from KTX file with mip-maps. Returns true if successful. */
/* nu_levels is a return parameter that returns the number of mipmap levels found. */
/* textures_out is a return parameter for an array of detexTexture pointers that is allocated, */
/* free with free(). textures_out[i] are allocated textures corresponding to each level, free */
/* with free().	 */
DETEX_API bool detexLoadKTXFileWithMipmaps(const char *filename, int max_mipmaps, detexTexture ***textures_out,
	int *nu_levels_out);

/* Load texture from KTX file (first mip-map only). Returns true if successful. */
/* The texture is allocated, free with free(). */
DETEX_API bool detexLoadKTXFile(const char *filename, detexTexture **texture_out);

/* Save textures to KTX file (multiple mip-maps levels). Return true if succesful. */
DETEX_API bool detexSaveKTXFileWithMipmaps(detexTexture **textures, int nu_levels, const char *filename);

/* Save texture to KTX file (single mip-map level). Returns true if succesful. */
DETEX_API bool detexSaveKTXFile(detexTexture *texture, const char *filename);

/* Load texture from DDS file with mip-maps. Returns true if successful. */
/* nu_levels is a return parameter that returns the number of mipmap levels found. */
/* textures_out is a return parameter for an array of detexTexture pointers that is allocated, */
/* free with free(). textures_out[i] are allocated textures corresponding to each level, free */
/* with free(). */
DETEX_API bool detexLoadDDSFileWithMipmaps(const char *filename, int max_mipmaps, detexTexture ***textures_out,
	int *nu_levels_out);

/* Load texture from DDS file (first mip-map only). Returns true if successful. */
/* The texture is allocated, free with free(). */
DETEX_API bool detexLoadDDSFile(const char *filename, detexTexture **texture_out);

/* Save textures to DDS file (multiple mip-maps levels). Return true if succesful. */
DETEX_API bool detexSaveDDSFileWithMipmaps(detexTexture **textures, int nu_levels, const char *filename);

/* Save texture to DDS file (single mip-map level). Returns true if succesful. */
DETEX_API bool detexSaveDDSFile(detexTexture *texture, const char *filename);

/* Load texture file (type autodetected from extension) with mipmaps. */
DETEX_API bool detexLoadTextureFileWithMipmaps(const char *filename, int max_mipmaps, detexTexture ***textures_out,
	int *nu_levels_out);

/* Load texture file (type autodetected from extension). */
DETEX_API bool detexLoadTextureFile(const char *filename, detexTexture **texture_out);

/* Load texture from raw file (first mip-map only) given the format and dimensions */
/* in texture. Returns true if successful. */
/* The texture->data is allocated, free with free(). */
DETEX_API bool detexLoadRawFile(const char *filename, detexTexture *texture);

/* Save texture to raw file (first mip-map only) given the format and dimensions */
/* in texture. Returns true if successful. */
DETEX_API bool detexSaveRawFile(detexTexture *texture, const char *filename);

/* Return pixel size in bytes for pixel format or texture format (decompressed). */
static DETEX_INLINE_ONLY int detexGetPixelSize(uint32_t pixel_format) {
	return 1 + ((pixel_format & 0xF00) >> 8);
}

/* Return the number of components of a pixel format or texture format. */
static DETEX_INLINE_ONLY int detexGetNumberOfComponents(uint32_t pixel_format) {
	return 1 + ((pixel_format & 0x30) >> 4);
}

/* Return the component size in bytes of a pixel format or texture format. */
static DETEX_INLINE_ONLY int detexGetComponentSize(uint32_t pixel_format) {
	return 1 << (pixel_format & 0x3);
}

/* Return the approximate precision in bits of the components of a pixel format. */
static DETEX_INLINE_ONLY uint32_t detexGetComponentPrecision(uint32_t pixel_format) {
	return detexGetComponentSize(pixel_format) * 8 -
		((pixel_format & DETEX_PIXEL_FORMAT_FLOAT_BIT) != 0) * 5 *
		(1 + (detexGetComponentSize(pixel_format) == 4));
}

/* Return the total size of a compressed texture. */
static DETEX_INLINE_ONLY uint32_t detexTextureSize(uint32_t width_in_blocks,
uint32_t height_in_blocks, uint32_t pixel_format) {
	return width_in_blocks * height_in_blocks * detexGetPixelSize(pixel_format) * 16;
}

/* Return whether a pixel or texture format has an alpha component. */
static DETEX_INLINE_ONLY uint32_t detexFormatHasAlpha(uint32_t pixel_format) {
	return (pixel_format & DETEX_PIXEL_FORMAT_ALPHA_COMPONENT_BIT) != 0;
}


/* Return the compressed texture type index of a texture format. */
static DETEX_INLINE_ONLY uint32_t detexGetCompressedFormat(uint32_t texture_format) {
	return texture_format >> 24;
}

/* Return the block size of a compressed texture format in bytes. */
static DETEX_INLINE_ONLY uint32_t detexGetCompressedBlockSize(uint32_t texture_format) {
	return 8 + ((texture_format & DETEX_TEXTURE_FORMAT_128BIT_BLOCK_BIT) >> 20);
}

/* Return whether a texture format is compressed. */
static DETEX_INLINE_ONLY uint32_t detexFormatIsCompressed(uint32_t texture_format) {
	return detexGetCompressedFormat(texture_format) != DETEX_COMPRESSED_TEXTURE_FORMAT_INDEX_UNCOMPRESSED;
}

/* Return the pixel format of a texture format. */
static DETEX_INLINE_ONLY uint32_t detexGetPixelFormat(uint32_t texture_format) {
	return texture_format & DETEX_TEXTURE_FORMAT_PIXEL_FORMAT_MASK;
}


DETEX_API extern const uint8_t detex_clamp0to255_table[767];

/* Clamp an integer value in the range -255 to 511 to the the range 0 to 255. */
static DETEX_INLINE_ONLY uint8_t detexClamp0To255(int x) {
	return detex_clamp0to255_table[x + 255];
}

/* Clamp a float point value to the range 0.0 to 1.0f. */
static DETEX_INLINE_ONLY float detexClamp0To1(float f) {
	if (f < 0.0f)
		return 0.0f;
	else if (f > 1.0f)
		return 1.0f;
	else
		return f;
}


/* Integer division using look-up tables, used by BC1/2/3 and RGTC (BC4/5) */
/* decompression. */

DETEX_API extern const uint8_t detex_division_by_3_table[768];

static DETEX_INLINE_ONLY uint32_t detexDivide0To767By3(uint32_t value) {
	return detex_division_by_3_table[value];
}

DETEX_API extern const uint8_t detex_division_by_7_table[1792];

static DETEX_INLINE_ONLY uint32_t detexDivide0To1791By7(uint32_t value) {
	return detex_division_by_7_table[value];
}

static DETEX_INLINE_ONLY int8_t detexSignInt32(int v) {
	return (int8_t)((v >> 31) | - (- v >> 31));
}

static DETEX_INLINE_ONLY int detexDivideMinus895To895By7(int value) {
	return (int8_t)detex_division_by_7_table[abs(value)] * detexSignInt32(value);
}

DETEX_API extern const uint8_t detex_division_by_5_table[1280];

static DETEX_INLINE_ONLY uint32_t detexDivide0To1279By5(uint32_t value) {
	return detex_division_by_5_table[value];
}

static DETEX_INLINE_ONLY int detexDivideMinus639To639By5(int value) {
	return (int8_t)detex_division_by_5_table[abs(value)] * detexSignInt32(value);
}


/*
 * Define some short functions for pixel packing/unpacking. The compiler will
 * take care of optimization by inlining and removing unused functions.
 *
 * The pixel format used corresponds to formats with an RGB component order,
 * including:
 *
 * DETEX_PIXEL_FORMAT_RGB8, DETEX_PIXEL_FORMAT_RGBA8
 *	detexPack32RGB8Alpha0xFF, detexPack32R8, detexPack32G8, detexPack32B8,
 *	detexPixel32GetR8, detexPixel32GetG8, detexPixel32GetB8
 * DETEX_PIXEL_FORMAT_RGBA8
 *	detexPack32RGBA8, detexPack32A8, detexPixel32GetA8
 * DETEX_PIXEL_FORMAT_RG16, DETEX_PIXEL_FORMAT_SIGNED_RG16,
 * DETEX_PIXEL_FORMAT_FLOAT_RG16
 *	detexPack32RG16, detexPack32R16, detexPack32G16, detexPack32RG16,
 *	detexPixel32GetR16, detexPixel32GetG16
 * DETEX_PIXEL_FORMAT_FLOAT_RGBX16, DETEX_PIXEL_FORMAT_SIGNED_FLOAT_RGBX16
 *	detexPack64RGB16, detexPack64R16, detexPack64G16, detexPack64B16,
 *	detexPixel64GetR16, detexPixel64GetG16, detexPixel64GetB16
 */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ || !defined(__BYTE_ORDER__)

static DETEX_INLINE_ONLY uint32_t detexPack32RGBA8(int r, int g, int b, int a) {
	return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) |
		((uint32_t)a << 24);
}

static DETEX_INLINE_ONLY uint32_t detexPack32RGB8Alpha0xFF(int r, int g, int b) {
	return detexPack32RGBA8(r, g, b, 0xFF);
}

static DETEX_INLINE_ONLY uint32_t detexPack32R8(int r) {
	return (uint32_t)r;
}

static DETEX_INLINE_ONLY uint32_t detexPack32G8(int g) {
	return (uint32_t)g << 8;
}

static DETEX_INLINE_ONLY uint32_t detexPack32B8(int b) {
	return (uint32_t)b << 16;
}

static DETEX_INLINE_ONLY uint32_t detexPack32A8(int a) {
	return (uint32_t)a << 24;
}

static DETEX_INLINE_ONLY uint32_t detexPack32RG8(uint32_t r8, uint32_t g8) {
	return r8 | (g8 << 8);
}

static DETEX_INLINE_ONLY uint32_t detexPack32R16(uint32_t r16) {
	return r16;
}

static DETEX_INLINE_ONLY uint32_t detexPack32G16(uint32_t g16) {
	return g16 << 16;
}

static DETEX_INLINE_ONLY uint32_t detexPack32RG16(uint32_t r16, uint32_t g16) {
	return r16 | (g16 << 16);
}

static DETEX_INLINE_ONLY uint64_t detexPack64R16(uint32_t r16) {
	return r16;
}

static DETEX_INLINE_ONLY uint64_t detexPack64G16(uint32_t g16) {
	return g16 << 16;
}

static DETEX_INLINE_ONLY uint64_t detexPack64B16(uint32_t b16) {
	return (uint64_t)b16 << 32;
}

static DETEX_INLINE_ONLY uint64_t detexPack64A16(uint32_t a16) {
	return (uint64_t)a16 << 48;
}

static DETEX_INLINE_ONLY uint64_t detexPack64RGB16(uint16_t r16, uint16_t g16, uint16_t b16) {
	return (uint64_t)r16 | ((uint64_t)g16 << 16) | ((uint64_t)b16 << 32);
}

static DETEX_INLINE_ONLY uint64_t detexPack64RGBA16(uint16_t r16, uint16_t g16, uint16_t b16, uint16_t a16) {
	return (uint64_t)r16 | ((uint64_t)g16 << 16) | ((uint64_t)b16 << 32) | ((uint64_t)a16 << 48);
}

static DETEX_INLINE_ONLY uint32_t detexPixel32GetR8(uint32_t pixel) {
	return pixel & 0xFF;
}

static DETEX_INLINE_ONLY uint32_t detexPixel32GetG8(uint32_t pixel) {
	return (pixel & 0xFF00) >> 8;
}

static DETEX_INLINE_ONLY uint32_t detexPixel32GetB8(uint32_t pixel) {
	return (pixel & 0xFF0000) >> 16;
}

static DETEX_INLINE_ONLY uint32_t detexPixel32GetA8(uint32_t pixel) {
	return (pixel & 0xFF000000) >> 24;
}

static DETEX_INLINE_ONLY int detexPixel32GetSignedR8(uint32_t pixel) {
	return (int8_t)(pixel & 0xFF);
}

static DETEX_INLINE_ONLY int detexPixel32GetSignedG8(uint32_t pixel) {
	return (int8_t)((pixel & 0xFF00) >> 8);
}

static DETEX_INLINE_ONLY uint32_t detexPixel32GetR16(uint32_t pixel) {
	return pixel & 0x0000FFFF;
}

static DETEX_INLINE_ONLY uint32_t detexPixel32GetG16(uint32_t pixel) {
	return (pixel & 0xFFFF0000) >> 16;
}

static DETEX_INLINE_ONLY int detexPixel32GetSignedR16(uint32_t pixel) {
	return (int16_t)(pixel & 0x0000FFFF);
}

static DETEX_INLINE_ONLY int detexPixel32GetSignedG16(uint32_t pixel) {
	return (int16_t)((pixel & 0xFFFF0000) >> 16);
}

static DETEX_INLINE_ONLY uint64_t detexPixel64GetR16(uint64_t pixel) {
	return pixel & 0xFFFF;
}

static DETEX_INLINE_ONLY uint64_t detexPixel64GetG16(uint64_t pixel) {
	return (pixel & 0xFFFF0000) >> 16;
}

static DETEX_INLINE_ONLY uint64_t detexPixel64GetB16(uint64_t pixel) {
	return (pixel & 0xFFFF00000000) >> 32;
}

static DETEX_INLINE_ONLY uint64_t detexPixel64GetA16(uint64_t pixel) {
	return (pixel & 0xFFFF000000000000) >> 48;
}

#define DETEX_PIXEL32_ALPHA_BYTE_OFFSET 3

#else

#error Big-endian byte order not supported.

static DETEX_INLINE_ONLY uint32_t detexPack32RGBA8(int r, int g, int b, int a) {
	return a | ((uint32_t)b << 8) | ((uint32_t)g << 16) | ((uint32_t)r << 24);
}

static DETEX_INLINE_ONLY uint32_t detexPack32RGB8Alpha0xFF(int r, int g, int b) {
	return pack_rgba(r, g, b, 0xFF);
}

static DETEX_INLINE_ONLY uint32_t detexPack32R8(int r) {
	return (uint32_t)r << 24;
}

static DETEX_INLINE_ONLY uint32_t detexPack32G8(int g) {
	return (uint32_t)g << 16;
}

static DETEX_INLINE_ONLY uint32_t detexPack32B8(int b) {
	return (uint32_t)b << 8;
}

static DETEX_INLINE_ONLY uint32_t detexPack32A8(int a) {
	return a;
}

static DETEX_INLINE_ONLY uint32_t detexPack32RG16(uint32_t r16, uint32_t g16) {
	return g16 | (r16 << 16);
}

static DETEX_INLINE_ONLY int detexPixel32GetR8(uint32_t pixel) {
	return (pixel & 0xFF000000) >> 24;
}

static DETEX_INLINE_ONLY int detexPixel32GetG8(uint32_t pixel) {
	return (pixel & 0xFF0000) >> 16;
}

static DETEX_INLINE_ONLY int detexPixel32GetB8(uint32_t pixel) {
	return (pixel & 0xFF00) >> 8;
}

static DETEX_INLINE_ONLY int detexPixel32GetA8(uint32_t pixel) {
	return pixel & 0xFF;
}

static DETEX_INLINE_ONLY uint32_t detexPixel32GetR16(uint32_t pixel) {
	return ((pixel & 0xFF000000) >> 24) | ((pixel & 0x00FF0000) >> 8);
}

static DETEX_INLINE_ONLY uint32_t detexPixel32GetG16(uint32_t pixel) {
	return ((pixel & 0x0000FF00) >> 8) | ((pixel & 0x000000FF) << 8);
}

#define DETEX_PIXEL32_ALPHA_BYTE_OFFSET 0

#endif

__END_DECLS

#endif

