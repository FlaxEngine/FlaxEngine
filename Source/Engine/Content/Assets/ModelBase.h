// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Models/Config.h"
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

#define MODEL_HEADER_VERSION (byte)2
#define MODEL_MESH_VERSION (byte)2

class MeshBase;
class StreamModelLODTask;
struct RenderContextBatch;

/// <summary>
/// Base class for mesh LOD objects. Contains a collection of the meshes.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API ModelLODBase : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ModelLODBase);

protected:
    int32 _lodIndex = 0;

    explicit ModelLODBase(const SpawnParams& params)
        : ScriptingObject(params)
    {
    }

public:
    /// <summary>
    /// The screen size to switch LODs. Bottom limit of the model screen size to render this LOD.
    /// </summary>
    API_FIELD() float ScreenSize = 1.0f;

    /// <summary>
    /// Gets the model LOD index.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetLODIndex() const
    {
        return _lodIndex;
    }

    /// <summary>
    /// Determines whether any mesh has been initialized.
    /// </summary>
    bool HasAnyMeshInitialized() const;

public:
    /// <summary>
    /// Gets the bounding box combined for all meshes in this model LOD.
    /// </summary>
    API_PROPERTY() BoundingBox GetBox() const;

    /// <summary>
    /// Get model bounding box in transformed world matrix.
    /// </summary>
    /// <param name="world">World matrix</param>
    /// <returns>Bounding box</returns>
    BoundingBox GetBox(const Matrix& world) const;

    /// <summary>
    /// Get model bounding box in transformed world.
    /// </summary>
    /// <param name="transform">The instance transformation.</param>
    /// <param name="deformation">The meshes deformation container (optional).</param>
    /// <returns>Bounding box</returns>
    BoundingBox GetBox(const Transform& transform, const class MeshDeformation* deformation = nullptr) const;

public:
    /// <summary>
    /// Gets the amount of meshes in this LOD.
    /// </summary>
    virtual int32 GetMeshesCount() const = 0;

    /// <summary>
    /// Gets the specific mesh in this LOD.
    /// </summary>
    virtual const MeshBase* GetMesh(int32 index) const = 0;

    /// <summary>
    /// Gets the specific mesh in this LOD.
    /// </summary>
    API_FUNCTION(Sealed) virtual MeshBase* GetMesh(int32 index) = 0;

    /// <summary>
    /// Gets the meshes in this LOD.
    /// </summary>
    virtual void GetMeshes(Array<const MeshBase*>& meshes) const = 0;

    /// <summary>
    /// Gets the meshes in this LOD.
    /// </summary>
    API_FUNCTION(Sealed) virtual void GetMeshes(API_PARAM(Out) Array<MeshBase*>& meshes) = 0;
};

