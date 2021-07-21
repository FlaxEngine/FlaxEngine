// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"

class NavMesh;
class NavMeshBoundsVolume;

/// <summary>
/// Scene navigation subsystem.
/// </summary>
class FLAXENGINE_API SceneNavigation
{
public:

    /// <summary>
    /// The list of registered navigation bounds volumes (on the scene).
    /// </summary>
    Array<NavMeshBoundsVolume*> Volumes;

    /// <summary>
    /// The list of registered navigation meshes (on the scene).
    /// </summary>
    Array<NavMesh*> Meshes;

    /// <summary>
    /// Gets the total navigation volumes bounds.
    /// </summary>
    BoundingBox GetNavigationBounds();

    /// <summary>
    /// Finds the navigation volume bounds that have intersection with the given world-space bounding box.
    /// </summary>
    /// <param name="bounds">The bounds.</param>
    /// <returns>The intersecting volume or null if none found.</returns>
    NavMeshBoundsVolume* FindNavigationBoundsOverlap(const BoundingBox& bounds);
};
