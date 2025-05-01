// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

namespace CSG
{
    /// <summary>
    /// Half edge structure
    /// </summary>
    struct HalfEdge
    {
        //           ^
        //           |       polygon
        // next      |
        // half-edge |
        //           |       half-edge
        // vertex	 *<====================== 
        //           ---------------------->*
        //				  twin-half-edge

        int32 NextIndex;
        int32 TwinIndex;
        int32 VertexIndex;
        int32 PolygonIndex;

        // TODO: remove this code for faster performance
        HalfEdge()
        {
            NextIndex = 0;
            TwinIndex = 0;
            VertexIndex = 0;
            PolygonIndex = 0;
        }

        bool operator==(const HalfEdge& b) const
        {
            return NextIndex == b.NextIndex
                    && TwinIndex == b.TwinIndex
                    && VertexIndex == b.VertexIndex
                    && PolygonIndex == b.PolygonIndex;
        }

        bool operator!=(const HalfEdge& b) const
        {
            return NextIndex != b.NextIndex
                    || TwinIndex != b.TwinIndex
                    || VertexIndex != b.VertexIndex
                    || PolygonIndex != b.PolygonIndex;
        }
    };
};
