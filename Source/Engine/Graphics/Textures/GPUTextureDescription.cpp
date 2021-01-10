// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GPUTextureDescription.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Graphics/Config.h"

namespace
{
    int32 CalculateMipMapCount(int32 requestedLevel, int32 width)
    {
        //int32 size = width;
        //int32 maxMipMap = 1 + Math::CeilToInt(Math::Log(size) / Math::Log(2.0));
        //return requestedLevel == 0 ? maxMipMap : Math::Min(requestedLevel, maxMipMap);

        if (requestedLevel == 0)
            requestedLevel = GPU_MAX_TEXTURE_MIP_LEVELS;

        int32 maxMipMap = 1;
        while (width > 1)
        {
            width >>= 1;
            maxMipMap++;
        }

        return Math::Min(requestedLevel, maxMipMap);
    }
}

const Char* ToString(TextureDimensions value)
{
    const Char* result;
    switch (value)
    {
    case TextureDimensions::Texture:
        result = TEXT("Texture");
        break;
    case TextureDimensions::VolumeTexture:
        result = TEXT("VolumeTexture");
        break;
    case TextureDimensions::CubeTexture:
        result = TEXT("CubeTexture");
        break;
    default:
        result = TEXT("");
    }
    return result;
}

void GPUTextureDescription::Clear()
{
    Platform::MemoryClear(this, sizeof(GPUTextureDescription));
    MultiSampleLevel = MSAALevel::None;
}

GPUTextureDescription GPUTextureDescription::New1D(int32 width, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount, int32 arraySize)
{
    GPUTextureDescription desc;
    desc.Dimensions = TextureDimensions::Texture;
    desc.Width = width;
    desc.Height = 1;
    desc.Depth = 1;
    desc.ArraySize = arraySize;
    desc.MipLevels = CalculateMipMapCount(mipCount, width);
    desc.Format = format;
    desc.MultiSampleLevel = MSAALevel::None;
    desc.Flags = textureFlags;
    desc.Usage = GPUResourceUsage::Default;
    desc.DefaultClearColor = Color::Black;
    return desc;
}

GPUTextureDescription GPUTextureDescription::New2D(int32 width, int32 height, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount, int32 arraySize, MSAALevel msaaLevel)
{
    GPUTextureDescription desc;
    desc.Dimensions = TextureDimensions::Texture;
    desc.Width = width;
    desc.Height = height;
    desc.Depth = 1;
    desc.ArraySize = arraySize;
    desc.MipLevels = CalculateMipMapCount(mipCount, Math::Max(width, height));
    desc.Format = format;
    desc.MultiSampleLevel = msaaLevel;
    desc.Flags = textureFlags;
    desc.DefaultClearColor = Color::Black;
    desc.Usage = GPUResourceUsage::Default;
    return desc;
}

GPUTextureDescription GPUTextureDescription::New3D(const Vector3& size, PixelFormat format, GPUTextureFlags textureFlags)
{
    return New3D((int32)size.X, (int32)size.Y, (int32)size.Z, 1, format, textureFlags);
}

GPUTextureDescription GPUTextureDescription::New3D(int32 width, int32 height, int32 depth, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount)
{
    GPUTextureDescription desc;
    desc.Dimensions = TextureDimensions::VolumeTexture;
    desc.Width = width;
    desc.Height = height;
    desc.Depth = depth;
    desc.ArraySize = 1;
    desc.MipLevels = CalculateMipMapCount(mipCount, Math::Max(width, height, depth));
    desc.Format = format;
    desc.MultiSampleLevel = MSAALevel::None;
    desc.Flags = textureFlags;
    desc.DefaultClearColor = Color::Black;
    desc.Usage = GPUResourceUsage::Default;
    return desc;
}

GPUTextureDescription GPUTextureDescription::NewCube(int32 size, PixelFormat format, GPUTextureFlags textureFlags, int32 mipCount)
{
    auto desc = New2D(size, size, format, textureFlags, mipCount, 6, MSAALevel::None);
    desc.Dimensions = TextureDimensions::CubeTexture;
    return desc;
}

bool GPUTextureDescription::Equals(const GPUTextureDescription& other) const
{
    return Dimensions == other.Dimensions
            && Width == other.Width
            && Height == other.Height
            && Depth == other.Depth
            && ArraySize == other.ArraySize
            && MipLevels == other.MipLevels
            && Format == other.Format
            && MultiSampleLevel == other.MultiSampleLevel
            && Flags == other.Flags
            && Usage == other.Usage
            && Color::NearEqual(DefaultClearColor, other.DefaultClearColor);
}

String GPUTextureDescription::ToString() const
{
    // TODO: add tool to Format to string

    String flags;
    if (Flags == GPUTextureFlags::None)
    {
        flags = TEXT("None");
    }
    else
    {
        // TODO: create tool to auto convert flag enums to string

#define CONVERT_FLAGS_FLAGS_2_STR(value) if(Flags & GPUTextureFlags::value) { if (flags.HasChars()) flags += TEXT('|'); flags += TEXT(#value); }
        CONVERT_FLAGS_FLAGS_2_STR(ShaderResource);
        CONVERT_FLAGS_FLAGS_2_STR(RenderTarget);
        CONVERT_FLAGS_FLAGS_2_STR(UnorderedAccess);
        CONVERT_FLAGS_FLAGS_2_STR(DepthStencil);
        CONVERT_FLAGS_FLAGS_2_STR(PerMipViews);
        CONVERT_FLAGS_FLAGS_2_STR(PerSliceViews);
        CONVERT_FLAGS_FLAGS_2_STR(ReadOnlyDepthView);
        CONVERT_FLAGS_FLAGS_2_STR(BackBuffer);
#undef CONVERT_FLAGS_FLAGS_2_STR
    }

    return String::Format(TEXT("Size: {0}x{1}x{2}[{3}], Type: {4}, Mips: {5}, Format: {6}, MSAA: {7}, Flags: {8}, Usage: {9}"),
                          Width,
                          Height,
                          Depth,
                          ArraySize,
                          ::ToString(Dimensions),
                          MipLevels,
                          (int32)Format,
                          ::ToString(MultiSampleLevel),
                          flags,
                          (int32)Usage);
}

uint32 GetHash(const GPUTextureDescription& key)
{
    uint32 hashCode = key.Width;
    hashCode = (hashCode * 397) ^ key.Height;
    hashCode = (hashCode * 397) ^ key.Depth;
    hashCode = (hashCode * 397) ^ key.ArraySize;
    hashCode = (hashCode * 397) ^ (uint32)key.Dimensions;
    hashCode = (hashCode * 397) ^ key.MipLevels;
    hashCode = (hashCode * 397) ^ (uint32)key.Format;
    hashCode = (hashCode * 397) ^ (uint32)key.MultiSampleLevel;
    hashCode = (hashCode * 397) ^ (uint32)key.Flags;
    hashCode = (hashCode * 397) ^ (uint32)key.Usage;
    hashCode = (hashCode * 397) ^ key.DefaultClearColor.GetHashCode();
    return hashCode;
}
