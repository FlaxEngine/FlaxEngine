// Copyright (c) Wojciech Figat. All rights reserved.

#include "CSGMesh.h"

#if COMPILE_WITH_CSG_BUILDER

#include "Engine/Core/Collections/Array.h"

using namespace CSG;

bool CSG::Mesh::HasMode(Mode mode) const
{
    for (int32 i = 0; i < _brushesMeta.Count(); i++)
    {
        if (_brushesMeta[i].Mode == mode)
            return true;
    }

    return false;
}

void CSG::Mesh::PerformOperation(Mesh* other)
{
    ASSERT(other->_brushesMeta.Count() > 0);

    // Check if has more than one submeshes
    if (other->_brushesMeta.Count() > 1)
    {
        // Just add all subbrushes
        Add(other);
    }
    else
    {
        auto mode = other->_brushesMeta[0].Mode;

        // Switch between mesh operation mode
        switch (mode)
        {
        case Mode::Additive:
        {
            // Check if both meshes do not intersect
            if (AABB::IsOutside(_bounds, other->GetBounds())) // TODO: test every sub bounds not whole _bounds
            {
                // Add vertices to the mesh without any additional calculations
                Add(other);
            }
            else
            {
                // Remove common part from other mesh and then combine them
                //intersect(other, Remove, Keep);
                Add(other);

                // TODO: ogarnac to bo to jest zle xD
            }
        }
        break;

        case Mode::Subtractive:
        {
            // Check if both meshes do not intersect
            if (AABB::IsOutside(_bounds, other->GetBounds())) // TODO: test every sub bounds not whole _bounds
            {
                // Do nothing
            }
            else
            {
                // Perform intersection operations
                intersect(other, Remove, Keep);
                other->intersect(this, Flip, Remove);

                // Merge meshes
                Add(other);
            }
        }
        break;
        }
    }
}

void CSG::Mesh::Add(const Mesh* other)
{
    ASSERT(this != other && other);

    // Cache data
    int32 baseIndexVertices = _vertices.Count();
    int32 baseIndexSurfaces = _surfaces.Count();
    int32 baseIndexEdges = _edges.Count();
    int32 baseIndexPolygons = _polygons.Count();
    auto oVertices = other->GetVertices();
    auto oSurfaces = other->GetSurfaces();
    auto oEdges = other->GetEdges();
    auto oPolygons = other->GetPolygons();
    auto oMeta = &other->_brushesMeta;

    // Clone vertices
    for (int32 i = 0; i < oVertices->Count(); i++)
    {
        _vertices.Add(oVertices->At(i));
    }

    // Clone surfaces
    for (int32 i = 0; i < oSurfaces->Count(); i++)
    {
        _surfaces.Add(oSurfaces->At(i));
    }

    // Clone edges
    for (int32 i = 0; i < oEdges->Count(); i++)
    {
        HalfEdge edge = oEdges->At(i);
        edge.PolygonIndex += baseIndexPolygons;
        edge.TwinIndex += baseIndexEdges;
        edge.NextIndex += baseIndexEdges;
        edge.VertexIndex += baseIndexVertices;
        _edges.Add(edge);
    }

    // Clone polygons
    for (int32 i = 0; i < oPolygons->Count(); i++)
    {
        Polygon polygon = oPolygons->At(i);
        polygon.SurfaceIndex += baseIndexSurfaces;
        polygon.FirstEdgeIndex += baseIndexEdges;
        _polygons.Add(polygon);
    }

    // Clone meta
    for (int32 i = 0; i < oMeta->Count(); i++)
    {
        BrushMeta meta = oMeta->At(i);
        meta.StartSurfaceIndex += baseIndexSurfaces;
        _brushesMeta.Add(meta);
    }

    // Increase bounds
    _bounds.Add(other->GetBounds());
}

void CSG::Mesh::intersect(const Mesh* other, PolygonOperation insideOp, PolygonOperation outsideOp)
{
    // insideOp - operation for polygons being inside the other brush
    // outsideOp - operation for polygons being outside the other brush

    // Prevent from redundant action
    if (insideOp == Keep && outsideOp == Keep)
        return;

    // Check every sub brush from other mesh
    int32 count = other->_brushesMeta.Count();
    for (int32 subMeshIndex = 0; subMeshIndex < count; subMeshIndex++)
    {
        auto& brushMeta = other->_brushesMeta[subMeshIndex];

        // Check if can intersect with that sub mesh data
        if (!AABB::IsOutside(brushMeta.Bounds, _bounds))
        {
            PolygonOperation iOp = insideOp;
            PolygonOperation oOp = outsideOp;

            if (brushMeta.Mode == Mode::Subtractive)
            {
                iOp = Remove;
                oOp = Keep;
            }

            intersectSubMesh(other, subMeshIndex, iOp, oOp);
        }
    }

    // Update bounds
    updateBounds();
}

