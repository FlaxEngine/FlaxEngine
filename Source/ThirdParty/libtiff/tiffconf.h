/* libtiff/tiffconf.h.  Generated from tiffconf.h.in by configure.  */
/*
  Configuration defines for installed libtiff.
  This file maintained for backward compatibility. Do not use definitions
  from this file in your programs.
*/

/* clang-format off */
/* clang-format disabled because CMake scripts are very sensitive to the
 * formatting of this file. configure_file variables of type "@VAR@" are
 * modified by clang-format and won't be substituted.
 */

#ifndef _TIFFCONF_
#define _TIFFCONF_


#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>


/* Signed 16-bit type */
/* #undef TIFF_INT16_T */

/* Signed 32-bit type */
/* #undef TIFF_INT32_T */

/* Signed 64-bit type */
/* #undef TIFF_INT64_T */

/* Signed 8-bit type */
/* #undef TIFF_INT8_T */

/* Unsigned 16-bit type */
/* #undef TIFF_UINT16_T */

/* Unsigned 32-bit type */
/* #undef TIFF_UINT32_T */

/* Unsigned 64-bit type */
/* #undef TIFF_UINT64_T */

/* Unsigned 8-bit type */
/* #undef TIFF_UINT8_T */

/* Signed size type */
#define TIFF_SSIZE_T int64_t

/* Compatibility stuff. */

/* Define as 0 or 1 according to the floating point format supported by the
   machine */
#define HAVE_IEEEFP 1

/* The concept of HOST_FILLORDER is broken. Since libtiff 4.5.1
 * this macro will always be hardcoded to FILLORDER_LSB2MSB on all
 * architectures, to reflect past long behavior of doing so on x86 architecture.
 * Note however that the default FillOrder used by libtiff is FILLORDER_MSB2LSB,
 * as mandated per the TIFF specification.
 * The influence of HOST_FILLORDER is only when passing the 'H' mode in
 * TIFFOpen().
 * You should NOT rely on this macro to decide the CPU endianness!
 * This macro will be removed in libtiff 4.6
 */
#define HOST_FILLORDER FILLORDER_LSB2MSB

/* Native cpu byte order: 1 if big-endian (Motorola) or 0 if little-endian
   (Intel) */
#define HOST_BIGENDIAN 0

/* Support CCITT Group 3 & 4 algorithms */
#define CCITT_SUPPORT 1

/* Support JPEG compression (requires IJG JPEG library) */
#define JPEG_SUPPORT 1

/* Support JBIG compression (requires JBIG-KIT library) */
/* #undef JBIG_SUPPORT */

/* Support LERC compression */
#define LERC_SUPPORT 1

/* Support LogLuv high dynamic range encoding */
#define LOGLUV_SUPPORT 1

/* Support LZW algorithm */
#define LZW_SUPPORT 1

/* Support NeXT 2-bit RLE algorithm */
#define NEXT_SUPPORT 1

/* Support Old JPEG compression (read contrib/ojpeg/README first! Compilation
   fails with unpatched IJG JPEG library) */
#define OJPEG_SUPPORT 1

/* Support Macintosh PackBits algorithm */
#define PACKBITS_SUPPORT 1

/* Support Pixar log-format algorithm (requires Zlib) */
#define PIXARLOG_SUPPORT 1

/* Support ThunderScan 4-bit RLE algorithm */
#define THUNDER_SUPPORT 1

/* Support Deflate compression */
#define ZIP_SUPPORT 1

/* Support libdeflate enhanced compression */
/* #undef LIBDEFLATE_SUPPORT */

/* Support strip chopping (whether or not to convert single-strip uncompressed
   images to multiple strips of ~8Kb to reduce memory usage) */
#define STRIPCHOP_DEFAULT TIFF_STRIPCHOP

/* Enable SubIFD tag (330) support */
#define SUBIFD_SUPPORT 1

/* Treat extra sample as alpha (default enabled). The RGBA interface will
   treat a fourth sample with no EXTRASAMPLE_ value as being ASSOCALPHA. Many
   packages produce RGBA files but don't mark the alpha properly. */
#define DEFAULT_EXTRASAMPLE_AS_ALPHA 1

/* Pick up YCbCr subsampling info from the JPEG data stream to support files
   lacking the tag (default enabled). */
#define CHECK_JPEG_YCBCR_SUBSAMPLING 1

/* Support MS MDI magic number files as TIFF */
#define MDI_SUPPORT 1

/*
 * Feature support definitions.
 * XXX: These macros are obsoleted. Don't use them in your apps!
 * Macros stays here for backward compatibility and should be always defined.
 */
#define COLORIMETRY_SUPPORT
#define YCBCR_SUPPORT
#define CMYK_SUPPORT
#define ICC_SUPPORT
#define PHOTOSHOP_SUPPORT
#define IPTC_SUPPORT

#endif /* _TIFFCONF_ */

/* clang-format on */
