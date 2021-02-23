// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SpriteAtlas.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Loading/Tasks/LoadAssetDataTask.h"
#include "Engine/Content/Upgraders/TextureAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/GPUDevice.h"

const SpriteHandle SpriteHandle::Invalid = { nullptr, INVALID_INDEX };

REGISTER_BINARY_ASSET(SpriteAtlas, "FlaxEngine.SpriteAtlas", ::New<TextureAssetUpgrader>(), true);

bool SpriteHandle::GetSprite(Sprite* result) const
{
    if (IsValid())
    {
        *result = Atlas->Sprites[Index];
        return true;
    }
    return false;
}

bool SpriteHandle::IsValid() const
{
    return Atlas && Index >= 0 && Atlas->Sprites.Count() > Index;
}

GPUTexture* SpriteHandle::GetAtlasTexture() const
{
    ASSERT(Atlas);
    return Atlas->GetTexture();
}

SpriteAtlas::SpriteAtlas(const SpawnParams& params, const AssetInfo* info)
    : TextureBase(params, info)
{
}

SpriteHandle SpriteAtlas::FindSprite(const StringView& name) const
{
    SpriteHandle result(const_cast<SpriteAtlas*>(this), -1);

    for (int32 i = 0; i < Sprites.Count(); i++)
    {
        if (name == Sprites[i].Name)
        {
            result.Index = i;
            break;
        }
    }

    return result;
}

SpriteHandle SpriteAtlas::AddSprite(const Sprite& sprite)
{
    const int32 index = Sprites.Count();

    Sprites.Add(sprite);

    return SpriteHandle(this, index);
}

void SpriteAtlas::RemoveSprite(int32 index)
{
    Sprites.RemoveAt(index);
}

#if USE_EDITOR

bool SpriteAtlas::SaveSprites()
{
    ScopeLock lock(Locker);

    // Load whole asset
    if (LoadChunks(ALL_ASSET_CHUNKS))
    {
        return true;
    }

    // Prepare asset data
    AssetInitData data;
    data.SerializedVersion = 4;
    data.CustomData.Copy(_texture.GetHeader());

    // Write sprites data
    MemoryWriteStream stream(1024);
    stream.WriteInt32(1); // Version
    stream.WriteInt32(Sprites.Count()); // Sprites Count
    for (int32 i = 0; i < Sprites.Count(); i++)
    {
        // Save sprite
        Sprite t = Sprites[i];
        stream.Write(&t.Area);
        stream.WriteString(t.Name, 49);
    }

    // Link sprites data (unlink after safe)
    auto dataChunk = GetOrCreateChunk(15);
    dataChunk->Data.Link(stream.GetHandle(), stream.GetLength());

    // Save (use silent mode to prevent asset reloading)
    bool saveResult = SaveAsset(data, true);
    dataChunk->Data.Release();
    if (saveResult)
    {
        LOG(Warning, "Failed to save sprite atlas \'{0}\'.", GetPath());
        return true;
    }

    return false;
}

#endif

bool SpriteAtlas::LoadSprites(ReadStream& stream)
{
#if USE_EDITOR
    // Sprites may be used on rendering thread so lock drawing for a while
    if (GPUDevice::Instance)
        GPUDevice::Instance->Locker.Lock();
#endif

    // Cleanup first
    Sprites.Clear();

    // Load tiles data
    int32 tilesVersion, tilesCount;
    stream.ReadInt32(&tilesVersion);
    if (tilesVersion != 1)
    {
#if USE_EDITOR
        if (GPUDevice::Instance)
            GPUDevice::Instance->Locker.Unlock();
#endif
        LOG(Warning, "Invalid tiles version.");
        return true;
    }
    stream.ReadInt32(&tilesCount);
    Sprites.Resize(tilesCount);
    for (int32 i = 0; i < tilesCount; i++)
    {
        // Load sprite
        Sprite& t = Sprites[i];
        stream.Read(&t.Area);
        stream.ReadString(&t.Name, 49);
    }

#if USE_EDITOR
    if (GPUDevice::Instance)
        GPUDevice::Instance->Locker.Unlock();
#endif
    return false;
}

Asset::LoadResult SpriteAtlas::load()
{
    // Get sprites data
    auto spritesDataChunk = GetChunk(15);
    if (spritesDataChunk == nullptr || spritesDataChunk->IsMissing())
        return LoadResult::MissingDataChunk;
    MemoryReadStream spritesData(spritesDataChunk->Get(), spritesDataChunk->Size());

    // Load sprites
    if (LoadSprites(spritesData))
    {
        LOG(Warning, "Cannot load sprites atlas data.");
        return LoadResult::Failed;
    }

    return TextureBase::load();
}

void SpriteAtlas::unload(bool isReloading)
{
    // Release sprites
    Sprites.Resize(0);

    // Base
    TextureBase::unload(isReloading);
}

AssetChunksFlag SpriteAtlas::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(15);
}
