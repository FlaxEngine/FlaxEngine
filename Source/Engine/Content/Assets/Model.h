// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ModelBase.h"
#include "Engine/Graphics/Models/Mesh.h"

/// <summary>
/// Represents single Level Of Detail for the model. Contains a collection of the meshes.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API ModelLOD : public ModelLODBase
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(ModelLOD, ModelLODBase);
    friend Model;
    friend Mesh;

private:
    uint32 _verticesCount = 0;
    Model* _model = nullptr;

    void Link(Model* model, int32 lodIndex)
    {
        _model = model;
        _lodIndex = lodIndex;
        _verticesCount = 0;
    }

public:
    /// <summary>
    /// The meshes array.
    /// </summary>
    API_FIELD(ReadOnly) Array<Mesh> Meshes;

    /// <summary>
    /// Gets the vertex count for this model LOD level.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetVertexCount() const
    {
        return _verticesCount;
    }

public:
    /// <summary>
    /// Determines if there is an intersection between the Model and a Ray in given world using given instance
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="world">World to test</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="mesh">Mesh, or null</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal, Mesh** mesh);

    /// <summary>
    /// Determines if there is an intersection between the Model and a Ray in given world using given instance
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="transform">The instance transformation.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="mesh">Mesh, or null</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal, Mesh** mesh);

    /// <summary>
    /// Draws the meshes. Binds vertex and index buffers and invokes the draw calls.
    /// </summary>
    /// <param name="context">The GPU context to draw with.</param>
    FORCE_INLINE void Render(GPUContext* context)
    {
        for (int32 i = 0; i < Meshes.Count(); i++)
            Meshes.Get()[i].Render(context);
    }

    /// <summary>
    /// Draws the meshes from the model LOD.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="material">The material to use for rendering.</param>
    /// <param name="world">The world transformation of the model.</param>
    /// <param name="flags">The object static flags.</param>
    /// <param name="receiveDecals">True if rendered geometry can receive decals, otherwise false.</param>
    /// <param name="drawModes">The draw passes to use for rendering this object.</param>
    /// <param name="perInstanceRandom">The random per-instance value (normalized to range 0-1).</param>
    /// <param name="sortOrder">Object sorting key.</param>
    API_FUNCTION() void Draw(API_PARAM(Ref) const RenderContext& renderContext, MaterialBase* material, API_PARAM(Ref) const Matrix& world, StaticFlags flags = StaticFlags::None, bool receiveDecals = true, DrawPass drawModes = DrawPass::Default, float perInstanceRandom = 0.0f, int8 sortOrder = 0) const
    {
        for (int32 i = 0; i < Meshes.Count(); i++)
            Meshes.Get()[i].Draw(renderContext, material, world, flags, receiveDecals, drawModes, perInstanceRandom, sortOrder);
    }

    /// <summary>
    /// Draws all the meshes from the model LOD.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="info">The packed drawing info data.</param>
    /// <param name="lodDitherFactor">The LOD transition dither factor.</param>
    FORCE_INLINE void Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info, float lodDitherFactor) const
    {
        for (int32 i = 0; i < Meshes.Count(); i++)
            Meshes.Get()[i].Draw(renderContext, info, lodDitherFactor);
    }

    /// <summary>
    /// Draws all the meshes from the model LOD.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="info">The packed drawing info data.</param>
    /// <param name="lodDitherFactor">The LOD transition dither factor.</param>
    FORCE_INLINE void Draw(const RenderContextBatch& renderContextBatch, const Mesh::DrawInfo& info, float lodDitherFactor) const
    {
        for (int32 i = 0; i < Meshes.Count(); i++)
            Meshes.Get()[i].Draw(renderContextBatch, info, lodDitherFactor);
    }

public:
    // [ModelLODBase]
    int32 GetMeshesCount() const override;
    const MeshBase* GetMesh(int32 index) const override;
    MeshBase* GetMesh(int32 index) override;
    void GetMeshes(Array<MeshBase*>& meshes) override;
    void GetMeshes(Array<const MeshBase*>& meshes) const override;
};

