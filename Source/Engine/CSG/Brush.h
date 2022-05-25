// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Math/Plane.h"
#include "Engine/Core/Math/AABB.h"
#include "Engine/Core/Math/Vector2.h"

class Scene;

namespace CSG
{
    /// <summary>
    /// CSG plane surface
    /// </summary>
    struct Surface : Plane
    {
        Guid Material;
        Vector2 TexCoordScale;
        Vector2 TexCoordOffset;
        float TexCoordRotation;
        float ScaleInLightmap;

    public:

        /// <summary>
        /// Default constructor
        /// </summary>
        Surface()
            : Plane()
            , TexCoordScale(1)
            , TexCoordOffset(0.0f)
            , TexCoordRotation(0)
            , ScaleInLightmap(1)
        {
        }

        Surface(const Plane& plane)
            : Plane(plane)
            , Material(Guid::Empty)
            , TexCoordScale(1)
            , TexCoordOffset(0.0f)
            , TexCoordRotation(0)
            , ScaleInLightmap(1)
        {
        }

        Surface(const Surface& plane)
            : Plane(plane)
            , Material(plane.Material)
            , TexCoordScale(1)
            , TexCoordOffset(0.0f)
            , TexCoordRotation(0)
            , ScaleInLightmap(1)
        {
        }

        Surface(const Vector3& normal, float d)
            : Plane(normal, d)
            , Material(Guid::Empty)
            , TexCoordScale(1)
            , TexCoordOffset(0.0f)
            , TexCoordRotation(0)
            , ScaleInLightmap(1)
        {
        }

        Surface(const Vector3& point1, const Vector3& point2, const Vector3& point3)
            : Plane(point1, point2, point3)
            , Material(Guid::Empty)
            , TexCoordScale(1)
            , TexCoordOffset(0.0f)
            , TexCoordRotation(0)
            , ScaleInLightmap(1)
        {
        }

    public:

        static Vector3 Intersection(const Vector3& start, const Vector3& end, float sdist, float edist)
        {
            Vector3 vector = end - start;
            float length = edist - sdist;
            float delta = edist / length;

            return end - (delta * vector);
        }

        Vector3 Intersection(const Vector3& start, const Vector3& end) const
        {
            return Intersection(start, end, Distance(start), Distance(end));
        }

        float Distance(float x, float y, float z) const
        {
            return
            (
                (Normal.X * x) +
                (Normal.Y * y) +
                (Normal.Z * z) -
                (D)
            );
        }

        float Distance(const Vector3& vertex) const
        {
            return
            (
                (Normal.X * vertex.X) +
                (Normal.Y * vertex.Y) +
                (Normal.Z * vertex.Z) -
                (D)
            );
        }

        static PlaneIntersectionType OnSide(float distance)
        {
            if (distance > DistanceEpsilon)
                return PlaneIntersectionType::Front;
            if (distance < -DistanceEpsilon)
                return PlaneIntersectionType::Back;
            return PlaneIntersectionType::Intersecting;
        }

        PlaneIntersectionType OnSide(float x, float y, float z) const
        {
            return OnSide(Distance(x, y, z));
        }

        PlaneIntersectionType OnSide(const Vector3& vertex) const
        {
            return OnSide(Distance(vertex));
        }

        PlaneIntersectionType OnSide(const AABB& box) const
        {
            Vector3 min;
            Vector3 max;

            max.X = static_cast<float>((Normal.X >= 0.0f) ? box.MinX : box.MaxX);
            max.Y = static_cast<float>((Normal.Y >= 0.0f) ? box.MinY : box.MaxY);
            max.Z = static_cast<float>((Normal.Z >= 0.0f) ? box.MinZ : box.MaxZ);

            min.X = static_cast<float>((Normal.X >= 0.0f) ? box.MaxX : box.MinX);
            min.Y = static_cast<float>((Normal.Y >= 0.0f) ? box.MaxY : box.MinY);
            min.Z = static_cast<float>((Normal.Z >= 0.0f) ? box.MaxZ : box.MinZ);

            float distance = Vector3::Dot(Normal, max) - D;

            if (distance > DistanceEpsilon)
                return PlaneIntersectionType::Front;

            distance = Vector3::Dot(Normal, min) - D;

            if (distance < -DistanceEpsilon)
                return PlaneIntersectionType::Back;

            return PlaneIntersectionType::Intersecting;
        }
    };

    /// <summary>
    /// CSG brush object
    /// </summary>
    class FLAXENGINE_API Brush
    {
    public:

        /// <summary>
        /// Gets the scene.
        /// </summary>
        /// <returns>Scene</returns>
        virtual Scene* GetBrushScene() const = 0;

        /// <summary>
        /// Returns true if brush affects world
        /// </summary>
        /// <returns>True if use CSG brush during level geometry building</returns>
        virtual bool CanUseCSG() const
        {
            return true;
        }

        /// <summary>
        /// Gets the CSG brush object ID.
        /// </summary>
        /// <returns>The unique ID.</returns>
        virtual Guid GetBrushID() const = 0;

        /// <summary>
        /// Gets CSG brush mode
        /// </summary>
        /// <returns>Mode</returns>
        virtual Mode GetBrushMode() const = 0;

        /// <summary>
        /// Gets brush surfaces
        /// </summary>
        /// <param name="surfaces">Result list of surfaces</param>
        virtual void GetSurfaces(Array<Surface, HeapAllocation>& surfaces) = 0;

        /// <summary>
        /// Gets brush surfaces amount
        /// </summary>
        /// <returns>The mount of the brush surfaces.</returns>
        virtual int32 GetSurfacesCount() = 0;

    public:

        /// <summary>
        /// Called when brush data gets modified (will request rebuilding in Editor).
        /// </summary>
        virtual void OnBrushModified();
    };
};
