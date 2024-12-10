// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Helper class with Delaunay triangulation algorithm implementation (2D space).
/// </summary>
API_CLASS(Internal, Static, Namespace="FlaxEngine.Utilities") class Delaunay2D
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Delaunay2D);
public:
    struct Triangle
    {
        int32 Indices[3];
        bool IsBad;

        Triangle()
        {
            IsBad = false;
        }

        Triangle(int32 a, int32 b, int32 c)
        {
            Indices[0] = a;
            Indices[1] = b;
            Indices[2] = c;
            IsBad = false;
        }
    };

    /// <summary>
    /// Triangulates input vertices array into the list of triangle vertices.
    /// </summary>
    /// <param name="vertices">Input list of vertices.</param>
    /// <returns>Result list of triangles. Each triangle is made out of sequence of 3 vertices. Empty if no valid triangle built.</returns>
    API_FUNCTION() static Array<Float2> Triangulate(const Array<Float2>& vertices)
    {
        Array<Triangle> triangles;
        Triangulate(vertices, triangles);
        Array<Float2> result;
        result.Resize(triangles.Count() * 3);
        int32 c = 0;
        for (const Triangle& t : triangles)
        {
            result.Get()[c++] = vertices[t.Indices[0]];
            result.Get()[c++] = vertices[t.Indices[1]];
            result.Get()[c++] = vertices[t.Indices[2]];
        }
        return result;
    }

    template<typename TrianglesArray = Triangle>
    static void Triangulate(const Array<Float2>& vertices, TrianglesArray& triangles)
    {
        // Skip if no change to produce any triangles
         if (vertices.Count() < 3)
            return;

        Array<Float2> points = vertices;
        Array<Edge> polygon;

        Rectangle rect;
        rect.Location = vertices[0];
        rect.Size = Float2::Zero;
        for (int32 i = 1; i < vertices.Count(); i++)
        {
            rect = Rectangle::Union(rect, vertices[i]);
        }

        const float deltaMax = Math::Max(rect.GetWidth(), rect.GetHeight());
        const Float2 center = rect.GetCenter();

        points.Add(Float2(center.X - 20 * deltaMax, center.Y - deltaMax));
        points.Add(Float2(center.X, center.Y + 20 * deltaMax));
        points.Add(Float2(center.X + 20 * deltaMax, center.Y - deltaMax));

        triangles.Add(Triangle(vertices.Count() + 0, vertices.Count() + 1, vertices.Count() + 2));

        for (int32 i = 0; i < vertices.Count(); i++)
        {
            polygon.Clear();

            for (int32 j = 0; j < triangles.Count(); j++)
            {
                if (CircumCircleContains(points, triangles[j], i))
                {
                    triangles[j].IsBad = true;
                    polygon.Add(Edge(triangles[j].Indices[0], triangles[j].Indices[1]));
                    polygon.Add(Edge(triangles[j].Indices[1], triangles[j].Indices[2]));
                    polygon.Add(Edge(triangles[j].Indices[2], triangles[j].Indices[0]));
                }
            }

            for (int32 j = 0; j < triangles.Count(); j++)
            {
                if (triangles[j].IsBad)
                {
                    triangles.RemoveAt(j);
                    j--;
                }
            }

            for (int32 j = 0; j < polygon.Count(); j++)
            {
                for (int32 k = j + 1; k < polygon.Count(); k++)
                {
                    if (EdgeCompare(points, polygon[j], polygon[k]))
                    {
                        polygon[j].IsBad = true;
                        polygon[k].IsBad = true;
                    }
                }
            }

            for (int32 j = 0; j < polygon.Count(); j++)
            {
                if (polygon[j].IsBad)
                {
                    continue;
                }
                triangles.Add(Triangle(polygon[j].Indices[0], polygon[j].Indices[1], i));
            }
        }

        for (int32 i = 0; i < triangles.Count(); i++)
        {
            bool invalid = false;
            for (int32 j = 0; j < 3; j++)
            {
                if (triangles[i].Indices[j] >= vertices.Count())
                {
                    invalid = true;
                    break;
                }
            }
            if (invalid)
            {
                triangles.RemoveAt(i);
                i--;
            }
        }
    }

private:
    struct Edge
    {
        int32 Indices[2];
        bool IsBad;

        Edge()
        {
            IsBad = false;
        }

        Edge(int32 a, int32 b)
        {
            IsBad = false;
            Indices[0] = a;
            Indices[1] = b;
        }
    };

    static bool CircumCircleContains(const Array<Float2>& vertices, const Triangle& triangle, int32 vertexIndex)
    {
        Float2 p1 = vertices[triangle.Indices[0]];
        Float2 p2 = vertices[triangle.Indices[1]];
        Float2 p3 = vertices[triangle.Indices[2]];

        float ab = p1.X * p1.X + p1.Y * p1.Y;
        float cd = p2.X * p2.X + p2.Y * p2.Y;
        float ef = p3.X * p3.X + p3.Y * p3.Y;

        Float2 circum(
            (ab * (p3.Y - p2.Y) + cd * (p1.Y - p3.Y) + ef * (p2.Y - p1.Y)) / (p1.X * (p3.Y - p2.Y) + p2.X * (p1.Y - p3.Y) + p3.X * (p2.Y - p1.Y)),
            (ab * (p3.X - p2.X) + cd * (p1.X - p3.X) + ef * (p2.X - p1.X)) / (p1.Y * (p3.X - p2.X) + p2.Y * (p1.X - p3.X) + p3.Y * (p2.X - p1.X)));

        circum *= 0.5f;
        float r = Float2::DistanceSquared(p1, circum);
        float d = Float2::DistanceSquared(vertices[vertexIndex], circum);
        return d <= r;
    }

    static bool EdgeCompare(const Array<Float2>& vertices, const Edge& a, const Edge& b)
    {
        if (Float2::Distance(vertices[a.Indices[0]], vertices[b.Indices[0]]) < ZeroTolerance && 
            Float2::Distance(vertices[a.Indices[1]], vertices[b.Indices[1]]) < ZeroTolerance)
            return true;
        if (Float2::Distance(vertices[a.Indices[0]], vertices[b.Indices[1]]) < ZeroTolerance && 
            Float2::Distance(vertices[a.Indices[1]], vertices[b.Indices[0]]) < ZeroTolerance)
            return true;
        return false;
    }
};
