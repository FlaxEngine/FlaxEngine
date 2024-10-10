// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "ModelBase.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/Models/Config.h"
#include "Engine/Graphics/Models/SkeletonData.h"
#include "Engine/Graphics/Models/SkinnedModelLOD.h"

class StreamSkinnedModelLODTask;

/// <summary>
/// Skinned model asset that contains model object made of meshes that can be rendered on the GPU using skeleton bones skinning.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SkinnedModel : public ModelBase
{
    DECLARE_BINARY_ASSET_HEADER(SkinnedModel, 5);
    friend SkinnedMesh;
    friend StreamSkinnedModelLODTask;
public:
    // Skeleton mapping descriptor.
    struct FLAXENGINE_API SkeletonMapping
    {
        // Target skeleton.
        AssetReference<SkinnedModel> TargetSkeleton;
        // Source skeleton.
        AssetReference<SkinnedModel> SourceSkeleton;
        // The node-to-node mapping for the fast animation sampling for the skinned model skeleton nodes. Each item is index of the source skeleton node into target skeleton node.
        Span<int32> NodesMapping;
    };

private:
    struct SkeletonMappingData
    {
        AssetReference<SkinnedModel> SourceSkeleton;
        Span<int32> NodesMapping;
    };

    bool _initialized = false;
    int32 _loadedLODs = 0;
    StreamSkinnedModelLODTask* _streamingTask = nullptr;
    Dictionary<Asset*, SkeletonMappingData> _skeletonMappingCache;

public:
    /// <summary>
    /// Model level of details. The first entry is the highest quality LOD0 followed by more optimized versions.
    /// </summary>
    API_FIELD(ReadOnly) Array<SkinnedModelLOD, FixedAllocation<MODEL_MAX_LODS>> LODs;

    /// <summary>
    /// The skeleton bones hierarchy.
    /// </summary>
    SkeletonData Skeleton;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="SkinnedModel"/> class.
    /// </summary>
    ~SkinnedModel();

public:
    /// <summary>
    /// Gets a value indicating whether this instance is initialized. 
    /// </summary>
    FORCE_INLINE bool IsInitialized() const
    {
        return _initialized;
    }

    /// <summary>
    /// Gets the amount of loaded model LODs.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetLoadedLODs() const
    {
        return _loadedLODs;
    }

    /// <summary>
    /// Clamps the index of the LOD to be valid for rendering (only loaded LODs).
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The resident LOD index.</returns>
    FORCE_INLINE int32 ClampLODIndex(int32 index) const
    {
        return Math::Clamp(index, HighestResidentLODIndex(), LODs.Count() - 1);
    }

    /// <summary>
    /// Gets index of the highest resident LOD (may be equal to LODs.Count if no LOD has been uploaded). Note: LOD=0 is the highest (top quality)
    /// </summary>
    FORCE_INLINE int32 HighestResidentLODIndex() const
    {
        return LODs.Count() - _loadedLODs;
    }

    /// <summary>
    /// Determines whether any LOD has been initialized.
    /// </summary>
    bool HasAnyLODInitialized() const;

    /// <summary>
    /// Determines whether this model can be rendered.
    /// </summary>
    FORCE_INLINE bool CanBeRendered() const
    {
        return _loadedLODs > 0;
    }

    /// <summary>
    /// Gets the skeleton nodes hierarchy.
    /// </summary>
    API_PROPERTY() const Array<SkeletonNode>& GetNodes() const
    {
        return Skeleton.Nodes;
    }

    /// <summary>
    /// Gets the skeleton bones hierarchy.
    /// </summary>
    API_PROPERTY() const Array<SkeletonBone>& GetBones() const
    {
        return Skeleton.Bones;
    }

    /// <summary>
    /// Finds the node with the given name.
    /// </summary>
    /// <param name="name">The name of the node.</param>
    /// <returns>The index of the node or -1 if not found.</returns>
    API_FUNCTION() FORCE_INLINE int32 FindNode(const StringView& name) const
    {
        return Skeleton.FindNode(name);
    }

    /// <summary>
    /// Finds the bone with the given name.
    /// </summary>
    /// <param name="name">The name of the node used by the bone.</param>
    /// <returns>The index of the bone or -1 if not found.</returns>
    API_FUNCTION() FORCE_INLINE int32 FindBone(const StringView& name) const
    {
        return FindBone(FindNode(name));
    }

    /// <summary>
    /// Finds the bone that is using a given node index.
    /// </summary>
    /// <param name="nodeIndex">The index of the node.</param>
    /// <returns>The index of the bone or -1 if not found.</returns>
    API_FUNCTION() FORCE_INLINE int32 FindBone(int32 nodeIndex) const
    {
        return Skeleton.FindBone(nodeIndex);
    }

    /// <summary>
    /// Gets the blend shapes names used by the skinned model meshes (from LOD 0 only).
    /// </summary>
    API_PROPERTY() Array<String> GetBlendShapes();

public:
    /// <summary>
    /// Requests the LOD data asynchronously (creates task that will gather chunk data or null if already here).
    /// </summary>
    /// <param name="lodIndex">Index of the LOD.</param>
    /// <returns>Task that will gather chunk data or null if already here.</returns>
    ContentLoadTask* RequestLODDataAsync(int32 lodIndex);

    /// <summary>
    /// Gets the model LOD data (links bytes).
    /// </summary>
    /// <param name="lodIndex">Index of the LOD.</param>
    /// <param name="data">The data (may be missing if failed to get it).</param>
    void GetLODData(int32 lodIndex, BytesContainer& data) const;

    /// <summary>
    /// Gets the skeleton mapping for a given asset (animation or other skinned model). Uses identity mapping or manually created retargeting setup.
    /// </summary>
    /// <param name="source">The source asset (animation or other skinned model) to get mapping to its skeleton.</param>
    /// <returns>The skeleton mapping for the source asset into this skeleton.</returns>
    SkeletonMapping GetSkeletonMapping(Asset* source);

    /// <summary>
    /// Determines if there is an intersection between the SkinnedModel and a Ray in given world using given instance.
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="world">World to test</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="mesh">Mesh, or null</param>
    /// <param name="lodIndex">Level Of Detail index</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal, SkinnedMesh** mesh, int32 lodIndex = 0);

    /// <summary>
    /// Determines if there is an intersection between the SkinnedModel and a Ray in given world using given instance.
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="transform">Instance transformation</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="mesh">Mesh, or null</param>
    /// <param name="lodIndex">Level Of Detail index</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal, SkinnedMesh** mesh, int32 lodIndex = 0);

    /// <summary>
    /// Gets the model bounding box in custom matrix world space (rig pose transformed by matrix, not animated).
    /// </summary>
    /// <param name="world">The transformation matrix.</param>
    /// <param name="lodIndex">The Level Of Detail index.</param>
    /// <returns>The bounding box.</returns>
    API_FUNCTION() BoundingBox GetBox(const Matrix& world, int32 lodIndex = 0) const;

    /// <summary>
    /// Gets the model bounding box in local space (rig pose, not animated).
    /// </summary>
    /// <param name="lodIndex">The Level Of Detail index.</param>
    /// <returns>The bounding box.</returns>
    API_FUNCTION() BoundingBox GetBox(int32 lodIndex = 0) const;

    /// <summary>
    /// Draws the meshes. Binds vertex and index buffers and invokes the draw calls.
    /// </summary>
    /// <param name="context">The GPU context to draw with.</param>
    /// <param name="lodIndex">The Level Of Detail index.</param>
    void Render(GPUContext* context, int32 lodIndex = 0)
    {
        LODs[lodIndex].Render(context);
    }

    /// <summary>
    /// Draws the model.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="info">The packed drawing info data.</param>
    void Draw(const RenderContext& renderContext, const SkinnedMesh::DrawInfo& info);

    /// <summary>
    /// Draws the model.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="info">The packed drawing info data.</param>
    void Draw(const RenderContextBatch& renderContextBatch, const SkinnedMesh::DrawInfo& info);

    /// <summary>
    /// Setups the model LODs collection including meshes creation.
    /// </summary>
    /// <param name="meshesCountPerLod">The meshes count per lod array (amount of meshes per LOD).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupLODs(const Span<int32>& meshesCountPerLod);

    /// <summary>
    /// Setups the skinned model skeleton. Uses the same nodes layout for skeleton bones and calculates the offset matrix by auto.
    /// </summary>
    /// <param name="nodes">The nodes hierarchy. The first node must be a root one (with parent index equal -1).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupSkeleton(const Array<SkeletonNode>& nodes);

    /// <summary>
    /// Setups the skinned model skeleton.
    /// </summary>
    /// <param name="nodes">The nodes hierarchy. The first node must be a root one (with parent index equal -1).</param>
    /// <param name="bones">The bones hierarchy.</param>
    /// <param name="autoCalculateOffsetMatrix">If true then the OffsetMatrix for each bone will be auto-calculated by the engine, otherwise the provided values will be used.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupSkeleton(const Array<SkeletonNode>& nodes, const Array<SkeletonBone>& bones, bool autoCalculateOffsetMatrix);

#if USE_EDITOR
    /// <summary>
    /// Saves this asset to the file. Supported only in Editor.
    /// </summary>
    /// <remarks>If you use saving with the GPU mesh data then the call has to be provided from the thread other than the main game thread.</remarks>
    /// <param name="withMeshDataFromGpu">True if save also GPU mesh buffers, otherwise will keep data in storage unmodified. Valid only if saving the same asset to the same location and it's loaded.</param>
    /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
    /// <returns>True if cannot save data, otherwise false.</returns>
    API_FUNCTION() bool Save(bool withMeshDataFromGpu = false, const StringView& path = StringView::Empty);
#endif

private:
    /// <summary>
    /// Initializes this skinned model to an empty collection of meshes. Ensure to init SkeletonData manually after the call.
    /// </summary>
    /// <param name="meshesCountPerLod">The meshes count per lod array (amount of meshes per LOD).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(const Span<int32>& meshesCountPerLod);

    void ClearSkeletonMapping();
    void OnSkeletonMappingSourceAssetUnloaded(Asset* obj);

#if USE_EDITOR
public:
    // Skeleton retargeting setup (internal use only - accessed by Editor)
    API_STRUCT(NoDefault) struct SkeletonRetarget
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(SkeletonRetarget);
        // Source asset id.
        API_FIELD() Guid SourceAsset;
        // Skeleton asset id to use for remapping.
        API_FIELD() Guid SkeletonAsset;
        // Skeleton nodes remapping table (maps this skeleton node name to other skeleton node).
        API_FIELD() Dictionary<String, String> NodesMapping;
    };
    // Gets or sets the skeleton retarget entries (accessed in Editor only).
    API_PROPERTY() const Array<SkeletonRetarget>& GetSkeletonRetargets() const { return _skeletonRetargets; }
    API_PROPERTY() void SetSkeletonRetargets(const Array<SkeletonRetarget>& value) { Locker.Lock(); _skeletonRetargets = value; ClearSkeletonMapping(); Locker.Unlock(); }
private:
#else
    struct SkeletonRetarget
    {
        Guid SourceAsset, SkeletonAsset;
        Dictionary<String, String, HeapAllocation> NodesMapping;
    };
#endif
    Array<SkeletonRetarget> _skeletonRetargets;

public:
    // [ModelBase]
    uint64 GetMemoryUsage() const override;
    void SetupMaterialSlots(int32 slotsCount) override;
    int32 GetLODsCount() const override;
    void GetMeshes(Array<MeshBase*>& meshes, int32 lodIndex = 0) override;
    void InitAsVirtual() override;
    void CancelStreaming() override;
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override;
#endif

    // [StreamableResource]
    int32 GetMaxResidency() const override;
    int32 GetCurrentResidency() const override;
    int32 GetAllocatedResidency() const override;
    bool CanBeUpdated() const override;
    Task* UpdateAllocation(int32 residency) override;
    Task* CreateStreamingTask(int32 residency) override;
    void CancelStreamingTasks() override;

protected:
    // [ModelBase]
    LoadResult load() override;
    void unload(bool isReloading) override;
    bool init(AssetInitData& initData) override;
    AssetChunksFlag getChunksToPreload() const override;
};
