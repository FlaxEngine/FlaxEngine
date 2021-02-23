// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Mesh.h"

class MemoryReadStream;

/// <summary>
/// Represents single Level Of Detail for the model. Contains a collection of the meshes.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API ModelLOD : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(ModelLOD, PersistentScriptingObject);
    friend Model;
    friend Mesh;
private:

    Model* _model = nullptr;
    uint32 _verticesCount;

public:

    /// <summary>
    /// The screen size to switch LODs. Bottom limit of the model screen size to render this LOD.
    /// </summary>
    API_FIELD() float ScreenSize = 1.0f;

    /// <summary>
    /// The meshes array.
    /// </summary>
    API_FIELD(ReadOnly) Array<Mesh> Meshes;

    /// <summary>
    /// Determines whether any mesh has been initialized.
    /// </summary>
    /// <returns>True if any mesh has been initialized, otherwise false.</returns>
    FORCE_INLINE bool HasAnyMeshInitialized() const
    {
        // Note: we initialize all meshes at once so the last one can be used to check it.
        return Meshes.HasItems() && Meshes.Last().IsInitialized();
    }

    /// <summary>
    /// Gets the vertex count for this model LOD level.
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetVertexCount() const
    {
        return _verticesCount;
    }

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
    bool Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal, Mesh** mesh);

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
    /// Draws the meshes from the model LOD.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="material">The material to use for rendering.</param>
    /// <param name="world">The world transformation of the model.</param>
    /// <param name="flags">The object static flags.</param>
    /// <param name="receiveDecals">True if rendered geometry can receive decals, otherwise false.</param>
    /// <param name="drawModes">The draw passes to use for rendering this object.</param>
    /// <param name="perInstanceRandom">The random per-instance value (normalized to range 0-1).</param>
    API_FUNCTION() void Draw(API_PARAM(Ref) const RenderContext& renderContext, MaterialBase* material, API_PARAM(Ref) const Matrix& world, StaticFlags flags = StaticFlags::None, bool receiveDecals = true, DrawPass drawModes = DrawPass::Default, float perInstanceRandom = 0.0f) const
    {
        for (int32 i = 0; i < Meshes.Count(); i++)
        {
            Meshes[i].Draw(renderContext, material, world, flags, receiveDecals, drawModes, perInstanceRandom);
        }
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
        {
            Meshes[i].Draw(renderContext, info, lodDitherFactor);
        }
    }
};
