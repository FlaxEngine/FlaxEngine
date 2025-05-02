// Copyright (c) Wojciech Figat. All rights reserved.

#include "CSGMesh.h"

#if COMPILE_WITH_CSG_BUILDER

using namespace CSG;

void CSG::Mesh::resolvePolygon(Polygon& polygon, PolygonOperation op)
{
    switch (op)
    {
    case Keep:
        break;
    case Remove:
    {
        polygon.Visible = false;
        break;
    }
    case Flip:
    {
        polygon.Inverted = true;
        break;
    }
    default:
        break;
    }
}

void CSG::Mesh::edgeSplit(int32 edgeIndex, const Vector3& vertex)
{
    // Splits a half edge
    /*
    original:

    thisEdge
    *<======================
    ---------------------->*
    twin

    split:

    newEdge		thisEdge
    *<=========*<===========
    --------->*----------->*
    thisTwin	newTwin
    */

    HalfEdge& thisEdge = _edges[edgeIndex];
    auto thisTwinIndex = thisEdge.TwinIndex;
    auto& thisTwin = _edges[thisTwinIndex];
    auto thisEdgeIndex = thisTwin.TwinIndex;

    HalfEdge newEdge;
    auto newEdgeIndex = _edges.Count();

    HalfEdge newTwin;
    auto newTwinIndex = newEdgeIndex + 1;
    auto vertexIndex = _vertices.Count();

    newEdge.PolygonIndex = thisEdge.PolygonIndex;
    newTwin.PolygonIndex = thisTwin.PolygonIndex;

    newEdge.VertexIndex = thisEdge.VertexIndex;
    thisEdge.VertexIndex = vertexIndex;

    newTwin.VertexIndex = thisTwin.VertexIndex;
    thisTwin.VertexIndex = vertexIndex;

    newEdge.NextIndex = thisEdge.NextIndex;
    thisEdge.NextIndex = newEdgeIndex;

    newTwin.NextIndex = thisTwin.NextIndex;
    thisTwin.NextIndex = newTwinIndex;

    newEdge.TwinIndex = thisTwinIndex;
    thisTwin.TwinIndex = newEdgeIndex;

    thisEdge.TwinIndex = newTwinIndex;
    newTwin.TwinIndex = thisEdgeIndex;

    _edges.Add(newEdge);
    _edges.Add(newTwin);
    _vertices.Add(vertex);
}

