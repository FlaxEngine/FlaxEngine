// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Graphics/DynamicBuffer.h"

class SkinnedMesh;
class SkinnedModel;
class GPUBuffer;

/// <summary>
/// The blend shape vertex data optimized for runtime meshes morphing.
/// </summary>
struct BlendShapeVertex
{
    /// <summary>
    /// The position offset.
    /// </summary>
    Vector3 PositionDelta;

    /// <summary>
    /// The normal vector offset (tangent Z).
    /// </summary>
    Vector3 NormalDelta;

    /// <summary>
    /// The index of the vertex in the mesh to blend.
    /// </summary>
    uint32 VertexIndex;
};

template<>
struct TIsPODType<BlendShapeVertex>
{
    enum { Value = true };
};

/// <summary>
/// Data container for the blend shape.
/// </summary>
class BlendShape
{
public:

    /// <summary>
    /// The name of the blend shape.
    /// </summary>
    String Name;

    /// <summary>
    /// The weight of the blend shape.
    /// </summary>
    float Weight;

    /// <summary>
    /// True if blend shape contains deltas for normal vectors of the mesh, otherwise calculations related to tangent space can be skipped.
    /// </summary>
    bool UseNormals;

    /// <summary>
    /// The minimum index of the vertex in all blend shape vertices. Used to optimize blend shapes updating.
    /// </summary>
    uint32 MinVertexIndex;

    /// <summary>
    /// The maximum index of the vertex in all blend shape vertices. Used to optimize blend shapes updating.
    /// </summary>
    uint32 MaxVertexIndex;

    /// <summary>
    /// The list of shape vertices.
    /// </summary>
    Array<BlendShapeVertex> Vertices;
};

/// <summary>
/// The blend shapes runtime instance data. Handles blend shapes updating, blending and preparing for skinned mesh rendering.
/// </summary>
class BlendShapesInstance
{
public:

    /// <summary>
    /// The runtime data for blend shapes used for on a mesh.
    /// </summary>
    class MeshInstance
    {
    public:

        bool IsUsed;
        bool IsDirty;
        uint32 DirtyMinVertexIndex;
        uint32 DirtyMaxVertexIndex;
        DynamicVertexBuffer VertexBuffer;

        MeshInstance();
        ~MeshInstance();
    };

public:

    /// <summary>
    /// The blend shapes weights (pair of blend shape name and the weight).
    /// </summary>
    Array<Pair<String, float>> Weights;

    /// <summary>
    /// Flag that marks if blend shapes weights has been modified.
    /// </summary>
    bool WeightsDirty = false;

    /// <summary>
    /// The blend shapes meshes data.
    /// </summary>
    Dictionary<SkinnedMesh*, MeshInstance*> Meshes;

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="BlendShapesInstance"/> class.
    /// </summary>
    ~BlendShapesInstance();

    /// <summary>
    /// Updates the instanced meshes. Performs the blend shapes blending (only if weights were modified).
    /// </summary>
    /// <param name="skinnedModel">The skinned model used for blend shapes.</param>
    void Update(SkinnedModel* skinnedModel);

    /// <summary>
    /// Clears the runtime data.
    /// </summary>
    void Clear();
};
