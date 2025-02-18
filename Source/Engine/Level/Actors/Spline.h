// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "Engine/Animations/Curve.h"

/// <summary>
/// Spline shape actor that defines spatial curve with utility functions for general purpose usage.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Other/Spline\"), ActorToolbox(\"Other\")")
class FLAXENGINE_API Spline : public Actor
{
    DECLARE_SCENE_OBJECT(Spline);
    typedef BezierCurveKeyframe<Transform> Keyframe;
private:
    bool _loop = false;
    BoundingBox _localBounds;

public:
    /// <summary>
    /// The spline bezier curve points represented as series of transformations in 3D space (with tangents). Points are stored in local-space of the actor.
    /// </summary>
    /// <remarks>Ensure to call UpdateSpline() after editing curve to reflect the changes.</remarks>
    BezierCurve<Transform> Curve;

    /// <summary>
    /// Whether to use spline as closed loop. In that case, ensure to place start and end at the same location.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(0), EditorDisplay(\"Spline\")")
    bool GetIsLoop() const;

    /// <summary>
    /// Whether to use spline as closed loop. In that case, ensure to place start and end at the same location.
    /// </summary>
    API_PROPERTY() void SetIsLoop(bool value);

public:
    /// <summary>
    /// Evaluates the spline curve at the given time and calculates the point location in 3D (world-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve point location (world-space).</returns>
    API_FUNCTION() Vector3 GetSplinePoint(float time) const;

    /// <summary>
    /// Evaluates the spline curve at the given time and calculates the point location in 3D (local-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve point location (local-space).</returns>
    API_FUNCTION() Vector3 GetSplineLocalPoint(float time) const;

    /// <summary>
    /// Evaluates the spline curve at the given time and calculates the point rotation in 3D (world-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve point rotation (world-space).</returns>
    API_FUNCTION() Quaternion GetSplineOrientation(float time) const;

    /// <summary>
    /// Evaluates the spline curve at the given time and calculates the point rotation in 3D (local-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve point rotation (local-space).</returns>
    API_FUNCTION() Quaternion GetSplineLocalOrientation(float time) const;

    /// <summary>
    /// Evaluates the spline curve at the given time and calculates the point scale in 3D (world-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve point scale (world-space).</returns>
    API_FUNCTION() Vector3 GetSplineScale(float time) const;

    /// <summary>
    /// Evaluates the spline curve at the given time and calculates the point scale in 3D (local-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve point scale (local-space).</returns>
    API_FUNCTION() Vector3 GetSplineLocalScale(float time) const;

    /// <summary>
    /// Evaluates the spline curve at the given time and calculates the transformation in 3D (world-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve point transformation (world-space).</returns>
    API_FUNCTION() Transform GetSplineTransform(float time) const;

    /// <summary>
    /// Evaluates the spline curve at the given time and calculates the transformation in 3D (local-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve point transformation (local-space).</returns>
    API_FUNCTION() Transform GetSplineLocalTransform(float time) const;

    /// <summary>
    /// Evaluates the spline curve direction (forward vector, aka position 1st derivative) at the given time in 3D (world-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve direction (world-space).</returns>
    API_FUNCTION() Vector3 GetSplineDirection(float time) const;

    /// <summary>
    /// Evaluates the spline curve direction (forward vector, aka position 1st derivative) at the given time in 3D (local-space).
    /// </summary>
    /// <param name="time">The time value. Can be negative or larger than curve length (curve will loop or clamp).</param>
    /// <returns>The calculated curve direction (local-space).</returns>
    API_FUNCTION() Vector3 GetSplineLocalDirection(float time) const;

    /// <summary>
    /// Evaluates the spline curve at the given index (world-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <returns>The curve point location (world-space).</returns>
    API_FUNCTION() Vector3 GetSplinePoint(int32 index) const;

    /// <summary>
    /// Evaluates the spline curve at the given index (local-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <returns>The curve point location (local-space).</returns>
    API_FUNCTION() Vector3 GetSplineLocalPoint(int32 index) const;

    /// <summary>
    /// Evaluates the spline curve at the given index (world-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <returns>The curve point transformation (world-space).</returns>
    API_FUNCTION() Transform GetSplineTransform(int32 index) const;

    /// <summary>
    /// Evaluates the spline curve at the given index (local-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <returns>The curve point transformation (local-space).</returns>
    API_FUNCTION() Transform GetSplineLocalTransform(int32 index) const;

    /// <summary>
    /// Gets the spline curve point tangent at the given index (world-space).
    /// </summary>
    /// <remarks>Tangents are stored relative to the curve point but this methods converts them to be in world-space.</remarks>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="isIn">True if get arrive tangent, otherwise gets leave tangent (in or out).</param>
    /// <returns>The transformation of the tangent to set (world-space).</returns>
    API_FUNCTION() Transform GetSplineTangent(int32 index, bool isIn);

