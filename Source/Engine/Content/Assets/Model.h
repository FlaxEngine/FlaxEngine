// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "ModelBase.h"
#include "Engine/Graphics/Models/ModelLOD.h"

// Note: we use the first chunk as a header, next is the highest quality lod and then lower ones
//
// Example:
// Chunk 0: Header
// Chunk 1: LOD0
// Chunk 2: LOD1
// ..
//
#define MODEL_LOD_TO_CHUNK_INDEX(lod) (lod + 1)

class Mesh;
class StreamModelLODTask;

/// <summary>
/// Model asset that contains model object made of meshes which can rendered on the GPU.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Model : public ModelBase
{
DECLARE_BINARY_ASSET_HEADER(Model, 25);
    friend Mesh;
    friend StreamModelLODTask;
private:

    int32 _loadedLODs = 0;
    StreamModelLODTask* _streamingTask = nullptr;

public:

    /// <summary>
    /// Model level of details. The first entry is the highest quality LOD0 followed by more optimized versions.
    /// </summary>
    API_FIELD(ReadOnly) Array<ModelLOD, FixedAllocation<MODEL_MAX_LODS>> LODs;

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="Model"/> class.
    /// </summary>
    ~Model();

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
    FORCE_INLINE bool HasAnyLODInitialized() const
    {
        return LODs.HasItems() && LODs.Last().HasAnyMeshInitialized();
    }

    /// <summary>
    /// Determines whether this model can be rendered.
    /// </summary>
    FORCE_INLINE bool CanBeRendered() const
    {
        return _loadedLODs > 0;
    }

public:

    /// <summary>
    /// Requests the LOD data asynchronously (creates task that will gather chunk data or null if already here).
    /// </summary>
    /// <param name="lodIndex">Index of the LOD.</param>
    /// <returns>Task that will gather chunk data or null if already here.</returns>
    ContentLoadTask* RequestLODDataAsync(int32 lodIndex)
    {
        const int32 chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
        return RequestChunkDataAsync(chunkIndex);
    }

    /// <summary>
    /// Gets the model LOD data (links bytes).
    /// </summary>
    /// <param name="lodIndex">Index of the LOD.</param>
    /// <param name="data">The data (may be missing if failed to get it).</param>
    void GetLODData(int32 lodIndex, BytesContainer& data) const
    {
        const int32 chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
        GetChunkData(chunkIndex, data);
    }

public:

    /// <summary>
    /// Determines if there is an intersection between the Model and a Ray in given world using given instance.
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="world">World to test</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="mesh">Mesh, or null</param>
    /// <param name="lodIndex">Level Of Detail index</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal, Mesh** mesh, int32 lodIndex = 0);

    /// <summary>
    /// Gets the model bounding box in custom matrix world space.
    /// </summary>
    /// <param name="world">The transformation matrix.</param>
    /// <param name="lodIndex">The Level Of Detail index.</param>
    /// <returns>The bounding box.</returns>
    API_FUNCTION() BoundingBox GetBox(const Matrix& world, int32 lodIndex = 0) const;

    /// <summary>
    /// Gets the model bounding box in local space.
    /// </summary>
    /// <param name="lodIndex">The Level Of Detail index.</param>
    /// <returns>The bounding box.</returns>
    API_FUNCTION() BoundingBox GetBox(int32 lodIndex = 0) const;

public:

    /// <summary>
    /// Draws the meshes. Binds vertex and index buffers and invokes the draw calls.
    /// </summary>
    /// <param name="context">GPU context to draw with.</param>
    /// <param name="lodIndex">The Level Of Detail index.</param>
    void Render(GPUContext* context, int32 lodIndex = 0)
    {
        LODs[lodIndex].Render(context);
    }

    /// <summary>
    /// Draws the model.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="material">The material to use for rendering.</param>
    /// <param name="world">The world transformation of the model.</param>
    /// <param name="flags">The object static flags.</param>
    /// <param name="receiveDecals">True if rendered geometry can receive decals, otherwise false.</param>
    API_FUNCTION() void Draw(API_PARAM(Ref) const RenderContext& renderContext, MaterialBase* material, API_PARAM(Ref) const Matrix& world, StaticFlags flags = StaticFlags::None, bool receiveDecals = true) const;

    /// <summary>
    /// Draws the model.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="info">The packed drawing info data.</param>
    void Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info);

public:

    /// <summary>
    /// Setups the model LODs collection including meshes creation.
    /// </summary>
    /// <param name="meshesCountPerLod">The meshes count per lod array (amount of meshes per LOD).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupLODs(const Span<int32>& meshesCountPerLod);

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
    /// Initializes this model to an empty collection of LODs with meshes.
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
