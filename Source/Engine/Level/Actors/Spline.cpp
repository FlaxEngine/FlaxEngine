// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Spline.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Animations/CurveSerialization.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"

Spline::Spline(const SpawnParams& params)
    : Actor(params)
    , _localBounds(Vector3::Zero, Vector3::Zero)
{
}

bool Spline::GetIsLoop() const
{
    return _loop;
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

Vector3 Spline::GetSplineDirection(float time) const
{
    return _transform.LocalToWorldVector(GetSplineLocalDirection(time));
}

Vector3 Spline::GetSplineLocalDirection(float time) const
{
    if (Curve.GetKeyframes().Count() == 0)
        return Vector3::Forward;
    Transform t;
    Curve.EvaluateFirstDerivative(t, time, _loop);
    t.Translation.Normalize();
    return t.Translation;
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
    CHECK_RETURN(index >= 0 && index < GetSplinePointsCount(), Transform::Identity)
    return _transform.LocalToWorld(Curve[index].Value);
}

Transform Spline::GetSplineLocalTransform(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < GetSplinePointsCount(), Transform::Identity)
    return Curve[index].Value;
}

Transform Spline::GetSplineTangent(int32 index, bool isIn)
{
    return _transform.LocalToWorld(GetSplineLocalTangent(index, isIn));
}

Transform Spline::GetSplineLocalTangent(int32 index, bool isIn)
{
    CHECK_RETURN(index >= 0 && index < GetSplinePointsCount(), Transform::Identity)
    const auto& k = Curve[index];
    const auto& tangent = isIn ? k.TangentIn : k.TangentOut;
    return tangent + k.Value;
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
    constexpr int32 slices = 20;
    constexpr float step = 1.0f / (float)(slices - 1);
    const Vector3 scale = _transform.Scale;
    for (int32 i = 1; i < Curve.GetKeyframes().Count(); i++)
    {
        const auto& a = Curve[i - 1];
        const auto& b = Curve[i];
        Vector3 prevPoint = a.Value.Translation * scale;

        const float tangentScale = Math::Abs(b.Time - a.Time) / 3.0f;
        Vector3 leftTangent, rightTangent;
        AnimationUtils::GetTangent(a.Value.Translation, a.TangentOut.Translation, tangentScale, leftTangent);
        AnimationUtils::GetTangent(b.Value.Translation, b.TangentIn.Translation, tangentScale, rightTangent);

        for (int32 slice = 1; slice < slices; slice++)
        {
            const float t = (float)slice * step;
            Vector3 pos;
            AnimationUtils::Bezier(a.Value.Translation, leftTangent, rightTangent, b.Value.Translation, t, pos);
            pos *= scale;
            sum += (float)Vector3::Distance(pos, prevPoint);
            prevPoint = pos;
        }
    }
    return sum;
}

float Spline::GetSplineSegmentLength(int32 index) const
{
    if (index == 0)
        return 0.0f;
    CHECK_RETURN(index > 0 && index < GetSplinePointsCount(), 0.0f);
    float sum = 0.0f;
    constexpr int32 slices = 20;
    constexpr float step = 1.0f / (float)(slices - 1);
    const auto& a = Curve[index - 1];
    const auto& b = Curve[index];
    const Vector3 scale = _transform.Scale;
    Vector3 prevPoint = a.Value.Translation * scale;
    {
        const float tangentScale = Math::Abs(b.Time - a.Time) / 3.0f;
        Vector3 leftTangent, rightTangent;
        AnimationUtils::GetTangent(a.Value.Translation, a.TangentOut.Translation, tangentScale, leftTangent);
        AnimationUtils::GetTangent(b.Value.Translation, b.TangentIn.Translation, tangentScale, rightTangent);

        for (int32 slice = 1; slice < slices; slice++)
        {
            const float t = (float)slice * step;
            Vector3 pos;
            AnimationUtils::Bezier(a.Value.Translation, leftTangent, rightTangent, b.Value.Translation, t, pos);
            pos *= scale;
            sum += (float)Vector3::Distance(pos, prevPoint);
            prevPoint = pos;
        }
    }
    return sum;
}

float Spline::GetSplineTime(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < GetSplinePointsCount(), 0.0f)
    return Curve[index].Time;
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
            const float distanceSquared = (float)Vector3::DistanceSquared(point, result.Translation);
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

