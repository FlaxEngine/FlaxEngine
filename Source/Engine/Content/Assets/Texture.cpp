// Copyright (c) Wojciech Figat. All rights reserved.

#include "Texture.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/TextureAssetUpgrader.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Scripting/Internal/MainThreadManagedInvokeAction.h"
#include "Engine/Tools/TextureTool/TextureTool.h"

REGISTER_BINARY_ASSET_WITH_UPGRADER(Texture, "FlaxEngine.Texture", TextureAssetUpgrader, true);

Texture::Texture(const SpawnParams& params, const AssetInfo* info)
    : TextureBase(params, info)
{
}

TextureFormatType Texture::GetFormatType() const
{
    return _texture.GetFormatType();
}

bool Texture::IsNormalMap() const
{
    return _texture.GetFormatType() == TextureFormatType::NormalMap;
}

#if USE_EDITOR

bool Texture::Save(const StringView& path)
{
    return Save(path, nullptr);
}

bool Texture::Save(const StringView& path, const InitData* customData)
{
    if (OnCheckSave())
        return true;
    ScopeLock lock(Locker);

    AssetInitData data;
    const class StreamingTexture* texture = StreamingTexture();

    // Use a temporary chunks for data storage for virtual assets
    FlaxChunk* tmpChunks[ASSET_FILE_DATA_CHUNKS];
    Platform::MemoryClear(tmpChunks, sizeof(tmpChunks));
    Array<FlaxChunk> chunks;
    if (IsVirtual())
        chunks.Resize(ASSET_FILE_DATA_CHUNKS);
#define GET_CHUNK(index) (IsVirtual() ? tmpChunks[index] = &chunks[index] : GetOrCreateChunk(index))

    // Save header
    data.CustomData.Copy(texture->GetHeader());

    // Validate custom data
    if (customData == nullptr)
        customData = _customData;
    if (IsVirtual())
    {
        // Virtual asset must have a custom data provided
        if (!customData)
        {
            LOG(Error, "To save virtual texture you need to initialize it first with a valid data.");
            return true;
        }
    }
    else if (customData)
    {
        // Normal asset if has custom data passed it must match the texture info
        if (customData->Mips.Count() != texture->TotalMipLevels() ||
            customData->ArraySize != texture->TotalArraySize() ||
            customData->Format != Format() ||
            customData->Width != Width() ||
            customData->Height != Height())
        {
            LOG(Error, "Invalid custom texture data to save.");
            return true;
        }
    }

    // Get texture data to chunks
    if (customData)
    {
        for (int32 mipIndex = 0; mipIndex < texture->TotalMipLevels(); mipIndex++)
        {
            auto chunk = GET_CHUNK(mipIndex);

            uint32 rowPitch, slicePitch;
            const int32 mipWidth = Math::Max(1, Width() >> mipIndex);
            const int32 mipHeight = Math::Max(1, Height() >> mipIndex);
            RenderTools::ComputePitch(Format(), mipWidth, mipHeight, rowPitch, slicePitch);

            chunk->Data.Allocate(slicePitch * texture->TotalArraySize());

            auto& mipData = customData->Mips[mipIndex];
            if (mipData.Data.Length() != (int32)mipData.SlicePitch * texture->TotalArraySize())
            {
                LOG(Error, "Invalid custom texture data (slice pitch * array size is different than data bytes count).");
                return true;
            }

            // Faster path if source and destination data layout matches
            if (rowPitch == mipData.RowPitch && slicePitch == mipData.SlicePitch)
            {
                const auto src = mipData.Data.Get();
                const auto dst = chunk->Data.Get();

                Platform::MemoryCopy(dst, src, slicePitch * texture->TotalArraySize());
            }
            else
            {
                const auto copyRowSize = Math::Min(mipData.RowPitch, rowPitch);

                for (int32 slice = 0; slice < texture->TotalArraySize(); slice++)
                {
                    auto src = mipData.Data.Get() + slice * mipData.SlicePitch;
                    auto dst = chunk->Data.Get() + slice * slicePitch;

                    for (int32 line = 0; line < texture->TotalHeight(); line++)
                    {
                        Platform::MemoryCopy(dst, src, copyRowSize);

                        src += mipData.RowPitch;
                        dst += rowPitch;
                    }
                }
            }
        }
    }
    else
    {
        // Load all chunks
        if (LoadChunks(ALL_ASSET_CHUNKS))
            return true;
    }

#undef GET_CHUNK

    // Save
    data.SerializedVersion = SerializedVersion;
    if (IsVirtual())
        Platform::MemoryCopy(_header.Chunks, tmpChunks, sizeof(_header.Chunks));
    const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
    if (IsVirtual())
        Platform::MemoryClear(_header.Chunks, sizeof(_header.Chunks));
    if (saveResult)
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

#endif

bool Texture::LoadFile(const StringView& path, bool generateMips)
{
    if (!IsVirtual())
    {
        LOG(Error, "Loading image from file is supported only for virtual textures.");
        return true;
    }

    TextureData textureData;
    if (TextureTool::ImportTexture(path, textureData))
    {
        return true;
    }

    auto initData = New<InitData>();
    initData->FromTextureData(textureData, generateMips);

    return Init(initData);
}

Texture* Texture::FromFile(const StringView& path, bool generateMips)
{
    auto texture = Content::CreateVirtualAsset<Texture>();
    if (texture->LoadFile(path, generateMips))
    {
        texture->DeleteObject();
        texture = nullptr;
    }
    return texture;
}