PolygonSplitResult CSG::Mesh::polygonSplit(const Surface& cuttingPlane, int32 inputPolygonIndex, Polygon** outputPolygon)
{
    // Splits a polygon into two pieces, or categorizes it as outside, inside or aligned

    // Note: This method is not optimized! Code is simplified for clarity!
    //		  for example: Plane.Distance / Plane.OnSide should be inlined manually and shouldn't use enums, but floating point values directly!

    Polygon& inputPolygon = _polygons[inputPolygonIndex];
    int32 prev = inputPolygon.FirstEdgeIndex;
    int32 current = _edges[prev].NextIndex;
    int32 next = _edges[current].NextIndex;
    int32 last = next;
    int32 enterEdge = INVALID_INDEX;
    int32 exitEdge = INVALID_INDEX;

    auto prevVertex = _vertices[_edges[prev].VertexIndex];
    auto prevDistance = cuttingPlane.Distance(prevVertex); // distance to previous vertex
    auto prevSide = Surface::OnSide(prevDistance); // side of plane of previous vertex

    auto currentVertex = _vertices[_edges[current].VertexIndex];
    auto currentDistance = cuttingPlane.Distance(currentVertex); // distance to current vertex
    auto currentSide = Surface::OnSide(currentDistance); // side of plane of current vertex

    do
    {
        auto nextVertex = _vertices[_edges[next].VertexIndex];
        auto nextDistance = cuttingPlane.Distance(nextVertex); // distance to next vertex
        auto nextSide = Surface::OnSide(nextDistance); // side of plane of next vertex

        if (prevSide != currentSide) // check if edge crossed the plane ...
        {
            if (currentSide != PlaneIntersectionType::Intersecting) // prev:inside/outside - current:inside/outside - next:??
            {
                if (prevSide != PlaneIntersectionType::Intersecting) // prev:inside/outside - current:outside        - next:??
                {
                    // Calculate intersection of edge with plane split the edge into two, inserting the new vertex
                    auto newVertex = Surface::Intersection(prevVertex, currentVertex, prevDistance, currentDistance);
                    edgeSplit(current, newVertex);

                    if (prevSide == PlaneIntersectionType::Back) // prev:inside         - current:outside        - next:??
                    {
                        // edge01 exits:
                        //						
                        //      outside
                        //         1
                        //         *
                        // ......./........ intersect
                        //       /   
                        //      0     
                        //      inside

                        exitEdge = current;
                    }
                    else if (prevSide == PlaneIntersectionType::Front) // prev:outside - current:inside - next:??
                    {
                        // edge01 enters:
                        //
                        //      outside
                        //      0	 
                        //       \
                        // .......\........ intersect
                        //         *  
                        //         1   
                        //      inside

                        enterEdge = current;
                    }

                    prev = _edges[prev].NextIndex;
                    prevSide = PlaneIntersectionType::Intersecting;

                    if (exitEdge != INVALID_INDEX && enterEdge != INVALID_INDEX)
                        break;

                    current = _edges[prev].NextIndex;
                    currentVertex = _vertices[_edges[current].VertexIndex];

                    next = _edges[current].NextIndex;
                    nextVertex = _vertices[_edges[next].VertexIndex];
                }
            }
            else // prev:??                - current:intersects - next:??
            {
                if (prevSide == PlaneIntersectionType::Intersecting || // prev:intersects        - current:intersects - next:??
                    nextSide == PlaneIntersectionType::Intersecting || // prev:??                - current:intersects - next:intersects
                    prevSide == nextSide) // prev:inside/outde      - current:intersects - next:inside/outde
                {
                    if (prevSide == PlaneIntersectionType::Back || // prev:inside            - current:intersects - next:intersects/inside
                        nextSide == PlaneIntersectionType::Back) // prev:intersects/inside - current:intersects - next:inside
                    {
                        //      outside
                        // 0       1
                        // --------*....... intersect
                        //          \
                        //           2
                        //       inside
                        //
                        //      outside
                        //         1      2
                        // ........*------- intersect
                        //        / 
                        //       0
                        //      inside
                        //
                        //     outside
                        //        1
                        //........*....... intersect
                        //       / \
                        //      0   2
                        //      inside
                        //

                        prevSide = PlaneIntersectionType::Back;
                        enterEdge = exitEdge = INVALID_INDEX;
                        break;
                    }
                    else if (prevSide == PlaneIntersectionType::Front || // prev:outside            - current:intersects - next:intersects/outside
                        nextSide == PlaneIntersectionType::Front) // prev:intersects/outside - current:intersects - next:outside
                    {
                        //     outside
                        //          2
                        //         /
                        //..------*....... intersect
                        //  0     1
                        //     inside
                        //					 
                        //     outside
                        //      0
                        //       \
                        //........*------- intersect
                        //        1      2
                        //     inside
                        //
                        //     outside
                        //      0   2
                        //       \ /
                        //........*....... intersect
                        //        1
                        //     inside
                        //					 

                        prevSide = PlaneIntersectionType::Front;
                        enterEdge = exitEdge = INVALID_INDEX;
                        break;
                    }
                }
                else // prev:inside/outside - current:intersects - next:inside/outside
                {
                    if (prevSide == PlaneIntersectionType::Back) // prev:inside         - current:intersects - next:outside
                    {
                        // find exit edge:
                        //
                        //      outside
                        //           2
                        //        1 /
                        // ........*....... intersect
                        //        / 
                        //       0   
                        //       inside

                        exitEdge = current;
                        if (enterEdge != INVALID_INDEX)
                            break;
                    }
                    else // prev:outside        - current:intersects - next:inside
                    {
                        // find enter edge:
                        //
                        //      outside
                        //       0    
                        //        \ 1
                        // ........*....... intersect
                        //          \
                        //           2  
                        //       inside

                        enterEdge = current;
                        if (exitEdge != INVALID_INDEX)
                            break;
                    }
                }
            }
        }

        prev = current;
        current = next;
        next = _edges[next].NextIndex;

        prevDistance = currentDistance;
        currentDistance = nextDistance;
        prevSide = currentSide;
        currentSide = nextSide;
        prevVertex = currentVertex;
        currentVertex = nextVertex;
    } while (next != last);

    // We should never have only one edge crossing the plane
    ASSERT((enterEdge == INVALID_INDEX) == (exitEdge == INVALID_INDEX));

    // Check if we have an edge that exits and an edge that enters the plane and split the polygon into two if we do
    if (enterEdge != INVALID_INDEX && exitEdge != INVALID_INDEX)
    {
        // enter   .
        //        .
        //  =====>*----->
        //        .
        //
        // outside. inside
        //        .
        //  <-----*<=====
        //        .
        //        .  exit

        Polygon outsidePolygon;
        HalfEdge outsideEdge, insideEdge;

        auto outsidePolygonIndex = _polygons.Count();
        auto outsideEdgeIndex = _edges.Count();
        auto insideEdgeIndex = outsideEdgeIndex + 1;

        outsideEdge.TwinIndex = insideEdgeIndex;
        insideEdge.TwinIndex = outsideEdgeIndex;

        insideEdge.PolygonIndex = inputPolygonIndex;
        outsideEdge.PolygonIndex = outsidePolygonIndex;

        HalfEdge* exitEdgeP = &_edges[exitEdge];
        HalfEdge* enterEdgeP = &_edges[enterEdge];

        outsideEdge.VertexIndex = exitEdgeP->VertexIndex;
        insideEdge.VertexIndex = enterEdgeP->VertexIndex;

        outsideEdge.NextIndex = exitEdgeP->NextIndex;
        insideEdge.NextIndex = enterEdgeP->NextIndex;

        exitEdgeP->NextIndex = insideEdgeIndex;
        enterEdgeP->NextIndex = outsideEdgeIndex;

        outsidePolygon.FirstEdgeIndex = outsideEdgeIndex;
        inputPolygon.FirstEdgeIndex = insideEdgeIndex;

        outsidePolygon.Visible = inputPolygon.Visible;
        outsidePolygon.Inverted = inputPolygon.Inverted;
        outsidePolygon.SurfaceIndex = inputPolygon.SurfaceIndex;

        _edges.Add(outsideEdge);
        _edges.Add(insideEdge);

        // calculate the bounds of the polygons
        outsidePolygon.Bounds.Clear();
        auto first = _edges[outsidePolygon.FirstEdgeIndex];
        auto iterator = first;
        do
        {
            outsidePolygon.Bounds.Add(_vertices[iterator.VertexIndex]);
            iterator.PolygonIndex = outsidePolygonIndex;
            iterator = _edges[iterator.NextIndex];
        } while (iterator != first);

        inputPolygon.Bounds.Clear();
        first = _edges[inputPolygon.FirstEdgeIndex];
        iterator = first;
        do
        {
            inputPolygon.Bounds.Add(_vertices[iterator.VertexIndex]);
            iterator = _edges[iterator.NextIndex];
        } while (iterator != first);

        _polygons.Add(outsidePolygon);
        *outputPolygon = &_polygons[outsidePolygonIndex];

        return Split;
    }

    switch (prevSide)
    {
    case PlaneIntersectionType::Back:
        return CompletelyInside;
    case PlaneIntersectionType::Front:
        return CompletelyOutside;

    default:
    {
        auto polygonPlane = _surfaces[inputPolygon.SurfaceIndex];
        auto result = Vector3::Dot(polygonPlane.Normal, cuttingPlane.Normal);
        return result > 0 ? PlaneAligned : PlaneOppositeAligned;
    }
    }
}

#endif
