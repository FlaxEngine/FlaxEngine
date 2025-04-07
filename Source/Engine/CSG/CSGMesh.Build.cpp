// Copyright (c) Wojciech Figat. All rights reserved.

#include "CSGMesh.h"

#if COMPILE_WITH_CSG_BUILDER

#include "Engine/Core/Log.h"

void CSG::Mesh::Build(Brush* parentBrush)
{
    struct EdgeIntersection
    {
        int32 EdgeIndex;
        int32 PlaneIndices[2];

        EdgeIntersection()
        {
            EdgeIndex = INVALID_INDEX;
        }

        EdgeIntersection(int32 edgeIndex, int32 planeIndexA, int32 planeIndexB)
        {
            EdgeIndex = edgeIndex;
            PlaneIndices[0] = planeIndexA;
            PlaneIndices[1] = planeIndexB;
        }
    };

    struct PointIntersection
    {
        // TODO: optimize this shit?
        int32 VertexIndex;
        Array<EdgeIntersection> Edges;
        Array<int32> PlaneIndices;

        PointIntersection()
        {
            VertexIndex = INVALID_INDEX;
        }

        PointIntersection(int32 vertexIndex, const Array<int32>& planes)
        {
            VertexIndex = vertexIndex;
            PlaneIndices = planes;
        }
    };

    // Clear
    _bounds.Clear();
    _surfaces.Clear();
    _polygons.Clear();
    _edges.Clear();
    _vertices.Clear();
    _brushesMeta.Clear();

    // Get brush planes
    if (parentBrush == nullptr)
        return;
    auto mode = parentBrush->GetBrushMode();
    parentBrush->GetSurfaces(_surfaces);
    int32 surfacesCount = _surfaces.Count();
    if (surfacesCount > 250)
    {
        LOG(Error, "CSG brush has too many planes: {0}. Cannot process it!", surfacesCount);
        return;
    }

    // Fix duplicated planes (coplanar ones)
    for (int32 i = 0; i < surfacesCount; i++)
    {
        for (int32 j = i + 1; j < surfacesCount; j++)
        {
            if (Surface::NearEqual(_surfaces[i], _surfaces[j]))
            {
                // Change to blank plane
                _surfaces[i].Normal = Vector3::Up;
                _surfaces[i].D = MAX_float;
                continue;
            }
        }
    }

    Array<PointIntersection> pointIntersections(surfacesCount * surfacesCount);
    Array<int32> intersectingPlanes;

    // Find all point intersections where 3 (or more planes) intersect
    for (int32 planeIndex1 = 0; planeIndex1 < surfacesCount - 2; planeIndex1++)
    {
        Surface plane1 = _surfaces[planeIndex1];
        for (int32 planeIndex2 = planeIndex1 + 1; planeIndex2 < surfacesCount - 1; planeIndex2++)
        {
            Surface plane2 = _surfaces[planeIndex2];
            for (int32 planeIndex3 = planeIndex2 + 1; planeIndex3 < surfacesCount; planeIndex3++)
            {
                Surface plane3 = _surfaces[planeIndex3];
                int32 vertexIndex;

                // Calculate the intersection point
                Vector3 vertex = Plane::Intersection(plane1, plane2, plane3);

                // Check if the intersection is valid
                if (vertex.IsNaN() || vertex.IsInfinity())
                    continue;

                intersectingPlanes.Clear();
                intersectingPlanes.Add(planeIndex1);
                intersectingPlanes.Add(planeIndex2);
                intersectingPlanes.Add(planeIndex3);

                for (int32 planeIndex4 = 0; planeIndex4 < surfacesCount; planeIndex4++)
                {
                    if (planeIndex4 == planeIndex1 || planeIndex4 == planeIndex2 || planeIndex4 == planeIndex3)
                        continue;

                    Surface& plane4 = _surfaces[planeIndex4];
                    PlaneIntersectionType side = plane4.OnSide(vertex);
                    if (side == PlaneIntersectionType::Intersecting)
                    {
                        if (planeIndex4 < planeIndex3)
                            // Already found this vertex
                            goto SkipIntersection;

                        // We've found another plane which goes trough our found intersection point
                        intersectingPlanes.Add(planeIndex4);
                    }
                    else if (side == PlaneIntersectionType::Front)
                        // Intersection is outside of brush
                        goto SkipIntersection;
                }

                vertexIndex = _vertices.Count();
                _vertices.Add(vertex);

                // Add intersection point to our list
                pointIntersections.Add(PointIntersection(vertexIndex, intersectingPlanes));

            SkipIntersection:
                ;
            }
        }
    }

    int32 foundPlanes[2];

    // Find all our intersection edges which are formed by a pair of planes
    // (this could probably be done inside the previous loop)
    for (int32 i = 0; i < pointIntersections.Count(); i++)
    {
        PointIntersection& pointIntersectionA = pointIntersections[i];
        for (int32 j = i + 1; j < pointIntersections.Count(); j++)
        {
            auto& pointIntersectionB = pointIntersections[j];
            auto& planesIndicesA = pointIntersectionA.PlaneIndices;
            auto& planesIndicesB = pointIntersectionB.PlaneIndices;

            int32 foundPlaneIndex = 0;
            for (int32 k = 0; k < planesIndicesA.Count(); k++)
            {
                int32 currentPlaneIndex = planesIndicesA[k];

                if (!planesIndicesB.Contains(currentPlaneIndex))
                    continue;

                foundPlanes[foundPlaneIndex] = currentPlaneIndex;
                foundPlaneIndex++;

                if (foundPlaneIndex == 2)
                    break;
            }

            // If foundPlaneIndex is 0 or 1 then either this combination does not exist, or only goes trough one point 
            if (foundPlaneIndex < 2)
                continue;

            // Create our found intersection edge
            HalfEdge halfEdgeA;
            int32 halfEdgeAIndex = _edges.Count();

            HalfEdge halfEdgeB;
            int32 halfEdgeBIndex = halfEdgeAIndex + 1;

            halfEdgeA.TwinIndex = halfEdgeBIndex;
            halfEdgeB.TwinIndex = halfEdgeAIndex;

            halfEdgeA.VertexIndex = pointIntersectionA.VertexIndex;
            halfEdgeB.VertexIndex = pointIntersectionB.VertexIndex;

            // Add it to our points
            pointIntersectionA.Edges.Add(EdgeIntersection(
                halfEdgeAIndex,
                foundPlanes[0],
                foundPlanes[1]));
            pointIntersectionB.Edges.Add(EdgeIntersection(
                halfEdgeBIndex,
                foundPlanes[0],
                foundPlanes[1]));

            _edges.Add(halfEdgeA);
            _edges.Add(halfEdgeB);
        }
    }

    // TODO: snap vertices to remove gaps (snapping facctor can be defined by the parent brush)

    for (int32 i = 0; i < surfacesCount; i++)
    {
        Polygon polygon;
        polygon.SurfaceIndex = i;
        polygon.Visible = true;
        _polygons.Add(polygon);
    }

    for (int32 i = pointIntersections.Count() - 1; i >= 0; i--)
    {
        auto& pointIntersection = pointIntersections[i];
        auto& pointEdges = pointIntersection.Edges;

        // Make sure that we have at least 2 edges ...
        // This may happen when a plane only intersects at a single edge.
        if (pointEdges.Count() <= 2)
        {
            pointIntersections.RemoveAt(i);
            continue;
        }

        auto vertexIndex = pointIntersection.VertexIndex;
        auto& vertex = _vertices[vertexIndex];

        for (int32 j = 0; j < pointEdges.Count() - 1; j++)
        {
            auto& edge1 = pointEdges[j];
            for (int32 k = j + 1; k < pointEdges.Count(); k++)
            {
                auto& edge2 = pointEdges[k];

                int32 planeIndex1 = INVALID_INDEX;
                int32 planeIndex2 = INVALID_INDEX;

                // Determine if and which of our 2 planes are identical
                if (edge1.PlaneIndices[0] == edge2.PlaneIndices[0])
                {
                    planeIndex1 = 0;
                    planeIndex2 = 0;
                }
                else if (edge1.PlaneIndices[0] == edge2.PlaneIndices[1])
                {
                    planeIndex1 = 0;
                    planeIndex2 = 1;
                }
                else if (edge1.PlaneIndices[1] == edge2.PlaneIndices[0])
                {
                    planeIndex1 = 1;
                    planeIndex2 = 0;
                }
                else if (edge1.PlaneIndices[1] == edge2.PlaneIndices[1])
                {
                    planeIndex1 = 1;
                    planeIndex2 = 1;
                }
                else
                {
                    continue;
                }

                HalfEdge* ingoing;
                HalfEdge* outgoing;
                int32 outgoingIndex;

                Surface& shared_plane = _surfaces[edge1.PlaneIndices[planeIndex1]];
                Surface& edge1_plane = _surfaces[edge1.PlaneIndices[1 - planeIndex1]];
                Surface& edge2_plane = _surfaces[edge2.PlaneIndices[1 - planeIndex2]];

                Vector3 direction = Vector3::Cross(shared_plane.Normal, edge1_plane.Normal);

                // Determine the orientation of our two edges to determine which edge is in-going, and which one is out-going
                if (Vector3::Dot(direction, edge2_plane.Normal) < 0)
                {
                    ingoing = &_edges[edge2.EdgeIndex];
                    outgoingIndex = _edges[edge1.EdgeIndex].TwinIndex;
                    outgoing = &_edges[outgoingIndex];
                }
                else
                {
                    ingoing = &_edges[edge1.EdgeIndex];
                    outgoingIndex = _edges[edge2.EdgeIndex].TwinIndex;
                    outgoing = &_edges[outgoingIndex];
                }

                // Link the out-going half-edge to the in-going half-edge
                ingoing->NextIndex = outgoingIndex;

                // Add reference to polygon to half-edge, and make sure our polygon has a reference to a half-edge 
                // Since a half-edge, in this case, serves as a circular linked list this just works.
                auto polygonIndex = edge1.PlaneIndices[planeIndex1];

                ingoing->PolygonIndex = polygonIndex;
                outgoing->PolygonIndex = polygonIndex;

                auto& polygon = _polygons[polygonIndex];
                polygon.FirstEdgeIndex = outgoingIndex;
                polygon.Bounds.Add(vertex);
            }
        }

        // Add the intersection point to the area of our bounding box
        _bounds.Add(vertex);
    }

    // Setup base brush meta
    BrushMeta meta;
    meta.Mode = mode;
    meta.StartSurfaceIndex = 0;
    meta.SurfacesCount = surfacesCount;
    meta.Bounds = _bounds;
    meta.Parent = parentBrush;
    _brushesMeta.Add(meta);
}

#endif