    /// <summary>
    /// Gets the spline curve point tangent at the given index (local-space).
    /// </summary>
    /// <remarks>Tangents are stored relative to the curve point but this methods converts them to be in local-space of the actor.</remarks>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="isIn">True if get arrive tangent, otherwise gets leave tangent (in or out).</param>
    /// <returns>The transformation of the tangent to set (world-space).</returns>
    API_FUNCTION() Transform GetSplineLocalTangent(int32 index, bool isIn);

    /// <summary>
    /// Gets the amount of points in the spline.
    /// </summary>
    API_PROPERTY() int32 GetSplinePointsCount() const;

    /// <summary>
    /// Gets the total duration of the spline curve (time of the last point).
    /// </summary>
    API_PROPERTY() float GetSplineDuration() const;

    /// <summary>
    /// Gets the total length of the spline curve (distance between all the points).
    /// </summary>
    API_PROPERTY() float GetSplineLength() const;

    /// <summary>
    /// Gets the length of the spline segment (distance between pair of two points).
    /// </summary>
    /// <param name="index">The index of the segment end index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <returns>The spline segment length.</returns>
    API_FUNCTION() float GetSplineSegmentLength(int32 index) const;

    /// <summary>
    /// Gets the time of the spline keyframe.
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <returns>The spline time.</returns>
    API_FUNCTION() float GetSplineTime(int32 index) const;

    /// <summary>
    /// Calculates the closest point to the given location and returns the spline time at that point.
    /// </summary>
    /// <param name="point">The point in world-space to find the spline point that is closest to it.</param>
    /// <returns>The spline time.</returns>
    API_FUNCTION() float GetSplineTimeClosestToPoint(const Vector3& point) const;

    /// <summary>
    /// Calculates the closest point to the given location.
    /// </summary>
    /// <param name="point">The point in world-space to find the spline point that is closest to it.</param>
    /// <returns>The spline position.</returns>
    API_FUNCTION() Vector3 GetSplinePointClosestToPoint(const Vector3& point) const;

    /// <summary>
    /// Gets the spline curve points list (world-space).
    /// </summary>
    /// <param name="points">The result points collection.</param>
    API_FUNCTION() void GetSplinePoints(API_PARAM(Out) Array<Vector3>& points) const;

    /// <summary>
    /// Gets the spline curve points list (local-space).
    /// </summary>
    /// <param name="points">The result points collection.</param>
    API_FUNCTION() void GetSplineLocalPoints(API_PARAM(Out) Array<Vector3>& points) const;

    /// <summary>
    /// Gets the spline curve points list (world-space).
    /// </summary>
    /// <param name="points">The result points collection.</param>
    API_FUNCTION() void GetSplinePoints(API_PARAM(Out) Array<Transform>& points) const;

    /// <summary>
    /// Gets the spline curve points list (local-space).
    /// </summary>
    /// <param name="points">The result points collection.</param>
    API_FUNCTION() void GetSplineLocalPoints(API_PARAM(Out) Array<Transform>& points) const;

public:
    /// <summary>
    /// Clears the spline to be empty.
    /// </summary>
    API_FUNCTION() void ClearSpline();

    /// <summary>
    /// Removes the spline curve point at the given index.
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="updateSpline">True if update spline after removing the point, otherwise false.</param>
    API_FUNCTION() void RemoveSplinePoint(int32 index, bool updateSpline = true);

    /// <summary>
    /// Sets the spline curve at the given index (world-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="point">The location of the point to set (world-space).</param>
    /// <param name="updateSpline">True if update spline after editing the point, otherwise false.</param>
    API_FUNCTION() void SetSplinePoint(int32 index, const Vector3& point, bool updateSpline = true);

    /// <summary>
    /// Sets the spline curve at the given index (local-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="point">The location of the point to set (local-space).</param>
    /// <param name="updateSpline">True if update spline after editing the point, otherwise false.</param>
    API_FUNCTION() void SetSplineLocalPoint(int32 index, const Vector3& point, bool updateSpline = true);

    /// <summary>
    /// Sets the spline curve at the given index (world-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="point">The transformation of the point to set (world-space).</param>
    /// <param name="updateSpline">True if update spline after editing the point, otherwise false.</param>
    API_FUNCTION() void SetSplineTransform(int32 index, const Transform& point, bool updateSpline = true);

    /// <summary>
    /// Sets the spline curve at the given index (local-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="point">The transformation of the point to set (local-space).</param>
    /// <param name="updateSpline">True if update spline after editing the point, otherwise false.</param>
    API_FUNCTION() void SetSplineLocalTransform(int32 index, const Transform& point, bool updateSpline = true);

