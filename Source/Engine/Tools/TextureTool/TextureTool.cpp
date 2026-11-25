// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_TEXTURE_TOOL

#include "TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Serialization/JsonWriter.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/PixelFormatSampler.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"

#if USE_EDITOR
#include "Engine/Core/Collections/Dictionary.h"
namespace
{
    Dictionary<String, bool> TexturesHasAlphaCache;
}
#endif

String TextureTool::Options::ToString() const
{
    return String::Format(TEXT("Type: {}, IsAtlas: {}, NeverStream: {}, IndependentChannels: {}, sRGB: {}, GenerateMipMaps: {}, FlipY: {}, InvertRed: {}, InvertGreen: {}, InvertBlue {}, Invert Alpha {}, Scale: {}, MaxSize: {}, Resize: {}, PreserveAlphaCoverage: {}, PreserveAlphaCoverageReference: {}, SizeX: {}, SizeY: {}"),
                          ScriptingEnum::ToString(Type),
                          IsAtlas,
                          NeverStream,
                          IndependentChannels,
                          sRGB,
                          GenerateMipMaps,
                          FlipY,
                          InvertRedChannel,
                          InvertGreenChannel,
                          InvertBlueChannel,
                          InvertAlphaChannel,
                          Scale,
                          MaxSize,
                          MaxSize,
                          Resize,
                          PreserveAlphaCoverage,
                          PreserveAlphaCoverageReference,
                          SizeX,
                          SizeY
    );
}

void TextureTool::Options::Serialize(SerializeStream& stream, const void* otherObj)
{
    stream.JKEY("Type");
    stream.Enum(Type);

    stream.JKEY("IsAtlas");
    stream.Bool(IsAtlas);

    stream.JKEY("NeverStream");
    stream.Bool(NeverStream);

    stream.JKEY("Compress");
    stream.Bool(Compress);

    stream.JKEY("IndependentChannels");
    stream.Bool(IndependentChannels);

    stream.JKEY("sRGB");
    stream.Bool(sRGB);

    stream.JKEY("GenerateMipMaps");
    stream.Bool(GenerateMipMaps);

    stream.JKEY("FlipY");
    stream.Bool(FlipY);

    stream.JKEY("FlipX");
    stream.Bool(FlipX);

    stream.JKEY("InvertRedChannel");
    stream.Bool(InvertRedChannel);

    stream.JKEY("InvertGreenChannel");
    stream.Bool(InvertGreenChannel);

    stream.JKEY("InvertBlueChannel");
    stream.Bool(InvertBlueChannel);

    stream.JKEY("InvertAlphaChannel");
    stream.Bool(InvertAlphaChannel);

    stream.JKEY("ReconstructZChannel");
    stream.Bool(ReconstructZChannel);

    stream.JKEY("Resize");
    stream.Bool(Resize);

    stream.JKEY("KeepAspectRatio");
    stream.Bool(KeepAspectRatio);

    stream.JKEY("PreserveAlphaCoverage");
    stream.Bool(PreserveAlphaCoverage);

    stream.JKEY("PreserveAlphaCoverageReference");
    stream.Float(PreserveAlphaCoverageReference);

    stream.JKEY("TextureGroup");
    stream.Int(TextureGroup);

    stream.JKEY("Scale");
    stream.Float(Scale);

    stream.JKEY("MaxSize");
    stream.Int(MaxSize);

    stream.JKEY("SizeX");
    stream.Int(SizeX);

    stream.JKEY("SizeY");
    stream.Int(SizeY);

    stream.JKEY("Sprites");
    stream.StartArray();
    for (int32 i = 0; i < Sprites.Count(); i++)
    {
        auto& s = Sprites[i];

        stream.StartObject();

        stream.JKEY("Position");
        stream.Float2(s.Area.Location);

        stream.JKEY("Size");
        stream.Float2(s.Area.Size);

        stream.JKEY("Name");
        stream.String(s.Name);

        stream.EndObject();
    }
    stream.EndArray();
}

