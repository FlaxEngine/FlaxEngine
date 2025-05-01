// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Renderer/Lightmaps.h"

class SceneLightmapsData;

/// <summary>
/// Shadows Of Mordor static light map
/// </summary>
class Lightmap
{
private:
    SceneLightmapsData* _manager;
    int32 _index;
#if USE_EDITOR
    int32 _size;
#endif
    AssetReference<Texture> _textures[3];

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Lightmap"/> class.
    /// </summary>
    /// <param name="manager">The manager.</param>
    /// <param name="index">The index.</param>
    /// <param name="info">The information.</param>
    Lightmap(SceneLightmapsData* manager, int32 index, const SavedLightmapInfo& info);

public:
    /// <summary>
    /// Gets attached texture objects
    /// </summary>
    /// <param name="lightmap0">Lightmap 0 texture</param>
    /// <param name="lightmap1">Lightmap 1 texture</param>
    /// <param name="lightmap2">Lightmap 2 texture</param>
    void GetTextures(GPUTexture** lightmap0, GPUTexture** lightmap1, GPUTexture** lightmap2) const
    {
        *lightmap0 = _textures[0] ? _textures[0]->GetTexture() : nullptr;
        *lightmap1 = _textures[1] ? _textures[1]->GetTexture() : nullptr;
        *lightmap2 = _textures[2] ? _textures[2]->GetTexture() : nullptr;
    }

    /// <summary>
    /// Gets attached texture objects
    /// </summary>
    /// <param name="lightmaps">Lightmaps textures array</param>
    void GetTextures(GPUTexture* lightmaps[3]) const
    {
        lightmaps[0] = _textures[0] ? _textures[0]->GetTexture() : nullptr;
        lightmaps[1] = _textures[1] ? _textures[1]->GetTexture() : nullptr;
        lightmaps[2] = _textures[2] ? _textures[2]->GetTexture() : nullptr;
    }

    /// <summary>
    /// Gets attached texture objects
    /// </summary>
    /// <param name="lightmaps">Lightmaps textures array</param>
    void GetTextures(Texture* lightmaps[3]) const
    {
        lightmaps[0] = _textures[0].Get();
        lightmaps[1] = _textures[1].Get();
        lightmaps[2] = _textures[2].Get();
    }

    /// <summary>
    /// Gets lightmap info
    /// </summary>
    /// <param name="info">Lightmap info</param>
    void GetInfo(SavedLightmapInfo& info) const
    {
        info.Lightmap0 = _textures[0].GetID();
        info.Lightmap1 = _textures[1].GetID();
        info.Lightmap2 = _textures[2].GetID();
    }

    /// <summary>
    /// Update texture (change it to another asset)
    /// </summary>
    /// <param name="texture">New lightmap texture asset</param>
    /// <param name="index">Texture index</param>
    void UpdateTexture(Texture* texture, int32 index);

    /// <summary>
    /// Ensure that all textures have that size, if not resizing is performed
    /// </summary>
    /// <param name="size">Target size per texture</param>
    void EnsureSize(int32 size);

    /// <summary>
    /// Determines whether this lightmap is ready (textures can be used by the renderer).
    /// </summary>
    bool IsReady() const;

private:
#if USE_EDITOR
    bool OnInitLightmap(class TextureData& image);
#endif
};
