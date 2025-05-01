// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Render2D/SpriteAtlas.h"
#include "Engine/Content/Asset.h"
#include "Engine/ContentImporters/Types.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Engine/Graphics/Textures/TextureData.h"

/// <summary>
/// Asset which contains set of asset items thumbnails (cached previews).
/// </summary>
API_CLASS(Sealed, NoSpawn, Namespace="FlaxEditor") class PreviewsCache : public SpriteAtlas
{
DECLARE_BINARY_ASSET_HEADER(PreviewsCache, TexturesSerializedVersion);
private:

    class FlushTask : public ThreadPoolTask
    {
    private:

        PreviewsCache* _cache;
        TextureData _data;

    public:

        FlushTask(PreviewsCache* cache)
            : _cache(cache)
        {
        }

    public:

        /// <summary>
        /// Gets the texture data container.
        /// </summary>
        TextureData& GetData()
        {
            return _data;
        }

    protected:

        // [ThreadPoolTask]
        bool Run() override;
        void OnEnd() override;
    };

private:

    Array<Guid> _assets;
    bool _isDirty = false;
    FlushTask* _flushTask = nullptr;

public:

    /// <summary>
    /// Determines whether this atlas is ready (is loaded and has texture streamed).
    /// </summary>
    API_PROPERTY() bool IsReady() const;

    /// <summary>
    /// Finds the preview icon for given asset ID.
    /// </summary>
    /// <param name="id">The asset id to find preview for it.</param>
    /// <returns>The output sprite slot handle or invalid if invalid in nothing found.</returns>
    API_FUNCTION() SpriteHandle FindSlot(const Guid& id);

    /// <summary>
    /// Determines whether this atlas has one or more free slots for the asset preview.
    /// </summary>
    API_PROPERTY() bool HasFreeSlot() const;

    /// <summary>
    /// Occupies the atlas slot.
    /// </summary>
    /// <param name="source">The source texture to insert.</param>
    /// <param name="id">The asset identifier.</param>
    /// <returns>The added sprite slot handle or invalid if invalid in failed to occupy slot.</returns>
    API_FUNCTION() SpriteHandle OccupySlot(GPUTexture* source, const Guid& id);

    /// <summary>
    /// Releases the used slot.
    /// </summary>
    /// <param name="id">The asset identifier.</param>
    /// <returns>True if slot has been release, otherwise it was not found.</returns>
    API_FUNCTION() bool ReleaseSlot(const Guid& id);

    /// <summary>
    /// Flushes atlas data from the GPU to the asset storage (saves data).
    /// </summary>
    API_FUNCTION() void Flush();

    /// <summary>
    /// Determines whether this instance is flushing.
    /// </summary>
    /// <returns>True if this previews cache is flushing, otherwise false.</returns>
    FORCE_INLINE bool IsFlushing() const
    {
        return _flushTask != nullptr;
    }

public:

#if COMPILE_WITH_ASSETS_IMPORTER

    /// <summary>
    /// Creates a new atlas.
    /// </summary>
    /// <param name="outputPath">The output asset file path.</param>
    /// <returns>True if this previews cache is flushing, otherwise false.</returns>
    API_FUNCTION() static bool Create(const StringView& outputPath);

private:

    static CreateAssetResult create(CreateAssetContext& context);

#endif

protected:

    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
