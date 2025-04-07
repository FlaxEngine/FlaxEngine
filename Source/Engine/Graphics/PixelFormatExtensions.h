// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "PixelFormat.h"

/// <summary>
/// Extensions to <see cref="PixelFormat"/>.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API PixelFormatExtensions
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(PixelFormatExtensions);
public:
    /// <summary>
    /// Initializes cache.
    /// </summary>
    static void Init();

public:
    /// <summary>
    /// Calculates the size of a <see cref="PixelFormat"/> in bytes.
    /// </summary>
    /// <param name="format">The Pixel format.</param>
    /// <returns>size of in bytes</returns>
    API_FUNCTION() FORCE_INLINE static int32 SizeInBytes(const PixelFormat format)
    {
        return SizeInBits(format) / 8;
    }

    /// <summary>
    /// Calculates the size of a <see cref="PixelFormat"/> in bits.
    /// </summary>
    /// <param name="format">The pixel format.</param>
    /// <returns>The size in bits</returns>
    API_FUNCTION() static int32 SizeInBits(PixelFormat format);

    /// <summary>
    /// Calculate the size of the alpha channel in bits depending on the pixel format.
    /// </summary>
    /// <param name="format">The pixel format</param>
    /// <returns>The size in bits</returns>
    API_FUNCTION() static int32 AlphaSizeInBits(PixelFormat format);

    /// <summary>
    /// Determines whether the specified <see cref="PixelFormat"/> contains alpha channel.
    /// </summary>
    /// <param name="format">The Pixel Format.</param>
    /// <returns><c>true</c> if the specified <see cref="PixelFormat"/> has alpha; otherwise, <c>false</c>.</returns>
    API_FUNCTION() FORCE_INLINE static bool HasAlpha(const PixelFormat format)
    {
        return AlphaSizeInBits(format) != 0;
    }

    /// <summary>
    /// Determines whether the specified <see cref="PixelFormat"/> is depth stencil.
    /// </summary>
    /// <param name="format">The Pixel Format.</param>
    /// <returns><c>true</c> if the specified <see cref="PixelFormat"/> is depth stencil; otherwise, <c>false</c>.</returns>
    API_FUNCTION() static bool IsDepthStencil(PixelFormat format);

    /// <summary>
    /// Determines whether the specified <see cref="PixelFormat"/> has stencil bits.
    /// </summary>
    /// <param name="format">The Pixel Format.</param>
    /// <returns><c>true</c> if the specified <see cref="PixelFormat"/> has stencil bits; otherwise, <c>false</c>.</returns>
    API_FUNCTION() static bool HasStencil(PixelFormat format);

    /// <summary>
    /// Determines whether the specified <see cref="PixelFormat"/> is Typeless.
    /// </summary>
    /// <param name="format">The <see cref="PixelFormat"/>.</param>
    /// <param name="partialTypeless">Enable/disable partially typeless formats.</param>
    /// <returns><c>true</c> if the specified <see cref="PixelFormat"/> is Typeless; otherwise, <c>false</c>.</returns>
    API_FUNCTION() static bool IsTypeless(PixelFormat format, bool partialTypeless = true);

    /// <summary>
    /// Returns true if the <see cref="PixelFormat"/> is valid.
    /// </summary>
    /// <param name="format">A format to validate</param>
    /// <returns>True if the <see cref="PixelFormat"/> is valid.</returns>
    API_FUNCTION() static bool IsValid(PixelFormat format);

    /// <summary>
    /// Returns true if the <see cref="PixelFormat"/> is a compressed format.
    /// </summary>
    /// <param name="format">The format to check for compressed.</param>
    /// <returns>True if the <see cref="PixelFormat"/> is a compressed format.</returns>
    API_FUNCTION() static bool IsCompressed(PixelFormat format);

    /// <summary>
    /// Returns true if the <see cref="PixelFormat"/> is a compressed format from BC formats family (BC1, BC2, BC3, BC4, BC5, BC6H, BC7).
    /// </summary>
    /// <param name="format">The format to check for compressed.</param>
    /// <returns>True if the <see cref="PixelFormat"/> is a compressed format from BC formats family.</returns>
    API_FUNCTION() static bool IsCompressedBC(PixelFormat format);

    /// <summary>
    /// Returns true if the <see cref="PixelFormat"/> is a compressed format from ASTC formats family (various block sizes).
    /// </summary>
    /// <param name="format">The format to check for compressed.</param>
    /// <returns>True if the <see cref="PixelFormat"/> is a compressed format from ASTC formats family.</returns>
    API_FUNCTION() static bool IsCompressedASTC(PixelFormat format);

    /// <summary>
    /// Determines whether the specified <see cref="PixelFormat"/> is video.
    /// </summary>
    /// <param name="format">The <see cref="PixelFormat"/>.</param>
    /// <returns><c>true</c> if the specified <see cref="PixelFormat"/> is video; otherwise, <c>false</c>.</returns>
    API_FUNCTION() static bool IsVideo(PixelFormat format);

    /// <summary>
    /// Determines whether the specified <see cref="PixelFormat"/> is a sRGB format.
    /// </summary>
    /// <param name="format">The <see cref="PixelFormat"/>.</param>
    /// <returns><c>true</c> if the specified <see cref="PixelFormat"/> is a sRGB format; otherwise, <c>false</c>.</returns>
    API_FUNCTION() static bool IsSRGB(PixelFormat format);

    /// <summary>
    /// Determines whether the specified <see cref="PixelFormat"/> is HDR (either 16 or 32bits Float)
    /// </summary>
    /// <param name="format">The format.</param>
    /// <returns><c>true</c> if the specified pixel format is HDR (Floating poInt); otherwise, <c>false</c>.</returns>
    API_FUNCTION() static bool IsHDR(PixelFormat format);

    /// <summary>
    /// Determines whether the specified format is in RGBA order.
    /// </summary>
    /// <param name="format">The format.</param>
    /// <returns><c>true</c> if the specified format is in RGBA order; otherwise, <c>false</c>.</returns>
    API_FUNCTION() static bool IsRgbAOrder(PixelFormat format);

    /// <summary>
    /// Determines whether the specified format is in BGRA order.
    /// </summary>
    /// <param name="format">The format.</param>
    /// <returns><c>true</c> if the specified format is in BGRA order; otherwise, <c>false</c>.</returns>
    API_FUNCTION() static bool IsBGRAOrder(PixelFormat format);

    /// <summary>
    /// Determines whether the specified format contains normalized data. It indicates that values stored in an integer format are to be mapped to the range [-1,1] (for signed values) or [0,1] (for unsigned values) when they are accessed and converted to floating point.
    /// </summary>
    /// <param name="format">The <see cref="PixelFormat"/>.</param>
    /// <returns>True if given format contains normalized data type, otherwise false.</returns>
    API_FUNCTION() static bool IsNormalized(PixelFormat format);

    /// <summary>
    /// Determines whether the specified format is integer data type (signed or unsigned).
    /// </summary>
    /// <param name="format">The <see cref="PixelFormat"/>.</param>
    /// <returns>True if given format contains integer data type (signed or unsigned), otherwise false.</returns>
    API_FUNCTION() static bool IsInteger(PixelFormat format);

    /// <summary>
    /// Computes the format components count (number of R, G, B, A channels).
    /// </summary>
    /// <param name="format">The <see cref="PixelFormat"/>.</param>
    /// <returns>The components count.</returns>
    API_FUNCTION() static int32 ComputeComponentsCount(PixelFormat format);

    /// <summary>
    /// Computes the amount of pixels per-axis stored in the a single block of the format (eg. 4 for BC-family). Returns 1 for uncompressed formats.
    /// </summary>
    /// <param name="format">The <see cref="PixelFormat"/>.</param>
    /// <returns>The block pixels count.</returns>
    API_FUNCTION() static int32 ComputeBlockSize(PixelFormat format);

    /// <summary>
    /// Finds the equivalent sRGB format to the provided format.
    /// </summary>
    /// <param name="format">The non sRGB format.</param>
    /// <returns>The equivalent sRGB format if any, the provided format else.</returns>
    API_FUNCTION() static PixelFormat TosRGB(PixelFormat format);

    /// <summary>
    /// Finds the equivalent non sRGB format to the provided sRGB format.
    /// </summary>
    /// <param name="format">The non sRGB format.</param>
    /// <returns>The equivalent non sRGB format if any, the provided format else.</returns>
    API_FUNCTION() static PixelFormat ToNonsRGB(PixelFormat format);

    /// <summary>
    /// Converts the format to typeless.
    /// </summary>
    /// <param name="format">The format.</param>
    /// <returns>The typeless format.</returns>
    API_FUNCTION() static PixelFormat MakeTypeless(PixelFormat format);

    /// <summary>
    /// Converts the typeless format to float.
    /// </summary>
    /// <param name="format">The typeless format.</param>
    /// <returns>The float format.</returns>
    API_FUNCTION() static PixelFormat MakeTypelessFloat(PixelFormat format);

    /// <summary>
    /// Converts the typeless format to unorm.
    /// </summary>
    /// <param name="format">The typeless format.</param>
    /// <returns>The unorm format.</returns>
    API_FUNCTION() static PixelFormat MakeTypelessUNorm(PixelFormat format);

public:
    static PixelFormat FindShaderResourceFormat(PixelFormat format, bool sRGB);
    static PixelFormat FindUnorderedAccessFormat(PixelFormat format);
    static PixelFormat FindDepthStencilFormat(PixelFormat format);
    static PixelFormat FindUncompressedFormat(PixelFormat format);

private:
    // Internal bindings
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) static void GetSamplerInternal(PixelFormat format, API_PARAM(Out) int32& pixelSize, API_PARAM(Out) void** read, API_PARAM(Out) void** write);
#endif
};
