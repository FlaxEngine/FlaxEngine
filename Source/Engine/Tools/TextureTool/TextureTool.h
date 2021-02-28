// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_TEXTURE_TOOL

#include "Engine/Render2D/SpriteAtlas.h"
#include "Engine/Graphics/Textures/Types.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Serialization/ISerializable.h"

class JsonWriter;

/// <summary>
/// Textures importing, processing and exporting utilities.
/// </summary>
class FLAXENGINE_API TextureTool
{
public:

    /// <summary>
    /// Importing texture options
    /// </summary>
    struct Options : public ISerializable
    {
        /// <summary>
        /// Texture format type
        /// </summary>
        TextureFormatType Type;

        /// <summary>
        /// True if texture should be imported as a texture atlas resource
        /// </summary>
        bool IsAtlas;

        /// <summary>
        /// True if disable dynamic texture streaming
        /// </summary>
        bool NeverStream;

        /// <summary>
        /// Enables/disables texture data compression.
        /// </summary>
        bool Compress;

        /// <summary>
        /// True if texture channels have independent data
        /// </summary>
        bool IndependentChannels;

        /// <summary>
        /// True if use sRGB format for texture data. Recommended for color maps and diffuse color textures.
        /// </summary>
        bool sRGB;

        /// <summary>
        /// True if generate mip maps chain for the texture.
        /// </summary>
        bool GenerateMipMaps;

        /// <summary>
        /// True if flip Y coordinate of the texture.
        /// </summary>
        bool FlipY;

        /// <summary>
        /// True if resize the texture.
        /// </summary>
        bool Resize;

        /// <summary>
        /// True if preserve alpha coverage in generated mips for alpha test reference. Scales mipmap alpha values to preserve alpha coverage based on an alpha test reference value.
        /// </summary>
        bool PreserveAlphaCoverage;

        /// <summary>
        /// The reference value for the alpha coverage preserving.
        /// </summary>
        float PreserveAlphaCoverageReference;

        /// <summary>
        /// The import texture scale.
        /// </summary>
        float Scale;

        /// <summary>
        /// Custom texture size X, use only if Resize texture flag is set.
        /// </summary>
        int32 SizeX;

        /// <summary>
        /// Custom texture size Y, use only if Resize texture flag is set.
        /// </summary>
        int32 SizeY;

        /// <summary>
        /// Maximum size of the texture (for both width and height).
        /// Higher resolution textures will be resized during importing process.
        /// </summary>
        int32 MaxSize;

        /// <summary>
        /// Function used for fast importing textures used by internal parts of the engine
        /// </summary>
        Function<bool(TextureData&)> InternalLoad;

        /// <summary>
        /// The sprites for the sprite sheet import mode.
        /// </summary>
        Array<Sprite> Sprites;

    public:

        /// <summary>
        /// Init
        /// </summary>
        Options();

        /// <summary>
        /// Gets string that contains information about options
        /// </summary>
        /// <returns>String</returns>
        String ToString() const;

    public:

        // [ISerializable]
        void Serialize(SerializeStream& stream, const void* otherObj) override;
        void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    };

public:

#if USE_EDITOR

    /// <summary>
    /// Checks whenever the given texture file contains alpha channel data with values different than solid fill of 1 (non fully opaque).
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <returns>True if has alpha channel, otherwise false.</returns>
    static bool HasAlpha(const StringView& path);

#endif

    /// <summary>
    /// Imports the texture.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="textureData">The output data.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ImportTexture(const StringView& path, TextureData& textureData);

    /// <summary>
    /// Imports the texture.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="textureData">The output data.</param>
    /// <param name="options">The import options.</param>
    /// <param name="errorMsg">The error message container.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ImportTexture(const StringView& path, TextureData& textureData, Options options, String& errorMsg);

    /// <summary>
    /// Exports the texture.
    /// </summary>
    /// <param name="path">The output file path.</param>
    /// <param name="textureData">The output data.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ExportTexture(const StringView& path, const TextureData& textureData);

    /// <summary>
    /// Converts the specified source texture data into an another format.
    /// </summary>
    /// <param name="dst">The destination data.</param>
    /// <param name="src">The source data.</param>
    /// <param name="dstFormat">The destination data format. Must be other than source data type.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Convert(TextureData& dst, const TextureData& src, const PixelFormat dstFormat);