void TextureTool::Options::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Restore general import options
    Type = JsonTools::GetEnum(stream, "Type", Type);
    IsAtlas = JsonTools::GetBool(stream, "IsAtlas", IsAtlas);
    NeverStream = JsonTools::GetBool(stream, "NeverStream", NeverStream);
    Compress = JsonTools::GetBool(stream, "Compress", Compress);
    IndependentChannels = JsonTools::GetBool(stream, "IndependentChannels", IndependentChannels);
    sRGB = JsonTools::GetBool(stream, "sRGB", sRGB);
    GenerateMipMaps = JsonTools::GetBool(stream, "GenerateMipMaps", GenerateMipMaps);
    FlipY = JsonTools::GetBool(stream, "FlipY", FlipY);
    FlipX = JsonTools::GetBool(stream, "FlipX", FlipX);
    InvertRedChannel = JsonTools::GetBool(stream, "InvertRedChannel", InvertRedChannel);
    InvertGreenChannel = JsonTools::GetBool(stream, "InvertGreenChannel", InvertGreenChannel);
    InvertBlueChannel = JsonTools::GetBool(stream, "InvertBlueChannel", InvertBlueChannel);
    InvertAlphaChannel = JsonTools::GetBool(stream, "InvertAlphaChannel", InvertAlphaChannel);
    ReconstructZChannel = JsonTools::GetBool(stream, "ReconstructZChannel", ReconstructZChannel);
    Resize = JsonTools::GetBool(stream, "Resize", Resize);
    KeepAspectRatio = JsonTools::GetBool(stream, "KeepAspectRatio", KeepAspectRatio);
    PreserveAlphaCoverage = JsonTools::GetBool(stream, "PreserveAlphaCoverage", PreserveAlphaCoverage);
    PreserveAlphaCoverageReference = JsonTools::GetFloat(stream, "PreserveAlphaCoverageReference", PreserveAlphaCoverageReference);
    TextureGroup = JsonTools::GetInt(stream, "TextureGroup", TextureGroup);
    Scale = JsonTools::GetFloat(stream, "Scale", Scale);
    SizeX = JsonTools::GetInt(stream, "SizeX", SizeX);
    SizeY = JsonTools::GetInt(stream, "SizeY", SizeY);
    MaxSize = JsonTools::GetInt(stream, "MaxSize", MaxSize);

    // Load sprites
    // Note: we use it if no sprites in texture header has been loaded earlier
    auto* spritesMember = stream.FindMember("Sprites");
    if (spritesMember != stream.MemberEnd() && Sprites.IsEmpty())
    {
        auto& spritesArray = spritesMember->value;
        ASSERT(spritesArray.IsArray());
        Sprites.EnsureCapacity(spritesArray.Size());

        for (uint32 i = 0; i < spritesArray.Size(); i++)
        {
            Sprite s;
            auto& stData = spritesArray[i];

            s.Area.Location = JsonTools::GetFloat2(stData, "Position", Float2::Zero);
            s.Area.Size = JsonTools::GetFloat2(stData, "Size", Float2::One);
            s.Name = JsonTools::GetString(stData, "Name");

            Sprites.Add(s);
        }
    }
}

#if USE_EDITOR

bool TextureTool::HasAlpha(const StringView& path)
{
    // Try to hit the cache (eg. if texture was already imported before)
    if (!TexturesHasAlphaCache.ContainsKey(path))
    {
        TextureData textureData;
        if (ImportTexture(path, textureData))
            return false;
    }
    return TexturesHasAlphaCache[path];
}

#endif

bool TextureTool::ImportTexture(const StringView& path, TextureData& textureData)
{
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);
    LOG(Info, "Importing texture from \'{0}\'", path);
    const auto startTime = DateTime::NowUTC();

    // Detect texture format type
    ImageType type;
    if (GetImageType(path, type))
        return true;

    // Load raw bytes from file
    Array<byte> bytes;
    if (File::ReadAllBytes(path, bytes))
    {
        LOG(Warning, "Failed to load file data.");
        return true;
    }

    // Import
    bool hasAlpha = false;
#if COMPILE_WITH_DIRECTXTEX
    const auto failed = ImportTextureDirectXTex(type, ToSpan(bytes), textureData, hasAlpha);
#elif COMPILE_WITH_STB
    const auto failed = ImportTextureStb(type, ToSpan(bytes), textureData, hasAlpha);
#else
    const auto failed = true;
    LOG(Warning, "Importing textures is not supported on this platform.");
#endif

    if (failed)
    {
        LOG(Warning, "Importing texture failed.");
    }
    else
    {
#if USE_EDITOR
        TexturesHasAlphaCache[path] = hasAlpha;
#endif
        LOG(Info, "Texture imported in {0} ms", static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()));
    }

    return failed;
}

bool TextureTool::ImportTexture(const StringView& path, TextureData& textureData, Options options, String& errorMsg)
{
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);
    LOG(Info, "Importing texture from \'{0}\'. Options: {1}", path, options.ToString());
    const auto startTime = DateTime::NowUTC();

    // Detect texture format type
    ImageType type;
    Array<byte> bytes;
    if (options.InternalLoad.IsBinded())
    {
        type = ImageType::Internal;
    }
    else
    {
        if (GetImageType(path, type))
            return true;

        // Load raw bytes from file
        if (File::ReadAllBytes(path, bytes))
        {
            LOG(Warning, "Failed to load file data.");
            return true;
        }
    }

    // Clamp values
    options.MaxSize = Math::Clamp(options.MaxSize, 1, GPU_MAX_TEXTURE_SIZE);
    options.SizeX = Math::Clamp(options.SizeX, 1, GPU_MAX_TEXTURE_SIZE);
    options.SizeY = Math::Clamp(options.SizeY, 1, GPU_MAX_TEXTURE_SIZE);

    // Import
    bool hasAlpha = false;
