// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"
#include "FoliageInstance.h"
#include "FoliageCluster.h"
#include "FoliageType.h"
#include "Engine/Level/Actor.h"

/// <summary>
/// Represents a foliage actor that contains a set of instanced meshes.
/// </summary>
/// <seealso cref="Actor" />
API_CLASS() class FLAXENGINE_API Foliage final : public Actor
{
    DECLARE_SCENE_OBJECT(Foliage);
private:
    bool _disableFoliageTypeEvents;
    int32 _sceneRenderingKey = -1;

public:
    /// <summary>
    /// The allocated foliage instances. It's read-only.
    /// </summary>
    ChunkedArray<FoliageInstance, FOLIAGE_INSTANCE_CHUNKS_SIZE> Instances;

#if FOLIAGE_USE_SINGLE_QUAD_TREE
    /// <summary>
    /// The root cluster. Contains all the instances and it's the starting point of the quad-tree hierarchy. Null if no foliage added. It's read-only.
    /// </summary>
    FoliageCluster* Root = nullptr;

    /// <summary>
    /// The allocated foliage clusters. It's read-only.
    /// </summary>
    ChunkedArray<FoliageCluster, FOLIAGE_CLUSTER_CHUNKS_SIZE> Clusters;
#endif

    /// <summary>
    /// The foliage instances types used by the current foliage actor. It's read-only.
    /// </summary>
    API_FIELD(ReadOnly, Attributes="HideInEditor, NoSerialize")
    Array<FoliageType> FoliageTypes;

public:
    /// <summary>
    /// Gets the total amount of the instanced of foliage.
    /// </summary>
    /// <returns>The foliage instances count.</returns>
    API_PROPERTY() int32 GetInstancesCount() const;

    /// <summary>
    /// Gets the foliage instance by index.
    /// </summary>
    /// <param name="index">The zero-based index of the foliage instance.</param>
    /// <returns>The foliage instance data.</returns>
    API_FUNCTION() FoliageInstance GetInstance(int32 index) const;

    /// <summary>
    /// Gets the total amount of the types of foliage.
    /// </summary>
    /// <returns>The foliage types count.</returns>
    API_PROPERTY() int32 GetFoliageTypesCount() const;

    /// <summary>
    /// Gets the foliage type.
    /// </summary>
    /// <param name="index">The zero-based index of the foliage type.</param>
    /// <returns>The foliage type.</returns>
    API_FUNCTION() FoliageType* GetFoliageType(int32 index);

    /// <summary>
    /// Adds the type of the foliage.
    /// </summary>
    /// <param name="model">The model to assign. It cannot be null nor already used by the other instance type (it must be unique within the given foliage actor).</param>
    API_FUNCTION() void AddFoliageType(Model* model);

    /// <summary>
    /// Removes the foliage instance type and all foliage instances using this type.
    /// </summary>
    /// <param name="index">The zero-based index of the foliage instance type.</param>
    API_FUNCTION() void RemoveFoliageType(int32 index);

    /// <summary>
    /// Gets the total amount of the instanced that use the given foliage type.
    /// </summary>
    /// <param name="index">The zero-based index of the foliage type.</param>
    /// <returns>The foliage type instances count.</returns>
    API_FUNCTION() int32 GetFoliageTypeInstancesCount(int32 index) const;

    /// <summary>
    /// Adds the new foliage instance. Ensure to always call <see cref="RebuildClusters"/> after editing foliage to sync cached data (call it once after editing one or more instances).
    /// </summary>
    /// <remarks>Input instance bounds, instance random and world matrix are ignored (recalculated).</remarks>
    /// <param name="instance">The instance.</param>
    API_FUNCTION() void AddInstance(API_PARAM(Ref) const FoliageInstance& instance);

    /// <summary>
    /// Removes the foliage instance. Ensure to always call <see cref="RebuildClusters"/> after editing foliage to sync cached data (call it once after editing one or more instances).
    /// </summary>
    /// <param name="index">The zero-based index of the instance to remove.</param>
    API_FUNCTION() void RemoveInstance(int32 index)
    {
        RemoveInstance(Instances.IteratorAt(index));
    }

    /// <summary>
    /// Removes the foliage instance. Ensure to always call <see cref="RebuildClusters"/> after editing foliage to sync cached data (call it once after editing one or more instances).
    /// </summary>
    /// <param name="i">The iterator from foliage instances that points to the instance to remove.</param>
    void RemoveInstance(ChunkedArray<FoliageInstance, FOLIAGE_INSTANCE_CHUNKS_SIZE>::Iterator i);

    /// <summary>
    /// Sets the foliage instance transformation. Ensure to always call <see cref="RebuildClusters"/> after editing foliage to sync cached data (call it once after editing one or more instances).
    /// </summary>
    /// <param name="index">The zero-based index of the foliage instance.</param>
    /// <param name="value">The value.</param>
    API_FUNCTION() void SetInstanceTransform(int32 index, API_PARAM(Ref) const Transform& value);

    /// <summary>
    /// Called when foliage type model is loaded.
    /// </summary>
    /// <param name="index">The zero-based index of the foliage type.</param>
    void OnFoliageTypeModelLoaded(int32 index);

    /// <summary>
    /// Rebuilds the foliage clusters used as internal acceleration structures (quad tree).
    /// </summary>
    API_FUNCTION() void RebuildClusters();

    /// <summary>
    /// Updates the cull distance for all foliage instances and for created clusters.
    /// </summary>
    API_FUNCTION() void UpdateCullDistance();

    /// <summary>
    /// Clears all foliage instances. Preserves the foliage types and other properties.
    /// </summary>
    API_FUNCTION() void RemoveAllInstances();

public:
    /// <summary>
    /// Gets the global density scale for all foliage instances. The default value is 1. Use values from range 0-1. Lower values decrease amount of foliage instances in-game. Use it to tweak game performance for slower devices.
    /// </summary>
    API_PROPERTY() static float GetGlobalDensityScale();

    /// <summary>
    /// Sets the global density scale for all foliage instances. The default value is 1. Use values from range 0-1. Lower values decrease amount of foliage instances in-game. Use it to tweak game performance for slower devices.
    /// </summary>
    API_PROPERTY() static void SetGlobalDensityScale(float value);

private:
    void AddToCluster(ChunkedArray<FoliageCluster, FOLIAGE_CLUSTER_CHUNKS_SIZE>& clusters, FoliageCluster* cluster, FoliageInstance& instance);
#if !FOLIAGE_USE_SINGLE_QUAD_TREE && FOLIAGE_USE_DRAW_CALLS_BATCHING
    struct DrawKey
    {
        IMaterial* Mat;
        const Mesh* Geo;
        int32 Lightmap;

        bool operator==(const DrawKey& other) const
        {
            return Mat == other.Mat && Geo == other.Geo && Lightmap == other.Lightmap;
        }

        friend uint32 GetHash(const DrawKey& key)
        {
            uint32 hash = (uint32)((int64)(key.Mat) >> 3);
            hash ^= (uint32)((int64)(key.Geo) >> 3) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= (uint32)key.Lightmap;
            return hash;
        }
    };

    typedef Array<struct BatchedDrawCall, InlinedAllocation<8>> DrawCallsList;
    typedef Dictionary<DrawKey, struct BatchedDrawCall, class RendererAllocation> BatchedDrawCalls;
    void DrawInstance(RenderContext& renderContext, FoliageInstance& instance, const FoliageType& type, Model* model, int32 lod, float lodDitherFactor, DrawCallsList* drawCallsLists, BatchedDrawCalls& result) const;
    void DrawCluster(RenderContext& renderContext, FoliageCluster* cluster, const FoliageType& type, DrawCallsList* drawCallsLists, BatchedDrawCalls& result) const;
#else
    void DrawCluster(RenderContext& renderContext, FoliageCluster* cluster, Mesh::DrawInfo& draw);
#endif
#if !FOLIAGE_USE_SINGLE_QUAD_TREE
    void DrawClusterGlobalSDF(class GlobalSignDistanceFieldPass* globalSDF, const BoundingBox& globalSDFBounds, FoliageCluster* cluster, const FoliageType& type);
    void DrawClusterGlobalSA(class GlobalSurfaceAtlasPass* globalSA, const Vector4& cullingPosDistance, FoliageCluster* cluster, const FoliageType& type, const BoundingBox& localBounds);
    void DrawFoliageJob(int32 i);
    RenderContextBatch* _renderContextBatch;
#endif
    void DrawType(RenderContext& renderContext, const FoliageType& type, DrawCallsList* drawCallsLists);

public:
    /// <summary>
    /// Determines if there is an intersection between the current object or any it's child and a ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="instanceIndex">When the method completes, contains zero-based index of the foliage instance that is the closest to the ray.</param>
    /// <returns>True whether the two objects intersected, otherwise false.</returns>
    API_FUNCTION() bool Intersects(API_PARAM(Ref) const Ray& ray, API_PARAM(Out) Real& distance, API_PARAM(Out) Vector3& normal, API_PARAM(Out) int32& instanceIndex);

public:
    // [Actor]
    void Draw(RenderContext& renderContext) override;
    void Draw(RenderContextBatch& renderContextBatch) override;
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnLayerChanged() override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
