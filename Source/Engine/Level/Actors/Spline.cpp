// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Spline.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Animations/CurveSerialization.h"
#include <ThirdParty/mono-2.0/mono/metadata/object.h>

Spline::Spline(const SpawnParams& params)
    : Actor(params)
{
}

void Spline::SetIsLoop(bool value)
{
    if (_loop != value)
    {
        _loop = value;
        UpdateSpline();
    }
}

Vector3 Spline::GetSplinePoint(float time) const
{
    Transform t;
    Curve.Evaluate(t, time, _loop);
    return _transform.LocalToWorld(t.Translation);
}

Vector3 Spline::GetSplineLocalPoint(float time) const
{
    Transform t;
    Curve.Evaluate(t, time, _loop);
    return t.Translation;
}

Quaternion Spline::GetSplineOrientation(float time) const
{
    Transform t;
    Curve.Evaluate(t, time, _loop);
    Quaternion::Multiply(_transform.Orientation, t.Orientation, t.Orientation);
    t.Orientation.Normalize();
    return t.Orientation;
}

Quaternion Spline::GetSplineLocalOrientation(float time) const
{
    Transform t;
    Curve.Evaluate(t, time, _loop);
    return t.Orientation;
}

Vector3 Spline::GetSplineScale(float time) const
{
    Transform t;
    Curve.Evaluate(t, time, _loop);
    return _transform.Scale * t.Scale;
}

Vector3 Spline::GetSplineLocalScale(float time) const
{
    Transform t;
    Curve.Evaluate(t, time, _loop);
    return t.Scale;
}

Transform Spline::GetSplineTransform(float time) const
{
    Transform t;
    Curve.Evaluate(t, time, _loop);
    return _transform.LocalToWorld(t);
}

Transform Spline::GetSplineLocalTransform(float time) const
{
    Transform t;
    Curve.Evaluate(t, time, _loop);
    return t;
}

Vector3 Spline::GetSplinePoint(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < GetSplinePointsCount(), Vector3::Zero)
    return _transform.LocalToWorld(Curve[index].Value.Translation);
}

Vector3 Spline::GetSplineLocalPoint(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < GetSplinePointsCount(), Vector3::Zero)
    return Curve[index].Value.Translation;
}

Transform Spline::GetSplineTransform(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < GetSplinePointsCount(), Vector3::Zero)
    return _transform.LocalToWorld(Curve[index].Value);
}

Transform Spline::GetSplineLocalTransform(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < GetSplinePointsCount(), Vector3::Zero)
    return Curve[index].Value;
}

int32 Spline::GetSplinePointsCount() const
{
    return Curve.GetKeyframes().Count();
}

float Spline::GetSplineDuration() const
{
    return Curve.GetLength();
}

float Spline::GetSplineLength() const
{
    float sum = 0.0f;
    for (int32 i = 1; i < Curve.GetKeyframes().Count(); i++)
        sum += Vector3::DistanceSquared(Curve[i - 1].Value.Translation * _transform.Scale, Curve[i].Value.Translation * _transform.Scale);
    return Math::Sqrt(sum);
}

namespace
{
    void FindTimeClosestToPoint(const Vector3& point, const Spline::Keyframe& start, const Spline::Keyframe& end, float& bestDistanceSquared, float& bestTime)
    {
        // TODO: implement sth more analytical than brute-force solution
        const int32 slices = 100;
        const float step = 1.0f / (float)slices;
        const float length = Math::Abs(end.Time - start.Time);
        for (int32 i = 0; i <= slices; i++)
        {
            const float t = (float)i * step;
            Transform result;
            Spline::Keyframe::Interpolate(start, end, t, length, result);
            const float distanceSquared = Vector3::DistanceSquared(point, result.Translation);
            if (distanceSquared < bestDistanceSquared)
            {
                bestDistanceSquared = distanceSquared;
                bestTime = start.Time + t * length;
            }
        }
    }
}

float Spline::GetSplineTimeClosestToPoint(const Vector3& point) const
{
    const int32 pointsCount = Curve.GetKeyframes().Count();
    if (pointsCount == 0)
        return 0.0f;
    if (pointsCount == 1)
        return Curve[0].Time;
    const Vector3 localPoint = _transform.WorldToLocal(point);
    float bestDistanceSquared = MAX_float;
    float bestTime = 0.0f;
    for (int32 i = 1; i < pointsCount; i++)
        FindTimeClosestToPoint(localPoint, Curve[i - 1], Curve[i], bestDistanceSquared, bestTime);
    if (_loop)
        FindTimeClosestToPoint(localPoint, Curve[pointsCount - 1], Curve[0], bestDistanceSquared, bestTime);
    return bestTime;
}

Vector3 Spline::GetSplinePointClosestToPoint(const Vector3& point) const
{
    return GetSplinePoint(GetSplineTimeClosestToPoint(point));
}

void Spline::GetSplinePoints(Array<Vector3>& points) const
{
    for (auto& e : Curve.GetKeyframes())
        points.Add(_transform.LocalToWorld(e.Value.Translation));
}