/// <summary>
/// Model asset that contains model object made of meshes which can rendered on the GPU.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Model : public ModelBase
{
    DECLARE_BINARY_ASSET_HEADER(Model, 30);
    friend Mesh;

public:
    /// <summary>
    /// Model level of details. The first entry is the highest quality LOD0 followed by more optimized versions.
    /// </summary>
    API_FIELD(ReadOnly) Array<ModelLOD, FixedAllocation<MODEL_MAX_LODS>> LODs;

    /// <summary>
    /// The generated Sign Distant Field (SDF) for this model (merged all meshes). Use GenerateSDF to update it.
    /// </summary>
    API_FIELD(ReadOnly) SDFData SDF;

public:
    /// <summary>
    /// Determines whether any LOD has been initialized.
    /// </summary>
    bool HasAnyLODInitialized() const;

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
    bool Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal, Mesh** mesh, int32 lodIndex = 0);

    /// <summary>
    /// Determines if there is an intersection between the Model and a Ray in given world using given instance.
    /// </summary>
    /// <param name="ray">The ray to test</param>
    /// <param name="transform">The instance transformation.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="mesh">Mesh, or null</param>
    /// <param name="lodIndex">Level Of Detail index</param>
    /// <returns>True whether the two objects intersected</returns>
    bool Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal, Mesh** mesh, int32 lodIndex = 0);

    /// <summary>
    /// Gets the model bounding box in custom matrix world space.
    /// </summary>
    /// <param name="world">The transformation matrix.</param>
    /// <param name="lodIndex">The Level Of Detail index.</param>
    /// <returns>The bounding box.</returns>
    API_FUNCTION() BoundingBox GetBox(const Matrix& world, int32 lodIndex = 0) const;

    /// <summary>
    /// Gets the model bounding box in custom transformation.
    /// </summary>
    /// <param name="transform">The instance transformation.</param>
    /// <param name="lodIndex">The Level Of Detail index.</param>
    /// <returns>The bounding box.</returns>
    API_FUNCTION() BoundingBox GetBox(const Transform& transform, int32 lodIndex = 0) const
    {
        return LODs[lodIndex].GetBox(transform);
    }

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
    /// <param name="sortOrder">Object sorting key.</param>
    API_FUNCTION() void Draw(API_PARAM(Ref) const RenderContext& renderContext, MaterialBase* material, API_PARAM(Ref) const Matrix& world, StaticFlags flags = StaticFlags::None, bool receiveDecals = true, int8 sortOrder = 0) const;

    /// <summary>
    /// Draws the model.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="info">The packed drawing info data.</param>
    void Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info);

    /// <summary>
    /// Draws the model.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="info">The packed drawing info data.</param>
    void Draw(const RenderContextBatch& renderContextBatch, const Mesh::DrawInfo& info);

public:
    /// <summary>
    /// Setups the model LODs collection including meshes creation.
    /// </summary>
    /// <param name="meshesCountPerLod">The meshes count per lod array (amount of meshes per LOD).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool SetupLODs(const Span<int32>& meshesCountPerLod);

    /// <summary>
    /// Generates the Sign Distant Field for this model.
    /// </summary>
    /// <remarks>Can be called in async in case of SDF generation on a CPU (assuming model is not during rendering).</remarks>
    /// <param name="resolutionScale">The SDF texture resolution scale. Use higher values for more precise data but with significant performance and memory overhead.</param>
    /// <param name="lodIndex">The index of the LOD to use for the SDF building.</param>
    /// <param name="cacheData">If true, the generated SDF texture data will be cached on CPU (in asset chunk storage) to allow saving it later, otherwise it will be runtime for GPU-only. Ignored for virtual assets or in build.</param>
    /// <param name="backfacesThreshold">Custom threshold (in range 0-1) for adjusting mesh internals detection based on the percentage of test rays hit triangle backfaces. Use lower value for more dense mesh.</param>
    /// <param name="useGPU">Enables using GPU for SDF generation, otherwise CPU will be used (async via Job System).</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool GenerateSDF(float resolutionScale = 1.0f, int32 lodIndex = 6, bool cacheData = true, float backfacesThreshold = 0.6f, bool useGPU = true);

    /// <summary>
    /// Sets set SDF data (releases the current one).
    /// </summary>
    API_FUNCTION() void SetSDF(const SDFData& sdf);

private:
    /// <summary>
    /// Initializes this model to an empty collection of LODs with meshes.
    /// </summary>
    /// <param name="meshesCountPerLod">The meshes count per lod array (amount of meshes per LOD).</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Init(const Span<int32>& meshesCountPerLod);

    // [ModelBase]
    bool LoadHeader(ReadStream& stream, byte& headerVersion);
#if USE_EDITOR
    friend class ImportModel;
    bool SaveHeader(WriteStream& stream) const override;
    static bool SaveHeader(WriteStream& stream, const ModelData& modelData);
    bool Save(bool withMeshDataFromGpu, Function<FlaxChunk*(int32)>& getChunk) const override;
#endif

public:
    // [ModelBase]
    void SetupMaterialSlots(int32 slotsCount) override;
    int32 GetLODsCount() const override;
    const ModelLODBase* GetLOD(int32 lodIndex) const override;
    ModelLODBase* GetLOD(int32 lodIndex) override;
    const MeshBase* GetMesh(int32 meshIndex, int32 lodIndex = 0) const override;
    MeshBase* GetMesh(int32 meshIndex, int32 lodIndex = 0) override;
    void GetMeshes(Array<const MeshBase*>& meshes, int32 lodIndex = 0) const override;
    void GetMeshes(Array<MeshBase*>& meshes, int32 lodIndex = 0) override;
    void InitAsVirtual() override;

    // [StreamableResource]
    int32 GetMaxResidency() const override;
    int32 GetAllocatedResidency() const override;

protected:
    // [ModelBase]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
