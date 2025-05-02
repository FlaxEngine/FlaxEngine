// Copyright (c) Wojciech Figat. All rights reserved.

#include "ImportTexture.h"
#if COMPILE_WITH_ASSETS_IMPORTER
#include "Engine/Core/Log.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include "Engine/ContentImporters/ImportIES.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/Assets/IESProfile.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"

bool IsSpriteAtlasOrTexture(const String& typeName)
{
    return typeName == Texture::TypeName || typeName == SpriteAtlas::TypeName;
}

bool ImportTexture::TryGetImportOptions(const StringView& path, Options& options)
{
#if IMPORT_TEXTURE_CACHE_OPTIONS
    if (FileSystem::FileExists(path))
    {
        auto tmpFile = ContentStorageManager::GetStorage(path);
        AssetInitData data;
        if (tmpFile
            && tmpFile->GetEntriesCount() == 1
            && IsSpriteAtlasOrTexture(tmpFile->GetEntry(0).TypeName)
            && !tmpFile->LoadAssetHeader(0, data)
            && data.SerializedVersion >= 4)
        {
            // For sprite atlas try to get sprites from the last chunk
            if (tmpFile->GetEntry(0).TypeName == SpriteAtlas::TypeName)
            {
                auto chunk15 = data.Header.Chunks[15];
                if (chunk15 != nullptr && !tmpFile->LoadAssetChunk(chunk15) && chunk15->Data.IsValid())
                {
                    MemoryReadStream stream(chunk15->Data.Get(), chunk15->Data.Length());
                    int32 tilesVersion, tilesCount;
                    stream.Read(tilesVersion);
                    if (tilesVersion == 1)
                    {
                        options.Sprites.Clear();
                        stream.Read(tilesCount);
                        for (int32 i = 0; i < tilesCount; i++)
                        {
                            // Load sprite
                            Sprite t;
                            stream.Read(t.Area);
                            stream.Read(t.Name, 49);
                            options.Sprites.Add(t);
                        }
                    }
                }
            }

            // Check import meta
            rapidjson_flax::Document metadata;
            metadata.Parse((const char*)data.Metadata.Get(), data.Metadata.Length());
            if (metadata.HasParseError() == false)
            {
                options.Deserialize(metadata, nullptr);
                return true;
            }
        }
    }
#endif
    return false;
}

void ImportTexture::InitOptions(CreateAssetContext& context, Options& options)
{
    // Gather import options
    if (context.CustomArg != nullptr)
    {
        // Copy options
        options = *static_cast<Options*>(context.CustomArg);
        ASSERT_LOW_LAYER(options.Sprites.Count() >= 0);
    }
    else
    {
        // Restore the previous settings or use default ones
        if (!TryGetImportOptions(context.TargetAssetPath, options))
        {
            LOG(Warning, "Missing texture import options. Using default values.");
        }
    }

    // Tweak options
    if (options.IsAtlas)
    {
        // Disable streaming for atlases
        // TODO: maybe we could use streaming for atlases?
        options.NeverStream = true;

        // Add default tile if has no sprites
        if (options.Sprites.IsEmpty())
            options.Sprites.Add({ Rectangle(Float2::Zero, Float2::One), TEXT("Default") });
    }
    options.MaxSize = Math::Min(options.MaxSize, GPU_MAX_TEXTURE_SIZE);
}

