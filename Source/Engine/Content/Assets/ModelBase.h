// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Models/MaterialSlot.h"
#include "Engine/Streaming/StreamableResource.h"

// Note: we use the first chunk as a header, next is the highest quality lod and then lower ones
//
// Example:
// Chunk 0: Header
// Chunk 1: LOD0
// Chunk 2: LOD1
// ..
//
#define MODEL_LOD_TO_CHUNK_INDEX(lod) (lod + 1)

class MeshBase;

/// <summary>
/// Base class for asset types that can contain a model resource.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API ModelBase : public BinaryAsset, public StreamableResource
{
DECLARE_ASSET_HEADER(ModelBase);
protected:

    explicit ModelBase(const SpawnParams& params, const AssetInfo* info, StreamingGroup* group)
        : BinaryAsset(params, info)
        , StreamableResource(group)
    {
    }

public:

    /// <summary>
    /// The minimum screen size to draw this model (the bottom limit). Used to cull small models. Set to 0 to disable this feature.
    /// </summary>
    API_FIELD() float MinScreenSize = 0.0f;

    /// <summary>
    /// The list of material slots.
    /// </summary>
    API_FIELD(ReadOnly) Array<MaterialSlot> MaterialSlots;

    /// <summary>
    /// Gets the amount of the material slots used by this model asset.
    /// </summary>
    API_PROPERTY() int32 GetMaterialSlotsCount() const
    {
        return MaterialSlots.Count();
    }

    /// <summary>
    /// Resizes the material slots collection. Updates meshes that were using removed slots.
    /// </summary>
    API_FUNCTION() virtual void SetupMaterialSlots(int32 slotsCount);

    /// <summary>
    /// Gets the material slot by the name.
    /// </summary>
    /// <param name="name">The slot name.</param>
    /// <returns>The material slot with the given name or null if cannot find it (asset may be not loaded yet).</returns>
    API_FUNCTION() MaterialSlot* GetSlot(const StringView& name);

    /// <summary>
    /// Gets amount of the level of details in the model.
    /// </summary>
    virtual int32 GetLODsCount() const = 0;

    /// <summary>
    /// Gets the meshes for a particular LOD index.
    /// </summary>
    virtual void GetMeshes(Array<MeshBase*>& meshes, int32 lodIndex = 0) = 0;
};
