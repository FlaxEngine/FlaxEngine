// Copyright (c) Wojciech Figat. All rights reserved.

#include "SpriteAtlasPacker.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Content/Upgraders/TextureAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/Textures/TextureSampler.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Utilities/RectPack.h"
#if USE_EDITOR
#include "Engine/Platform/FileSystem.h"
#include "Engine/Content/Content.h"
#endif
#include "Engine/Streaming/Streaming.h"

REGISTER_BINARY_ASSET_WITH_UPGRADER(SpriteAtlasPacker, "FlaxEngine.SpriteAtlasPacker", TextureAssetUpgrader, true);

SpriteAtlasPacker::SpriteAtlasPacker(const SpawnParams& params, const AssetInfo* info)
    : SpriteAtlas(params, info)
{
}

#if USE_EDITOR

bool SpriteAtlasPacker::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    return SaveInternal(path);
}

bool SpriteAtlasPacker::SaveInternal(const StringView& path)
{
    ScopeLock lock(Locker);
    SaveContext context(this);

    // Load whole asset
    if (LoadChunks(ALL_ASSET_CHUNKS))
        return true;

    // Write packing data
    MemoryWriteStream packingStream(1024);
    if (SavePacking(packingStream))
        return true;
    context.LinkChunk(14, ToSpan(packingStream));

    // Write sprites data
    MemoryWriteStream spritesStream(1024);
    if (SaveSprites(spritesStream))
        return true;
    context.LinkChunk(15, ToSpan(spritesStream));

    // Save
    AssetInitData data;
    data.SerializedVersion = SerializedVersion;
    data.CustomData.Copy(_texture.GetHeader());
    return context.Save(path, data);
}

bool SpriteAtlasPacker::SavePacking(WriteStream& stream)
{
    stream.Write(1); // Version
    stream.Write(Options);
    stream.Write(Textures);
    return false;
}

void SpriteAtlasPacker::OnDependencyModified(BinaryAsset* asset)
{
    if (EnumHasAllFlags(Options.Flags, PackFlags::AutoPack))
    {
        LOG(Info, "Auto-repacking sprite atlas '{}' ('{}' modified)", GetPath(), asset->GetPath());
        Pack();
        Save();
    }
}

#endif

bool SpriteAtlasPacker::Pack()
{
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);
    ScopeLock lock(Locker);
    _texture.CancelStreamingTasks();
    int32 padding = Math::Clamp(Options.Padding, 0, 32);

    // Collect textures to pack
    typedef RectPackNode<int32> AtlasNode;
    struct Slot
    {
        AssetReference<TextureBase> Texture;
        Int2 Size;
        AtlasNode* Node;

        bool operator<(const Slot& other) const
        {
            return Size.MulValues() > other.Size.MulValues();
        }
    };
    Array<Slot> slots;
    slots.EnsureCapacity(Textures.Count());
    int32 totalArea = 0;
    for (auto& e : Textures)
    {
        if (auto* asset = Content::Load<TextureBase>(e.Texture))
        {
            auto& slot = slots.AddOne();
            slot.Texture = asset;
            slot.Size = asset->Size();
            if (e.CustomSize.MulValues() > 0)
                slot.Size = e.CustomSize;
            totalArea += (slot.Size + padding).MulValues();
        }
    }
    totalArea += padding * 2 * (int32)Math::Sqrt((float)totalArea);
    if (slots.Count() == 0)
    {
        Sprites.Clear();
        _texture.UnloadTexture();
        return false;
    }

    // Calculate atlas size
    Int2 size = Options.Size;
    if (EnumHasAllFlags(Options.Flags, PackFlags::UseCustomSize))
    {
        int32 sizeArea = size.MulValues();
        if (sizeArea < totalArea)
        {
            LOG(Error, "Cannot pack '{}' with {} textures of total estimated area {} pixels into {}x{} atlas ({} area)", GetPath(), slots.Count(), totalArea, size.X, size.Y, sizeArea);
            return true;
        }
    }
    else if (slots.Count() == 1)
    {
        size = slots.First().Size;
    }
    else
    {
        int32 side = (int32)Math::Sqrt((float)totalArea * 1.5f);
        if (!Math::IsPowerOfTwo(side))
            side = Math::AlignUp(side, 64);
        size = side;
    }

    // Sort textures by area (descending)
    Sorting::QuickSort(slots);

    // Pack textures into the atlas
    RectPackAtlas<AtlasNode> atlas;
    int32 retry = 10;
