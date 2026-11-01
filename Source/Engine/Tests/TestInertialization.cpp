// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Animations/Inertialization.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Quaternion.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    constexpr int32 Count = 3;
    constexpr float FrameTime = 1.0f / 60.0f;
    constexpr float BlendTime = 0.2f;

    // Old pose: bones moving along -X and turning about Y (one frame apart). New pose: somewhere else, ahead of that motion.
    void SetupOldPose(Transform* pose, float time)
    {
        for (int32 i = 0; i < Count; i++)
        {
            pose[i].Translation = Vector3(10.0f * i - 50.0f * time, 5.0f, -3.0f * i);
            pose[i].Orientation = Quaternion::Euler(0.0f, 30.0f - 90.0f * time + 10.0f * i, 0.0f);
            pose[i].Scale = Float3::One;
        }
    }

    void SetupNewPose(Transform* pose)
    {
        for (int32 i = 0; i < Count; i++)
        {
            pose[i].Translation = Vector3(10.0f * i - 20.0f, 5.0f, -3.0f * i);
            pose[i].Orientation = Quaternion::Euler(0.0f, -40.0f + 10.0f * i, 0.0f);
            pose[i].Scale = Float3::One;
        }
    }

    void Blend(const Inertialization::BoneOffset* offsets, const Transform* target, float time, Transform* result)
    {
        for (int32 i = 0; i < Count; i++)
            result[i] = target[i];
        Inertialization::Apply(offsets, Count, time, result);
    }

    float GetYaw(const Quaternion& q)
    {
        return q.GetEuler().Y;
    }

    // Signed angle (in radians) of the rotation about the axis
    float GetTwist(const Quaternion& q, const Float3& axis)
    {
        return 2.0f * Math::Atan2(q.X * axis.X + q.Y * axis.Y + q.Z * axis.Z, q.W);
    }

    void CheckSame(const Transform* a, const Transform* b, float tolerance)
    {
        for (int32 i = 0; i < Count; i++)
        {
            CHECK(Vector3::Distance(a[i].Translation, b[i].Translation) < tolerance);
            CHECK(Quaternion::AngleBetween(a[i].Orientation, b[i].Orientation) < tolerance);
            CHECK(Float3::Distance(a[i].Scale, b[i].Scale) < tolerance);
        }
    }
}

