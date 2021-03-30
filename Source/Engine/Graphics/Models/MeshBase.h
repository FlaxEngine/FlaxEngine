// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Scripting/ScriptingObject.h"

class Task;

/// <summary>
/// Base class for model resources meshes.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API MeshBase : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_MINIMAL(MeshBase);
protected:

    bool _use16BitIndexBuffer;
    BoundingBox _box;
    BoundingSphere _sphere;
    uint32 _vertices;
    uint32 _triangles;

    explicit MeshBase(const SpawnParams& params)
        : PersistentScriptingObject(params)
    {
    }

public:

    /// <summary>
    /// Gets the triangle count.
    /// </summary>
    /// <returns>The triangles</returns>
    API_PROPERTY() FORCE_INLINE int32 GetTriangleCount() const
    {
        return _triangles;
    }

    /// <summary>
    /// Gets the vertex count.
    /// </summary>
    /// <returns>The vertices</returns>
    API_PROPERTY() FORCE_INLINE int32 GetVertexCount() const
    {
        return _vertices;
    }

    /// <summary>
    /// Gets the box.
    /// </summary>
    /// <returns>The bounding box.</returns>
    API_PROPERTY() FORCE_INLINE const BoundingBox& GetBox() const
    {
        return _box;
    }

    /// <summary>
    /// Gets the sphere.
    /// </summary>
    /// <returns>The bounding sphere.</returns>
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
    /// <returns>True if failed, otherwise false</returns>
    virtual bool DownloadDataCPU(MeshBufferType type, BytesContainer& result) const = 0;
};
