// Copyright (c) Wojciech Figat. All rights reserved.

#include "Inertialization.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Quaternion.h"

namespace
{
    // Offsets smaller than this are treated as no offset (cm for translation, radians for rotation, scale units)
    constexpr float MinOffset = 1e-5f;

    float GetVelocity(float value, float valueBefore, float deltaTime)
    {
        return deltaTime > ZeroTolerance ? (value - valueBefore) / deltaTime : 0.0f;
    }

    // Splits the vector offset into its direction and length and sets up the curve along the direction
    void SetupVector(const Float3& offset, const Float3& offsetBefore, float deltaTime, float duration, Float3& direction, Inertialization::Curve& curve)
    {
        const float length = offset.Length();
        if (length < MinOffset)
        {
            direction = Float3::Zero;
            curve = Inertialization::Curve();
            return;
        }
        direction = offset / length;
        curve.Setup(length, GetVelocity(length, Float3::Dot(offsetBefore, direction), deltaTime), duration);
    }

    // Gets the shortest path rotation from b to a (so a = result * b)
    Quaternion GetRotationOffset(const Quaternion& a, const Quaternion& b)
    {
        Quaternion result = a * Quaternion::Invert(b);
        if (result.W < 0.0f)
            result = Quaternion(-result.X, -result.Y, -result.Z, -result.W);
        result.Normalize();
        return result;
    }

    // Gets the signed angle (in radians) of the rotation about the axis
    float GetTwistAngle(const Quaternion& rotation, const Float3& axis)
    {
        const float projected = rotation.X * axis.X + rotation.Y * axis.Y + rotation.Z * axis.Z;
        return 2.0f * Math::Atan2(projected, rotation.W);
    }
}

void Inertialization::Curve::Setup(float value, float velocity, float duration)
{
    if (duration <= ZeroTolerance || value <= 0.0f)
    {
        *this = Curve();
        return;
    }

    // Moving toward zero too fast would overshoot past it within the duration, so reach zero sooner
    if (velocity < 0.0f)
        duration = Math::Min(duration, -5.0f * value / velocity);

    // Moving away from zero would swing past both poses before coming back, so never go further than where the blend started
    velocity = Math::Min(velocity, 0.0f);

    // Start with the acceleration that keeps the jerk at the start low, never accelerating away from zero
    const float t = duration;
    const float t2 = t * t;
    const float acceleration = Math::Max((-8.0f * velocity * t - 20.0f * value) / t2, 0.0f);

    // The quintic that starts at (value, velocity, acceleration) and ends at zero with zero velocity and acceleration at the duration
    const float at2 = acceleration * t2;
    const float vt = velocity * t;
    C5 = -(at2 + 6.0f * vt + 12.0f * value) / (2.0f * t2 * t2 * t);
    C4 = (3.0f * at2 + 16.0f * vt + 30.0f * value) / (2.0f * t2 * t2);
    C3 = -(3.0f * at2 + 12.0f * vt + 20.0f * value) / (2.0f * t2 * t);
    C2 = acceleration * 0.5f;
    C1 = velocity;
    C0 = value;
    Duration = duration;
}

float Inertialization::Curve::Evaluate(float time) const
{
    if (time >= Duration)
        return 0.0f;
    if (time <= 0.0f)
        return C0;
    return ((((C5 * time + C4) * time + C3) * time + C2) * time + C1) * time + C0;
}

void Inertialization::Start(const Transform* previous, const Transform* beforePrevious, const Transform* target, int32 count, float previousDeltaTime, float duration, BoneOffset* offsets)
{
    for (int32 i = 0; i < count; i++)
    {
        const Transform& p = previous[i];
        const Transform& b = beforePrevious[i];
        const Transform& t = target[i];
        BoneOffset& offset = offsets[i];

        // The offset velocity comes from the old pose's motion over the last frame (the new pose is taken as still at the switch)
        SetupVector((Float3)(p.Translation - t.Translation), (Float3)(b.Translation - t.Translation), previousDeltaTime, duration, offset.TranslationDirection, offset.Translation);
        SetupVector(p.Scale - t.Scale, b.Scale - t.Scale, previousDeltaTime, duration, offset.ScaleDirection, offset.Scale);

        const Quaternion rotation = GetRotationOffset(p.Orientation, t.Orientation);
        const Float3 axisRaw(rotation.X, rotation.Y, rotation.Z);
        const float axisLength = axisRaw.Length();
        const float angle = 2.0f * Math::Atan2(axisLength, rotation.W);
        if (angle < MinOffset || axisLength < ZeroTolerance)
        {
            offset.RotationAxis = Float3::Zero;
            offset.Rotation = Curve();
        }
        else
        {
            offset.RotationAxis = axisRaw / axisLength;
            // Near a half turn the angle one frame before can wrap around to the other side, so take the shortest way between them
            const float angleBefore = GetTwistAngle(GetRotationOffset(b.Orientation, t.Orientation), offset.RotationAxis);
            const float angleChange = Math::UnwindRadians(angle - angleBefore);
            offset.Rotation.Setup(angle, GetVelocity(angleChange, 0.0f, previousDeltaTime), duration);
        }
    }
}

void Inertialization::Apply(const BoneOffset* offsets, int32 count, float time, Transform* pose)
{
    for (int32 i = 0; i < count; i++)
    {
        const BoneOffset& offset = offsets[i];
        Transform& t = pose[i];
        if (time < offset.Translation.Duration)
            t.Translation += (Vector3)(offset.TranslationDirection * offset.Translation.Evaluate(time));
        if (time < offset.Scale.Duration)
            t.Scale += offset.ScaleDirection * offset.Scale.Evaluate(time);
        if (time < offset.Rotation.Duration)
        {
            Quaternion rotation;
            Quaternion::RotationAxis(offset.RotationAxis, offset.Rotation.Evaluate(time), rotation);
            t.Orientation = rotation * t.Orientation;
            t.Orientation.Normalize();
        }
    }
}