TEST_CASE("Inertialization")
{
    SECTION("Curve")
    {
        Inertialization::Curve curve;
        curve.Setup(1.0f, -2.0f, BlendTime);
        CHECK(Math::NearEqual(curve.Evaluate(0.0f), 1.0f));
        CHECK(Math::NearEqual(curve.Evaluate(BlendTime), 0.0f));
        CHECK(Math::NearEqual(curve.Evaluate(1.0f), 0.0f));

        // Starts with the given velocity and ends at rest
        const float h = 1e-4f;
        CHECK(Math::Abs((curve.Evaluate(h) - curve.Evaluate(0.0f)) / h + 2.0f) < 0.01f);
        CHECK(Math::Abs((curve.Evaluate(BlendTime - h) - curve.Evaluate(BlendTime - 2 * h)) / h) < 0.01f);

        // Fast motion toward zero reaches it sooner instead of overshooting past it
        curve.Setup(1.0f, -50.0f, BlendTime);
        CHECK(curve.Duration < BlendTime);
        for (float t = 0.0f; t <= BlendTime; t += 0.001f)
            CHECK(curve.Evaluate(t) > -0.01f);

        // Motion away from zero isn't kept: never goes above the start, and heads straight to zero
        curve.Setup(1.0f, 50.0f, BlendTime);
        CHECK(curve.C1 == 0.0f);
        CHECK(curve.Duration == BlendTime);
        float last = curve.Evaluate(0.0f);
        CHECK(Math::NearEqual(last, 1.0f));
        for (float t = 0.0f; t <= BlendTime; t += 0.001f)
        {
            const float value = curve.Evaluate(t);
            CHECK(value <= last + 1e-5f);
            CHECK(value > -0.01f);
            last = value;
        }

        // No offset, no curve
        curve.Setup(0.0f, 5.0f, BlendTime);
        CHECK(curve.Evaluate(0.0f) == 0.0f);
        curve.Setup(1.0f, 5.0f, 0.0f);
        CHECK(curve.Evaluate(0.0f) == 0.0f);
    }

    SECTION("Switch")
    {
        Transform before[Count], previous[Count], target[Count], result[Count];
        SetupOldPose(before, -FrameTime);
        SetupOldPose(previous, 0.0f);
        SetupNewPose(target);
        Inertialization::BoneOffset offsets[Count];
        Inertialization::Start(previous, before, target, Count, FrameTime, BlendTime, offsets);

        // Starts at the old pose
        Blend(offsets, target, 0.0f, result);
        CheckSame(result, previous, 0.001f);

        // Ends at the new pose
        Blend(offsets, target, BlendTime, result);
        CheckSame(result, target, 0.0001f);
        Blend(offsets, target, BlendTime * 0.5f, result);
        CHECK(Vector3::Distance(result[0].Translation, target[0].Translation) > 0.1f);

        // No velocity jump: the output keeps moving like the old pose did at the switch
        const float h = 1e-3f;
        Transform start[Count], next[Count];
        Blend(offsets, target, 0.0f, start);
        Blend(offsets, target, h, next);
        for (int32 i = 0; i < Count; i++)
        {
            const float oldSpeed = (float)(previous[i].Translation.X - before[i].Translation.X) / FrameTime;
            const float speed = (float)(next[i].Translation.X - start[i].Translation.X) / h;
            CHECK(Math::Abs(speed - oldSpeed) < Math::Abs(oldSpeed) * 0.05f);
            const float oldTurn = (GetYaw(previous[i].Orientation) - GetYaw(before[i].Orientation)) / FrameTime;
            const float turn = (GetYaw(next[i].Orientation) - GetYaw(start[i].Orientation)) / h;
            CHECK(Math::Abs(turn - oldTurn) < Math::Abs(oldTurn) * 0.05f);
        }
    }

    SECTION("Moving Away From The New Pose")
    {
        // Same as the switch, but the old pose heads away from the new pose
        Transform before[Count], previous[Count], target[Count], result[Count];
        SetupOldPose(before, FrameTime);
        SetupOldPose(previous, 0.0f);
        SetupNewPose(target);
        Inertialization::BoneOffset offsets[Count];
        Inertialization::Start(previous, before, target, Count, FrameTime, BlendTime, offsets);
        Blend(offsets, target, 0.0f, result);
        CheckSame(result, previous, 0.001f);
        for (float t = 0.0f; t <= BlendTime; t += 0.005f)
        {
            Blend(offsets, target, t, result);
            for (int32 i = 0; i < Count; i++)
            {
                CHECK(Vector3::Distance(result[i].Translation, target[i].Translation) <= Vector3::Distance(previous[i].Translation, target[i].Translation) + 0.001f);
                CHECK(Quaternion::AngleBetween(result[i].Orientation, target[i].Orientation) <= Quaternion::AngleBetween(previous[i].Orientation, target[i].Orientation) + 0.01f);
            }
        }
    }

    SECTION("Request During Blend")
    {
        Transform before[Count], previous[Count], target[Count];
        SetupOldPose(before, -FrameTime);
        SetupOldPose(previous, 0.0f);
        SetupNewPose(target);
        Inertialization::BoneOffset offsets[Count];
        Inertialization::Start(previous, before, target, Count, FrameTime, BlendTime, offsets);

        // Halfway through, switch again to another pose
        const float time = BlendTime * 0.5f;
        Transform outBefore[Count], out[Count], other[Count], result[Count];
        Blend(offsets, target, time - FrameTime, outBefore);
        Blend(offsets, target, time, out);
        SetupOldPose(other, 1.0f);
        Inertialization::BoneOffset otherOffsets[Count];
        Inertialization::Start(out, outBefore, other, Count, FrameTime, BlendTime, otherOffsets);

        // Doesn't pop: starts from the current output and moves on the way it did
        Blend(otherOffsets, other, 0.0f, result);
        CheckSame(result, out, 0.001f);
        const float h = 1e-3f;
        Transform next[Count];
        Blend(otherOffsets, other, h, next);
        for (int32 i = 0; i < Count; i++)
        {
            // Compare along the offset direction (the curve keeps the velocity along it)
            const Float3 direction = otherOffsets[i].TranslationDirection;
            const float oldSpeed = Float3::Dot((Float3)(out[i].Translation - outBefore[i].Translation), direction) / FrameTime;
            const float speed = Float3::Dot((Float3)(next[i].Translation - result[i].Translation), direction) / h;
            CHECK(Math::Abs(speed - oldSpeed) < Math::Max(Math::Abs(oldSpeed) * 0.1f, 1.0f));
        }

        // Ends at the latest pose
        Blend(otherOffsets, other, BlendTime, result);
        CheckSame(result, other, 0.0001f);
    }

    SECTION("Multi Axis Rotation")
    {
        // The old pose turns about all axes at once and the offset axis is tilted
        Transform before[Count], previous[Count], target[Count], start[Count], next[Count];
        for (int32 i = 0; i < Count; i++)
        {
            const float t = 0.0f;
            const float tb = FrameTime; // Heading toward the new pose
            previous[i] = Transform(Vector3((float)i, 0.0f, 0.0f), Quaternion::Euler(20.0f + 60.0f * t, 30.0f + 90.0f * t + i * 5.0f, 10.0f - 40.0f * t));
            before[i] = Transform(Vector3((float)i, 0.0f, 0.0f), Quaternion::Euler(20.0f + 60.0f * tb, 30.0f + 90.0f * tb + i * 5.0f, 10.0f - 40.0f * tb));
            target[i] = Transform(Vector3((float)i, 0.0f, 0.0f), Quaternion::Euler(-30.0f, 70.0f, 45.0f + i * 10.0f));
        }
        Inertialization::BoneOffset offsets[Count];
        Inertialization::Start(previous, before, target, Count, FrameTime, BlendTime, offsets);
        Blend(offsets, target, 0.0f, start);
        CheckSame(start, previous, 0.001f);
        Blend(offsets, target, BlendTime, next);
        CheckSame(next, target, 0.0001f);

        // Keeps the old turn speed about the offset axis
        const float h = 1e-3f;
        Blend(offsets, target, h, next);
        for (int32 i = 0; i < Count; i++)
        {
            const Float3 axis = offsets[i].RotationAxis;
            CHECK(Math::Abs(axis.LengthSquared() - 1.0f) < 0.001f);
            const float oldTurn = GetTwist(previous[i].Orientation * Quaternion::Invert(before[i].Orientation), axis) / FrameTime;
            const float turn = GetTwist(next[i].Orientation * Quaternion::Invert(start[i].Orientation), axis) / h;
            CHECK(Math::Abs(turn - oldTurn) < Math::Abs(oldTurn) * 0.1f + 0.05f);
        }
    }

    SECTION("Half Turn")
    {
        // The offset is just under a half turn while it was just over one a frame before: the velocity must stay the old turn speed, not wrap around
        Transform before[Count], previous[Count], target[Count];
        for (int32 i = 0; i < Count; i++)
        {
            before[i] = Transform(Vector3::Zero, Quaternion::Euler(0.0f, 181.0f, 0.0f));
            previous[i] = Transform(Vector3::Zero, Quaternion::Euler(0.0f, 179.0f, 0.0f));
            target[i] = Transform::Identity;
        }
        Inertialization::BoneOffset offsets[Count];
        Inertialization::Start(previous, before, target, Count, FrameTime, BlendTime, offsets);
        const float oldTurn = 2.0f * DegreesToRadians / FrameTime;
        for (int32 i = 0; i < Count; i++)
            CHECK(Math::Abs(Math::Abs(offsets[i].Rotation.C1) - oldTurn) < oldTurn * 0.05f);
    }

    SECTION("Same Pose")
    {
        Transform pose[Count], result[Count];
        SetupNewPose(pose);
        Inertialization::BoneOffset offsets[Count];
        Inertialization::Start(pose, pose, pose, Count, FrameTime, BlendTime, offsets);
        Blend(offsets, pose, 0.05f, result);
        CheckSame(result, pose, 0.00001f);
    }
}
