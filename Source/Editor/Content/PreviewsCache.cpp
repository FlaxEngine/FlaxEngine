// Copyright (c) Wojciech Figat. All rights reserved.

#include "PreviewsCache.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Content/Upgraders/TextureAssetUpgrader.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/GPUDevice.h"

// Default asset preview icon size (both width and height since it's a square)
#define ASSET_ICON_SIZE 64

// Default assets previews atlas size
#define ASSETS_ICONS_ATLAS_SIZE 1024

// Default assets previews atlas margin between icons
#define ASSETS_ICONS_ATLAS_MARGIN 4

// Default format for assets previews atlas texture
#define ASSETS_ICONS_ATLAS_FORMAT PixelFormat::R8G8B8A8_UNorm

// Util macros
#define ASSETS_ICONS_PER_ROW (int)((float)ASSETS_ICONS_ATLAS_SIZE / (ASSET_ICON_SIZE + ASSETS_ICONS_ATLAS_MARGIN))
#define ASSETS_ICONS_PER_ATLAS (ASSETS_ICONS_PER_ROW*ASSETS_ICONS_PER_ROW)

bool PreviewsCache::FlushTask::Run()
{
    // Check if has valid data downloaded
    if (_data.GetMipLevels() != 1)
    {
        LOG(Warning, "Failed to flush asset previews atlas \'{0}\'.", _cache->ToString());
        return true;
    }
    auto mipData = _data.GetData(0, 0);
    ASSERT(mipData->DepthPitch == RenderTools::CalculateTextureMemoryUsage(_cache->GetTexture()->Format(), _cache->Width(), _cache->Height(), 1));

    ScopeLock lock(_cache->Locker);

    // Link chunks (don't allocate additional memory)
    auto mipChunk = _cache->GetOrCreateChunk(0);
    auto dataChunk = _cache->GetOrCreateChunk(15);
    mipChunk->Data.Link(mipData->Data);
    dataChunk->Data.Link((byte*)_cache->_assets.Get(), sizeof(Guid) * _cache->_assets.Count());

    // Prepare asset data
    AssetInitData data;
    data.SerializedVersion = 4;
    data.CustomData.Copy(_cache->_texture.GetHeader());

    // Save (use silent mode to prevent asset reloading)
    bool saveResult = _cache->SaveAsset(data, true);
    mipChunk->Data.Release();
    dataChunk->Data.Release();
    if (saveResult)
    {
        LOG(Warning, "Failed to save asset previews atlas \'{0}\'.", _cache->ToString());
        return true;
    }

    // Clear flag
    _cache->_isDirty = false;

    return false;
}

void PreviewsCache::FlushTask::OnEnd()
{
    ASSERT(_cache->_flushTask == this);
    _cache->_flushTask = nullptr;

    // Base
    ThreadPoolTask::OnEnd();
}

REGISTER_BINARY_ASSET_WITH_UPGRADER(PreviewsCache, "FlaxEditor.PreviewsCache", TextureAssetUpgrader, false);

PreviewsCache::PreviewsCache(const SpawnParams& params, const AssetInfo* info)
    : SpriteAtlas(params, info)
{
}

bool PreviewsCache::IsReady() const
{
    return IsLoaded() && GetTexture()->MipLevels() > 0;
}

SpriteHandle PreviewsCache::FindSlot(const Guid& id)
{
    if (WaitForLoaded())
        return SpriteHandle::Invalid;
    int32 index;
    if (_assets.Find(id, index))
    {
        const String spriteName = StringUtils::ToString(index);
        return FindSprite(spriteName);
    }
    return SpriteHandle::Invalid;
}

Asset::LoadResult PreviewsCache::load()
{
    // Load previews data
    auto previewsMetaChunk = GetChunk(15);
    if (previewsMetaChunk == nullptr || previewsMetaChunk->IsMissing())
        return LoadResult::MissingDataChunk;
    if (previewsMetaChunk->Size() != ASSETS_ICONS_PER_ATLAS * sizeof(Guid))
        return LoadResult::Failed;
    _assets.Set(previewsMetaChunk->Get<Guid>(), ASSETS_ICONS_PER_ATLAS);

    // Verify if cached assets still exist (don't store thumbnails for removed files)
    AssetInfo assetInfo;
    for (Guid& id : _assets)
    {
        if (id.IsValid() && Content::GetAsset(id) == nullptr && !Content::GetAssetInfo(id, assetInfo))
        {
            // Free slot (no matter the texture contents)
            id = Guid::Empty;
        }
    }

    // Setup atlas sprites array
    Sprite sprite;
    sprite.Area.Size = static_cast<float>(ASSET_ICON_SIZE) / ASSETS_ICONS_ATLAS_SIZE;
    const float positionScale = static_cast<float>(ASSET_ICON_SIZE + ASSETS_ICONS_ATLAS_MARGIN) / ASSETS_ICONS_ATLAS_SIZE;
    const float positionOffset = static_cast<float>(ASSETS_ICONS_ATLAS_MARGIN) / ASSETS_ICONS_ATLAS_SIZE;
    for (int32 i = 0; i < ASSETS_ICONS_PER_ATLAS; i++)
    {
        sprite.Area.Location = Float2(static_cast<float>(i % ASSETS_ICONS_PER_ROW), static_cast<float>(i / ASSETS_ICONS_PER_ROW)) * positionScale + positionOffset;
        sprite.Name = StringUtils::ToString(i);
        Sprites.Add(sprite);
    }

    _isDirty = false;
    return TextureBase::load();
}

