// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/AABB.h"
#include "Engine/Core/Collections/Array.h"
#include "Brush.h"
#include "HalfEdge.h"
#include "Polygon.h"
#include "Engine/Graphics/Models/Types.h"

#define CSG_MESH_UV_SCALE (1.0f / 100.0f)

#if COMPILE_WITH_CSG_BUILDER

namespace CSG
{
    class RawData;

    enum PolygonOperation
    {
        Keep,
        Remove,
        Flip
    };

    /// <summary>
    /// CSG mesh object
    /// </summary>
    class Mesh
    {
    private:

        struct BrushMeta
        {
            Mode Mode;
            int32 StartSurfaceIndex;
            int32 SurfacesCount;
            AABB Bounds;
            Brush* Parent;
        };

    private:

        AABB _bounds;
        Array<Polygon> _polygons;
        Array<HalfEdge> _edges;
        Array<Vector3> _vertices;
        Array<Surface> _surfaces;
        Array<BrushMeta> _brushesMeta;

    public:

        /// <summary>
        /// Mesh index (assigned by the builder)
        /// </summary>
        int32 Index = 0;

        /// <summary>
        /// Gets mesh bounds area
        /// </summary>
        /// <returns>Mesh bounds</returns>
        AABB GetBounds() const
        {
            return _bounds;
        }

        /// <summary>
        /// Check if any of this brush sub-brushes is on given type
        /// </summary>
        /// <returns>True if brush is using given mode</returns>
        bool HasMode(Mode mode) const;

        /// <summary>
        /// Gets array with polygons
        /// </summary>
        /// <returns>Polygon</returns>
        const Array<Polygon>* GetPolygons() const
        {
            return &_polygons;
        }

        /// <summary>
        /// Gets array with surfaces
        /// </summary>
        /// <returns>Surfaces</returns>
        const Array<Surface>* GetSurfaces() const
        {
            return &_surfaces;
        }

        /// <summary>
        /// Gets array with edges
        /// </summary>
        /// <returns>Half edges</returns>
        const Array<HalfEdge>* GetEdges() const
        {
            return &_edges;
        }

        /// <summary>
        /// Gets array with vertices
        /// </summary>
        /// <returns>Vertices</returns>
        const Array<Vector3>* GetVertices() const
        {
            return &_vertices;
        }

    public:

        /// <summary>
        /// Build mesh from brush
        /// </summary>
        /// <param name="parentBrush">Parent brush to use</param>
        void Build(Brush* parentBrush);

        /// <summary>
        /// Triangulate mesh
        /// </summary>
        /// <param name="data">Result data</param>
        /// <param name="cacheVB">Cache data</param>
        /// <returns>True if cannot generate valid mesh data (due to missing triangles etc.)</returns>
        bool Triangulate(RawData& data, Array<MeshVertex>& cacheVB) const;

        /// <summary>
        /// Perform CSG operation with another mesh
        /// </summary>
        /// <param name="other">Other mesh data to process</param>
        void PerformOperation(Mesh* other);

        /// <summary>
        /// Add other mesh data
        /// </summary>
        /// <param name="other">Other mesh to merge with</param>
        void Add(const Mesh* other);

    private:

        void intersect(const Mesh* other, PolygonOperation insideOp, PolygonOperation outsideOp);
        void intersectSubMesh(const Mesh* other, int32 subMeshIndex, PolygonOperation insideOp, PolygonOperation outsideOp);
        void updateBounds();
        static void resolvePolygon(Polygon& polygon, PolygonOperation op);
        void edgeSplit(int32 edgeIndex, const Vector3& vertex);
        PolygonSplitResult polygonSplit(const Surface& cuttingPlane, int32 inputPolygonIndex, Polygon** outputPolygon);
        void doPolygonsOperation(bool isInverted, bool visibility);
    };

    typedef Array<Mesh*> MeshesArray;
};

#endif