void CSG::Mesh::intersectSubMesh(const Mesh* other, int32 subMeshIndex, PolygonOperation insideOp, PolygonOperation outsideOp)
{
    // Cache data
    auto oSurfaces = other->GetSurfaces();
    auto& brushMeta = other->_brushesMeta[subMeshIndex];
    auto oBounds = brushMeta.Bounds;
    int32 startBrushSurface = brushMeta.StartSurfaceIndex;
    int32 endBrushSurface = startBrushSurface + brushMeta.SurfacesCount;

    // Check every polygon (iterate from end since we can ass new polygons and we don't want to process them)
    for (int32 i = _polygons.Count() - 1; i >= 0; i--)
    {
        auto& polygon = _polygons[i];
        if (!polygon.Visible || polygon.FirstEdgeIndex == INVALID_INDEX)
            continue;
        AABB polygonBounds = polygon.Bounds;

        auto finalResult = CompletelyInside;

        // A quick check if the polygon lies outside the planes we're cutting our polygons with
        if (!AABB::IsOutside(oBounds, polygonBounds))
        {
            PolygonSplitResult intermediateResult;
            Polygon* outsidePolygon;
            for (int32 otherIndex = startBrushSurface; otherIndex < endBrushSurface; otherIndex++)
            {
                auto& cuttingSurface = oSurfaces->At(otherIndex);

                auto side = cuttingSurface.OnSide(polygonBounds);
                if (side == PlaneIntersectionType::Front)
                {
                    finalResult = CompletelyOutside;
                    continue;
                }
                if (side == PlaneIntersectionType::Back)
                {
                    continue;
                }

                intermediateResult = polygonSplit(cuttingSurface, i, &outsidePolygon);

                if (intermediateResult == CompletelyOutside)
                {
                    finalResult = CompletelyOutside;
                    break;
                }
                if (intermediateResult == Split)
                {
                    resolvePolygon(*outsidePolygon, outsideOp);
                }
                else if (intermediateResult != CompletelyInside)
                {
                    finalResult = intermediateResult;
                }
            }
        }
        else
        {
            finalResult = CompletelyOutside;
        }

        switch (finalResult)
        {
        case CompletelyInside:
        {
            resolvePolygon(_polygons[i], insideOp);
            break;
        }
        case CompletelyOutside:
        {
            resolvePolygon(_polygons[i], outsideOp);
            break;
        }

            // The polygon can only be visible if it's part of the last brush that shares it's surface area, 
            // otherwise we'd get overlapping polygons if two brushes overlap.
            // When the (final) polygon is aligned with one of the cutting planes, we know it lies on the surface of
            // the CSG node we're cutting the polygons with. We also know that this node is not the node this polygon belongs to
            // because we've done that check earlier on. So we flag this polygon as being invisible.
        case PlaneAligned:
            // TODO: check this case
            //polygon.Visible = false;
            //aligned.Add(inputPolygon);
            break;
        case PlaneOppositeAligned:
            // TODO: check this case
            //polygon.Visible = false;
            //revAligned.Add(inputPolygon);
            break;

        case Split:
            break;
        default:
            break;
        }
    }
}

void CSG::Mesh::updateBounds()
{
    _bounds.Clear();

    int32 triangleIndices[3];
    for (int32 i = 0; i < _polygons.Count(); i++)
    {
        auto& polygon = _polygons[i];
        if (!polygon.Visible || polygon.FirstEdgeIndex == INVALID_INDEX)
            continue;

        HalfEdge iterator = _edges[polygon.FirstEdgeIndex];
        triangleIndices[0] = iterator.VertexIndex;
        iterator = _edges[iterator.NextIndex];
        triangleIndices[1] = iterator.VertexIndex;
        const HalfEdge& polygonFirst = _edges[polygon.FirstEdgeIndex];

        while (iterator != polygonFirst)
        {
            iterator = _edges.At(iterator.NextIndex);
            triangleIndices[2] = iterator.VertexIndex;

            // Validate triangle
            if (triangleIndices[0] != triangleIndices[1] && triangleIndices[0] != triangleIndices[2] && triangleIndices[1] != triangleIndices[2])
            {
                for (int32 q = 0; q < 3; q++)
                {
                    Vector3 pos = _vertices[triangleIndices[q]];
                    _bounds.Add(pos);
                }
            }

            triangleIndices[1] = triangleIndices[2];
        }
    }
}

void CSG::Mesh::doPolygonsOperation(bool isInverted, bool visibility)
{
    for (int32 i = 0; i < _polygons.Count(); i++)
    {
        if (_polygons[i].Inverted == isInverted)
            _polygons[i].Visible = visibility;
    }
}

#endif