void Spline::RemoveSplinePoint(int32 index, bool updateSpline)
{
    CHECK(index >= 0 && index < GetSplinePointsCount());
    Curve.GetKeyframes().RemoveAtKeepOrder(index);
    if (updateSpline)
        UpdateSpline();
}

void Spline::SetSplinePoint(int32 index, const Vector3& point, bool updateSpline)
{
    CHECK(index >= 0 && index < GetSplinePointsCount());
    Curve[index].Value.Translation = _transform.WorldToLocal(point);
    if (updateSpline)
        UpdateSpline();
}

void Spline::SetSplineLocalPoint(int32 index, const Vector3& point, bool updateSpline)
{
    CHECK(index >= 0 && index < GetSplinePointsCount());
    Curve[index].Value.Translation = point;
    if (updateSpline)
        UpdateSpline();
}

void Spline::SetSplineTransform(int32 index, const Transform& point, bool updateSpline)
{
    CHECK(index >= 0 && index < GetSplinePointsCount());
    Curve[index].Value = _transform.WorldToLocal(point);
    if (updateSpline)
        UpdateSpline();
}

void Spline::SetSplineLocalTransform(int32 index, const Transform& point, bool updateSpline)
{
    CHECK(index >= 0 && index < GetSplinePointsCount());
    Curve[index].Value = point;
    if (updateSpline)
        UpdateSpline();
}

void Spline::SetSplineTangent(int32 index, const Transform& point, bool isIn, bool updateSpline)
{
    SetSplineLocalTangent(index, _transform.WorldToLocal(point), isIn, updateSpline);
}

void Spline::SetSplineLocalTangent(int32 index, const Transform& point, bool isIn, bool updateSpline)
{
    CHECK(index >= 0 && index < GetSplinePointsCount());
    auto& k = Curve[index];
    auto& tangent = isIn ? k.TangentIn : k.TangentOut;
    tangent = point - k.Value;
    if (updateSpline)
        UpdateSpline();
}

void Spline::SetSplinePointTime(int32 index, float time, bool updateSpline)
{
    CHECK(index >= 0 && index < GetSplinePointsCount());
    Curve[index].Time = time;
    if (updateSpline)
        UpdateSpline();
}

void Spline::AddSplinePoint(const Vector3& point, bool updateSpline)
{
    const Keyframe k(Curve.IsEmpty() ? 0.0f : Curve.GetKeyframes().Last().Time + 1.0f, Transform(_transform.WorldToLocal(point)));
    Curve.GetKeyframes().Add(k);
    if (updateSpline)
        UpdateSpline();
}

