// Copyright (c) Wojciech Figat. All rights reserved.
// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel

#pragma once

#include "Vector2.h"

struct Matrix;
struct Rectangle;

/// <summary>
/// Describes the viewport dimensions.
/// </summary>
API_STRUCT(InBuild) struct FLAXENGINE_API Viewport
{
public:
    union
    {
        struct
        {
            // Position of the pixel coordinate of the upper-left corner of the viewport.
            float X;

            // Position of the pixel coordinate of the upper-left corner of the viewport.
            float Y;
        };

        // Upper left corner location.
        Float2 Location;
    };

    union
    {
        struct
        {
            // Width dimension of the viewport.
            float Width;

            // Height dimension of the viewport.
            float Height;
        };

        // Size
        Float2 Size;
    };

    // Minimum depth of the clip volume.
    float MinDepth;

    // Maximum depth of the clip volume.
    float MaxDepth;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Viewport() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Viewport"/> struct.
    /// </summary>
    /// <param name="x">The x coordinate of the upper-left corner of the viewport in pixels.</param>
    /// <param name="y">The y coordinate of the upper-left corner of the viewport in pixels.</param>
    /// <param name="width">The width of the viewport in pixels.</param>
    /// <param name="height">The height of the viewport in pixels.</param>
    /// <param name="minDepth">The minimum depth of the clip volume.</param>
    /// <param name="maxDepth">The maximum depth of the clip volumes.</param>
    Viewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f)
        : X(x)
        , Y(y)
        , Width(width)
        , Height(height)
        , MinDepth(minDepth)
        , MaxDepth(maxDepth)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Viewport"/> struct.
    /// </summary>
    /// <param name="size">The viewport size.</param>
    explicit Viewport(const Float2& size)
        : X(0)
        , Y(0)
        , Width(size.X)
        , Height(size.Y)
        , MinDepth(0.0f)
        , MaxDepth(1.0f)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Viewport"/> struct.
    /// </summary>
    /// <param name="bounds">A bounding rectangle that defines the location and size of the viewport in a render target.</param>
    Viewport(const Rectangle& bounds);

public:
    String ToString() const;

public:
    // Gets the aspect ratio used by the viewport.
    float GetAspectRatio() const
    {
        return Height != 0.0f ? Width / Height : 0.0f;
    }

    // Gets the size of the viewport.
    Rectangle GetBounds() const;

    // Sets the size of the viewport.
    void SetBounds(const Rectangle& bounds);

public:
    bool operator==(const Viewport& other) const
    {
        return X == other.X && Y == other.Y && Width == other.Width && Height == other.Height && MinDepth == other.MinDepth && MaxDepth == other.MaxDepth;
    }

    bool operator!=(const Viewport& other) const
    {
        return X != other.X || Y != other.Y || Width != other.Width || Height != other.Height || MinDepth != other.MinDepth || MaxDepth != other.MaxDepth;
    }

public:
    /// <summary>
    /// Projects a 3D vector from object space into screen space.
    /// </summary>
    /// <param name="source">The vector to project.</param>
    /// <param name="vp">A combined World*View*Projection matrix.</param>
    /// <param name="result">The projected vector.</param>
    void Project(const Vector3& source, const Matrix& vp, Vector3& result) const;

    /// <summary>
    /// Converts a screen space point into a corresponding point in world space.
    /// </summary>
    /// <param name="source">The vector to un-project.</param>
    /// <param name="ivp">An inverted combined World*View*Projection matrix.</param>
    /// <param name="result">The un-projected vector</param>
    void Unproject(const Vector3& source, const Matrix& ivp, Vector3& result) const;
};

template<>
struct TIsPODType<Viewport>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Viewport, "X:{0} Y:{1} Width:{2} Height:{3} MinDepth:{4} MaxDeth:{5}", v.X, v.Y, v.Width, v.Height, v.MinDepth, v.MaxDepth);