    /// <summary>
    /// Resizes the specified source texture data into an another dimensions.
    /// </summary>
    /// <param name="dst">The destination data.</param>
    /// <param name="src">The source data.</param>
    /// <param name="dstWidth">The destination data width.</param>
    /// <param name="dstHeight">The destination data height.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool Resize(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight);

public:

    typedef Color (*ReadPixel)(const void*);
    typedef void (*WritePixel)(const void*, const Color&);

    struct PixelFormatSampler
    {
        PixelFormat Format;
        int32 PixelSize;
        ReadPixel Sample;
        WritePixel Store;
    };

    /// <summary>
    /// Determines whether this tool can sample the specified format to read texture samples and returns the sampler object.
    /// </summary>
    /// <param name="format">The format.</param>
    /// <returns>The pointer to the sampler object or null if cannot sample the given format.</returns>
    static const PixelFormatSampler* GetSampler(PixelFormat format);

    /// <summary>
    /// Stores the color into the specified texture data (uses no interpolation).
    /// </summary>
    /// <remarks>
    /// Use GetSampler for the texture sampler.
    /// </remarks>
    /// <param name="sampler">The texture data sampler.</param>
    /// <param name="x">The X texture coordinates (normalized to range 0-width).</param>
    /// <param name="y">The Y texture coordinates (normalized to range 0-height).</param>
    /// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
    /// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
    /// <param name="color">The color to store.</param>
    static void Store(const PixelFormatSampler* sampler, int32 x, int32 y, const void* data, int32 rowPitch, const Color& color);

    /// <summary>
    /// Samples the specified texture data (uses no interpolation).
    /// </summary>
    /// <remarks>
    /// Use GetSampler for the texture sampler.
    /// </remarks>
    /// <param name="sampler">The texture data sampler.</param>
    /// <param name="uv">The texture coordinates (normalized to range 0-1).</param>
    /// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
    /// <param name="size">The size of the input texture (in pixels).</param>
    /// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
    /// <returns>The sampled color.</returns>
    static Color SamplePoint(const PixelFormatSampler* sampler, const Vector2& uv, const void* data, const Int2& size, int32 rowPitch);

    /// <summary>
    /// Samples the specified texture data (uses no interpolation).
    /// </summary>
    /// <remarks>
    /// Use GetSampler for the texture sampler.
    /// </remarks>
    /// <param name="sampler">The texture data sampler.</param>
    /// <param name="x">The X texture coordinates (normalized to range 0-width).</param>
    /// <param name="y">The Y texture coordinates (normalized to range 0-height).</param>
    /// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
    /// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
    /// <returns>The sampled color.</returns>
    static Color SamplePoint(const PixelFormatSampler* sampler, int32 x, int32 y, const void* data, int32 rowPitch);

    /// <summary>
    /// Samples the specified texture data (uses linear interpolation).
    /// </summary>
    /// <remarks>
    /// Use GetSampler for the texture sampler.
    /// </remarks>
    /// <param name="sampler">The texture data sampler.</param>
    /// <param name="uv">The texture coordinates (normalized to range 0-1).</param>
    /// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
    /// <param name="size">The size of the input texture (in pixels).</param>
    /// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
    /// <returns>The sampled color.</returns>
    static Color SampleLinear(const PixelFormatSampler* sampler, const Vector2& uv, const void* data, const Int2& size, int32 rowPitch);

private:

    enum class ImageType
    {
        DDS,
        TGA,
        PNG,
        BMP,
        GIF,
        TIFF,
        JPEG,
        HDR,
        RAW,
        Internal,
    };

    static bool GetImageType(const StringView& path, ImageType& type);

#if COMPILE_WITH_DIRECTXTEX
    static bool ExportTextureDirectXTex(ImageType type, const StringView& path, const TextureData& textureData);
    static bool ImportTextureDirectXTex(ImageType type, const StringView& path, TextureData& textureData, bool& hasAlpha);
    static bool ImportTextureDirectXTex(ImageType type, const StringView& path, TextureData& textureData, const Options& options, String& errorMsg, bool& hasAlpha);
    static bool ConvertDirectXTex(TextureData& dst, const TextureData& src, const PixelFormat dstFormat);
    static bool ResizeDirectXTex(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight);
#endif
#if COMPILE_WITH_STB
    static bool ExportTextureStb(ImageType type, const StringView& path, const TextureData& textureData);
    static bool ImportTextureStb(ImageType type, const StringView& path, TextureData& textureData, bool& hasAlpha);
    static bool ResizeStb(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight);
#endif
};

#endif
