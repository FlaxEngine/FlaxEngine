// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"
#include "Engine/Core/Collections/ChunkedArray.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Core/ISerializable.h"

/// <summary>
/// The foliage instances scaling modes.
/// </summary>
API_ENUM() enum class FoliageScalingModes
{
    /// <summary>
    /// The uniform scaling. All axes are scaled the same.
    /// </summary>
    Uniform,

    /// <summary>
    /// The free scaling. Each axis can have custom scale.
    /// </summary>
    Free,

    /// <summary>
    /// The lock XZ plane axis. Axes X and Z are constrained to-gather and axis Y is free.
    /// </summary>
    LockXZ,

    /// <summary>
    /// The lock XY plane axis. Axes X and Y are constrained to-gather and axis Z is free.
    /// </summary>
    LockXY,

    /// <summary>
    /// The lock YZ plane axis. Axes Y and Z are constrained to-gather and axis X is free.
    /// </summary>
    LockYZ,
};

/// <summary>
/// Foliage mesh instances type descriptor. Defines the shared properties of the spawned mesh instances.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API FoliageType : public ScriptingObject, public ISerializable
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(FoliageType);
    friend Foliage;
private:
    uint8 _isReady : 1;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="FoliageType"/> class.
    /// </summary>
    FoliageType();

    FoliageType(const FoliageType& other)
        : FoliageType()
    {
        *this = other;
    }

    FoliageType& operator=(const FoliageType& other);

public:
    /// <summary>
    /// The parent foliage actor.
    /// </summary>
    API_FIELD(ReadOnly) Foliage* Foliage;

    /// <summary>
    /// The foliage type index.
    /// </summary>
    API_FIELD(ReadOnly) int32 Index;

    /// <summary>
    /// The model to draw by the instances.
    /// </summary>
    API_FIELD() AssetReference<Model> Model;

    /// <summary>
    /// The shared model instance entries.
    /// </summary>
    ModelInstanceEntries Entries;

#if !FOLIAGE_USE_SINGLE_QUAD_TREE
    /// <summary>
    /// The root cluster. Contains all the instances and it's the starting point of the quad-tree hierarchy. Null if no foliage added. It's read-only.
    /// </summary>
    FoliageCluster* Root = nullptr;

    /// <summary>
    /// The allocated foliage clusters. It's read-only.
    /// </summary>
    ChunkedArray<FoliageCluster, FOLIAGE_CLUSTER_CHUNKS_SIZE> Clusters;
#endif

public:
    /// <summary>
    /// Gets the foliage instance type materials buffer (overrides). 
    /// </summary>
    API_PROPERTY() Array<MaterialBase*> GetMaterials() const;

    /// <summary>
    /// Sets the foliage instance type materials buffer (overrides). 
    /// </summary>
    API_PROPERTY() void SetMaterials(const Array<MaterialBase*>& value);

public:
    /// <summary>
    /// The per-instance cull distance.
    /// </summary>
    API_FIELD() float CullDistance = 10000.0f;

    /// <summary>
    /// The per-instance cull distance randomization range (randomized per instance and added to master CullDistance value).
    /// </summary>
    API_FIELD() float CullDistanceRandomRange = 1000.0f;

    /// <summary>
    /// The scale in lightmap (for instances of this foliage type). Can be used to adjust static lighting quality for the foliage instances.
    /// </summary>
    API_FIELD() float ScaleInLightmap = 1.0f;

    /// <summary>
    /// The draw passes to use for rendering this foliage type.
    /// </summary>
    API_FIELD() DrawPass DrawModes = DrawPass::Depth | DrawPass::GBuffer | DrawPass::Forward;

    /// <summary>
    /// The shadows casting mode.
    /// </summary>
    API_FIELD() ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

    /// <summary>
    /// The foliage instances density defined in instances count per 1000x1000 units area.
    /// </summary>
    API_FIELD() float PaintDensity = 1.0f;

    /// <summary>
    /// The minimum radius between foliage instances.
    /// </summary>
    API_FIELD() float PaintRadius = 0.0f;

    /// <summary>
    /// The minimum ground slope angle to paint foliage on it (in degrees).
    /// </summary>
    API_FIELD() float PaintGroundSlopeAngleMin = 0.0f;

    /// <summary>
    /// The maximum ground slope angle to paint foliage on it (in degrees).
    /// </summary>
    API_FIELD() float PaintGroundSlopeAngleMax = 45.0f;

    /// <summary>
    /// The scaling mode.
    /// </summary>
    API_FIELD() FoliageScalingModes PaintScaling = FoliageScalingModes::Uniform;

    /// <summary>
    /// The scale minimum values per axis.
    /// </summary>
    API_FIELD() Float3 PaintScaleMin = Float3::One;

    /// <summary>
    /// The scale maximum values per axis.
    /// </summary>
    API_FIELD() Float3 PaintScaleMax = Float3::One;

    /// <summary>
    /// The per-instance random offset range on axis Y.
    /// </summary>
    API_FIELD() Float2 PlacementOffsetY = Float2::Zero;

    /// <summary>
    /// The random pitch angle range (uniform in both ways around normal vector).
    /// </summary>
    API_FIELD() float PlacementRandomPitchAngle = 0.0f;

    /// <summary>
    /// The random roll angle range (uniform in both ways around normal vector).
    /// </summary>
    API_FIELD() float PlacementRandomRollAngle = 0.0f;

    /// <summary>
    /// The density scaling scale applied to the global scale for the foliage instances of this type. Can be used to boost or reduce density scaling effect on this foliage type. Default is 1.
    /// </summary>
    API_FIELD() float DensityScalingScale = 1.0f;

    /// <summary>
    /// Determines whenever this meshes can receive decals.
    /// </summary>
    API_FIELD() int8 ReceiveDecals : 1;

    /// <summary>
    /// Flag used to determinate whenever use global foliage density scaling for instances of this foliage type.
    /// </summary>
    API_FIELD() int8 UseDensityScaling : 1;

    /// <summary>
    /// If checked, instances will be aligned to normal of the placed surface.
    /// </summary>
    API_FIELD() int8 PlacementAlignToNormal : 1;

    /// <summary>
    /// If checked, instances will use randomized yaw when placed. Random yaw uses will rotation range over the Y axis.
    /// </summary>
    API_FIELD() int8 PlacementRandomYaw : 1;

public:
    /// <summary>
    /// Determines whether this instance is ready (model is loaded).
    /// </summary>
    FORCE_INLINE bool IsReady() const
    {
        return _isReady != 0;
    }

    /// <summary>
    /// Gets the random scale for the foliage instance of this type.
    /// </summary>
    Float3 GetRandomScale() const;

private:
    void OnModelChanged();
    void OnModelLoaded();

public:
    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
