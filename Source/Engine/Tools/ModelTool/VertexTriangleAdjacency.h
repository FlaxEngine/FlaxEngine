// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_MODEL_TOOL && USE_EDITOR

#include "Engine/Core/Config.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// The VertexTriangleAdjacency class computes a vertex-triangle adjacency map from a given index buffer.
/// </summary>
class VertexTriangleAdjacency
{
public:

    /// <summary>
    /// Construction from an existing index buffer
    /// </summary>
    /// <param name="indices">The index buffer.</param>
    /// <param name="indicesCount">The number of triangles in the buffer.</param>
    /// <param name="vertexCount">The number of referenced vertices. This value is computed automatically if 0 is specified.</param>
    /// <param name="computeNumTriangles">If you want the class to compute a list containing the number of referenced triangles per vertex per vertex - pass true.</param>
    VertexTriangleAdjacency(uint32* indices, int32 indicesCount, uint32 vertexCount = 0, bool computeNumTriangles = true);

    /// <summary>
    /// Destructor
    /// </summary>
    ~VertexTriangleAdjacency();

public:

    /// <summary>
    /// The offset table
    /// </summary>
    uint32* OffsetTable;

    /// <summary>
    /// The adjacency table.
    /// </summary>
    uint32* AdjacencyTable;

    /// <summary>
    /// The table containing the number of referenced triangles per vertex.
    /// </summary>
    uint32* LiveTriangles;

    /// <summary>
    /// The total number of referenced vertices.
    /// </summary>
    uint32 NumVertices;

public:

    /// <summary>
    /// Gets all triangles adjacent to a vertex.
    /// </summary>
    /// <param name="vertexIndex">The index of the vertex.</param>
    /// <returns>A pointer to the adjacency list.</returns>
    uint32* GetAdjacentTriangles(uint32 vertexIndex) const
    {
        ASSERT(vertexIndex >= 0 && vertexIndex < NumVertices);
        return &AdjacencyTable[OffsetTable[vertexIndex]];
    }

    /// <summary>
    /// Gets the number of triangles that are referenced by a vertex. This function returns a reference that can be modified.
    /// </summary>
    /// <param name="vertexIndex">The index of the vertex.</param>
    /// <returns>The number of referenced triangles</returns>
    uint32& GetNumTrianglesPtr(uint32 vertexIndex) const
    {
        ASSERT(vertexIndex >= 0 && vertexIndex < NumVertices);
        ASSERT(nullptr != LiveTriangles);
        return LiveTriangles[vertexIndex];
    }
};

#endif
