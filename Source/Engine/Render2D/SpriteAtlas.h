// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/ISerializable.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Content/BinaryAsset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/Textures/TextureBase.h"

class SpriteAtlas;
class GPUTexture;

/// <summary>
/// Contains information about single atlas slot with sprite texture.
/// </summary>
API_STRUCT() struct Sprite
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Sprite);

    /// <summary>
    /// The normalized area of the sprite in the atlas (in range [0;1]).
    /// </summary>
    API_FIELD() Rectangle Area;

    /// <summary>
    /// The sprite name.
    /// </summary>
    API_FIELD() String Name;
};

/// <summary>
/// Handle to sprite atlas slot with a single sprite texture.
/// </summary>
API_STRUCT() struct FLAXENGINE_API SpriteHandle : ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_MINIMAL(SpriteHandle);

    /// <summary>
    /// Invalid sprite handle.
    /// </summary>
    static const SpriteHandle Invalid;

    /// <summary>
    /// The parent atlas.
    /// </summary>
    API_FIELD() AssetReference<SpriteAtlas> Atlas;

    /// <summary>
    /// The atlas sprites array index.
    /// </summary>
    API_FIELD() int32 Index;

    /// <summary>
    /// Initializes a new instance of the <see cref="SpriteHandle"/> struct.
    /// </summary>
    SpriteHandle()
    {
        Index = -1;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SpriteHandle"/> struct.
    /// </summary>
    /// <param name="atlas">The sprite atlas.</param>
    /// <param name="index">The sprite slot index.</param>
    SpriteHandle(SpriteAtlas* atlas, int32 index)
        : Atlas(atlas)
    {
        Index = index;
    }

    /// <summary>
    /// Tries to get sprite info.
    /// </summary>
    /// <param name="result">The result.</param>
    /// <returns>True if data is valid, otherwise false.</returns>
    bool GetSprite(Sprite* result) const;

    /// <summary>
    /// Returns true if sprite is valid.
    /// </summary>
    /// <returns>True if this sprite handle is valid, otherwise false.</returns>
    bool IsValid() const;

    /// <summary>
    /// Gets the sprite atlas texture.
    /// </summary>
    /// <returns>The texture object.</returns>
    GPUTexture* GetAtlasTexture() const;
};

/// <summary>
/// Sprite atlas asset that contains collection of sprites combined into a single texture.
/// </summary>
/// <seealso cref="TextureBase" />
API_CLASS(NoSpawn) class FLAXENGINE_API SpriteAtlas : public TextureBase
{
DECLARE_BINARY_ASSET_HEADER(SpriteAtlas, TexturesSerializedVersion);
public:

    /// <summary>
    /// List with all tiles in the sprite atlas.
    /// </summary>
    API_FIELD() Array<Sprite> Sprites;

public:

    /// <summary>
    /// Gets the sprites count.
    /// </summary>
    /// <returns>The sprites count.</returns>
    API_PROPERTY() int32 GetSpritesCount() const
    {
        return Sprites.Count();
    }

    /// <summary>
    /// Gets the sprite data.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The sprite data.</returns>
    API_FUNCTION() Sprite GetSprite(const int32 index) const
    {
        return Sprites[index];
    }

    /// <summary>
    /// Sets the sprite data.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <param name="value">The sprite data.</param>
    /// <returns>The sprite handle.</returns>
    API_FUNCTION() void SetSprite(const int32 index, API_PARAM(Ref) const Sprite& value)
    {
        Sprites[index] = value;
    }

    /// <summary>
    /// Finds the sprite by the name.
    /// </summary>
    /// <param name="name">The name.</param>
    /// <returns>The sprite handle.</returns>
    API_FUNCTION() SpriteHandle FindSprite(const StringView& name) const;

    /// <summary>
    /// Adds the sprite.
    /// </summary>
    /// <param name="sprite">The sprite.</param>
    /// <returns>The sprite handle.</returns>
    API_FUNCTION() SpriteHandle AddSprite(const Sprite& sprite);

    /// <summary>
    /// Removes the sprite.
    /// </summary>
    /// <param name="index">The sprite index.</param>
    API_FUNCTION() void RemoveSprite(int32 index);

#if USE_EDITOR

    /// <summary>
    /// Save the sprites (texture content won't be modified).
    /// </summary>
    /// <returns>True if cannot save, otherwise false.</returns>
    API_FUNCTION() bool SaveSprites();

#endif

protected:

    bool LoadSprites(ReadStream& stream);

protected:

    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