/// <summary>
/// Base class for asset types that can contain a model resource.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API ModelBase : public BinaryAsset, public StreamableResource
{
    DECLARE_ASSET_HEADER(ModelBase);
    friend MeshBase;
    friend StreamModelLODTask;

protected:
    bool _initialized = false;
    int32 _loadedLODs = 0;
    StreamModelLODTask* _streamingTask = nullptr;

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
    ~ModelBase();

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
    /// Gets a value indicating whether this instance is initialized. 
    /// </summary>
    FORCE_INLINE bool IsInitialized() const
    {
        return _initialized;
    }

    /// <summary>
    /// Clamps the index of the LOD to be valid for rendering (only loaded LODs).
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The resident LOD index.</returns>
    FORCE_INLINE int32 ClampLODIndex(int32 index) const
    {
        return Math::Clamp(index, HighestResidentLODIndex(), GetLODsCount() - 1);
    }

    /// <summary>
    /// Gets index of the highest resident LOD (it may be equal to LODs.Count if no LOD has been uploaded). Note: LOD=0 is the highest (top quality)
    /// </summary>
    FORCE_INLINE int32 HighestResidentLODIndex() const
    {
        return GetLODsCount() - _loadedLODs;
    }

    /// <summary>
    /// Gets the amount of loaded model LODs.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetLoadedLODs() const
    {
        return _loadedLODs;
    }

    /// <summary>
    /// Determines whether this model can be rendered.
    /// </summary>
    FORCE_INLINE bool CanBeRendered() const
    {
        return _loadedLODs > 0;
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
    API_PROPERTY(Sealed) virtual int32 GetLODsCount() const = 0;

    /// <summary>
    /// Gets the mesh for a particular LOD index.
    /// </summary>
    virtual const ModelLODBase* GetLOD(int32 lodIndex) const = 0;

    /// <summary>
    /// Gets the mesh for a particular LOD index.
    /// </summary>
    API_FUNCTION(Sealed) virtual ModelLODBase* GetLOD(int32 lodIndex) = 0;

    /// <summary>
    /// Gets the mesh for a particular LOD index.
    /// </summary>
    virtual const MeshBase* GetMesh(int32 meshIndex, int32 lodIndex = 0) const = 0;

    /// <summary>
    /// Gets the mesh for a particular LOD index.
    /// </summary>
    API_FUNCTION(Sealed) virtual MeshBase* GetMesh(int32 meshIndex, int32 lodIndex = 0) = 0;

    /// <summary>
    /// Gets the meshes for a particular LOD index.
    /// </summary>
    virtual void GetMeshes(Array<const MeshBase*>& meshes, int32 lodIndex = 0) const = 0;

    /// <summary>
    /// Gets the meshes for a particular LOD index.
    /// </summary>
    API_FUNCTION(Sealed) virtual void GetMeshes(API_PARAM(Out) Array<MeshBase*>& meshes, int32 lodIndex = 0) = 0;

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
    /// <param name="data">The data (it may be missing if failed to get it).</param>
    void GetLODData(int32 lodIndex, BytesContainer& data) const;

#if USE_EDITOR
    /// <summary>
    /// Saves this asset to the file. Supported only in Editor.
    /// </summary>
    /// <remarks>If you use saving with the GPU mesh data then the call has to be provided from the thread other than the main game thread.</remarks>
    /// <param name="withMeshDataFromGpu">True if save also GPU mesh buffers, otherwise will keep data in storage unmodified. Valid only if saving the same asset to the same location, and it's loaded.</param>
    /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
    /// <returns>True when cannot save data, otherwise false.</returns>
    API_FUNCTION() bool Save(bool withMeshDataFromGpu = false, const StringView& path = StringView::Empty);
#endif

protected:
    friend class AssetExporters;
    friend class MeshAccessor;
    struct MeshData
    {
        uint32 Triangles, Vertices, IBStride;
        Array<const void*, FixedAllocation<MODEL_MAX_VB>> VBData;
        Array<class GPUVertexLayout*, FixedAllocation<MODEL_MAX_VB>> VBLayout;
        const void* IBData;
    };
    virtual bool LoadMesh(class MemoryReadStream& stream, byte meshVersion, MeshBase* mesh, MeshData* dataIfReadOnly = nullptr);
    bool LoadHeader(ReadStream& stream, byte& headerVersion);
#if USE_EDITOR
    virtual bool SaveHeader(WriteStream& stream) const;
    static bool SaveHeader(WriteStream& stream, const class ModelData& modelData);
    bool SaveLOD(WriteStream& stream, int32 lodIndex) const;
    static bool SaveLOD(WriteStream& stream, const ModelData& modelData, int32 lodIndex, bool(saveMesh)(WriteStream& stream, const ModelData& modelData, int32 lodIndex, int32 meshIndex) = nullptr);
    virtual bool SaveMesh(WriteStream& stream, const MeshBase* mesh) const;
    virtual bool Save(bool withMeshDataFromGpu, Function<FlaxChunk*(int32)>& getChunk) const;
#endif

public:
    // [BinaryAsset]
    void CancelStreaming() override;
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override;
    bool Save(const StringView& path = StringView::Empty) override;
#endif

    // [StreamableResource]
    int32 GetCurrentResidency() const override;
    bool CanBeUpdated() const override;
    Task* UpdateAllocation(int32 residency) override;
    Task* CreateStreamingTask(int32 residency) override;
    void CancelStreamingTasks() override;

protected:
    // [BinaryAsset]
    void unload(bool isReloading) override;
};
