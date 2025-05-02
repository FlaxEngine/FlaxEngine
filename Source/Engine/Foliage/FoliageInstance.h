// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Level/Scene/Lightmap.h"

/// <summary>
/// Foliage instanced mesh instance. Packed data with very little of logic. Managed by the foliage chunks and foliage actor itself.
/// </summary>
API_STRUCT(NoPod) struct FLAXENGINE_API FoliageInstance
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(FoliageInstance);

    /// <summary>
    /// The local-space transformation of the mesh relative to the foliage actor.
    /// </summary>
    API_FIELD() Transform Transform;

    /// <summary>
    /// The model drawing state.
    /// </summary>
    GeometryDrawStateData DrawState;

    /// <summary>
    /// The foliage type index. Foliage types are hold in foliage actor and shared by instances using the same model.
    /// </summary>
    API_FIELD() int32 Type;

    /// <summary>
    /// The per-instance random value from range [0;1].
    /// </summary>
    API_FIELD() float Random;

    /// <summary>
    /// The cull distance for this instance.
    /// </summary>
    float CullDistance;

    /// <summary>
    /// The cached instance bounds (in world space).
    /// </summary>
    API_FIELD() BoundingSphere Bounds;

    /// <summary>
    /// The lightmap entry for the foliage instance.
    /// </summary>
    LightmapEntry Lightmap;

public:
    bool operator==(const FoliageInstance& v) const
    {
        return Type == v.Type && Random == v.Random && Transform == v.Transform;
    }

    /// <summary>
    /// Determines whether this foliage instance has valid lightmap data.
    /// </summary>
    FORCE_INLINE bool HasLightmap() const
    {
        return Lightmap.TextureIndex != INVALID_INDEX;
    }

    /// <summary>
    /// Removes the lightmap data from the foliage instance.
    /// </summary>
    FORCE_INLINE void RemoveLightmap()
    {
        Lightmap.TextureIndex = INVALID_INDEX;
    }
};