#if COMPILE_WITH_DIRECTXTEX
    const auto failed = ImportTextureDirectXTex(type, ToSpan(bytes), textureData, options, errorMsg, hasAlpha);
#elif COMPILE_WITH_STB
    const auto failed = ImportTextureStb(type, ToSpan(bytes), textureData, options, errorMsg, hasAlpha);
#else
    const auto failed = true;
    LOG(Warning, "Importing textures is not supported on this platform.");
#endif

    if (failed)
    {
        LOG(Warning, "Importing texture failed. {0}", errorMsg);
    }
    else
    {
#if USE_EDITOR
        TexturesHasAlphaCache[path] = hasAlpha;
#endif
        LOG(Info, "Texture imported in {0} ms", static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()));
    }

    return failed;
}

bool TextureTool::ImportTexture(const Span<byte>& data, ImageType type, TextureData& textureData)
{
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);
    bool hasAlpha = false;
#if COMPILE_WITH_DIRECTXTEX
    const auto failed = ImportTextureDirectXTex(type, data, textureData, hasAlpha);
#elif COMPILE_WITH_STB
    const auto failed = ImportTextureStb(type, data, textureData, hasAlpha);
#else
    const auto failed = true;
    LOG(Warning, "Importing textures is not supported on this platform.");
#endif
    return failed;
}

bool TextureTool::ExportTexture(const StringView& path, const TextureData& textureData)
{
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);
    LOG(Info, "Exporting texture to \'{0}\'.", path);
    const auto startTime = DateTime::NowUTC();
    ImageType type;
    if (GetImageType(path, type))
        return true;
    if (textureData.Items.IsEmpty())
    {
        LOG(Warning, "Missing texture data.");
        return true;
    }

#if COMPILE_WITH_DIRECTXTEX
    const auto failed = ExportTextureDirectXTex(type, path, textureData);
#elif COMPILE_WITH_STB
    const auto failed = ExportTextureStb(type, path, textureData);
#else
    const auto failed = true;
    LOG(Warning, "Exporting textures is not supported on this platform.");
#endif

    if (failed)
    {
        LOG(Warning, "Exporting failed.");
    }
    else
    {
        LOG(Info, "Texture exported in {0} ms", static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()));
    }

    return failed;
}

bool TextureTool::Convert(TextureData& dst, const TextureData& src, const PixelFormat dstFormat)
{
    if (src.GetMipLevels() == 0)
    {
        LOG(Warning, "Missing source data.");
        return true;
    }
    if (src.Format == dstFormat)
    {
        LOG(Warning, "Source data and destination format are the same. Cannot perform conversion.");
        return true;
    }
    if (src.Depth != 1)
    {
        LOG(Warning, "Converting volume texture data is not supported.");
        return true;
    }
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);

#if COMPILE_WITH_DIRECTXTEX
    return ConvertDirectXTex(dst, src, dstFormat);
#elif COMPILE_WITH_STB
    return ConvertStb(dst, src, dstFormat);
#else
    LOG(Warning, "Converting textures is not supported on this platform.");
    return true;
#endif
}

static void CopyRows(byte* dst, const byte* src, uint32 dstRowPitch, uint32 srcRowPitch, uint32 lines)
{
    if (srcRowPitch == dstRowPitch)
        Platform::MemoryCopy(dst, src, dstRowPitch * lines);
    else
    {
        uint32 rowPitch = Math::Min(dstRowPitch, srcRowPitch);;
        for (uint32 row = 0; row < lines; row++)
        {
            Platform::MemoryCopy(dst, src, rowPitch);
            dst += dstRowPitch;
            src += srcRowPitch;
        }
    }
}

