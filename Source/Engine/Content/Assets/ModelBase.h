// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
// Chunk 15: SDF
#define MODEL_LOD_TO_CHUNK_INDEX(lod) (lod + 1)

class MeshBase;
struct RenderContextBatch;

/// <summary>
/// Base class for asset types that can contain a model resource.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API ModelBase : public BinaryAsset, public StreamableResource
{
    DECLARE_ASSET_HEADER(ModelBase);
public:
    /// <summary>
    /// The Sign Distant Field (SDF) data for the model.
    /// </summary>
    API_STRUCT() struct SDFData
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(SDFData);

        /// <summary>
        /// The SDF volume texture (merged all meshes).
        /// </summary>
        API_FIELD() GPUTexture* Texture = nullptr;

        /// <summary>
        /// The transformation scale from model local-space to the generated SDF texture space (local-space -> uv).
        /// </summary>
        API_FIELD() Float3 LocalToUVWMul;

        /// <summary>
        /// Amount of world-units per SDF texture voxel.
        /// </summary>
        API_FIELD() float WorldUnitsPerVoxel;

        /// <summary>
        /// The transformation offset from model local-space to the generated SDF texture space (local-space -> uv).
        /// </summary>
        API_FIELD() Float3 LocalToUVWAdd;

        /// <summary>
        /// The maximum distance stored in the SDF texture. Used to rescale normalized SDF into world-units (in model local space).
        /// </summary>
        API_FIELD() float MaxDistance;

        /// <summary>
        /// The bounding box of the SDF texture in the model local-space.
        /// </summary>
        API_FIELD() Float3 LocalBoundsMin;

        /// <summary>
        /// The SDF texture resolution scale used for building texture.
        /// </summary>
        API_FIELD() float ResolutionScale = 1.0f;

        /// <summary>
        /// The bounding box of the SDF texture in the model local-space.
        /// </summary>
        API_FIELD() Float3 LocalBoundsMax;

        /// <summary>
        /// The model LOD index used for the building.
        /// </summary>
        API_FIELD() int32 LOD = 6;
    };

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
