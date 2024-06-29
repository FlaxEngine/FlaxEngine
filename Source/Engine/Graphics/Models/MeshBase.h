// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Level/Types.h"
#include "Engine/Scripting/ScriptingObject.h"

struct GeometryDrawStateData;
struct RenderContext;
struct RenderContextBatch;
class Task;
class ModelBase;
class Lightmap;
class GPUBuffer;
class SkinnedMeshDrawData;
class BlendShapesInstance;

/// <summary>
/// Base class for model resources meshes.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API MeshBase : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MeshBase);
protected:
    ModelBase* _model;
    BoundingBox _box;
    BoundingSphere _sphere;
    int32 _index;
    int32 _lodIndex;
    uint32 _vertices;
    uint32 _triangles;
    int32 _materialSlotIndex;
    bool _use16BitIndexBuffer;

    explicit MeshBase(const SpawnParams& params)
        : ScriptingObject(params)
    {
    }

public:
    /// <summary>
    /// Gets the model owning this mesh.
    /// </summary>
    API_PROPERTY() FORCE_INLINE ModelBase* GetModelBase() const
    {
        return _model;
    }

    /// <summary>
    /// Gets the mesh parent LOD index.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetLODIndex() const
    {
        return _lodIndex;
    }

    /// <summary>
    /// Gets the mesh index.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetIndex() const
    {
        return _index;
    }

    /// <summary>
    /// Gets the triangle count.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetTriangleCount() const
    {
        return _triangles;
    }

    /// <summary>
    /// Gets the vertex count.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetVertexCount() const
    {
        return _vertices;
    }

    /// <summary>
    /// Gets the box.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const BoundingBox& GetBox() const
    {
        return _box;
    }

    /// <summary>
    /// Gets the sphere.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const BoundingSphere& GetSphere() const
    {
        return _sphere;
    }

    /// <summary>
    /// Determines whether this mesh is using 16 bit index buffer, otherwise it's 32 bit.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool Use16BitIndexBuffer() const
    {
        return _use16BitIndexBuffer;
    }

    /// <summary>
    /// Gets the index of the material slot to use during this mesh rendering.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetMaterialSlotIndex() const
    {
        return _materialSlotIndex;
    }

    /// <summary>
    /// Sets the index of the material slot to use during this mesh rendering.
    /// </summary>
    API_PROPERTY() void SetMaterialSlotIndex(int32 value);

    /// <summary>
    /// Sets the mesh bounds.
    /// </summary>
    /// <param name="box">The bounding box.</param>
    void SetBounds(const BoundingBox& box);

public:
    /// <summary>
    /// Extract mesh buffer data from GPU. Cannot be called from the main thread.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <returns>True if failed, otherwise false</returns>
    virtual bool DownloadDataGPU(MeshBufferType type, BytesContainer& result) const = 0;

    /// <summary>
    /// Extracts mesh buffer data from GPU in the async task.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <returns>Created async task used to gather the buffer data.</returns>
    virtual Task* DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const = 0;

    /// <summary>
    /// Extract mesh buffer data from CPU. Cached internally.
    /// </summary>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <param name="count">The amount of items inside the result buffer.</param>
    /// <returns>True if failed, otherwise false</returns>
    virtual bool DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const = 0;

public:
    /// <summary>
    /// Model instance drawing packed data.
    /// </summary>
    struct DrawInfo
    {
        /// <summary>
        /// The instance buffer to use during model rendering.
        /// </summary>
        ModelInstanceEntries* Buffer;

        /// <summary>
        /// The world transformation of the model.
        /// </summary>
        Matrix* World;

        /// <summary>
        /// The instance drawing state data container. Used for LOD transition handling and previous world transformation matrix updating.
        /// </summary>
        GeometryDrawStateData* DrawState;

        /// <summary>
        /// The instance deformation utility.
        /// </summary>
        MeshDeformation* Deformation;

        union
        {
            struct
            {
                /// <summary>
                /// The skinning.
                /// </summary>
                SkinnedMeshDrawData* Skinning;
            };

            struct
            {
                /// <summary>
                /// The lightmap.
                /// </summary>
                const Lightmap* Lightmap;

                /// <summary>
                /// The lightmap UVs.
                /// </summary>
                const Rectangle* LightmapUVs;
            };
        };

        /// <summary>
        /// The model instance vertex colors buffers (per-lod all meshes packed in a single allocation, array length equal to model lods count).
        /// </summary>
        GPUBuffer** VertexColors;

        /// <summary>
        /// The object static flags.
        /// </summary>
        StaticFlags Flags;

        /// <summary>
        /// The object draw modes.
        /// </summary>
        DrawPass DrawModes;

        /// <summary>
        /// The bounds of the model (used to select a proper LOD during rendering).
        /// </summary>
        BoundingSphere Bounds;

        /// <summary>
        /// The per-instance random value.
        /// </summary>
        float PerInstanceRandom;

        /// <summary>
        /// The LOD bias value.
        /// </summary>
        char LODBias;

        /// <summary>
        /// The forced LOD to use. Value -1 disables this feature.
        /// </summary>
        char ForcedLOD;

        /// <summary>
        /// The object sorting key.
        /// </summary>
        int8 SortOrder;
    };
};