bool TextureTool::Convert(int32 width, int32 height, PixelFormat dstFormat, Span<byte> dstData, uint32 dstRowPitch, PixelFormat srcFormat, const Span<byte> srcData, uint32 srcRowPitch)
{
    byte* dst = dstData.Get();
    const byte* src = srcData.Get();
    uint32 dstLines = dstData.Length() / dstRowPitch;
    uint32 srcLines = srcData.Length() / srcRowPitch;

    // Color32 source
    if (srcFormat == PixelFormat::R8G8B8A8_UNorm)
    {
        switch (dstFormat)
        {
        case PixelFormat::R8G8B8A8_SInt:
        case PixelFormat::R8G8B8A8_Typeless:
        case PixelFormat::R8G8B8A8_SNorm:
        case PixelFormat::R8G8B8A8_UInt:
        case PixelFormat::R8G8B8A8_UNorm:
        case PixelFormat::R8G8B8A8_UNorm_sRGB:
        case PixelFormat::R8G8_B8G8_UNorm:
        case PixelFormat::B8G8R8A8_Typeless:
        case PixelFormat::B8G8R8A8_UNorm:
        case PixelFormat::B8G8R8A8_UNorm_sRGB:
        case PixelFormat::B8G8R8X8_Typeless:
        case PixelFormat::B8G8R8X8_UNorm:
        case PixelFormat::B8G8R8X8_UNorm_sRGB:
            if (srcLines != dstLines)
                return true;
            CopyRows(dst, src, dstRowPitch, srcRowPitch, srcLines);
            if (PixelFormatExtensions::IsBGRAOrder(dstFormat))
            {
                // BGRA -> RGBA
                for (int32 y = 0; y < height; y++)
                {
                    for (int32 x = 0; x < width; x++)
                    {
                        auto& color = *((Color32*)(dst + y * dstRowPitch + x * sizeof(Color32)));
                        color = Color32(color.B, color.G, color.R, color.A);
                    }
                }
            }
            if (PixelFormatExtensions::IsSRGB(dstFormat))
            {
                // sRGB -> Linear
                for (int32 y = 0; y < height; y++)
                {
                    for (int32 x = 0; x < width; x++)
                    {
                        auto& color = *((Color32*)(dst + y * dstRowPitch + x * sizeof(Color32)));
                        color = Color32(Color::SrgbToLinear(Color(color)));
                    }
                }
            }
            return false;
        }
    }

    // Color source
    if (srcFormat == PixelFormat::R32G32B32A32_Float)
    {
        switch (dstFormat)
        {
        case PixelFormat::R32G32B32A32_Typeless:
        case PixelFormat::R32G32B32A32_Float:
            if (srcLines != dstLines)
                return true;
            CopyRows(dst, src, dstRowPitch, srcRowPitch, srcLines);
            return false;
        }
    }

    // Conversion with an in-built samplers
    auto srcSampler = PixelFormatSampler::Get(srcFormat);
    auto dstsampler = PixelFormatSampler::Get(dstFormat);
    if (srcSampler && dstsampler)
    {
        auto tempRowPitch = width * dstsampler->PixelSize;
        auto tempSlicePitch = tempRowPitch * dstLines;
        if (dstData.Length() / dstRowPitch < (uint32)height)
            return true;
        for (int32 y = 0; y < height; y++)
        {
            for (int32 x = 0; x < width; x++)
            {
                Color color = srcSampler->SamplePoint(src, x, y, srcRowPitch);
                dstsampler->Store(dst, x, y, tempRowPitch, color);
            }
        }
        return false;
    }

    // TODO: this could use Convert with TextureData wrapper to decompress textures (just like TextureTool::UpdateTexture does)

    if (PixelFormatExtensions::IsCompressed(dstFormat))
        LOG(Error, "Unsupported texture data format {0}. Compressed texture data cannot be accessed directly without decompressing first.", ScriptingEnum::ToString(dstFormat));
    else
        LOG(Error, "Unsupported texture data format {0}.", ScriptingEnum::ToString(dstFormat));
    return true;
}