    /// <summary>
    /// Sets the spline curve point tangent at the given index (world-space).
    /// </summary>
    /// <remarks>Tangents are stored relative to the curve point but this methods converts them to be in world-space.</remarks>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="point">The transformation of the tangent to set (world-space).</param>
    /// <param name="isIn">True if set arrive tangent, otherwise sets leave tangent (in or out).</param>
    /// <param name="updateSpline">True if update spline after editing the point, otherwise false.</param>
    API_FUNCTION() void SetSplineTangent(int32 index, const Transform& point, bool isIn, bool updateSpline = true);

    /// <summary>
    /// Sets the spline curve point tangent at the given index (local-space).
    /// </summary>
    /// <remarks>Tangents are stored relative to the curve point but this methods converts them to be in local-space of the actor.</remarks>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="point">The transformation of the tangent to set (local-space).</param>
    /// <param name="isIn">True if set arrive tangent, otherwise sets leave tangent (in or out).</param>
    /// <param name="updateSpline">True if update spline after editing the point, otherwise false.</param>
    API_FUNCTION() void SetSplineLocalTangent(int32 index, const Transform& point, bool isIn, bool updateSpline = true);

    /// <summary>
    /// Sets the spline curve point time at the given index (world-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="time">The time to set.</param>
    /// <param name="updateSpline">True if update spline after editing the point, otherwise false.</param>
    API_FUNCTION() void SetSplinePointTime(int32 index, float time, bool updateSpline = true);

    /// <summary>
    /// Adds the point to the spline curve (at the end).
    /// </summary>
    /// <param name="point">The location of the point to add to the curve (world-space).</param>
    /// <param name="updateSpline">True if update spline after editing the point, otherwise false.</param>
    API_FUNCTION() void AddSplinePoint(const Vector3& point, bool updateSpline = true);

    /// <summary>
    /// Adds the point to the spline curve (at the end).
    /// </summary>
    /// <param name="point">The location of the point to add to the curve (local-space).</param>
    /// <param name="updateSpline">True if update spline after adding the point, otherwise false.</param>
    API_FUNCTION() void AddSplineLocalPoint(const Vector3& point, bool updateSpline = true);

    /// <summary>
    /// Adds the point to the spline curve (at the end).
    /// </summary>
    /// <param name="point">The transformation of the point to add to the curve (world-space).</param>
    /// <param name="updateSpline">True if update spline after adding the point, otherwise false.</param>
    API_FUNCTION() void AddSplinePoint(const Transform& point, bool updateSpline = true);

    /// <summary>
    /// Adds the point to the spline curve (at the end).
    /// </summary>
    /// <param name="point">The transformation of the point to add to the curve (local-space).</param>
    /// <param name="updateSpline">True if update spline after adding the point, otherwise false.</param>
    API_FUNCTION() void AddSplineLocalPoint(const Transform& point, bool updateSpline = true);

    /// <summary>
    /// Inserts the spline curve point at the given index (world-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="time">The time value.</param>
    /// <param name="point">The location of the point to add to the curve (world-space).</param>
    /// <param name="updateSpline">True if update spline after removing the point, otherwise false.</param>
    API_FUNCTION() void InsertSplinePoint(int32 index, float time, const Transform& point, bool updateSpline = true);

    /// <summary>
    /// Inserts the spline curve point at the given index (local-space).
    /// </summary>
    /// <param name="index">The curve keyframe index. Zero-based, smaller than GetSplinePointsCount().</param>
    /// <param name="time">The time value.</param>
    /// <param name="point">The location of the point to add to the curve (local-space).</param>
    /// <param name="updateSpline">True if update spline after removing the point, otherwise false.</param>
    API_FUNCTION() void InsertSplineLocalPoint(int32 index, float time, const Transform& point, bool updateSpline = true);

    /// <summary>
    /// Updates the curve tangent points to make curve linear.
    /// </summary>
    API_FUNCTION() void SetTangentsLinear();

    /// <summary>
    /// Updates the curve tangent points to make curve smooth.
    /// </summary>
    API_FUNCTION() void SetTangentsSmooth();

public:
    /// <summary>
    /// Called when spline gets updated (eg. after curve modification).
    /// </summary>
    API_EVENT() Action SplineUpdated;

    /// <summary>
    /// Updates the spline after it was modified. Recreates the collision and/or any cached state that depends on the spline type.
    /// </summary>
    API_FUNCTION() virtual void UpdateSpline();

protected:
#if USE_EDITOR
    // Spline color getter for debug drawing, can be overriden by custom spline types.
    virtual Color GetSplineColor()
    {
        return Color::White;
    }
#endif

private:
    // Internal bindings
#if !COMPILE_WITHOUT_CSHARP
    API_FUNCTION(NoProxy) void GetKeyframes(MArray* data);
    API_FUNCTION(NoProxy) void SetKeyframes(MArray* data, int32 keySize);
#endif

public:
    // [Actor]
#if USE_EDITOR
    void OnDebugDraw() override;
    void OnDebugDrawSelected() override;
#endif
    void OnTransformChanged() override;
    void Initialize() override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
