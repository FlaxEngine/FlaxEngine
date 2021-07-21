// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ModelBase.h"
#include "Engine/Graphics/Models/Config.h"
#include "Engine/Graphics/Models/SkeletonData.h"
#include "Engine/Graphics/Models/SkinnedModelLOD.h"

class StreamSkinnedModelLODTask;

// Note: we use the first chunk as a header, next is the highest quality lod and then lower ones
//
// Example:
// Chunk 0: Header
// Chunk 1: LOD0
// Chunk 2: LOD1
// ..
//
#define SKINNED_MODEL_LOD_TO_CHUNK_INDEX(lod) (lod + 1)

/// <summary>
/// Skinned model asset that contains model object made of meshes that can be rendered on the GPU using skeleton bones skinning.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SkinnedModel : public ModelBase
{
DECLARE_BINARY_ASSET_HEADER(SkinnedModel, 4);
    friend SkinnedMesh;
    friend StreamSkinnedModelLODTask;
private:

    int32 _loadedLODs = 0;
    StreamSkinnedModelLODTask* _streamingTask = nullptr;

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
        return LODs.HasItems();
    }

    /// <summary>
    /// Gets the amount of loaded model LODs.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetLoadedLODs() const
    {
        return _loadedLODs;
    }

    /// <summary>
    /// Determines whether the specified index is a valid LOD index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>True if the specified index is a valid LOD index, otherwise false.</returns>
    FORCE_INLINE bool IsValidLODIndex(int32 index) const
    {
        return Math::IsInRange(index, 0, LODs.Count() - 1);
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
        return GetLODsCount() - _loadedLODs;
    }

    /// <summary>
    /// Determines whether any LOD has been initialized.
    /// </summary>
    /// <returns>True if any LOD has been initialized, otherwise false.</returns>
    bool HasAnyLODInitialized() const;

    /// <summary>
    /// Determines whether this model can be rendered.
    /// </summary>
    /// <returns>True if can render that model, otherwise false.</returns>
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
    /// <param name="name">Thr name of the node.</param>
    /// <returns>The index of the node or -1 if not found.</returns>
    API_FUNCTION() FORCE_INLINE int32 FindNode(const StringView& name)
    {
        return Skeleton.FindNode(name);
    }

    /// <summary>
    /// Finds the bone with the given name.
    /// </summary>
    /// <param name="name">Thr name of the node used by the bone.</param>
    /// <returns>The index of the bone or -1 if not found.</returns>
    API_FUNCTION() FORCE_INLINE int32 FindBone(const StringView& name)
    {
        return FindBone(FindNode(name));
    }

    /// <summary>
    /// Finds the bone that is using a given node index.
    /// </summary>
    /// <param name="nodeIndex">The index of the node.</param>
    /// <returns>The index of the bone or -1 if not found.</returns>
    API_FUNCTION() FORCE_INLINE int32 FindBone(int32 nodeIndex)
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

public:

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
    bool Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal, SkinnedMesh** mesh, int32 lodIndex = 0);

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

public:

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
    void Draw(RenderContext& renderContext, const SkinnedMesh::DrawInfo& info);

public:

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

public:

    // [ModelBase]
    void SetupMaterialSlots(int32 slotsCount) override;
    int32 GetLODsCount() const override;
    void GetMeshes(Array<MeshBase*>& meshes, int32 lodIndex = 0) override;
    void InitAsVirtual() override;
#if USE_EDITOR
    void GetReferences(Array<Guid>& output) const override;
#endif

    // [StreamableResource]
    int32 GetMaxResidency() const override;
    int32 GetCurrentResidency() const override;
    int32 GetAllocatedResidency() const override;
    bool CanBeUpdated() const override;
    Task* UpdateAllocation(int32 residency) override;
    Task* CreateStreamingTask(int32 residency) override;

protected:

    // [ModelBase]
    LoadResult load() override;
    void unload(bool isReloading) override;
    bool init(AssetInitData& initData) override;
    AssetChunksFlag getChunksToPreload() const override;
};
