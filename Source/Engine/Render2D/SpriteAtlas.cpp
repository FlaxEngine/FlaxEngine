// Copyright (c) Wojciech Figat. All rights reserved.

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

REGISTER_BINARY_ASSET_WITH_UPGRADER(SpriteAtlas, "FlaxEngine.SpriteAtlas", TextureAssetUpgrader, true);

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

int32 SpriteAtlas::GetSpritesCount() const
{
    return Sprites.Count();
}

Sprite SpriteAtlas::GetSprite(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < Sprites.Count(), Sprite());
    return Sprites.Get()[index];
}

void SpriteAtlas::GetSpriteArea(int32 index, Rectangle& result) const
{
    CHECK(index >= 0 && index < Sprites.Count());
    result = Sprites.Get()[index].Area;
}

void SpriteAtlas::SetSprite(int32 index, const Sprite& value)
{
    CHECK(index >= 0 && index < Sprites.Count());
    Sprites.Get()[index] = value;
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
    stream.Write(1); // Version
    stream.Write(Sprites.Count()); // Sprites Count
    for (Sprite& t : Sprites)
    {
        stream.Write(t.Area);
        stream.Write(t.Name, 49);
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
    stream.Read(tilesVersion);
    if (tilesVersion != 1)
    {
#if USE_EDITOR
        if (gpuDevice)
            gpuDevice->Locker.Unlock();
#endif
        LOG(Warning, "Invalid tiles version.");
        return true;
    }
    stream.Read(tilesCount);
    Sprites.Resize(tilesCount);
    for (Sprite& t : Sprites)
    {
        stream.Read(t.Area);
        stream.Read(t.Name, 49);
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
    Sprites.Resize(0);

    // Base
    TextureBase::unload(isReloading);
}

AssetChunksFlag SpriteAtlas::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(15);
}
