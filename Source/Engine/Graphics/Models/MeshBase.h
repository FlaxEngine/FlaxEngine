// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Scripting/ScriptingObject.h"

class Task;
class ModelBase;

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
    /// <returns>True if this mesh is using 16 bit index buffer, otherwise 32 bit index buffer.</returns>
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
};