bool TextureTool::GenerateMip(int32 width, int32 height, PixelFormat format, Span<byte> data, uint32 dataRowPitch, Span<byte> mipData, uint32 mipRowPitch, bool linear, int32 arraySize)
{
    const int32 mipWidth = Math::Max(1, width >> 1);
    const int32 mipHeight = Math::Max(1, height >> 1);
    const int32 pixelStride = PixelFormatExtensions::SizeInBytes(format);
    const uint32 dataSlicePitch = data.Length() / arraySize;
    const uint32 mipSlicePitch = mipData.Length() / arraySize;
    if (linear)
    {
        // Linear downscale filter
        switch (format)
        {
        // 4 component, 32 bit with 8 bits per component - use Color32 type
        case PixelFormat::R8G8B8A8_SInt:
        case PixelFormat::R8G8B8A8_Typeless:
        case PixelFormat::R8G8B8A8_SNorm:
        case PixelFormat::R8G8B8A8_UInt:
        case PixelFormat::R8G8B8A8_UNorm:
        case PixelFormat::R8G8B8A8_UNorm_sRGB:
        case PixelFormat::R8G8_B8G8_UNorm:
        case PixelFormat::B8G8R8A8_Typeless:
        case PixelFormat::B8G8R8A8_UNorm:
        case PixelFormat::B8G8R8A8_UNorm_sRGB:
        case PixelFormat::B8G8R8X8_Typeless:
        case PixelFormat::B8G8R8X8_UNorm:
        case PixelFormat::B8G8R8X8_UNorm_sRGB:
        {
            for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
            {
                const byte* dstData = mipData.Get() + arrayIndex * dataSlicePitch;
                const byte* srcData = data.Get() + arrayIndex * mipSlicePitch;
                for (int32 y = 0; y < mipHeight; y++)
                {
                    for (int32 x = 0; x < mipWidth; x++)
                    {
                        const auto dstIndex = y * mipRowPitch + x * pixelStride;
#define SAMPLE(var, dx, dy) const auto var = (Color)*(Color32*)(srcData + ((y * 2 + (dy)) * dataRowPitch + x * pixelStride * 2 + (pixelStride * (dx))))
                        SAMPLE(v00, 0, 0);
                        SAMPLE(v01, 0, 1);
                        SAMPLE(v10, 1, 0);
                        SAMPLE(v11, 1, 1);
#undef SAMPLE
                        *(Color32*)(dstData + dstIndex) = Color32((v00 + v01 + v10 + v11) * 0.25f);
                    }
                }
            }
            break;
        }
        // 4 component, 128 bit with 32 bits per component - use Color type
        case PixelFormat::R32G32B32A32_Float:
        case PixelFormat::R32G32B32A32_Typeless:
        {
            for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
            {
                const byte* dstData = mipData.Get() + arrayIndex * dataSlicePitch;
                const byte* srcData = data.Get() + arrayIndex * mipSlicePitch;
                for (int32 y = 0; y < mipHeight; y++)
                {
                    for (int32 x = 0; x < mipWidth; x++)
                    {
                        const auto dstIndex = y * mipRowPitch + x * pixelStride;
#define SAMPLE(var, dx, dy) const auto var = *(Color*)(srcData + ((y * 2 + (dy)) * dataRowPitch + x * pixelStride * 2 + (pixelStride * (dx))))
                        SAMPLE(v00, 0, 0);
                        SAMPLE(v01, 0, 1);
                        SAMPLE(v10, 1, 0);
                        SAMPLE(v11, 1, 1);
#undef SAMPLE
                        *(Color*)(dstData + dstIndex) = (v00 + v01 + v10 + v11) * 0.25f;
                    }
                }
            }
            break;
        }
        default:
            LOG(Error, "Unsupported texture data format {0}.", ScriptingEnum::ToString(format));
            return true;
        }
    }
    else
    {
        // Point downscale filter
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            byte* dstData = mipData.Get() + arrayIndex * dataSlicePitch;
            const byte* srcData = data.Get() + arrayIndex * mipSlicePitch;
            for (int32 y = 0; y < mipHeight; y++)
            {
                for (int32 x = 0; x < mipWidth; x++)
                {
                    const auto dstIndex = y * mipRowPitch + x * pixelStride;
                    const auto srcIndex = y * 2 * dataRowPitch + x * pixelStride * 2;
                    Platform::MemoryCopy(dstData + dstIndex, srcData + srcIndex, pixelStride);
                }
            }
        }

        // Fix right and bottom edges to preserve the original values
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            byte* dstData = mipData.Get() + arrayIndex * dataSlicePitch;
            const byte* srcData = data.Get() + arrayIndex * mipSlicePitch;
            for (int32 y = 0; y < mipHeight - 1; y++)
            {
                const int32 x = mipWidth - 1;
                const auto dstIndex = y * mipRowPitch + x * pixelStride;
                const auto srcIndex = y * 2 * dataRowPitch + x * pixelStride * 2 + pixelStride;
                Platform::MemoryCopy(dstData + dstIndex, srcData + srcIndex, pixelStride);
            }
            for (int32 x = 0; x < mipWidth - 1; x++)
            {
                const int32 y = mipHeight - 1;
                const auto dstIndex = y * mipRowPitch + x * pixelStride;
                const auto srcIndex = (y * 2 + 1) * dataRowPitch + x * pixelStride * 2;
                Platform::MemoryCopy(dstData + dstIndex, srcData + srcIndex, pixelStride);
            }
            {
                const int32 x = mipWidth - 1;
                const int32 y = mipHeight - 1;
                const auto dstIndex = y * mipRowPitch + x * pixelStride;
                const auto srcIndex = (y * 2 + 1) * dataRowPitch + x * pixelStride * 2 + pixelStride;
                Platform::MemoryCopy(dstData + dstIndex, srcData + srcIndex, pixelStride);
            }
        }
    }

    return false;
}

bool TextureTool::Resize(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight)
{
    if (src.GetMipLevels() == 0)
    {
        LOG(Warning, "Missing source data.");
        return true;
    }
    if (src.Width == dstWidth && src.Height == dstHeight)
    {
        LOG(Warning, "Source data and destination dimensions are the same. Cannot perform resizing.");
        return true;
    }
    if (src.Depth != 1)
    {
        LOG(Warning, "Resizing volume texture data is not supported.");
        return true;
    }
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);
#if COMPILE_WITH_DIRECTXTEX
    return ResizeDirectXTex(dst, src, dstWidth, dstHeight);
#elif COMPILE_WITH_STB
    return ResizeStb(dst, src, dstWidth, dstHeight);
#else
    LOG(Warning, "Resizing textures is not supported on this platform.");
    return true;
