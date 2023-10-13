// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "SpriteAtlas.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Loading/Tasks/LoadAssetDataTask.h"
#include "Engine/Content/Upgraders/TextureAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/GPUDevice.h"

const SpriteHandle SpriteHandle::Invalid = { nullptr, Guid::Empty };

REGISTER_BINARY_ASSET_WITH_UPGRADER(SpriteAtlas, "FlaxEngine.SpriteAtlas", TextureAssetUpgrader, true);

bool SpriteHandle::GetSprite(Sprite* result) const
{
    if (IsValid())
    {
        Atlas->Sprites.TryGet(Id, *result);
        return result != nullptr;
    }
    return false;
}

bool SpriteHandle::IsValid() const
{
    // TODO: Check that Guid is valid by making sure it's part of the Atlas?
    return Atlas && Id != Guid::Empty;
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

int32 SpriteAtlas::GetSpritesCount() const
{
    return Sprites.Count();
}

Sprite SpriteAtlas::GetSprite(Guid id) const
{
    CHECK_RETURN(Sprites.ContainsKey(id), Sprite())
    return Sprites.At(id);
}

void SpriteAtlas::SetSprite(Guid id, const Sprite& value)
{
    CHECK(Sprites.ContainsKey(id))
    Sprites.At(id) = value;
}

SpriteHandle SpriteAtlas::FindSprite(const StringView& name) const
{
    SpriteHandle result(const_cast<SpriteAtlas*>(this), Guid::Empty);
    for (const auto &pair : Sprites)
    {
        if (name == pair.Value.Name)
        {
            result.Id = pair.Key;
            break;
        }
    }
    return result;
}

SpriteHandle SpriteAtlas::AddSprite(const Sprite& sprite)
{
    Sprites[sprite.Id] = sprite;
    return SpriteHandle(this, sprite.Id);
}

void SpriteAtlas::RemoveSprite(Guid id)
{
    Sprites.Remove(id);
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
    stream.WriteInt32(2); // Version
    stream.WriteInt32(Sprites.Count()); // Sprites Count
    for (const auto &pair : Sprites)
    {
        const Sprite &t = pair.Value;

        stream.Write(t.Area);
        stream.WriteString(t.Name, 49);
        stream.WriteUint32(t.Id.Values[0]);
        stream.WriteUint32(t.Id.Values[1]);
        stream.WriteUint32(t.Id.Values[2]);
        stream.WriteUint32(t.Id.Values[3]);
    }

    // Link sprites data (unlink after safe)
    auto dataChunk = GetOrCreateChunk(15);
    dataChunk->Data.Link(stream.GetHandle(), stream.GetLength());

    // Save (use silent mode to prevent asset reloading)
    bool saveResult = SaveAsset(data, true);
    dataChunk->Data.Unlink();
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
    ScopeLock lock(Locker);

    // Sprites may be used on rendering thread so lock drawing for a while
#if USE_EDITOR
    GPUDevice* gpuDevice = GPUDevice::Instance;
    if (gpuDevice)
        gpuDevice->Locker.Lock();
#endif

    Sprites.Clear();

    int32 tilesVersion, tilesCount;
    stream.ReadInt32(&tilesVersion);
    if (tilesVersion > 2)
    {
#if USE_EDITOR
        if (gpuDevice)
            gpuDevice->Locker.Unlock();
#endif
        LOG(Warning, "Invalid tiles version.");
        return true;
    }
    stream.ReadInt32(&tilesCount);
    Sprites.EnsureCapacity(tilesCount);
    for (int i = 0; i < tilesCount; ++i)
    {
        Sprite t{};

        stream.Read(t.Area);
        stream.ReadString(&t.Name, 49);
        if (tilesVersion >= 2)
        {
            stream.ReadUint32(&t.Id.Values[0]);
            stream.ReadUint32(&t.Id.Values[1]);
            stream.ReadUint32(&t.Id.Values[2]);
            stream.ReadUint32(&t.Id.Values[3]);
        }
        else
        {
            t.Id = Guid::New();
        }

        Sprites[t.Id] = t;
    }

#if USE_EDITOR
    if (gpuDevice)
        gpuDevice->Locker.Unlock();
#endif
    return false;
}

Asset::LoadResult SpriteAtlas::load()
{
    auto spritesDataChunk = GetChunk(15);
    if (spritesDataChunk == nullptr || spritesDataChunk->IsMissing())
        return LoadResult::MissingDataChunk;
    MemoryReadStream spritesData(spritesDataChunk->Get(), spritesDataChunk->Size());

    if (LoadSprites(spritesData))
    {
        LOG(Warning, "Cannot load sprites atlas data.");
        return LoadResult::Failed;
    }

    return TextureBase::load();
}

void SpriteAtlas::unload(bool isReloading)
{
    Sprites.Clear();

    // Base
    TextureBase::unload(isReloading);
}

AssetChunksFlag SpriteAtlas::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(15);
}
