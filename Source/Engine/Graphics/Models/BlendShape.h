// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Vector3.h"

/// <summary>
/// The blend shape vertex data optimized for runtime meshes morphing.
/// </summary>
struct BlendShapeVertex
{
    /// <summary>
    /// The position offset.
    /// </summary>
    Float3 PositionDelta;

    /// <summary>
    /// The normal vector offset (tangent Z).
    /// </summary>
    Float3 NormalDelta;

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

    void LoadHeader(class ReadStream& stream, byte headerVersion);
    void Load(ReadStream& stream, byte meshVersion);
#if USE_EDITOR
    void SaveHeader(class WriteStream& stream) const;
    void Save(WriteStream& stream) const;
#endif
};
