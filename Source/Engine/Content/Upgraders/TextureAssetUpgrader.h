// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Core/Core.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/Assets/IESProfile.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Utilities/Encryption.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Render2D/SpriteAtlas.h"
#if USE_EDITOR
#include "Editor/Content/PreviewsCache.h"
#endif

/// <summary>
/// Texture Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class TextureAssetUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="TextureAssetUpgrader"/> class.
    /// </summary>
    TextureAssetUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 1, 4, &Upgrade_1_To_4 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    // ============================================
    //             Versions 1, 2 and 3:
    // Designed: long time ago in a galaxy far far away
    // Custom Data: not used
    // Chunk 0: Header (sprite atlas also stored sprites)
    // Chunk 1-14: Texture Mips
    // Chunk 15: Texture Import options
    // ============================================
    //
    // Note: we merge CubeTexture, Texture, AssetPreviewsCache, IESProfile and AssetSpriteAtlas storage backend.
    // This simplifies textures operations like importing/exporting and allows to reduce amount of code to maintain.
    //
    // Firstly store texture header inside a CustomData field and get it just after asset creation.
    // Secondly move texture import metadata (format, max size, etc.) to the json metadata.
    // Lastly store all texture mips in chunks 1-14 and use 15th and 16th chunk for custom data per asset type (sprites or thumbnails meta, etc.).
    // Note: mips order has been changed, Chunk[0..13] contain now Mip[0..13]. Internal storage layer should manage data chunks order inside a file, not an asset.
    // Additionally texture header contains a few bytes to be used by the assets (for eg. IES Profile uses it to store Brightness parameter)
    //
    // Old AssetPreviewsCache:
    // Chunk 0: header
    // Chunk 1: assets ID table
    // Chunk 2: atlas image
    // ============================================
    //                  Version 4:
    // Designed: 4/25/2017
    // Custom Data: Header
    // Metadata: Import Options
    // Chunk 0-13: Mips (from the highest quality to the lowest) 
    // Chunk 15: Custom Asset Data:
    //   - AssetSpriteAtlas: sprites
    //   - AssetPreviewsCache: asset previews meta
    // ============================================

    enum class PixelFormatOld
    {
        PF_Unknown = 0,
        PF_R8G8B8A8 = 1,
        PF_R16G16B16A16 = 2,
        PF_R32G32B32A32 = 3,
        PF_A8 = 4,
        PF_R16F = 5,
        PF_R32 = 6,
        PF_R16G16 = 7,
        PF_R32G32 = 8,
        PF_R10G10B10A2 = 9,
        PF_R11G11B10 = 10,
        PF_BC1 = 11,
        PF_BC2 = 12,
        PF_BC3 = 13,
        PF_BC4 = 14,
        PF_BC5 = 15,
        PF_B8G8R8A8 = 16,
        PF_DepthStencil = 17,
        PF_ShadowDepth = 18,
        PF_D16 = 19,
        PF_D24 = 20,
        PF_D32 = 21,
        PF_R16 = 22,
        PF_B5G6R5 = 23,
        PF_R8G8_SNORM = 24,
        PF_R8G8_UNORM = 25,
    };

    static PixelFormat PixelFormatOldToNew(PixelFormatOld format)
    {
        switch (format)
        {
        case PixelFormatOld::PF_Unknown:
            return PixelFormat::Unknown;
        case PixelFormatOld::PF_R8G8B8A8:
            return PixelFormat::R8G8B8A8_UNorm;
        case PixelFormatOld::PF_R16G16B16A16:
            return PixelFormat::R16G16B16A16_Float;
        case PixelFormatOld::PF_R32G32B32A32:
            return PixelFormat::R32G32B32A32_Float;
        case PixelFormatOld::PF_A8:
            return PixelFormat::A8_UNorm;
        case PixelFormatOld::PF_R16F:
            return PixelFormat::R16_Float;
        case PixelFormatOld::PF_R32:
            return PixelFormat::R32_Float;
        case PixelFormatOld::PF_R16G16:
            return PixelFormat::R16G16_Float;
        case PixelFormatOld::PF_R32G32:
            return PixelFormat::R32G32_Float;
        case PixelFormatOld::PF_R10G10B10A2:
            return PixelFormat::R10G10B10A2_UNorm;
        case PixelFormatOld::PF_R11G11B10:
            return PixelFormat::R11G11B10_Float;
        case PixelFormatOld::PF_BC1:
            return PixelFormat::BC1_UNorm;
        case PixelFormatOld::PF_BC2:
            return PixelFormat::BC2_UNorm;
        case PixelFormatOld::PF_BC3:
            return PixelFormat::BC3_UNorm;
        case PixelFormatOld::PF_BC4:
            return PixelFormat::BC4_UNorm;
        case PixelFormatOld::PF_BC5:
            return PixelFormat::BC5_UNorm;
        case PixelFormatOld::PF_B8G8R8A8:
            return PixelFormat::B8G8R8A8_UNorm;
        case PixelFormatOld::PF_DepthStencil:
            return PixelFormat::D24_UNorm_S8_UInt;
        case PixelFormatOld::PF_ShadowDepth:
            return PixelFormat::D32_Float;
        case PixelFormatOld::PF_D16:
            return PixelFormat::D16_UNorm;
        case PixelFormatOld::PF_D24:
            return PixelFormat::D24_UNorm_S8_UInt;
        case PixelFormatOld::PF_D32:
            return PixelFormat::D32_Float;
        case PixelFormatOld::PF_R16:
            return PixelFormat::R16_UNorm;
        case PixelFormatOld::PF_B5G6R5:
            return PixelFormat::B5G6R5_UNorm;
        case PixelFormatOld::PF_R8G8_SNORM:
            return PixelFormat::R8G8_SNorm;
        case PixelFormatOld::PF_R8G8_UNORM:
            return PixelFormat::R8G8_UNorm;
        default: ;
        }

        CRASH;
        return PixelFormat::Unknown;
    }

    static PixelFormatOld PixelFormatToOld(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::Unknown:
            return PixelFormatOld::PF_Unknown;
        case PixelFormat::R32G32B32A32_Typeless:
        case PixelFormat::R32G32B32A32_Float:
        case PixelFormat::R32G32B32A32_UInt:
        case PixelFormat::R32G32B32A32_SInt:
            return PixelFormatOld::PF_R32G32B32A32;
        //case PixelFormat::R32G32B32_Typeless: 
        //case PixelFormat::R32G32B32_Float: 
        //case PixelFormat::R32G32B32_UInt:
        //case PixelFormat::R32G32B32_SInt:
        case PixelFormat::R16G16B16A16_Typeless:
        case PixelFormat::R16G16B16A16_Float:
        case PixelFormat::R16G16B16A16_UNorm:
        case PixelFormat::R16G16B16A16_UInt:
        case PixelFormat::R16G16B16A16_SNorm:
        case PixelFormat::R16G16B16A16_SInt:
            return PixelFormatOld::PF_R16G16B16A16;
        case PixelFormat::R32G32_Typeless:
        case PixelFormat::R32G32_Float:
        case PixelFormat::R32G32_UInt:
        case PixelFormat::R32G32_SInt:
        case PixelFormat::R32G8X24_Typeless:
            return PixelFormatOld::PF_R32G32;
        case PixelFormat::D32_Float_S8X24_UInt:
        case PixelFormat::R32_Float_X8X24_Typeless:
        case PixelFormat::X32_Typeless_G8X24_UInt:
            return PixelFormatOld::PF_R32G32B32A32;
        case PixelFormat::R10G10B10A2_Typeless:
        case PixelFormat::R10G10B10A2_UNorm:
        case PixelFormat::R10G10B10A2_UInt:
            return PixelFormatOld::PF_R10G10B10A2;
        case PixelFormat::R11G11B10_Float:
            return PixelFormatOld::PF_R11G11B10;
        case PixelFormat::R8G8B8A8_Typeless:
        case PixelFormat::R8G8B8A8_UNorm:
        case PixelFormat::R8G8B8A8_UNorm_sRGB:
        case PixelFormat::R8G8B8A8_UInt:
        case PixelFormat::R8G8B8A8_SNorm:
        case PixelFormat::R8G8B8A8_SInt:
            return PixelFormatOld::PF_R8G8B8A8;
        case PixelFormat::R16G16_Typeless:
        case PixelFormat::R16G16_Float:
        case PixelFormat::R16G16_UNorm:
        case PixelFormat::R16G16_UInt:
        case PixelFormat::R16G16_SNorm:
        case PixelFormat::R16G16_SInt:
            return PixelFormatOld::PF_R16G16;
        case PixelFormat::R32_Typeless:
            return PixelFormatOld::PF_R32;
        case PixelFormat::D32_Float:
            return PixelFormatOld::PF_DepthStencil;
        case PixelFormat::R32_Float:
        case PixelFormat::R32_UInt:
        case PixelFormat::R32_SInt:
            return PixelFormatOld::PF_R32;
        case PixelFormat::R24G8_Typeless:
        case PixelFormat::D24_UNorm_S8_UInt:
        case PixelFormat::R24_UNorm_X8_Typeless:
            return PixelFormatOld::PF_DepthStencil;
        //case PixelFormat::X24_Typeless_G8_UInt:
        case PixelFormat::R8G8_Typeless:
        case PixelFormat::R8G8_UNorm:
        case PixelFormat::R8G8_UInt:
            return PixelFormatOld::PF_R8G8_UNORM;
        case PixelFormat::R8G8_SNorm:
        case PixelFormat::R8G8_SInt:
            return PixelFormatOld::PF_R8G8_SNORM;
        case PixelFormat::R16_Float:
            return PixelFormatOld::PF_R16F;
        case PixelFormat::R16_Typeless:
        case PixelFormat::D16_UNorm:
        case PixelFormat::R16_UNorm:
        case PixelFormat::R16_UInt:
        case PixelFormat::R16_SNorm:
        case PixelFormat::R16_SInt:
            return PixelFormatOld::PF_R16;
        case PixelFormat::R8_Typeless:
        case PixelFormat::R8_UNorm:
        case PixelFormat::R8_UInt:
        case PixelFormat::R8_SNorm:
        case PixelFormat::R8_SInt:
        case PixelFormat::A8_UNorm:
            return PixelFormatOld::PF_A8;
        case PixelFormat::R1_UNorm:
        //case PixelFormat::R9G9B9E5_Sharedexp:
        //case PixelFormat::R8G8_B8G8_UNorm:
        //case PixelFormat::G8R8_G8B8_UNorm:
        case PixelFormat::BC1_Typeless:
        case PixelFormat::BC1_UNorm:
        case PixelFormat::BC1_UNorm_sRGB:
            return PixelFormatOld::PF_BC1;
        case PixelFormat::BC2_Typeless:
        case PixelFormat::BC2_UNorm:
        case PixelFormat::BC2_UNorm_sRGB:
            return PixelFormatOld::PF_BC2;
        case PixelFormat::BC3_Typeless:
        case PixelFormat::BC3_UNorm:
        case PixelFormat::BC3_UNorm_sRGB:
            return PixelFormatOld::PF_BC3;
        case PixelFormat::BC4_Typeless:
        case PixelFormat::BC4_UNorm:
        case PixelFormat::BC4_SNorm:
            return PixelFormatOld::PF_BC4;
        case PixelFormat::BC5_Typeless:
        case PixelFormat::BC5_UNorm:
        case PixelFormat::BC5_SNorm:
            return PixelFormatOld::PF_BC5;
        case PixelFormat::B5G6R5_UNorm:
            return PixelFormatOld::PF_B5G6R5;
        //case PixelFormat::B5G5R5A1_UNorm:
        //case PixelFormat::B8G8R8A8_UNorm:
        //case PixelFormat::B8G8R8X8_UNorm:
        //case PixelFormat::R10G10B10_Xr_Bias_A2_UNorm:
        //case PixelFormat::B8G8R8A8_Typeless:
        //case PixelFormat::B8G8R8A8_UNorm_sRGB:
        //case PixelFormat::B8G8R8X8_Typeless:
        //case PixelFormat::B8G8R8X8_UNorm_sRGB:
        //case PixelFormat::BC6H_Typeless:
        //case PixelFormat::BC6H_Uf16:
        //case PixelFormat::BC6H_Sf16:
        //case PixelFormat::BC7_Typeless:
        //case PixelFormat::BC7_UNorm:
        //case PixelFormat::BC7_UNorm_sRGB: 
        default:
            break;
        }

        CRASH;
        return PixelFormatOld::PF_Unknown;
    }

    struct TextureHeader1
    {
        int32 Width;
        int32 Height;
        byte MipLevels;
        bool NeverStream;
        TextureFormatType Type;
        PixelFormatOld Format;
        bool IsSRGB;
    };

    struct TextureHeader2
    {
        int32 Width;
        int32 Height;
        byte MipLevels;
        bool NeverStream;
        TextureFormatType Type;
        PixelFormatOld Format;
        bool IsSRGB;
    };

    struct TextureHeader3
    {
        int32 Width;
        int32 Height;
        int32 MipLevels;
        bool IsCubeMap;
        bool NeverStream;
        TextureFormatType Type;
        PixelFormat Format;
        bool IsSRGB;
    };

    struct CubeTextureHeader1
    {
        int32 Size;
        byte MipLevels;
        PixelFormatOld Format;
        bool IsSRGB;
    };

    enum class TextureOwnerType1
    {
        Texture,
        CubeTexture,
        SpriteAtlas,
        IESProfile,
        PreviewsCache,
    };

    static bool GetOwnerType(AssetMigrationContext& context, TextureOwnerType1& type)
    {
        auto typeName = context.Input.Header.TypeName;

        if (typeName == Texture::TypeName)
        {
            type = TextureOwnerType1::Texture;
            return false;
        }
        if (typeName == CubeTexture::TypeName)
        {
            type = TextureOwnerType1::CubeTexture;
            return false;
        }
        if (typeName == SpriteAtlas::TypeName)
        {
            type = TextureOwnerType1::SpriteAtlas;
            return false;
        }
        if (typeName == IESProfile::TypeName)
        {
            type = TextureOwnerType1::IESProfile;
            return false;
        }
#if USE_EDITOR
        if (typeName == PreviewsCache::TypeName)
        {
            type = TextureOwnerType1::PreviewsCache;
            return false;
        }
#endif

        LOG(Warning, "Invalid input asset type.");
        return true;
    }

    static bool Upgrade_1_To_4_ImportOptions(AssetMigrationContext& context)
    {
        // Check if has no 15th chunk
        auto chunk = context.Input.Header.Chunks[15];
        if (chunk == nullptr || chunk->IsMissing())
            return false;

#if USE_EDITOR

        // Get import metadata json
        rapidjson_flax::Document importMeta;
        if (context.Input.Metadata.IsValid())
        {
            importMeta.Parse((const char*)context.Input.Metadata.Get(), context.Input.Metadata.Length());
            if (importMeta.HasParseError())
            {
                LOG(Warning, "Failed to parse import meta json.");
                return true;
            }
        }

        // Get import options json
        rapidjson_flax::Document importOptions;
        {
            // Decrypt bytes
            int32 length = chunk->Size();
            Encryption::DecryptBytes(chunk->Get(), length);

            // Load Json
            static_assert(sizeof(char) == sizeof(rapidjson_flax::Document::Ch), "Invalid rapidJson document char size.");
            int32 tmpJsonSize = length;
            importOptions.Parse((const char*)chunk->Get(), tmpJsonSize);
            if (importOptions.HasParseError())
            {
                LOG(Warning, "Failed to parse import options json.");
                return true;
            }
        }

        // Merge deprecated json meta from chunk 15 with import metadata
        JsonTools::MergeDocuments(importMeta, importOptions);

        // Save json metadata
        {
            rapidjson_flax::StringBuffer buffer;
            rapidjson_flax::Writer<rapidjson_flax::StringBuffer> writer(buffer);
            if (!importMeta.Accept(writer))
            {
                LOG(Warning, "Failed to serialize metadata json.");
                return true;
            }
            context.Output.Metadata.Copy((const byte*)buffer.GetString(), (uint32)buffer.GetSize());
        }

#endif

        return false;
    }

    static bool Upgrade_1_To_4_Texture(AssetMigrationContext& context)
    {
        // Get texture header
        auto headerChunk = context.Input.Header.Chunks[0];
        if (headerChunk == nullptr || headerChunk->IsMissing())
        {
            LOG(Warning, "Missing texture header chunk.");
            return true;
        }
        MemoryReadStream headerStream(headerChunk->Get(), headerChunk->Size());

        // Load texture header
        int32 textureHeaderVersion;
        headerStream.ReadInt32(&textureHeaderVersion);
        if (textureHeaderVersion != 1)
        {
            LOG(Warning, "Invalid texture header version.");
            return true;
        }
        TextureHeader1 textureHeader;
        headerStream.ReadBytes(&textureHeader, sizeof(textureHeader));

        // Create new header
        TextureHeader newHeader;
        newHeader.Width = textureHeader.Width;
        newHeader.Height = textureHeader.Height;
        newHeader.MipLevels = textureHeader.MipLevels;
        newHeader.Format = PixelFormatOldToNew(textureHeader.Format);
        newHeader.IsSRGB = textureHeader.IsSRGB;
        newHeader.NeverStream = textureHeader.NeverStream;
        newHeader.Type = textureHeader.Type;
        context.Output.CustomData.Copy(&newHeader);

        // Convert import options
        if (Upgrade_1_To_4_ImportOptions(context))
            return true;

        // Copy mip maps
        for (int32 mipIndex = 0; mipIndex < newHeader.MipLevels; mipIndex++)
        {
            auto srcChunk = context.Input.Header.Chunks[newHeader.MipLevels - mipIndex];
            if (srcChunk == nullptr || srcChunk->IsMissing())
            {
                LOG(Warning, "Missing data chunk with a mipmap");
                return true;
            }
            if (context.AllocateChunk(mipIndex))
                return true;
            auto dstChunk = context.Output.Header.Chunks[mipIndex];

            dstChunk->Data.Copy(srcChunk->Data);
        }

        return false;
    }

    static bool Upgrade_1_To_4_CubeTexture(AssetMigrationContext& context)
    {
        // Get texture header
        auto headerChunk = context.Input.Header.Chunks[0];
        if (headerChunk == nullptr || headerChunk->IsMissing())
        {
            LOG(Warning, "Missing cube texture header chunk.");
            return true;
        }
        MemoryReadStream headerStream(headerChunk->Get(), headerChunk->Size());

        // Load texture header
        int32 textureHeaderVersion;
        headerStream.ReadInt32(&textureHeaderVersion);
        if (textureHeaderVersion != 1)
        {
            LOG(Warning, "Invalid cube texture header version.");
            return true;
        }
        CubeTextureHeader1 textureHeader;
        headerStream.ReadBytes(&textureHeader, sizeof(textureHeader));

        // Create new header
        TextureHeader newHeader;
        newHeader.Width = textureHeader.Size;
        newHeader.Height = textureHeader.Size;
        newHeader.MipLevels = textureHeader.MipLevels;
        newHeader.Format = PixelFormatOldToNew(textureHeader.Format);
        newHeader.IsSRGB = textureHeader.IsSRGB;
        newHeader.Type = TextureFormatType::ColorRGB;
        newHeader.IsCubeMap = true;
        context.Output.CustomData.Copy(&newHeader);

        // Convert import options
        if (Upgrade_1_To_4_ImportOptions(context))
            return true;

        // Conversion from single chunk to normal texture chunks mapping mode (single mip per chunk)
        {
            auto cubeTextureData = context.Input.Header.Chunks[1];
            if (cubeTextureData == nullptr || cubeTextureData->IsMissing())
            {
                LOG(Warning, "Missing data chunk with a cubemap");
                return true;
            }
            uint32 dataSize = cubeTextureData->Size();
            auto data = cubeTextureData->Get();

            auto mipLevels = textureHeader.MipLevels;
            auto textureSize = textureHeader.Size;
            ASSERT(mipLevels > 0 && mipLevels <= GPU_MAX_TEXTURE_MIP_LEVELS);
            ASSERT(textureSize > 0 && textureSize <= GPU_MAX_TEXTURE_SIZE);

            //Platform::MemorySet(cubeTextureData.Get(), cubeTextureData.Length(), MAX_uint32);

            byte* position = (byte*)data;
            for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
            {
                uint32 mipSize = textureSize >> mipIndex;

                uint32 rowPitch, slicePitch;
                RenderTools::ComputePitch(PixelFormatOldToNew(textureHeader.Format), mipSize, mipSize, rowPitch, slicePitch);

                uint32 mipDataSize = slicePitch * 6;

                // Create mip
                if (context.AllocateChunk(mipIndex))
                    return true;
                auto dstChunk = context.Output.Header.Chunks[mipIndex];
                dstChunk->Data.Copy(position, mipDataSize);

                position += mipDataSize;
            }
            ASSERT(dataSize == (uint32)(position - data));
        }

        return false;
    }

    static bool Upgrade_1_To_4_SpriteAtlas(AssetMigrationContext& context)
    {
        // Get texture header
        auto headerChunk = context.Input.Header.Chunks[0];
        if (headerChunk == nullptr || headerChunk->IsMissing())
        {
            LOG(Warning, "Missing texture header chunk.");
            return true;
        }
        MemoryReadStream headerStream(headerChunk->Get(), headerChunk->Size());

        // Load texture header
        int32 textureHeaderVersion;
        headerStream.ReadInt32(&textureHeaderVersion);
        if (textureHeaderVersion != 1)
        {
            LOG(Warning, "Invalid texture header version.");
            return true;
        }
        TextureHeader1 textureHeader;
        headerStream.ReadBytes(&textureHeader, sizeof(textureHeader));

        // Create new header
        TextureHeader newHeader;
        newHeader.Width = textureHeader.Width;
        newHeader.Height = textureHeader.Height;
        newHeader.MipLevels = textureHeader.MipLevels;
        newHeader.Format = PixelFormatOldToNew(textureHeader.Format);
        newHeader.IsSRGB = textureHeader.IsSRGB;
        newHeader.NeverStream = textureHeader.NeverStream;
        newHeader.Type = textureHeader.Type;
        context.Output.CustomData.Copy(&newHeader);

        // Copy sprite atlas data from old header stream to chunk (chunk 15th)
        if (context.AllocateChunk(15))
            return true;
        context.Output.Header.Chunks[15]->Data.Copy(headerStream.GetPositionHandle(), headerStream.GetLength() - headerStream.GetPosition());

        // Convert import options
        if (Upgrade_1_To_4_ImportOptions(context))
            return true;

        // Copy mip maps
        for (int32 mipIndex = 0; mipIndex < newHeader.MipLevels; mipIndex++)
        {
            auto srcChunk = context.Input.Header.Chunks[newHeader.MipLevels - mipIndex];
            if (srcChunk == nullptr || srcChunk->IsMissing())
            {
                LOG(Warning, "Missing data chunk with a mipmap");
                return true;
            }
            if (context.AllocateChunk(mipIndex))
                return true;
            auto dstChunk = context.Output.Header.Chunks[mipIndex];

            dstChunk->Data.Copy(srcChunk->Data);
        }

        return false;
    }

    static bool Upgrade_1_To_4_IESProfile(AssetMigrationContext& context)
    {
        // Get texture header
        auto headerChunk = context.Input.Header.Chunks[0];
        if (headerChunk == nullptr || headerChunk->IsMissing())
        {
            LOG(Warning, "Missing IES profile texture header chunk.");
            return true;
        }
        MemoryReadStream headerStream(headerChunk->Get(), headerChunk->Size());

        // Load IES header
        int32 iesHeaderVersion;
        headerStream.ReadInt32(&iesHeaderVersion);
        if (iesHeaderVersion != 1)
        {
            LOG(Warning, "Invalid IES profile header.");
            return true;
        }
        float brightness, multiplier;
        headerStream.ReadFloat(&brightness);
        headerStream.ReadFloat(&multiplier);

        // Load texture header
        int32 textureHeaderVersion;
        headerStream.ReadInt32(&textureHeaderVersion);
        if (textureHeaderVersion != 1)
        {
            LOG(Warning, "Invalid texture header version.");
            return true;
        }
        TextureHeader1 textureHeader;
        headerStream.ReadBytes(&textureHeader, sizeof(textureHeader));
        if (textureHeader.MipLevels != 1)
        {
            LOG(Warning, "Invalid IES profile texture header.");
            return true;
        }

        // Create new header
        TextureHeader newHeader;
        newHeader.Width = textureHeader.Width;
        newHeader.Height = textureHeader.Height;
        newHeader.MipLevels = textureHeader.MipLevels;
        newHeader.Format = PixelFormatOldToNew(textureHeader.Format);
        newHeader.IsSRGB = textureHeader.IsSRGB;
        newHeader.NeverStream = textureHeader.NeverStream;
        newHeader.Type = textureHeader.Type;
        auto data = (IESProfile::CustomDataLayout*)newHeader.CustomData;
        data->Brightness = brightness;
        data->TextureMultiplier = multiplier;
        context.Output.CustomData.Copy(&newHeader);

        // Convert import options
        if (Upgrade_1_To_4_ImportOptions(context))
            return true;

        // Copy texture
        {
            auto srcChunk = context.Input.Header.Chunks[1];
            if (srcChunk == nullptr || srcChunk->IsMissing())
            {
                LOG(Warning, "Missing data chunk with a mipmap");
                return true;
            }
            if (context.AllocateChunk(0))
                return true;
            auto dstChunk = context.Output.Header.Chunks[0];

            dstChunk->Data.Copy(srcChunk->Data);
        }

        return false;
    }

    static bool Upgrade_1_To_4_PreviewsCache(AssetMigrationContext& context)
    {
        // Get texture header
        auto headerChunk = context.Input.Header.Chunks[0];
        if (headerChunk == nullptr || headerChunk->IsMissing())
        {
            LOG(Warning, "Missing texture header chunk.");
            return true;
        }
        MemoryReadStream headerStream(headerChunk->Get(), headerChunk->Size());

        // Load atlas header
        struct AtlasHeader1
        {
            int32 Version;
            int32 TextureHeaderVersion;
            TextureHeader1 TextureHeader;
        };
        AtlasHeader1 atlasHeader;
        headerStream.ReadBytes(&atlasHeader, sizeof(atlasHeader));
        if (atlasHeader.Version != 1 || atlasHeader.TextureHeaderVersion != 1)
        {
            LOG(Warning, "Invalid asset previews header.");
            return true;
        }
        auto& textureHeader = atlasHeader.TextureHeader;
        if (textureHeader.MipLevels != 1)
        {
            LOG(Warning, "Invalid asset previews texture header.");
            return true;
        }

        // Create new header
        TextureHeader newHeader;
        newHeader.Width = textureHeader.Width;
        newHeader.Height = textureHeader.Height;
        newHeader.MipLevels = textureHeader.MipLevels;
        newHeader.Format = PixelFormatOldToNew(textureHeader.Format);
        newHeader.IsSRGB = textureHeader.IsSRGB;
        newHeader.NeverStream = textureHeader.NeverStream;
        newHeader.Type = textureHeader.Type;
        context.Output.CustomData.Copy(&newHeader);

        // Copy asset previews IDs mapping data from old chunk to new one
        auto idsChunk = context.Input.Header.Chunks[1];
        if (idsChunk == nullptr || idsChunk->IsMissing())
        {
            LOG(Warning, "Missing asset previews IDs mapping data chunk.");
            return true;
        }
        if (context.AllocateChunk(15))
            return true;
        context.Output.Header.Chunks[15]->Data.Copy(idsChunk->Data);

        // Convert import options
        if (Upgrade_1_To_4_ImportOptions(context))
            return true;

        // Copy atlas image (older version has only single mip)
        {
            auto srcChunk = context.Input.Header.Chunks[2];
            if (srcChunk == nullptr || srcChunk->IsMissing())
            {
                LOG(Warning, "Missing data chunk with a mipmap");
                return true;
            }
            if (context.AllocateChunk(0))
                return true;
            auto dstChunk = context.Output.Header.Chunks[0];

            dstChunk->Data.Copy(srcChunk->Data);
        }

        return false;
    }

    static bool Upgrade_1_To_4(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 1 && context.Output.SerializedVersion == 4);

        // Peek asset type
        TextureOwnerType1 type;
        if (GetOwnerType(context, type))
            return true;

        switch (type)
        {
        case TextureOwnerType1::Texture:
            return Upgrade_1_To_4_Texture(context);
        case TextureOwnerType1::CubeTexture:
            return Upgrade_1_To_4_CubeTexture(context);
        case TextureOwnerType1::SpriteAtlas:
            return Upgrade_1_To_4_SpriteAtlas(context);
        case TextureOwnerType1::IESProfile:
            return Upgrade_1_To_4_IESProfile(context);
        case TextureOwnerType1::PreviewsCache:
            return Upgrade_1_To_4_PreviewsCache(context);
        }

        return true;
    }
};

#endif
