// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL && USE_EDITOR

#include "VertexTriangleAdjacency.h"
#include "Engine/Core/Math/Math.h"

VertexTriangleAdjacency::VertexTriangleAdjacency(uint32* indices, int32 indicesCount, uint32 vertexCount, bool computeNumTriangles)
{
    // Compute the number of referenced vertices if it wasn't specified by the caller
    const uint32* const indicesEnd = indices + indicesCount;
    if (vertexCount == 0)
    {
        for (uint32* triangle = indices; triangle != indicesEnd; triangle += 3)
        {
            ASSERT(nullptr != triangle);
            vertexCount = Math::Max(vertexCount, triangle[0]);
            vertexCount = Math::Max(vertexCount, triangle[1]);
            vertexCount = Math::Max(vertexCount, triangle[2]);
        }
    }

    NumVertices = vertexCount;
    uint32* pi;

    // Allocate storage
    if (computeNumTriangles)
    {
        pi = LiveTriangles = new uint32[vertexCount + 1];
        Platform::MemoryClear(LiveTriangles, sizeof(uint32) * (vertexCount + 1));
        OffsetTable = new uint32[vertexCount + 2] + 1;
    }
    else
    {
        pi = OffsetTable = new uint32[vertexCount + 2] + 1;
        Platform::MemoryClear(OffsetTable, sizeof(uint32) * (vertexCount + 1));
        LiveTriangles = nullptr; // Important, otherwise the d'tor would crash
    }

    // Get a pointer to the end of the buffer
    uint32* piEnd = pi + vertexCount;
    *piEnd++ = 0u;

    // First pass: compute the number of faces referencing each vertex
    for (uint32* triangle = indices; triangle != indicesEnd; triangle += 3)
    {
        pi[triangle[0]]++;
        pi[triangle[1]]++;
        pi[triangle[2]]++;
    }

    // Second pass: compute the final offset table
    int32 iSum = 0;
    uint32* piCurOut = OffsetTable;
    for (uint32* piCur = pi; piCur != piEnd; ++piCur, piCurOut++)
    {
        const int32 iLastSum = iSum;
        iSum += *piCur;
        *piCurOut = iLastSum;
    }
    pi = this->OffsetTable;

    // Third pass: compute the final table
    AdjacencyTable = new uint32[iSum];
    iSum = 0;
    for (uint32* triangle = indices; triangle != indicesEnd; triangle += 3, iSum++)
    {
        uint32 idx = triangle[0];
        AdjacencyTable[pi[idx]++] = iSum;

        idx = triangle[1];
        AdjacencyTable[pi[idx]++] = iSum;

        idx = triangle[2];
        AdjacencyTable[pi[idx]++] = iSum;
    }

    // Fourth pass: undo the offset computations made during the third pass
    // We could do this in a separate buffer, but this would be TIMES slower.
    OffsetTable--;
    *OffsetTable = 0u;
}

VertexTriangleAdjacency::~VertexTriangleAdjacency()
{
    delete[] OffsetTable;
    delete[] AdjacencyTable;
    delete[] LiveTriangles;
}

#endif