void Spline::GetSplineLocalPoints(Array<Vector3>& points) const
{
    for (auto& e : Curve.GetKeyframes())
        points.Add(e.Value.Translation);
}

void Spline::GetSplinePoints(Array<Transform>& points) const
{
    for (auto& e : Curve.GetKeyframes())
        points.Add(_transform.LocalToWorld(e.Value));
}

void Spline::GetSplineLocalPoints(Array<Transform>& points) const
{
    for (auto& e : Curve.GetKeyframes())
        points.Add(e.Value);
}

void Spline::ClearSpline()
{
    if (Curve.IsEmpty())
        return;
    Curve.Clear();
    UpdateSpline();
}

void Spline::AddSplinePoint(const Vector3& point, bool updateSpline)
{
    const Keyframe k(Curve.IsEmpty() ? 0.0f : Curve.GetKeyframes().Last().Time + 1.0f, Transform(point));
    Curve.GetKeyframes().Add(k);
    if (updateSpline)
        UpdateSpline();
}

void Spline::AddSplineLocalPoint(const Vector3& point, bool updateSpline)
{
    const Keyframe k(Curve.IsEmpty() ? 0.0f : Curve.GetKeyframes().Last().Time + 1.0f, Transform(_transform.WorldToLocal(point)));
    Curve.GetKeyframes().Add(k);
    if (updateSpline)
        UpdateSpline();
}

void Spline::AddSplinePoint(const Transform& point, bool updateSpline)
{
    const Keyframe k(Curve.IsEmpty() ? 0.0f : Curve.GetKeyframes().Last().Time + 1.0f, _transform.WorldToLocal(point));
    Curve.GetKeyframes().Add(k);
    if (updateSpline)
        UpdateSpline();
}

void Spline::AddSplineLocalPoint(const Transform& point, bool updateSpline)
{
    const Keyframe k(Curve.IsEmpty() ? 0.0f : Curve.GetKeyframes().Last().Time + 1.0f, point);
    Curve.GetKeyframes().Add(k);
    if (updateSpline)
        UpdateSpline();
}

void Spline::UpdateSpline()
{
}

void Spline::GetKeyframes(MonoArray* data)
{
    Platform::MemoryCopy(mono_array_addr_with_size(data, sizeof(Keyframe), 0), Curve.GetKeyframes().Get(), sizeof(Keyframe) * Curve.GetKeyframes().Count());
}

void Spline::SetKeyframes(MonoArray* data)
{
    const auto count = (int32)mono_array_length(data);
    Curve.GetKeyframes().Resize(count, false);
    Platform::MemoryCopy(Curve.GetKeyframes().Get(), mono_array_addr_with_size(data, sizeof(Keyframe), 0), sizeof(Keyframe) * count);
    UpdateSpline();
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

namespace
{
    void DrawSegment(Spline* spline, int32 start, int32 end, const Color& color, const Transform& transform, bool depthTest)
    {
        const auto& startKey = spline->Curve[start];
        const auto& endKey = spline->Curve[end];
        const Vector3 startPos = transform.LocalToWorld(startKey.Value.Translation);
        const Vector3 startTangent = transform.LocalToWorld(startKey.TangentOut.Translation);
        const Vector3 endPos = transform.LocalToWorld(endKey.Value.Translation);
        const Vector3 endTangent = transform.LocalToWorld(endKey.TangentIn.Translation);
        const float d = (endKey.Time - startKey.Time) / 3.0f;
        DEBUG_DRAW_BEZIER(startPos, startPos + startTangent * d, endPos + endTangent * d, endPos, color, 0.0f, depthTest);
    }

    void DrawSpline(Spline* spline, const Color& color, const Transform& transform, bool depthTest)
    {
        const int32 count = spline->Curve.GetKeyframes().Count();
        for (int32 i = 0; i < count; i++)
        {
            Vector3 p = transform.LocalToWorld(spline->Curve[i].Value.Translation);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(p, 5.0f), color, 0.0f, true);
            if (i != 0)
                DrawSegment(spline, i - 1, i, color, transform, depthTest);
        }
        if (spline->GetIsLoop() && count > 1)
            DrawSegment(spline, count - 1, 0, color, transform, depthTest);
    }
}

void Spline::OnDebugDraw()
{
    const Color color = GetSplineColor();
    DrawSpline(this, color.AlphaMultiplied(0.7f), _transform, true);

    // Base
    Actor::OnDebugDraw();
}

void Spline::OnDebugDrawSelected()
{
    const Color color = GetSplineColor();
    DrawSpline(this, color.AlphaMultiplied(0.3f), _transform, false);
    DrawSpline(this, color, _transform, true);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void Spline::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Spline);

    SERIALIZE_MEMBER(IsLoop, _loop);
    SERIALIZE(Curve);
}

void Spline::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(IsLoop, _loop);
    DESERIALIZE(Curve);

    // Initialize spline when loading data during gameplay
    if (IsDuringPlay())
    {
        UpdateSpline();
    }
}

void Spline::OnEnable()
{
    // Base
    Actor::OnEnable();

    // Initialize spline
    UpdateSpline();
}
