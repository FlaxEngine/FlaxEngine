// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "SpriteAtlas.h"
#include "Engine/Core/Math/Vector2.h"

/// <summary>
/// Sprite atlas 
/// </summary>
/// <seealso cref="SpriteAtlas" />
API_CLASS(NoSpawn) class FLAXENGINE_API SpriteAtlasPacker : public SpriteAtlas
{
    DECLARE_BINARY_ASSET_HEADER(SpriteAtlasPacker, TexturesSerializedVersion);
public:
    /// <summary>
    /// Options for packing sprites.
    /// </summary>
    API_ENUM(Attributes="Flags") enum class PackFlags
    {
        // Nothing.
        None = 0,
        // Enables using custom atlas size.
        UseCustomSize = 1 << 0,
        // Enables auto-packing atlas on load when one of the input textures gets modified (Editor-only).
        AutoPack = 1 << 1,
        // Enables mip-maps generation.
        GenerateMipMaps = 1 << 2,
        // Default options.
        Default = AutoPack | GenerateMipMaps,
    };

    /// <summary>
    /// Options for packing sprites into atlas.
    /// </summary>
    API_STRUCT() struct PackOptions
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(PackOptions);

        // Atlas texture resolution.
        API_FIELD(Attributes="EditIf(\"UseCustomSize\", Inline=true)") Int2 Size = Int2(1024, 1024);
        // Atlas packing options (flags).
        API_FIELD(Attributes="HideInEditor") PackFlags Flags = PackFlags::Default;
        // Atlas sprites padding (in pixels).
        API_FIELD() int32 Padding = 4;
    };

    /// <summary>
    /// Single texture to pack into Sprite Atlas.
    /// </summary>
    API_STRUCT(NoDefault) struct PackTexture
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(PackTexture);

        // Texture asset ID.
        API_FIELD(Attributes="AssetReference<TextureBase>(true)") Guid Texture = Guid::Empty;
        // Custom sprite size to use. Empty value (0, 0) uses soruce texture size by default.
        API_FIELD(Attributes="Limit(0, 1024)") Int2 CustomSize = Int2::Zero;

    };

public:
    /// <summary>
    /// Atlas packing options. To reflect changes call Pack method.
    /// </summary>
    API_FIELD() PackOptions Options;

    /// <summary>
    /// List with all textures to pack into the atlas. To reflect changes call Pack method.
    /// </summary>
    API_FIELD() Array<PackTexture> Textures;

public:
    /// <summary>
    /// Packs all textures into the atlas texture and updates the asset data.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool Pack();

private:
    bool SavePacking(WriteStream& stream);
    bool LoadPacking(ReadStream& stream);
#if USE_EDITOR
    bool SaveInternal(const StringView& path = StringView::Empty);
#endif

public:
    // [BinaryAsset]
#if USE_EDITOR
    bool Save(const StringView& path = StringView::Empty) override;
#endif
    uint64 GetMemoryUsage() const override;

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
#if USE_EDITOR
    void OnDependencyModified(BinaryAsset* asset) override;
#endif
};

template<>
struct TIsPODType<SpriteAtlasPacker::PackOptions>
{
    enum { Value = true };
};

template<>
struct TIsPODType<SpriteAtlasPacker::PackTexture>
{
    enum { Value = true };
};