CreateAssetResult ImportTexture::Create(CreateAssetContext& context, const TextureData& textureData, Options& options)
{
    // Check data
    bool isCubeMap = false;
    if (textureData.GetArraySize() != 1)
    {
        if (options.IsAtlas)
        {
            LOG(Warning, "Cannot import sprite atlas texture that has more than one array slice.");
            return CreateAssetResult::Error;
        }
        if (textureData.GetArraySize() != 6)
        {
            LOG(Warning, "Cannot import texture that has {0} array slices. Use single plane images (single 2D) or cube maps (6 slices).", textureData.GetArraySize());
            return CreateAssetResult::Error;
        }
        else
        {
            isCubeMap = true;

            if (textureData.Width != textureData.Height)
            {
                LOG(Warning, "Invalid cube texture size.");
                return CreateAssetResult::Error;
            }
        }
    }

    // Base
    if (isCubeMap)
    {
        IMPORT_SETUP(CubeTexture, 4);
    }
    else if (options.IsAtlas)
    {
        IMPORT_SETUP(SpriteAtlas, 4);
    }
    else
    {
        IMPORT_SETUP(Texture, 4);
    }

    // Fill texture header
    TextureHeader textureHeader;
    textureHeader.NeverStream = options.NeverStream;
    textureHeader.Width = textureData.Width;
    textureHeader.Height = textureData.Height;
    textureHeader.Format = textureData.Format;
    textureHeader.Type = options.Type;
    textureHeader.MipLevels = textureData.GetMipLevels();
    textureHeader.IsSRGB = PixelFormatExtensions::IsSRGB(textureHeader.Format);
    textureHeader.IsCubeMap = isCubeMap;
    textureHeader.TextureGroup = options.TextureGroup;
    ASSERT(textureHeader.MipLevels <= GPU_MAX_TEXTURE_MIP_LEVELS);

    // Save header
    context.Data.CustomData.Copy(&textureHeader);

    // Save atlas sprites data
    if (options.IsAtlas)
    {
        MemoryWriteStream stream(256);
        stream.Write(1); // Version
        stream.Write(options.Sprites.Count()); // Amount of tiles
        for (int32 i = 0; i < options.Sprites.Count(); i++)
        {
            auto& sprite = options.Sprites[i];
            stream.Write(sprite.Area);
            stream.Write(sprite.Name, 49);
        }
        if (context.AllocateChunk(15))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[15]->Data.Copy(ToSpan(stream));
    }

    // Save mip maps
    if (!isCubeMap)
    {
        for (int32 mipIndex = 0; mipIndex < textureHeader.MipLevels; mipIndex++)
        {
            auto mipData = textureData.GetData(0, mipIndex);

            if (context.AllocateChunk(mipIndex))
                return CreateAssetResult::CannotAllocateChunk;
            context.Data.Header.Chunks[mipIndex]->Data.Copy(mipData->Data.Get(), static_cast<uint32>(mipData->DepthPitch));
        }
    }
    else
    {
        // Allocate memory for a temporary buffer
        const uint32 imageSize = textureData.GetData(0, 0)->DepthPitch * 6;
        MemoryWriteStream imageData(imageSize);

        // Copy cube sides for every mip into separate chunks
        for (int32 mipLevelIndex = 0; mipLevelIndex < textureHeader.MipLevels; mipLevelIndex++)
        {
            // Write array slices to the stream
            imageData.SetPosition(0);
            for (int32 cubeFaceIndex = 0; cubeFaceIndex < 6; cubeFaceIndex++)
            {
                // Get image
                const auto image = textureData.GetData(cubeFaceIndex, mipLevelIndex);
                if (image == nullptr)
                {
                    LOG(Warning, "Cannot create cube texture '{0}'. Missing image slice.", context.InputPath);
                    return CreateAssetResult::Error;
                }
                ASSERT(image->DepthPitch < MAX_int32);

                // Copy data
                imageData.WriteBytes(image->Data.Get(), image->Data.Length());
            }

            // Copy mip
            if (context.AllocateChunk(mipLevelIndex))
                return CreateAssetResult::CannotAllocateChunk;
            context.Data.Header.Chunks[mipLevelIndex]->Data.Copy(imageData.GetHandle(), imageData.GetPosition());
        }
    }

#if IMPORT_TEXTURE_CACHE_OPTIONS

    // Create json with import context
    rapidjson_flax::StringBuffer importOptionsMetaBuffer;
    importOptionsMetaBuffer.Reserve(256);
    CompactJsonWriter importOptionsMeta(importOptionsMetaBuffer);
    importOptionsMeta.StartObject();
    {
        context.AddMeta(importOptionsMeta);
        options.Serialize(importOptionsMeta, nullptr);
    }
    importOptionsMeta.EndObject();
    context.Data.Metadata.Copy((const byte*)importOptionsMetaBuffer.GetString(), (uint32)importOptionsMetaBuffer.GetSize());

#endif

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportTexture::Create(CreateAssetContext& context, const TextureBase::InitData& textureData, Options& options)
{
    // Check data
    bool isCubeMap = false;
    if (textureData.ArraySize != 1)
    {
        if (options.IsAtlas)
        {
            LOG(Warning, "Cannot import sprite atlas texture that has more than one array slice.");
            return CreateAssetResult::Error;
        }
        if (textureData.ArraySize != 6)
        {
            LOG(Warning, "Cannot import texture that has {0} array slices. Use single plane images (single 2D) or cube maps (6 slices).", textureData.ArraySize);
            return CreateAssetResult::Error;
        }
        else
        {
            isCubeMap = true;

            if (textureData.Width != textureData.Height)
            {
                LOG(Warning, "Invalid cube texture size.");
                return CreateAssetResult::Error;
            }
        }
    }

    // Base
    if (isCubeMap)
    {
        IMPORT_SETUP(CubeTexture, 4);
    }
    else if (options.IsAtlas)
    {
        IMPORT_SETUP(SpriteAtlas, 4);
    }
    else
    {
        IMPORT_SETUP(Texture, 4);
    }

    // Fill texture header
    TextureHeader textureHeader;
    textureHeader.NeverStream = options.NeverStream;
    textureHeader.Width = textureData.Width;
    textureHeader.Height = textureData.Height;
    textureHeader.Format = textureData.Format;
    textureHeader.Type = options.Type;
    textureHeader.MipLevels = textureData.Mips.Count();
    textureHeader.IsSRGB = PixelFormatExtensions::IsSRGB(textureHeader.Format);
    textureHeader.IsCubeMap = isCubeMap;
    textureHeader.TextureGroup = options.TextureGroup;
    ASSERT(textureHeader.MipLevels <= GPU_MAX_TEXTURE_MIP_LEVELS);

    // Save header
    context.Data.CustomData.Copy(&textureHeader);

    // Save atlas sprites data
    if (options.IsAtlas)
    {
        MemoryWriteStream stream(256);
        stream.Write(1); // Version
        stream.Write(options.Sprites.Count()); // Amount of tiles
        for (int32 i = 0; i < options.Sprites.Count(); i++)
        {
            auto& sprite = options.Sprites[i];
            stream.Write(sprite.Area);
            stream.Write(sprite.Name, 49);
        }
        if (context.AllocateChunk(15))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[15]->Data.Copy(ToSpan(stream));
    }

    // Save mip maps
    if (!isCubeMap)
    {
        for (int32 mipIndex = 0; mipIndex < textureHeader.MipLevels; mipIndex++)
        {
            auto& mipData = textureData.Mips[mipIndex];

            if (context.AllocateChunk(mipIndex))
                return CreateAssetResult::CannotAllocateChunk;
            context.Data.Header.Chunks[mipIndex]->Data.Copy(mipData.Data.Get(), static_cast<uint32>(mipData.SlicePitch));
        }
    }
    else
    {
        // Allocate memory for a temporary buffer
        const uint32 imageSize = textureData.Mips[0].SlicePitch * 6;
        MemoryWriteStream imageData(imageSize);

        // Copy cube sides for every mip into separate chunks
        for (int32 mipLevelIndex = 0; mipLevelIndex < textureHeader.MipLevels; mipLevelIndex++)
        {
            // Write array slices to the stream
            imageData.SetPosition(0);
            for (int32 cubeFaceIndex = 0; cubeFaceIndex < 6; cubeFaceIndex++)
            {
                // Get image
                auto& image = textureData.Mips[mipLevelIndex];
                const auto data = image.Data.Get() + image.SlicePitch * cubeFaceIndex;

                // Copy data
                imageData.WriteBytes(data, static_cast<int32>(image.SlicePitch));
            }

            // Copy mip
            if (context.AllocateChunk(mipLevelIndex))
                return CreateAssetResult::CannotAllocateChunk;
            context.Data.Header.Chunks[mipLevelIndex]->Data.Copy(imageData.GetHandle(), imageData.GetPosition());
        }
    }

#if IMPORT_TEXTURE_CACHE_OPTIONS

    // Create json with import context
    rapidjson_flax::StringBuffer importOptionsMetaBuffer;
    importOptionsMetaBuffer.Reserve(256);
    CompactJsonWriter importOptionsMeta(importOptionsMetaBuffer);
    importOptionsMeta.StartObject();
    {
        context.AddMeta(importOptionsMeta);
        options.Serialize(importOptionsMeta, nullptr);
    }
    importOptionsMeta.EndObject();
    context.Data.Metadata.Copy((const byte*)importOptionsMetaBuffer.GetString(), (uint32)importOptionsMetaBuffer.GetSize());

#endif

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportTexture::Import(CreateAssetContext& context)
{
    Options options;
    InitOptions(context, options);

    // Import
    TextureData textureData;
    String errorMsg;
    if (TextureTool::ImportTexture(context.InputPath, textureData, options, errorMsg))
    {
        LOG(Error, "Cannot import texture. {0}", errorMsg);
        return CreateAssetResult::Error;
    }

    return Create(context, textureData, options);
}

CreateAssetResult ImportTexture::ImportAsTextureData(CreateAssetContext& context)
{
    ASSERT(context.CustomArg != nullptr);
    return Create(context, static_cast<TextureData*>(context.CustomArg));
}

CreateAssetResult ImportTexture::ImportAsInitData(CreateAssetContext& context)
{
    ASSERT(context.CustomArg != nullptr);
    return Create(context, static_cast<TextureBase::InitData*>(context.CustomArg));
}

CreateAssetResult ImportTexture::Create(CreateAssetContext& context, TextureData* textureData)
{
    Options options;
    return Create(context, *textureData, options);
}

CreateAssetResult ImportTexture::Create(CreateAssetContext& context, TextureBase::InitData* initData)
{
    Options options;
    return Create(context, *initData, options);
}

CreateAssetResult ImportTexture::ImportCube(CreateAssetContext& context)
{
    ASSERT(context.CustomArg != nullptr);
    return CreateCube(context, static_cast<TextureData*>(context.CustomArg));
}

CreateAssetResult ImportTexture::CreateCube(CreateAssetContext& context, TextureData* textureData)
{
    // Validate
    if (textureData == nullptr)
    {
        LOG(Warning, "Missing argument.");
        return CreateAssetResult::Error;
    }
    if (textureData->GetArraySize() != 6)
    {
        LOG(Warning, "Invalid cube texture array size.");
        return CreateAssetResult::Error;
    }
    if (textureData->Width != textureData->Height)
    {
        LOG(Warning, "Invalid cube texture size.");
        return CreateAssetResult::Error;
    }

    // Base
    IMPORT_SETUP(CubeTexture, 4);

    // Cache data
    int32 size = textureData->Width;
    PixelFormat format = textureData->Format;
    bool isSRGB = PixelFormatExtensions::IsSRGB(format);
    int32 mipLevels = textureData->GetMipLevels();

    // Fill texture header
    TextureHeader textureHeader;
    textureHeader.IsSRGB = isSRGB;
    textureHeader.Width = size;
    textureHeader.Height = size;
    textureHeader.IsCubeMap = true;
    textureHeader.NeverStream = true; // TODO: could we support streaming for cube textures?
    textureHeader.Type = TextureFormatType::Unknown;
    textureHeader.Format = format;
    textureHeader.MipLevels = mipLevels;
    ASSERT(textureHeader.MipLevels <= GPU_MAX_TEXTURE_MIP_LEVELS);

    // Log info
    LOG(Info, "Importing cube texture '{0}': size: {1}, format: {2}, mip levels: {3}, sRGB: {4}", context.TargetAssetPath, size, static_cast<int32>(format), static_cast<int32>(textureHeader.MipLevels), textureHeader.IsSRGB);

    // Save header
    context.Data.CustomData.Copy(&textureHeader);

    // Allocate memory for a temporary buffer
    const uint32 imageSize = textureData->GetData(0, 0)->DepthPitch * 6;
    MemoryWriteStream imageData(imageSize);

    // Copy cube sides for every mip into separate chunks
    for (int32 mipLevelIndex = 0; mipLevelIndex < mipLevels; mipLevelIndex++)
    {
        // Write array slices to the stream
        imageData.SetPosition(0);
        for (int32 cubeFaceIndex = 0; cubeFaceIndex < 6; cubeFaceIndex++)
        {
            // Get image
            auto image = textureData->GetData(cubeFaceIndex, mipLevelIndex);
            if (image == nullptr)
            {
                LOG(Warning, "Cannot create cube texture '{0}'. Missing image slice.", context.InputPath);
                return CreateAssetResult::Error;
            }
            ASSERT(image->DepthPitch < MAX_int32);

            // Copy data
            imageData.WriteBytes(image->Data.Get(), image->Data.Length());
        }

        // Copy mip
        if (context.AllocateChunk(mipLevelIndex))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[mipLevelIndex]->Data.Copy(imageData.GetHandle(), imageData.GetPosition());
    }

    return CreateAssetResult::Ok;
}

CreateAssetResult ImportTexture::ImportIES(class CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(IESProfile, 4);

    // Load file
    Array<byte> fileData;
    if (File::ReadAllBytes(context.InputPath, fileData))
    {
        return CreateAssetResult::InvalidPath;
    }
    fileData.Add('\0');

    // Load IES profile data
    ::ImportIES loader;
    if (loader.Load(fileData.Get()))
    {
        return CreateAssetResult::Error;
    }

    // Extract texture data
    Array<byte> rawData;
    const float multiplier = loader.ExtractInR16(rawData);

    // Fill texture header
    TextureHeader textureHeader;
    textureHeader.Width = loader.GetWidth();
    textureHeader.Height = loader.GetHeight();
    textureHeader.MipLevels = 1;
    textureHeader.Type = TextureFormatType::Unknown;
    textureHeader.Format = PixelFormat::R16_Float;
    auto data = (IESProfile::CustomDataLayout*)textureHeader.CustomData;
    static_assert(sizeof(IESProfile::CustomDataLayout) <= sizeof(textureHeader.CustomData), "Invalid Custom Data size in Texture Header.");
    data->Brightness = loader.GetBrightness();
    data->TextureMultiplier = multiplier;
    ASSERT(textureHeader.MipLevels <= GPU_MAX_TEXTURE_MIP_LEVELS);
    context.Data.CustomData.Copy(&textureHeader);

    // Set mip
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    context.Data.Header.Chunks[0]->Data.Copy(rawData);

    return CreateAssetResult::Ok;
}

#endif
