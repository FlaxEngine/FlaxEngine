// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_TEXTURE_TOOL

#include "TextureTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Math/Packed.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Int2.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Serialization/JsonWriter.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

#if USE_EDITOR
namespace
{
    Dictionary<String, bool> TexturesHasAlphaCache;
}
#endif

TextureTool::Options::Options()
{
    Type = TextureFormatType::ColorRGB;
    IsAtlas = false;
    NeverStream = false;
    Compress = true;
    IndependentChannels = false;
    sRGB = false;
    GenerateMipMaps = true;
    FlipY = false;
    Resize = false;
    PreserveAlphaCoverage = false;
    PreserveAlphaCoverageReference = 0.5f;
    Scale = 1.0f;
    SizeX = 1024;
    SizeY = 1024;
    MaxSize = GPU_MAX_TEXTURE_SIZE;
}

String TextureTool::Options::ToString() const
{
    return String::Format(TEXT("Type: {}, IsAtlas: {}, NeverStream: {}, IndependentChannels: {}, sRGB: {}, GenerateMipMaps: {}, FlipY: {}, Scale: {}, MaxSize: {}, Resize: {}, PreserveAlphaCoverage: {}, PreserveAlphaCoverageReference: {}, SizeX: {}, SizeY: {}"),
                          ::ToString(Type),
                          IsAtlas,
                          NeverStream,
                          IndependentChannels,
                          sRGB,
                          GenerateMipMaps,
                          FlipY,
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

    stream.JKEY("Resize");
    stream.Bool(Resize);

    stream.JKEY("PreserveAlphaCoverage");
    stream.Bool(PreserveAlphaCoverage);

    stream.JKEY("PreserveAlphaCoverageReference");
    stream.Float(PreserveAlphaCoverageReference);

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
        stream.Vector2(s.Area.Location);

        stream.JKEY("Size");
        stream.Vector2(s.Area.Size);

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
    Resize = JsonTools::GetBool(stream, "Resize", Resize);
    PreserveAlphaCoverage = JsonTools::GetBool(stream, "PreserveAlphaCoverage", PreserveAlphaCoverage);
    PreserveAlphaCoverageReference = JsonTools::GetFloat(stream, "PreserveAlphaCoverageReference", PreserveAlphaCoverageReference);
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

            s.Area.Location = JsonTools::GetVector2(stData, "Position", Vector2::Zero);
            s.Area.Size = JsonTools::GetVector2(stData, "Size", Vector2::One);
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

    // Export
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
    // Validate input
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

#if COMPILE_WITH_DIRECTXTEX
    return ConvertDirectXTex(dst, src, dstFormat);
#else
    LOG(Warning, "Converting textures is not supported on this platform.");
    return true;
#endif
}

bool TextureTool::Resize(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight)
{
    // Validate input
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

#if COMPILE_WITH_DIRECTXTEX
    return ResizeDirectXTex(dst, src, dstWidth, dstHeight);
#elif COMPILE_WITH_STB
    return ResizeStb(dst, src, dstWidth, dstHeight);
#else
    LOG(Warning, "Resizing textures is not supported on this platform.");
    return true;
#endif
}

TextureTool::PixelFormatSampler PixelFormatSamplers[] =
{
    {
        PixelFormat::R32G32B32A32_Float,
        sizeof(Vector4),
        [](const void* ptr)
        {
            return Color(*(Vector4*)ptr);
        },
        [](const void* ptr, const Color& color)
        {
            *(Vector4*)ptr = color.ToVector4();
        },
    },
    {
        PixelFormat::R32G32B32_Float,
        sizeof(Vector3),
        [](const void* ptr)
        {
            return Color(*(Vector3*)ptr, 1.0f);
        },
        [](const void* ptr, const Color& color)
        {
            *(Vector3*)ptr = color.ToVector3();
        },
    },
    {
        PixelFormat::R16G16B16A16_Float,
        sizeof(Half4),
        [](const void* ptr)
        {
            return Color(((Half4*)ptr)->ToVector4());
        },
        [](const void* ptr, const Color& color)
        {
            *(Half4*)ptr = Half4(color.R, color.G, color.B, color.A);
        },
    },
    {
        PixelFormat::R16G16B16A16_UNorm,
        sizeof(RGBA16UNorm),
        [](const void* ptr)
        {
            return Color(((RGBA16UNorm*)ptr)->ToVector4());
        },
        [](const void* ptr, const Color& color)
        {
            *(RGBA16UNorm*)ptr = RGBA16UNorm(color.R, color.G, color.B, color.A);
        }
    },
    {
        PixelFormat::R32G32_Float,
        sizeof(Vector2),
        [](const void* ptr)
        {
            return Color(((Vector2*)ptr)->X, ((Vector2*)ptr)->Y, 1.0f);
        },
        [](const void* ptr, const Color& color)
        {
            *(Vector2*)ptr = Vector2(color.R, color.G);
        },
    },
    {
        PixelFormat::R8G8B8A8_UNorm,
        sizeof(Color32),
        [](const void* ptr)
        {
            return Color(*(Color32*)ptr);
        },
        [](const void* ptr, const Color& color)
        {
            *(Color32*)ptr = Color32(color);
        },
    },
    {
        PixelFormat::R8G8B8A8_UNorm_sRGB,
        sizeof(Color32),
        [](const void* ptr)
        {
            return Color(*(Color32*)ptr);
        },
        [](const void* ptr, const Color& color)
        {
            *(Color32*)ptr = Color32(color);
        },
    },
    {
        PixelFormat::R16G16_Float,
        sizeof(Half2),
        [](const void* ptr)
        {
            const Vector2 rg = ((Half2*)ptr)->ToVector2();
            return Color(rg.X, rg.Y, 0, 1);
        },
        [](const void* ptr, const Color& color)
        {
            *(Half2*)ptr = Half2(color.R, color.G);
        },
    },
    {
        PixelFormat::R16G16_UNorm,
        sizeof(RG16UNorm),
        [](const void* ptr)
        {
            const Vector2 rg = ((RG16UNorm*)ptr)->ToVector2();
            return Color(rg.X, rg.Y, 0, 1);
        },
        [](const void* ptr, const Color& color)
        {
            *(RG16UNorm*)ptr = RG16UNorm(color.R, color.G);
        },
    },
    {
        PixelFormat::R32_Float,
        sizeof(float),
        [](const void* ptr)
        {
            return Color(*(float*)ptr, 0, 0, 1);
        },
        [](const void* ptr, const Color& color)
        {
            *(float*)ptr = color.R;
        },
    },
    {
        PixelFormat::R16_Float,
        sizeof(Half),
        [](const void* ptr)
        {
            return Color(Float16Compressor::Decompress(*(Half*)ptr), 0, 0, 1);
        },
        [](const void* ptr, const Color& color)
        {
            *(Half*)ptr = Float16Compressor::Compress(color.R);
        },
    },
    {
        PixelFormat::R16_UNorm,
        sizeof(uint16),
        [](const void* ptr)
        {
            return Color((float)*(uint16*)ptr / MAX_uint16, 0, 0, 1);
        },
        [](const void* ptr, const Color& color)
        {
            *(uint16*)ptr = (uint16)(color.R * MAX_uint16);
        },
    },
    {
        PixelFormat::R8_UNorm,
        sizeof(uint8),
        [](const void* ptr)
        {
            return Color((float)*(byte*)ptr / MAX_uint8, 0, 0, 1);
        },
        [](const void* ptr, const Color& color)
        {
            *(byte*)ptr = (byte)(color.R * MAX_uint8);
        },
    },
    {
        PixelFormat::A8_UNorm,
        sizeof(uint8),
        [](const void* ptr)
        {
            return Color(0, 0, 0, (float)*(byte*)ptr / MAX_uint8);
        },
        [](const void* ptr, const Color& color)
        {
            *(byte*)ptr = (byte)(color.A * MAX_uint8);
        },
    },
    {
        PixelFormat::B8G8R8A8_UNorm,
        sizeof(Color32),
        [](const void* ptr)
        {
            const Color32 bgra = *(Color32*)ptr;
            return Color(Color32(bgra.B, bgra.G, bgra.R, bgra.A));
        },
        [](const void* ptr, const Color& color)
        {
            *(Color32*)ptr = Color32(byte(color.B * MAX_uint8), byte(color.G * MAX_uint8), byte(color.R * MAX_uint8), byte(color.A * MAX_uint8));
        },
    },
    {
        PixelFormat::B8G8R8A8_UNorm_sRGB,
        sizeof(Color32),
        [](const void* ptr)
        {
            const Color32 bgra = *(Color32*)ptr;
            return Color(Color32(bgra.B, bgra.G, bgra.R, bgra.A));
        },
        [](const void* ptr, const Color& color)
        {
            *(Color32*)ptr = Color32(byte(color.B * MAX_uint8), byte(color.G * MAX_uint8), byte(color.R * MAX_uint8), byte(color.A * MAX_uint8));
        },
    },
    {
        PixelFormat::B8G8R8X8_UNorm,
        sizeof(Color32),
        [](const void* ptr)
        {
            const Color32 bgra = *(Color32*)ptr;
            return Color(Color32(bgra.B, bgra.G, bgra.R, MAX_uint8));
        },
        [](const void* ptr, const Color& color)
        {
            *(Color32*)ptr = Color32(byte(color.B * MAX_uint8), byte(color.G * MAX_uint8), byte(color.R * MAX_uint8), MAX_uint8);
        },
    },
    {
        PixelFormat::B8G8R8X8_UNorm_sRGB,
        sizeof(Color32),
        [](const void* ptr)
        {
            const Color32 bgra = *(Color32*)ptr;
            return Color(Color32(bgra.B, bgra.G, bgra.R, MAX_uint8));
        },
        [](const void* ptr, const Color& color)
        {
            *(Color32*)ptr = Color32(byte(color.B * MAX_uint8), byte(color.G * MAX_uint8), byte(color.R * MAX_uint8), MAX_uint8);
        },
    },
    {
        PixelFormat::R11G11B10_Float,
        sizeof(FloatR11G11B10),
        [](const void* ptr)
        {
            const Vector3 rgb = ((FloatR11G11B10*)ptr)->ToVector3();
            return Color(rgb.X, rgb.Y, rgb.Z);
        },
        [](const void* ptr, const Color& color)
        {
            *(FloatR11G11B10*)ptr = Float1010102(color.R, color.G, color.B, color.A);
        },
    },
};

const TextureTool::PixelFormatSampler* TextureTool::GetSampler(PixelFormat format)
{
    format = PixelFormatExtensions::MakeTypelessFloat(format);
    for (auto& sampler : PixelFormatSamplers)
    {
        if (sampler.Format == format)
            return &sampler;
    }
    return nullptr;
}

void TextureTool::Store(const PixelFormatSampler* sampler, int32 x, int32 y, const void* data, int32 rowPitch, const Color& color)
{
    ASSERT_LOW_LAYER(sampler);
    sampler->Store((byte*)data + rowPitch * y + sampler->PixelSize * x, color);
}

Color TextureTool::SamplePoint(const PixelFormatSampler* sampler, const Vector2& uv, const void* data, const Int2& size, int32 rowPitch)
{
    ASSERT_LOW_LAYER(sampler);

    const Int2 end = size - 1;
    const Int2 uvFloor(Math::Min(Math::FloorToInt(uv.X * size.X), end.X), Math::Min(Math::FloorToInt(uv.Y * size.Y), end.Y));

    return sampler->Sample((byte*)data + rowPitch * uvFloor.Y + sampler->PixelSize * uvFloor.X);
}

Color TextureTool::SamplePoint(const PixelFormatSampler* sampler, int32 x, int32 y, const void* data, int32 rowPitch)
{
    ASSERT_LOW_LAYER(sampler);
    return sampler->Sample((byte*)data + rowPitch * y + sampler->PixelSize * x);
}

Color TextureTool::SampleLinear(const PixelFormatSampler* sampler, const Vector2& uv, const void* data, const Int2& size, int32 rowPitch)
{
    ASSERT_LOW_LAYER(sampler);

    const Int2 end = size - 1;
    const Int2 uvFloor(Math::Min(Math::FloorToInt(uv.X * size.X), end.X), Math::Min(Math::FloorToInt(uv.Y * size.Y), end.Y));
    const Int2 uvNext(Math::Min(uvFloor.X + 1, end.X), Math::Min(uvFloor.Y + 1, end.Y));
    const Vector2 uvFraction(uv.X * size.Y - uvFloor.X, uv.Y * size.Y - uvFloor.Y);

    const Color v00 = sampler->Sample((byte*)data + rowPitch * uvFloor.Y + sampler->PixelSize * uvFloor.X);
    const Color v01 = sampler->Sample((byte*)data + rowPitch * uvFloor.Y + sampler->PixelSize * uvNext.X);
    const Color v10 = sampler->Sample((byte*)data + rowPitch * uvNext.Y + sampler->PixelSize * uvFloor.X);
    const Color v11 = sampler->Sample((byte*)data + rowPitch * uvNext.Y + sampler->PixelSize * uvNext.X);

    return Color::Lerp(Color::Lerp(v00, v01, uvFraction.X), Color::Lerp(v10, v11, uvFraction.X), uvFraction.Y);
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
    else
    {
        LOG(Warning, "Unknown file type.");
        return true;
    }

    return false;
}

#endif