#endif
}

bool TextureTool::UpdateTexture(GPUContext* context, GPUTexture* texture, int32 arrayIndex, int32 mipIndex, Span<byte> data, uint32 rowPitch, uint32 slicePitch, PixelFormat dataFormat)
{
    PROFILE_MEM(GraphicsTextures);
    PixelFormat textureFormat = texture->Format();

    // Basis Universal data transcoded into the runtime GPU format (supercompressed texture)
    if (dataFormat == PixelFormat::Basis)
    {
#if COMPILE_WITH_BASISU
        return UpdateTextureBasisUniversal(context, texture, arrayIndex, mipIndex, data, rowPitch, slicePitch, dataFormat);
#else
        LOG(Error, "Loading Basis Universal textures is not supported on this platform.");
#endif
    }

    // Try converting texture on the fly (slow)
    Array<byte> tempData;
    if (textureFormat != dataFormat)
    {
        PROFILE_CPU_NAMED("ConvertTexture");

        int32 mipWidth, mipHeight, mipDepth;
        texture->GetMipSize(mipIndex, mipWidth, mipHeight, mipDepth);

        auto dataSampler = PixelFormatSampler::Get(dataFormat);
        auto textureSampler = PixelFormatSampler::Get(textureFormat);
        if (dataSampler && textureSampler)
        {
            // Conversion with an in-built samplers
            auto tempRowPitch = mipWidth * textureSampler->PixelSize;
            auto tempSlicePitch = tempRowPitch * mipHeight;
            tempData.Resize(tempSlicePitch * mipDepth);
            ASSERT(data.Length() / rowPitch >= (uint32)mipHeight);
            for (int32 y = 0; y < mipHeight; y++)
            {
                for (int32 x = 0; x < mipWidth; x++)
                {
                    Color color = dataSampler->SamplePoint(data.Get(), x, y, rowPitch);
                    textureSampler->Store(tempData.Get(), x, y, tempRowPitch, color);
                }
            }
            data = ToSpan(tempData);
            rowPitch = tempRowPitch;
            slicePitch = tempSlicePitch;
        }
        else
        {
            // Conversion with external library
            TextureData src, dst;
            src.Width = mipWidth;
            src.Height = mipHeight;
            src.Depth = mipDepth;
            src.Format = dataFormat;
            auto& srcItem = src.Items.AddOne();
            auto& srcMip = srcItem.Mips.AddOne();
            srcMip.RowPitch = rowPitch;
            srcMip.DepthPitch = slicePitch;
            srcMip.Lines = slicePitch / rowPitch;
            srcMip.Data.Link(data);
            if (Convert(dst, src, textureFormat))
                return true;
            auto& dstMip = dst.Items[0].Mips[0];
            tempData.Set(dstMip.Data.Get(), dstMip.Data.Length());
            data = ToSpan(tempData);
            rowPitch = dstMip.RowPitch;
            slicePitch = dstMip.DepthPitch;
        }
    }

    // Update texture
    CHECK_RETURN_DEBUG((uint32)data.Length() >= slicePitch, true);
    context->UpdateTexture(texture, arrayIndex, mipIndex, data.Get(), rowPitch, slicePitch);
    return false;
}

