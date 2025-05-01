// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_TEXTURE_TOOL

#include "TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Serialization/JsonWriter.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Graphics/PixelFormatSampler.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Profiler/ProfilerCPU.h"

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
    LOG(Info, "Importing texture from \'{0}\'", path);
    const auto startTime = DateTime::NowUTC();

    // Detect texture format type
    ImageType type;
    if (GetImageType(path, type))
        return true;

    // Import
    bool hasAlpha = false;
#if COMPILE_WITH_DIRECTXTEX
    const auto failed = ImportTextureDirectXTex(type, path, textureData, hasAlpha);
#elif COMPILE_WITH_STB
    const auto failed = ImportTextureStb(type, path, textureData, hasAlpha);
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
    LOG(Info, "Importing texture from \'{0}\'. Options: {1}", path, options.ToString());
    const auto startTime = DateTime::NowUTC();

    // Detect texture format type
    ImageType type;
    if (options.InternalLoad.IsBinded())
    {
        type = ImageType::Internal;
    }
    else
    {
        if (GetImageType(path, type))
            return true;
    }

    // Clamp values
    options.MaxSize = Math::Clamp(options.MaxSize, 1, GPU_MAX_TEXTURE_SIZE);
    options.SizeX = Math::Clamp(options.SizeX, 1, GPU_MAX_TEXTURE_SIZE);
    options.SizeY = Math::Clamp(options.SizeY, 1, GPU_MAX_TEXTURE_SIZE);

    // Import
    bool hasAlpha = false;
#if COMPILE_WITH_DIRECTXTEX
    const auto failed = ImportTextureDirectXTex(type, path, textureData, options, errorMsg, hasAlpha);
#elif COMPILE_WITH_STB
    const auto failed = ImportTextureStb(type, path, textureData, options, errorMsg, hasAlpha);
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

bool TextureTool::ExportTexture(const StringView& path, const TextureData& textureData)
{
    PROFILE_CPU();
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

#if COMPILE_WITH_DIRECTXTEX
    return ConvertDirectXTex(dst, src, dstFormat);
#elif COMPILE_WITH_STB
    return ConvertStb(dst, src, dstFormat);
#else
    LOG(Warning, "Converting textures is not supported on this platform.");
    return true;
#endif
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
#if COMPILE_WITH_DIRECTXTEX
    return ResizeDirectXTex(dst, src, dstWidth, dstHeight);
#elif COMPILE_WITH_STB
    return ResizeStb(dst, src, dstWidth, dstHeight);
#else
    LOG(Warning, "Resizing textures is not supported on this platform.");
    return true;
#endif
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