void PreviewsCache::unload(bool isReloading)
{
    // Wait for flush end
    if (IsFlushing())
    {
        _flushTask->Cancel();
    }

    // Release data
    _assets.Clear();

    SpriteAtlas::unload(isReloading);
}

AssetChunksFlag PreviewsCache::getChunksToPreload() const
{
    // Preload previews ids data chunk
    return GET_CHUNK_FLAG(15);
}

bool PreviewsCache::HasFreeSlot() const
{
    // Unused slot is which ID is Empty
    // (Search from back to front since slots are allocated from front to back - it will be faster)
    return _assets.FindLast(Guid::Empty) != INVALID_INDEX;
}

SpriteHandle PreviewsCache::OccupySlot(GPUTexture* source, const Guid& id)
{
    if (WaitForLoaded())
        return SpriteHandle::Invalid;

    // Find this asset slot or use the first empty
    int32 index = _assets.Find(id);
    if (index == INVALID_INDEX)
        index = _assets.Find(Guid::Empty);
    if (index == INVALID_INDEX)
    {
        LOG(Warning, "Cannot find free slot in the asset previews atlas.");
        return SpriteHandle::Invalid;
    }

    ASSERT(IsReady());

    // Copy texture region
    uint32 x = ASSETS_ICONS_ATLAS_MARGIN + index % ASSETS_ICONS_PER_ROW * (ASSET_ICON_SIZE + ASSETS_ICONS_ATLAS_MARGIN);
    uint32 y = ASSETS_ICONS_ATLAS_MARGIN + index / ASSETS_ICONS_PER_ROW * (ASSET_ICON_SIZE + ASSETS_ICONS_ATLAS_MARGIN);
    GPUDevice::Instance->GetMainContext()->CopyTexture(GetTexture(), 0, x, y, 0, source, 0);

    // Occupy slot
    _assets[index] = id;

    // Get sprite handle
    const String spriteName = StringUtils::ToString(index);
    const auto slot = FindSprite(spriteName);
    if (!slot.IsValid())
    {
        LOG(Warning, "Cannot create sprite handle for asset preview.");
        return SpriteHandle::Invalid;
    }

    // Set dirty flag
    _isDirty = true;

    return slot;
}

bool PreviewsCache::ReleaseSlot(const Guid& id)
{
    bool result = false;
    ScopeLock lock(Locker);
    int32 index = _assets.Find(id);
    if (index != INVALID_INDEX)
    {
        _assets[index] = Guid::Empty;
        result = true;
    }
    return result;
}

void PreviewsCache::Flush()
{
    ScopeLock lock(Locker);

    // Check if is fully loaded and is dirty and is not during downloading
    if (_isDirty && !IsFlushing() && IsReady())
    {
        // Spawn flushing tasks sequence
        _flushTask = New<FlushTask>(this);
        auto downloadDataTask = GetTexture()->DownloadDataAsync(_flushTask->GetData());
        downloadDataTask->ContinueWith(_flushTask);
        downloadDataTask->Start();
    }
}

#if COMPILE_WITH_ASSETS_IMPORTER

bool PreviewsCache::Create(const StringView& outputPath)
{
    LOG(Info, "Creating new atlas '{0}' for assets previews cache. Size: {1}, capacity: {2}", outputPath, ASSETS_ICONS_ATLAS_SIZE, ASSETS_ICONS_PER_ATLAS);
    return AssetsImportingManager::Create(&create, outputPath);
}

CreateAssetResult PreviewsCache::create(CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(PreviewsCache, 4);

    // Create texture header (custom data)
    TextureHeader textureHeader;
    textureHeader.Width = ASSETS_ICONS_ATLAS_SIZE;
    textureHeader.Height = ASSETS_ICONS_ATLAS_SIZE;
    textureHeader.Format = ASSETS_ICONS_ATLAS_FORMAT;
    textureHeader.MipLevels = 1;
    textureHeader.NeverStream = true;
    context.Data.CustomData.Copy(&textureHeader);

    // Create blank image (chunk 0)
    uint64 imageSize = RenderTools::CalculateTextureMemoryUsage(ASSETS_ICONS_ATLAS_FORMAT, ASSETS_ICONS_ATLAS_SIZE, ASSETS_ICONS_ATLAS_SIZE, 1);
    ASSERT(imageSize <= MAX_int32);
    if (context.AllocateChunk(0))
        return CreateAssetResult::CannotAllocateChunk;
    auto mipChunk = context.Data.Header.Chunks[0];
    mipChunk->Data.Allocate((int32)imageSize);
    Platform::MemoryClear(mipChunk->Get(), mipChunk->Size());

    // Create IDs cache array (chunk 15)
    int32 idsSize = sizeof(Guid) * ASSETS_ICONS_PER_ATLAS;
    if (context.AllocateChunk(15))
        return CreateAssetResult::CannotAllocateChunk;
    auto dataChunk = context.Data.Header.Chunks[15];
    dataChunk->Data.Allocate(idsSize);
    Platform::MemoryClear(dataChunk->Get(), dataChunk->Size());

    return CreateAssetResult::Ok;
}

#endif
