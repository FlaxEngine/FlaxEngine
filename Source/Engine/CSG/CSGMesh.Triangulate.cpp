// Copyright (c) Wojciech Figat. All rights reserved.

#include "CSGMesh.h"

#if COMPILE_WITH_CSG_BUILDER

#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "CSGData.h"

bool CSG::Mesh::Triangulate(RawData& data, Array<MeshVertex>& cacheVB) const
{
    // Reject empty meshes
    int32 verticesCount = _vertices.Count();
    if (verticesCount == 0)
        return true;

    // Mesh triangles data
    cacheVB.Clear();
    cacheVB.EnsureCapacity(_polygons.Count() * 3);
    Array<MeshVertex> surfaceCacheVB(32);

    // Cache submeshes by material to lay them down
    // key- brush index, value- direcotry for surfaces (key: surface index, value: list with start vertex per triangle)
    Dictionary<int32, Dictionary<int32, Array<int32>>> polygonsPerBrush(64);

    // Build index buffer
    int32 firstI, secondI, thirdI;
    int32 triangleIndices[3];
    Vector2 uvs[3];
    MeshVertex rawVertex;
    for (int32 i = 0; i < _polygons.Count(); i++)
    {
        const Polygon& polygon = _polygons[i];
        if (!polygon.Visible || polygon.FirstEdgeIndex == INVALID_INDEX)
            continue;

        HalfEdge iterator = _edges[polygon.FirstEdgeIndex];
        firstI = iterator.VertexIndex;
        iterator = _edges[iterator.NextIndex];
        secondI = iterator.VertexIndex;
        int32 lastIndex = -1;
        const HalfEdge& polygonFirst = _edges[polygon.FirstEdgeIndex];

        while (iterator != polygonFirst)
        {
            if (lastIndex == iterator.NextIndex)
                break;
            lastIndex = iterator.NextIndex;
            iterator = _edges.At(lastIndex);
            thirdI = iterator.VertexIndex;

            // Validate triangle
            if (firstI != secondI && firstI != thirdI && secondI != thirdI)
            {
                // Cache triangle parent surface info
                const Surface& surface = _surfaces[polygon.SurfaceIndex];
                Vector3 uvPos, centerPos;
                Matrix trans, transRotation, finalTrans;

                // Build triangle indices
                if (polygon.Inverted)
                {
                    triangleIndices[0] = thirdI;
                    triangleIndices[1] = secondI;
                    triangleIndices[2] = firstI;
                }
                else
                {
                    triangleIndices[0] = firstI;
                    triangleIndices[1] = secondI;
                    triangleIndices[2] = thirdI;
                }

                Vector3 surfaceUp = surface.Normal;
                Vector3 surfaceForward = -Vector3::Cross(surfaceUp, Vector3::Right);
                if (surfaceForward.IsZero())
                    surfaceForward = Vector3::Forward;

                Matrix::CreateWorld(Vector3::Zero, surfaceForward, surfaceUp, trans);
                Matrix::RotationAxis(surfaceUp, surface.TexCoordRotation * DegreesToRadians, transRotation);
                Matrix::Multiply(transRotation, trans, finalTrans);
                Vector3::Transform(Vector3::Zero, finalTrans, centerPos);

                // Calculate normal vector
                Vector3 v0 = _vertices[triangleIndices[0]];
                Vector3 v1 = _vertices[triangleIndices[1]];
                Vector3 v2 = _vertices[triangleIndices[2]];
                Float3 vd0 = v1 - v0;
                Float3 vd1 = v2 - v0;
                Float3 normal = Float3::Normalize(vd0 ^ vd1);

                // Calculate texture uvs based on vertex position
                for (int32 q = 0; q < 3; q++)
                {
                    Vector3 pos = _vertices[triangleIndices[q]];
                    Vector3::Transform(pos, finalTrans, uvPos);
                    Float2 texCoord = Vector2(uvPos.X - centerPos.X, uvPos.Z - centerPos.Z);

                    // Apply surface uvs transformation
                    uvs[q] = (texCoord * (surface.TexCoordScale * CSG_MESH_UV_SCALE)) + surface.TexCoordOffset;
                }

                // Calculate tangent (it needs uvs)
                Float2 uvd0 = uvs[1] - uvs[0];
                Float2 uvd1 = uvs[2] - uvs[0];
                float dR = (uvd0.X * uvd1.Y - uvd1.X * uvd0.Y);
                if (Math::IsZero(dR))
                    dR = 1.0f;
                float r = 1.0f / dR;
                Float3 tangent = (vd0 * uvd1.Y - vd1 * uvd0.Y) * r;
                Float3 bitangent = (vd1 * uvd0.X - vd0 * uvd1.X) * r;
                tangent.Normalize();

                // Gram-Schmidt orthogonalize
                Float3 newTangentUnnormalized = tangent - normal * Float3::Dot(normal, tangent);
                const float length = newTangentUnnormalized.Length();

                // Workaround to handle degenerated case
                if (Math::IsZero(length))
                {
                    tangent = Float3::Cross(normal, Float3::UnitX);
                    if (Math::IsZero(tangent.Length()))
                        tangent = Float3::Cross(normal, Float3::UnitY);
                    tangent.Normalize();
                    bitangent = Float3::Cross(normal, tangent);
                }
                else
                {
                    tangent = newTangentUnnormalized / length;
                    bitangent.Normalize();
                }

                // Build triangle
                int32 vertexIndex = cacheVB.Count();
                for (int32 q = 0; q < 3; q++)
                {
                    rawVertex.Position = _vertices[triangleIndices[q]];
                    rawVertex.TexCoord = uvs[q];
                    rawVertex.Normal = normal;
                    rawVertex.Tangent = tangent;
                    rawVertex.Bitangent = bitangent;
                    rawVertex.LightmapUVs = Vector2::Zero;

                    cacheVB.Add(rawVertex);
                }

                // Find brush that produced that surface
                bool isValid = false;
                for (int32 brushIndex = 0; brushIndex < _brushesMeta.Count(); brushIndex++)
                {
                    auto& brushMeta = _brushesMeta[brushIndex];
                    if (Math::IsInRange(polygon.SurfaceIndex, brushMeta.StartSurfaceIndex, brushMeta.StartSurfaceIndex + brushMeta.SurfacesCount - 1))
                    {
                        // Cache triangle
                        polygonsPerBrush[brushIndex][polygon.SurfaceIndex].Add(vertexIndex);

                        isValid = true;
                        break;
                    }
                }
                ASSERT_LOW_LAYER(isValid);
            }

            secondI = thirdI;
        }
    }

    // Check if mesh has no triangles
    if (cacheVB.IsEmpty())
        return true;

    // TODO: preallocate data material slots and buffers so data.AddTriangle will be simple copy op

    // Setup result mesh data
    for (auto iPerBrush = polygonsPerBrush.Begin(); iPerBrush != polygonsPerBrush.End(); ++iPerBrush)
    {
        auto& brushMeta = _brushesMeta[iPerBrush->Key];

        for (auto iPerSurface = iPerBrush->Value.Begin(); iPerSurface != iPerBrush->Value.End(); ++iPerSurface)
        {
            int32 surfaceIndex = iPerSurface->Key;
            int32 brushSurfaceIndex = surfaceIndex - brushMeta.StartSurfaceIndex;
            int32 triangleCount = iPerSurface->Value.Count();
            int32 vertexCount = triangleCount * 3;
            Float2 lightmapUVsMin = Float2::Maximum;
            Float2 lightmapUVsMax = Float2::Minimum;
            const Surface& surface = _surfaces[surfaceIndex];

            // Generate lightmap uvs per brush surface
            {
                // We just assume that brush surface is now some kind of rectangle (after triangulation)
                // So we can project vertices locations into texture space uvs [0;1] based on minimum and maximum vertex location

                Vector3 surfaceNormal = surface.Normal;

                // Calculate bounds
                Vector2 min = Vector2::Maximum, max = Vector2::Minimum;
                Matrix projection, view, vp;
                Vector3 up = Vector3::Up;
                if (Vector3::NearEqual(surfaceNormal, Vector3::Up))
                    up = Vector3::Right;
                else if (Vector3::NearEqual(surfaceNormal, Vector3::Down))
                    up = Vector3::Forward;
                Matrix::LookAt(Vector3::Zero, surfaceNormal, up, view);
                Matrix::Ortho(1000, 1000, 0.00001f, 100000.0f, projection);
                Matrix::Multiply(view, projection, vp);
                Array<Vector2> pointsCache(vertexCount);
                for (int32 triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
                {
                    int32 triangleStartVertex = iPerSurface->Value[triangleIndex];
                    for (int32 k = 0; k < 3; k++)
                    {
                        auto& vertex = cacheVB[triangleStartVertex + k];

                        Vector3 projected = Vector3::Project(vertex.Position, 0, 0, 1000, 1000, 0, 1, vp);
                        Vector2 projectedXY = Vector2(projected);

                        min = Vector2::Min(projectedXY, min);
                        max = Vector2::Max(projectedXY, max);

                        pointsCache.Add(projectedXY);
                    }
                }

                // Normalize projected positions to get lightmap uvs
                Real projectedSize = (max - min).MaxValue();
                for (int32 triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
                {
                    int32 triangleStartVertex = iPerSurface->Value[triangleIndex];
                    for (int32 k = 0; k < 3; k++)
                    {
                        auto& vertex = cacheVB[triangleStartVertex + k];

                        vertex.LightmapUVs = (pointsCache[triangleIndex * 3 + k] - min) / projectedSize;

                        lightmapUVsMin = Float2::Min(lightmapUVsMin, vertex.LightmapUVs);
                        lightmapUVsMax = Float2::Max(lightmapUVsMax, vertex.LightmapUVs);
                    }
                }
            }

            // Write triangles
            surfaceCacheVB.Clear();
            for (int32 triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
            {
                int32 triangleStartVertex = iPerSurface->Value[triangleIndex];
                surfaceCacheVB.Add(cacheVB[triangleStartVertex + 0]);
                surfaceCacheVB.Add(cacheVB[triangleStartVertex + 1]);
                surfaceCacheVB.Add(cacheVB[triangleStartVertex + 2]);
            }
            Rectangle lightmapUVsBox(lightmapUVsMin, lightmapUVsMax - lightmapUVsMin);
            data.AddSurface(brushMeta.Parent, brushSurfaceIndex, surface.Material, surface.ScaleInLightmap, lightmapUVsBox, surfaceCacheVB.Get(), surfaceCacheVB.Count());
        }
    }

    return false;
}

#endif
