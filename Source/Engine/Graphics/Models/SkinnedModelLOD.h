// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "SkinnedMesh.h"

class MemoryReadStream;

/// <summary>
/// Represents single Level Of Detail for the skinned model. Contains a collection of the meshes.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SkinnedModelLOD : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(SkinnedModelLOD, PersistentScriptingObject);
    friend SkinnedModel;
private:

    SkinnedModel* _model = nullptr;

public:

    /// <summary>
    /// The screen size to switch LODs. Bottom limit of the model screen size to render this LOD.
    /// </summary>
    API_FIELD() float ScreenSize = 1.0f;

    /// <summary>
    /// The meshes array.
    /// </summary>
    API_FIELD(ReadOnly) Array<SkinnedMesh> Meshes;

    /// <summary>
    /// Determines whether any mesh has been initialized.
    /// </summary>
    bool HasAnyMeshInitialized() const;

public:

    /// <summary>
    /// Initializes the LOD from the data stream.
    /// </summary>
    /// <param name="stream">The stream.</param>
    /// <returns>True if fails, otherwise false.</returns>
    bool Load(MemoryReadStream& stream);

    /// <summary>
    /// Unloads the LOD meshes data (vertex buffers and cache). It won't dispose the meshes collection. The opposite to Load.
    /// </summary>
    void Unload();

    /// <summary>
    /// Cleanups the data.
    /// </summary>
    void Dispose();

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
    bool Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal, SkinnedMesh** mesh);

    /// <summary>
    /// Get model bounding box in transformed world for given instance buffer
    /// </summary>
    /// <param name="world">World matrix</param>
    /// <returns>Bounding box</returns>
    BoundingBox GetBox(const Matrix& world) const;

    /// <summary>
    /// Get model bounding box in transformed world for given instance buffer for only one mesh
    /// </summary>
    /// <param name="world">World matrix</param>
    /// <param name="meshIndex">esh index</param>
    /// <returns>Bounding box</returns>
    BoundingBox GetBox(const Matrix& world, int32 meshIndex) const;

    /// <summary>
    /// Gets the bounding box combined for all meshes in this model LOD.
    /// </summary>
    API_PROPERTY() BoundingBox GetBox() const;

    /// <summary>
    /// Draws the meshes. Binds vertex and index buffers and invokes the draw calls.
    /// </summary>
    /// <param name="context">The GPU context to draw with.</param>
    FORCE_INLINE void Render(GPUContext* context)
    {
        for (int32 i = 0; i < Meshes.Count(); i++)
        {
            Meshes[i].Render(context);
        }
    }

    /// <summary>
    /// Draws all the meshes from the model LOD.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="info">The packed drawing info data.</param>
    /// <param name="lodDitherFactor">The LOD transition dither factor.</param>
    FORCE_INLINE void Draw(const RenderContext& renderContext, const SkinnedMesh::DrawInfo& info, float lodDitherFactor) const
    {
        for (int32 i = 0; i < Meshes.Count(); i++)
        {
            Meshes[i].Draw(renderContext, info, lodDitherFactor);
        }
    }
};