PixelFormat TextureTool::GetTextureFormat(TextureFormatType textureType, PixelFormat dataFormat, int32 width, int32 height, bool sRGB)
{
    ASSERT(GPUDevice::Instance);
    constexpr auto minSupport = FormatSupport::Texture2D | FormatSupport::ShaderSample | FormatSupport::Mip;
#define CHECK_BLOCK_SIZE(x, y) (width % x == 0 && height % y == 0)

    // Basis Universal data transcoded into the runtime GPU format (supercompressed texture)
    if (dataFormat == PixelFormat::Basis)
    {
        // Check ASTC formats
        if (EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(PixelFormat::ASTC_4x4_UNorm).Support, minSupport) && CHECK_BLOCK_SIZE(4, 4))
            return sRGB ? PixelFormat::ASTC_4x4_UNorm_sRGB : PixelFormat::ASTC_4x4_UNorm;
        if (EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(PixelFormat::ASTC_6x6_UNorm).Support, minSupport) && CHECK_BLOCK_SIZE(6, 6))
            return sRGB ? PixelFormat::ASTC_6x6_UNorm_sRGB : PixelFormat::ASTC_6x6_UNorm;
        if (EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(PixelFormat::ASTC_8x8_UNorm).Support, minSupport) && CHECK_BLOCK_SIZE(8, 8))
            return sRGB ? PixelFormat::ASTC_8x8_UNorm_sRGB : PixelFormat::ASTC_8x8_UNorm;
        if (EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(PixelFormat::ASTC_10x10_UNorm).Support, minSupport) && CHECK_BLOCK_SIZE(10, 10))
            return sRGB ? PixelFormat::ASTC_10x10_UNorm_sRGB : PixelFormat::ASTC_10x10_UNorm;

        // Check BCn formats
        if (EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(PixelFormat::BC3_UNorm).Support, minSupport) && CHECK_BLOCK_SIZE(4, 4))
        {
            switch (textureType)
            {
            case TextureFormatType::ColorRGB:
                return sRGB ? PixelFormat::BC1_UNorm_sRGB : PixelFormat::BC1_UNorm;
            case TextureFormatType::ColorRGBA:
                return sRGB ? PixelFormat::BC3_UNorm_sRGB : PixelFormat::BC3_UNorm;
            case TextureFormatType::NormalMap:
                return PixelFormat::BC5_UNorm;
            case TextureFormatType::GrayScale:
                return PixelFormat::BC4_UNorm;
            // Basic Universal doesn't support alpha in BC7 (and it can be loaded only from LDR formats)
            /*case TextureFormatType::HdrRGBA:
                return PixelFormat::BC7_UNorm;*/
            case TextureFormatType::HdrRGB:
                return PixelFormat::BC6H_Uf16;
            }
        }

        // Use raw uncompressed as fallback
        switch (textureType)
        {
        case TextureFormatType::ColorRGB:
        case TextureFormatType::ColorRGBA:
            return sRGB ? PixelFormat::R8G8B8A8_UNorm_sRGB : PixelFormat::R8G8B8A8_UNorm;
        case TextureFormatType::NormalMap:
            return PixelFormat::R8G8B8A8_UNorm;
        case TextureFormatType::GrayScale:
            return PixelFormat::R8G8B8A8_UNorm;
        case TextureFormatType::HdrRGBA:
        case TextureFormatType::HdrRGB:
            return PixelFormat::R16G16B16A16_Float;
        default:
            return PixelFormat::Unknown;
        }
    }

    // Check if the data format can be used 'as-is'
    if (EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(dataFormat).Support, minSupport))
        return dataFormat;

    // Check if data can be converted at runtime (with perf cost)
    if ((dataFormat == PixelFormat::R16_UNorm || dataFormat == PixelFormat::R16_SNorm || dataFormat == PixelFormat::R16_Float) &&
        EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(PixelFormat::R32_Float).Support, minSupport) &&
        PixelFormatSampler::Get(dataFormat))
        return PixelFormat::R32_Float;
    if (dataFormat == PixelFormat::R16G16_UNorm && 
        EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(PixelFormat::R32G32_Float).Support, minSupport) &&
        PixelFormatSampler::Get(dataFormat))
        return PixelFormat::R32G32_Float;
    if ((dataFormat == PixelFormat::R16G16B16A16_UNorm || dataFormat == PixelFormat::R16G16B16A16_Float) &&
        EnumHasAllFlags(GPUDevice::Instance->GetFormatFeatures(PixelFormat::R32G32B32A32_Float).Support, minSupport) &&
        PixelFormatSampler::Get(dataFormat))
        return PixelFormat::R32G32B32A32_Float;

#undef CHECK_BLOCK_SIZE

    // Every potato supports rgba8
    return sRGB ? PixelFormat::R8G8B8A8_UNorm_sRGB : PixelFormat::R8G8B8A8_UNorm;
}

PixelFormat TextureTool::ToPixelFormat(TextureFormatType format, int32 width, int32 height, bool canCompress)
{
    const bool canUseBlockCompression = width % 4 == 0 && height % 4 == 0;
    if (canCompress && canUseBlockCompression)
    {
        switch (format)
        {
        case TextureFormatType::ColorRGB:
            return PixelFormat::BC1_UNorm;
        case TextureFormatType::ColorRGBA:
            return PixelFormat::BC3_UNorm;
        case TextureFormatType::NormalMap:
            return PixelFormat::BC5_UNorm;
        case TextureFormatType::GrayScale:
            return PixelFormat::BC4_UNorm;
        case TextureFormatType::HdrRGBA:
            return PixelFormat::BC7_UNorm;
        case TextureFormatType::HdrRGB:
#if PLATFORM_LINUX
            // TODO: support BC6H compression for Linux Editor
            return PixelFormat::BC7_UNorm;
#else
            return PixelFormat::BC6H_Uf16;
#endif
        default:
            return PixelFormat::Unknown;
        }
    }

    switch (format)
    {
    case TextureFormatType::ColorRGB:
        return PixelFormat::R8G8B8A8_UNorm;
    case TextureFormatType::ColorRGBA:
        return PixelFormat::R8G8B8A8_UNorm;
    case TextureFormatType::NormalMap:
        return PixelFormat::R16G16_UNorm;
    case TextureFormatType::GrayScale:
        return PixelFormat::R8_UNorm;
    case TextureFormatType::HdrRGBA:
        return PixelFormat::R16G16B16A16_Float;
    case TextureFormatType::HdrRGB:
        return PixelFormat::R11G11B10_Float;
    default:
        return PixelFormat::Unknown;
    }
}

