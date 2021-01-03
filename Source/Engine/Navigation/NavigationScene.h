// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "NavMeshData.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/RawDataAsset.h"

class Scene;
class NavMeshBoundsVolume;

/// <summary>
/// Scene object navigation data.
/// </summary>
class NavigationScene
{
    friend Scene;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="NavigationScene"/> class.
    /// </summary>
    /// <param name="scene">The scene.</param>
    NavigationScene(Scene* scene);

public:

    /// <summary>
    /// The parent scene.
    /// </summary>
    Scene* Scene;

    /// <summary>
    /// The flag used to mark that navigation data has been modified since load. Used to save runtime data to the file on scene serialization.
    /// </summary>
    bool IsDataDirty;

    /// <summary>
    /// The navmesh tiles data.
    /// </summary>
    NavMeshData Data;

    /// <summary>
    /// The cached navmesh data asset.
    /// </summary>
    AssetReference<RawDataAsset> DataAsset;

public:

    /// <summary>
    /// The list of registered navigation bounds volumes (in the scene).
    /// </summary>
    Array<NavMeshBoundsVolume*> Volumes;

    /// <summary>
    /// Gets the total navigation volumes bounds.
    /// </summary>
    /// <returns>The navmesh bounds.</returns>
    BoundingBox GetNavigationBounds();

    /// <summary>
    /// Finds the navigation volume bounds that have intersection with the given world-space bounding box.
    /// </summary>
    /// <param name="bounds">The bounds.</param>
    /// <returns>The intersecting volume or null if none found.</returns>
    NavMeshBoundsVolume* FindNavigationBoundsOverlap(const BoundingBox& bounds);

public:

    /// <summary>
    /// Saves the nav mesh tiles data to the asset. Supported only in builds with assets saving enabled (eg. editor) and not during gameplay (eg. design time).
    /// </summary>
    void SaveNavMesh();

    /// <summary>
    /// Clears the data.
    /// </summary>
    void ClearData()
    {
        if (Data.Tiles.HasItems())
        {
            IsDataDirty = true;
            Data.TileSize = 0.0f;
            Data.Tiles.Resize(0);
        }
    }

protected:

    void OnEnable();
    void OnDisable();
    void OnDataAssetLoaded();
};