RETRY:
    if (retry-- < 0)
    {
        LOG(Error, "Cannot pack '{}' with {} textures of total estimated area {} pixels into {}x{} atlas ({} area)", GetPath(), slots.Count(), totalArea, size.X, size.Y, size.MulValues());
        return true;
    }
    atlas.Init(size.X, size.Y, padding);
    for (auto& slot : slots)
    {
        slot.Node = atlas.Insert(slot.Size.X, slot.Size.Y);
        if (!slot.Node)
        {
            // Failed to insert tile so scale down textures and try again
            for (auto& e : slots)
            {
                e.Node = nullptr;
                e.Size = Float2(e.Size) * 0.75f;
                e.Size = Int2::Max(e.Size, Int2::One);
            }
            goto RETRY;
        }
    }
    if (!EnumHasAllFlags(Options.Flags, PackFlags::UseCustomSize) && slots.Count() > 1)
    {
        // Trim atlas size to packed sprites bounds
        size = Int2::Zero;
        for (auto& slot : slots)
        {
            Int2 bottomRight(slot.Node->X + slot.Node->Width, slot.Node->Y + slot.Node->Height);
            size = Int2::Max(size, bottomRight);
        }
    }

    // Update sprites
    Sprites.Resize(slots.Count());
    for (int32 i = 0; i < slots.Count(); i++)
    {
        auto& sprite = Sprites[i];
        const auto& slot = slots[i];
        sprite.Name = StringUtils::GetFileNameWithoutExtension(slot.Texture->GetPath());
        sprite.Area.Location.X = (float)slot.Node->X / size.X;
        sprite.Area.Location.Y = (float)slot.Node->Y / size.Y;
        sprite.Area.Size.X = (float)slot.Node->Width / size.X;
        sprite.Area.Size.Y = (float)slot.Node->Height / size.Y;
    }

    // Update asset dependencies
    ClearDependencies();
    if (EnumHasAllFlags(Options.Flags, PackFlags::AutoPack))
    {
        for (auto& slot : slots)
            AddDependency(slot.Texture);
    }

    // Create texture data
    TextureData textureData;
    textureData.Init(PixelFormat::R8G8B8A8_UNorm, size.X, size.Y, 1, EnumHasAllFlags(Options.Flags, PackFlags::GenerateMipMaps) ? -1 : 1);
    TextureMipData& textureMipData = *textureData.GetData(0, 0);
    textureMipData.Init(textureData.Width, textureData.Height, textureData.Format);
    Color32* mipData = textureMipData.Data.Get<Color32>();
    Platform::MemoryClear(mipData, textureMipData.Data.Length());

    // Copy textures into the atlas
    for (int32 i = 0; i < slots.Count(); i++)
    {
        const auto& sprite = Sprites[i];
        const auto& slot = slots[i];
        TextureSampler sampler;
        if (sampler.Init(slot.Texture))
            continue;

        PROFILE_CPU_NAMED("CopySprite");
        for (int32 y = 0; y < slot.Size.Y; y++)
        {
            Float2 uv;
            uv.Y = (float)y / slot.Size.Y;
            for (int32 x = 0; x < slot.Size.X; x++)
            {
                uv.X = (float)x / slot.Size.X;
                Color c = sampler.SampleLinear(uv);
                mipData[(slot.Node->Y + y) * textureData.Width + (slot.Node->X + x)] = Color32(c);
            }
        }
    }

    // Generate mip maps
    for (int32 mip = 1; mip < textureData.GetMipLevels(); mip++)
    {
        textureData.GenerateMip(mip, true);
    }

    // Set asset data
    for (int32 mip = 0; mip < textureData.GetMipLevels(); mip++)
    {
        auto chunk = GetOrCreateChunk(mip);
        chunk->Flags |= FlaxChunkFlags::KeepInMemory;
        chunk->Data.Swap(textureData.GetData(0, mip)->Data);
    }
    for (int32 unusedMip = textureData.GetMipLevels(); unusedMip < 14; unusedMip++)
        DeleteChunk(unusedMip);

    // Update GPU texture
    {
        TextureHeader header;
        header.Format = textureData.Format;
        header.Width = textureData.Width;
        header.Height = textureData.Height;
        header.Depth = 1;
        header.MipLevels = textureData.GetMipLevels();
        header.Type = TextureFormatType::ColorRGBA;
        header.NeverStream = true;
        if (_texture.Create(header))
        {
            LOG(Warning, "Cannot initialize texture.");
            return true;
        }
    }

    return false;
}

bool SpriteAtlasPacker::LoadPacking(ReadStream& stream)
{
    ScopeLock lock(Locker);
    Options = PackOptions();
    Textures.Clear();
    int32 version;
    stream.Read(version);
    if (version != 1)
    {
        LOG(Warning, "Invalid packing version {}.", version);
        return true;
    }
    stream.Read(Options);
    stream.Read(Textures);
    return false;
}

uint64 SpriteAtlasPacker::GetMemoryUsage() const
{
    Locker.Lock();
    uint64 result = SpriteAtlas::GetMemoryUsage();
    result += sizeof(SpriteAtlasPacker) - sizeof(SpriteAtlas);
    result += Textures.Capacity() * sizeof(PackTexture);
    Locker.Unlock();
    return result;
}

Asset::LoadResult SpriteAtlasPacker::load()
{
#if USE_EDITOR
    // Check if need to auto-repack atlas when one of the source asset was modified
    auto packingChunk = GetChunk(14);
    if (packingChunk && packingChunk->IsLoaded())
    {
        MemoryReadStream packingData(packingChunk->Get(), packingChunk->Size());
        if (LoadPacking(packingData))
        {
            LOG(Warning, "Cannot load sprites atlas data.");
            return LoadResult::Failed;
        }
        if (EnumHasAllFlags(Options.Flags, PackFlags::AutoPack))
        {
            // Repack atlas when one of used asset was modified after this atlas
            if (HasDependenciesModified())
            {
                LOG(Info, "Auto-repacking sprite atlas '{}' (dependency modified)", GetPath());
                Pack();
                SaveInternal();
                return LoadResult::Ok;
            }
        }
    }
#endif

    return SpriteAtlas::load();
}

void SpriteAtlasPacker::unload(bool isReloading)
{
    Textures.Resize(0);

    // Base
    SpriteAtlas::unload(isReloading);
}

AssetChunksFlag SpriteAtlasPacker::getChunksToPreload() const
{
    AssetChunksFlag chunks = SpriteAtlas::getChunksToPreload();
#if USE_EDITOR
    // Load packing info only in Editor
    chunks |= GET_CHUNK_FLAG(14);
#endif
    return chunks;
}