#if USE_EDITOR

bool TextureTool::WriteTextureData(BytesContainer& result, const TextureData& textureData, int32 mipIndex)
{
    if (textureData.Format == PixelFormat::Basis)
    {
        // Store as-is, each slice is stored in a separate block with the same size
        int32 maxDataSize = 0;
        for (int32 arrayIndex = 0; arrayIndex < textureData.Items.Count(); arrayIndex++)
        {
            auto& mipData = textureData.Items[arrayIndex].Mips[mipIndex];
            maxDataSize = Math::Max(maxDataSize, mipData.Data.Length());
        }
        result.Allocate(maxDataSize * textureData.GetArraySize());
        for (int32 arrayIndex = 0; arrayIndex < textureData.Items.Count(); arrayIndex++)
        {
            auto& mipData = textureData.Items[arrayIndex].Mips[mipIndex];
            byte* dst = result.Get() + maxDataSize * arrayIndex;
            Platform::MemoryCopy(dst, mipData.Data.Get(), mipData.Data.Length());
            Platform::MemoryClear(dst + mipData.Data.Length(), maxDataSize - mipData.Data.Length());
        }
        return false;
    }

    // Calculate the texture data storage layout
    uint32 rowPitch, slicePitch;
    const int32 mipWidth = Math::Max(1, textureData.Width >> mipIndex);
    const int32 mipHeight = Math::Max(1, textureData.Height >> mipIndex);
    RenderTools::ComputePitch(textureData.Format, mipWidth, mipHeight, rowPitch, slicePitch);
    result.Allocate(slicePitch * textureData.GetArraySize());

    // Copy array slices into mip data (sequential)
    for (int32 arrayIndex = 0; arrayIndex < textureData.Items.Count(); arrayIndex++)
    {
        auto& mipData = textureData.Items[arrayIndex].Mips[mipIndex];
        const byte* src = mipData.Data.Get();
        byte* dst = result.Get() + (slicePitch * arrayIndex);

        // Faster path if source and destination data layout matches
        if (rowPitch == mipData.RowPitch && slicePitch == mipData.DepthPitch)
        {
            Platform::MemoryCopy(dst, src, slicePitch);
        }
        else
        {
            const uint32 copyRowSize = Math::Min(mipData.RowPitch, rowPitch);
            for (uint32 line = 0; line < mipData.Lines; line++)
            {
                Platform::MemoryCopy(dst, src, copyRowSize);
                src += mipData.RowPitch;
                dst += rowPitch;
            }
        }
    }

    return false;
}

#endif

bool TextureTool::GetImageType(const StringView& path, ImageType& type)
{
    const auto extension = FileSystem::GetExtension(path).ToLower();
    if (extension == TEXT("tga"))
    {
        type = ImageType::TGA;
    }
    else if (extension == TEXT("dds"))
    {
        type = ImageType::DDS;
    }
    else if (extension == TEXT("png"))
    {
        type = ImageType::PNG;
    }
    else if (extension == TEXT("bmp"))
    {
        type = ImageType::BMP;
    }
    else if (extension == TEXT("gif"))
    {
        type = ImageType::GIF;
    }
    else if (extension == TEXT("tiff") || extension == TEXT("tif"))
    {
        type = ImageType::TIFF;
    }
    else if (extension == TEXT("hdr"))
    {
        type = ImageType::HDR;
    }
    else if (extension == TEXT("jpeg") || extension == TEXT("jpg"))
    {
        type = ImageType::JPEG;
    }
    else if (extension == TEXT("raw"))
    {
        type = ImageType::RAW;
    }
    else if (extension == TEXT("exr"))
    {
        type = ImageType::EXR;
    }
    else
    {
        LOG(Warning, "Unknown file type.");
        return true;
    }

    return false;
}

bool TextureTool::Transform(TextureData& texture, const Function<void(Color&)>& transformation)
{   
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);
    auto sampler = PixelFormatSampler::Get(texture.Format);
    if (!sampler)
        return true;
    for (auto& slice : texture.Items)
    {
        for (int32 mipIndex = 0; mipIndex < slice.Mips.Count(); mipIndex++)
        {
            auto& mip = slice.Mips[mipIndex];
            auto mipWidth = Math::Max(texture.Width >> mipIndex, 1);
            auto mipHeight = Math::Max(texture.Height >> mipIndex, 1);
            for (int32 y = 0; y < mipHeight; y++)
            {
                for (int32 x = 0; x < mipWidth; x++)
                {    
                    Color color = sampler->SamplePoint(mip.Data.Get(), x, y, mip.RowPitch);
                    transformation(color);
                    sampler->Store(mip.Data.Get(), x, y, mip.RowPitch, color);
                }
            }
        }
    }
    return false;
}

#endif
