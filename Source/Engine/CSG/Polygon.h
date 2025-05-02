// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/AABB.h"

namespace CSG
{
    enum PolygonSplitResult
    {
        /// <summary>
        /// Polygon is completely inside half-space defined by plane
        /// </summary>
        CompletelyInside,

        /// <summary>
        /// Polygon is completely outside half-space defined by plane
        /// </summary>
        CompletelyOutside,

        /// <summary>
        /// Polygon has been split into two parts by plane
        /// </summary>
        Split,

        /// <summary>
        /// Polygon is aligned with cutting plane and the polygons' normal points in the same direction
        /// </summary>
        PlaneAligned,

        /// <summary>
        /// Polygon is aligned with cutting plane and the polygons' normal points in the opposite direction
        /// </summary>
        PlaneOppositeAligned
    };

    /// <summary>
    /// Polygon structure
    /// </summary>
    struct Polygon
    {
        int32 FirstEdgeIndex;
        int32 SurfaceIndex;
        bool Visible;
        bool Inverted;
        AABB Bounds;

        Polygon()
        {
            // TODO: remove this constructor to boost performance??
            FirstEdgeIndex = INVALID_INDEX;
            SurfaceIndex = INVALID_INDEX;
            Inverted = false;
            Visible = false;
            Bounds = AABB();
        }
    };
};