void Spline::AddSplineLocalPoint(const Vector3& point, bool updateSpline)
{
    const Keyframe k(Curve.IsEmpty() ? 0.0f : Curve.GetKeyframes().Last().Time + 1.0f, Transform(point));
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

void Spline::InsertSplinePoint(int32 index, float time, const Transform& point, bool updateSpline)
{
    const Keyframe k(time, _transform.WorldToLocal(point));
    Curve.GetKeyframes().Insert(index, k);
    if (updateSpline)
        UpdateSpline();
}

void Spline::InsertSplineLocalPoint(int32 index, float time, const Transform& point, bool updateSpline)
{
    const Keyframe k(time, point);
    Curve.GetKeyframes().Insert(index, k);
    if (updateSpline)
        UpdateSpline();
}

void Spline::SetTangentsLinear()
{
    const int32 count = Curve.GetKeyframes().Count();
    if (count < 2)
        return;

    if (_loop)
        Curve[count - 1].Value = Curve[0].Value;
    for (int32 i = 0; i < count; i++)
    {
        auto& k = Curve[i];
        k.TangentIn = k.TangentOut = Transform::Identity;
    }

    UpdateSpline();
}

void Spline::SetTangentsSmooth()
{
    const int32 count = Curve.GetKeyframes().Count();
    if (count < 2)
        return;

    auto& keys = Curve.GetKeyframes();
    const int32 last = count - 2;
    if (_loop)
        Curve[count - 1].Value = Curve[0].Value;
    for (int32 i = 0; i <= last; i++)
    {
        auto& key = keys[i];
        const auto& prevKey = keys[i == 0 ? (_loop ? last : 0) : i - 1];
        const auto& nextKey = keys[i == last ? (_loop ? 0 : last) : i + 1];
        const float prevTime = _loop && i == 0 ? key.Time : prevKey.Time;
        const float nextTime = _loop && i == last ? key.Time : nextKey.Time;
        const Vector3 slope = key.Value.Translation - prevKey.Value.Translation + nextKey.Value.Translation - key.Value.Translation;
        const Vector3 tangent = slope / Math::Max(nextTime - prevTime, ZeroTolerance);
        key.TangentIn.Translation = -tangent;
        key.TangentOut.Translation = tangent;
    }

    UpdateSpline();
}

void Spline::UpdateSpline()
{
    auto& keyframes = Curve.GetKeyframes();
    const int32 count = keyframes.Count();

    // Always keep last point in the loop
    if (_loop && count > 1)
    {
        auto& first = keyframes[0];
        auto& last = keyframes[count - 1];
        last.Value = first.Value;
        last.TangentIn = first.TangentIn;
        last.TangentOut = first.TangentOut;
    }

    // Update bounds
    _localBounds = BoundingBox(count != 0 ? keyframes[0].Value.Translation : Vector3::Zero);
    for (int32 i = 1; i < count; i++)
        _localBounds.Merge(keyframes[i].Value.Translation);
    Matrix world;
    GetLocalToWorldMatrix(world);
    BoundingBox::Transform(_localBounds, world, _box);

    SplineUpdated();
}

#if !COMPILE_WITHOUT_CSHARP

void Spline::GetKeyframes(MArray* data)
{
    ASSERT(MCore::Array::GetLength(data) >= Curve.GetKeyframes().Count());
    Platform::MemoryCopy(MCore::Array::GetAddress(data), Curve.GetKeyframes().Get(), sizeof(Keyframe) * Curve.GetKeyframes().Count());
}

void Spline::SetKeyframes(MArray* data, int32 keySize)
{
    Curve = Span<byte>(MCore::Array::GetAddress<byte>(data), keySize * MCore::Array::GetLength(data));
    UpdateSpline();
}

#endif

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

namespace
{
    FORCE_INLINE float NodeSizeByDistance(const Vector3& nodePosition, bool scaleByDistance)
    {
        if (scaleByDistance)
            return (float)(Vector3::Distance(DebugDraw::GetViewPos(), nodePosition) / 100);
        return 5.0f;
    }

    void DrawSpline(Spline* spline, const Color& color, const Transform& transform, bool depthTest, bool scaleByDistance = false)
    {
        const int32 count = spline->Curve.GetKeyframes().Count();
        if (count == 0)
            return;
        Spline::Keyframe* prev = spline->Curve.GetKeyframes().Get();
        Vector3 prevPos = transform.LocalToWorld(prev->Value.Translation);
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(prevPos, NodeSizeByDistance(prevPos, scaleByDistance)), color, 0.0f, depthTest);
        for (int32 i = 1; i < count; i++)
        {
            Spline::Keyframe* next = prev + 1;
            Vector3 nextPos = transform.LocalToWorld(next->Value.Translation);
            DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(nextPos, NodeSizeByDistance(nextPos, scaleByDistance)), color, 0.0f, depthTest);
            const float d = (next->Time - prev->Time) / 3.0f;
            DEBUG_DRAW_BEZIER(prevPos, prevPos + prev->TangentOut.Translation * d, nextPos + next->TangentIn.Translation * d, nextPos, color, 0.0f, depthTest);
            prev = next;
            prevPos = nextPos;
        }
    }
}

void Spline::OnDebugDraw()
{
    DrawSpline(this, GetSplineColor().AlphaMultiplied(0.7f), _transform, true);

    // Base
    Actor::OnDebugDraw();
}

void Spline::OnDebugDrawSelected()
{
    DrawSpline(this, Color::White, _transform, false, true);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void Spline::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    Matrix world;
    GetLocalToWorldMatrix(world);
    BoundingBox::Transform(_localBounds, world, _box);
    BoundingSphere::FromBox(_box, _sphere);
}

void Spline::Initialize()
{
    // Base
    Actor::Initialize();

    auto& keyframes = Curve.GetKeyframes();
    const int32 count = keyframes.Count();

    // Update bounds
    _localBounds = BoundingBox(count != 0 ? keyframes[0].Value.Translation : Vector3::Zero);
    for (int32 i = 1; i < count; i++)
        _localBounds.Merge(keyframes[i].Value.Translation);
    Matrix world;
    GetLocalToWorldMatrix(world);
    BoundingBox::Transform(_localBounds, world, _box);
}

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
