// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/SceneInfo.h"

class Scene;
class Lightmap;

/// <summary>
/// Shadows Of Mordor static lighting data container (used per scene).
/// </summary>
class SceneLightmapsData
{
private:
    Array<Lightmap*> _lightmaps;
    Scene* _scene;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="StaticLightManager"/> class.
    /// </summary>
    /// <param name="scene">The parent scene.</param>
    SceneLightmapsData(Scene* scene);

    /// <summary>
    /// Finalizes an instance of the <see cref="StaticLightManager"/> class.
    /// </summary>
    ~SceneLightmapsData();

public:
    FORCE_INLINE Scene* GetScene() const
    {
        return _scene;
    }

    /// <summary>
    /// Gets lightmap at index
    /// </summary>
    /// <param name="index">Lightmap index</param>
    /// <returns>Lightmap or null if missing</returns>
    FORCE_INLINE Lightmap* GetLightmap(int32 index)
    {
        return index >= 0 && index < _lightmaps.Count() ? _lightmaps[index] : nullptr;
    }

    /// <summary>
    /// Gets loaded lightmap at index
    /// </summary>
    /// <param name="index">Lightmap index</param>
    /// <returns>Lightmap or null if missing or not ready</returns>
    FLAXENGINE_API Lightmap* GetReadyLightmap(int32 index);

    /// <summary>
    /// Gets lightmaps array
    /// </summary>
    /// <returns>Lightmaps</returns>
    FORCE_INLINE const Array<Lightmap*>* GetLightmaps() const
    {
        return &_lightmaps;
    }

public:
#if USE_EDITOR

    /// <summary>
    /// Gets path to the lightmaps cache folder
    /// </summary>
    /// <param name="result">Result path</param>
    void GetCacheFolder(String* result);

    /// <summary>
    /// Gets name for lightmap texture asset
    /// </summary>
    /// <param name="result">Result path</param>
    /// <param name="lightmapIndex">Lightmap index</param>
    /// <param name="textureIndex">Lightmap texture index</param>
    void GetCachedLightmapPath(String* result, int32 lightmapIndex, int32 textureIndex);

#endif

public:
    /// <summary>
    /// Clear baked lightmaps data
    /// </summary>
    void ClearLightmaps();

    /// <summary>
    /// Loads the lightmaps data.
    /// </summary>
    /// <param name="lightmaps">The serialized lightmaps info.</param>
    void LoadLightmaps(Array<SavedLightmapInfo>& lightmaps);

    /// <summary>
    /// Unloads the lightmaps.
    /// </summary>
    void UnloadLightmaps();

    /// <summary>
    /// Saves the lightmaps data.
    /// </summary>
    /// <param name="lightmaps">The serialized lightmaps info.</param>
    void SaveLightmaps(Array<SavedLightmapInfo>& lightmaps);

    /// <summary>
    /// Updates the lightmaps collection (capacity and lightmap textures size).
    /// </summary>
    /// <param name="count">The lightmaps count.</param>
    /// <param name="size">The textures size.</param>
    void UpdateLightmapsCollection(int32 count, int32 size);
};
