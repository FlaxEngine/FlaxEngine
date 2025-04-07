// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#if COMPILE_WITH_PHYSICS_COOKING
#include "Engine/Physics/CollisionCooking.h"
#endif

/// <summary>
/// Creating collision data asset utility
/// </summary>
class CreateCollisionData
{
public:
    /// <summary>
    /// Creates the CollisionData.
    /// </summary>
    /// <param name="context">The creating context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context);

#if COMPILE_WITH_PHYSICS_COOKING
    /// <summary>
    /// Cooks the mesh collision data and saves it to the asset using <see cref="CollisionData"/> format.
    /// </summary>
    /// <param name="outputPath">The output path.</param>
    /// <param name="arg">The input argument data.</param>
    /// <returns>True if failed, otherwise false. See log file to track errors better.</returns>
    static bool CookMeshCollision(const String& outputPath, CollisionCooking::Argument& arg);
#endif
};

#endif
