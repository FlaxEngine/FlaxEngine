// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Animations/OrientationWarp.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Graphics/Models/SkeletonData.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // Skeleton: 0 root, 1 pelvis, 2 spine_01, 3 spine_02, 4 chest, 5 hand (child of chest), 6 thigh, 7 foot, 8 ik_foot_root, 9 ik_foot, 10 ik_hand_root
    enum
    {
        Root,
        Pelvis,
        Spine1,
        Spine2,
        Chest,
        Hand,
        Thigh,
        Foot,
        IKFootRoot,
        IKFoot,
        IKHandRoot,
        Count
    };

    void SetupSkeleton(Array<SkeletonNode>& nodes)
    {
        const int32 parents[Count] = { -1, Root, Pelvis, Spine1, Spine2, Chest, Pelvis, Thigh, Root, IKFootRoot, Root };
        nodes.Resize(Count);
        for (int32 i = 0; i < Count; i++)
        {
            nodes[i].ParentIndex = parents[i];
            nodes[i].LocalTransform = Transform::Identity;
        }
    }

    void SetupPose(Transform* pose)
    {
        // A mid-stride pose with some twist so nothing lines up with the axes
        pose[Root] = Transform(Vector3::Zero);
        pose[Pelvis] = Transform(Vector3(4, 95, 2), Quaternion::Euler(5, 10, 3));
        pose[Spine1] = Transform(Vector3(0, 10, 1), Quaternion::Euler(-4, -6, 0));
        pose[Spine2] = Transform(Vector3(0, 12, 0), Quaternion::Euler(2, -3, 1));
        pose[Chest] = Transform(Vector3(0, 14, -1), Quaternion::Euler(3, 2, 0));
        pose[Hand] = Transform(Vector3(30, -5, 10), Quaternion::Euler(0, 0, 45));
        pose[Thigh] = Transform(Vector3(10, -5, 0), Quaternion::Euler(30, 0, 0));
        pose[Foot] = Transform(Vector3(0, -85, 5), Quaternion::Euler(-20, 5, 0));
        pose[IKFootRoot] = Transform(Vector3::Zero);
        pose[IKFoot] = Transform(Vector3(12, 8, 20), Quaternion::Euler(-10, 5, 0));
        pose[IKHandRoot] = Transform(Vector3(0, 0, 0));
    }

    Transform ModelTransform(const Array<SkeletonNode>& nodes, const Transform* pose, int32 index)
    {
        Transform result = pose[index];
        for (int32 i = nodes[index].ParentIndex; i != -1; i = nodes[i].ParentIndex)
            result = pose[i].LocalToWorld(result);
        result.Orientation.Normalize();
        return result;
    }

    // The heading (in degrees) of a model space rotation's forward vector about the Y axis
    float Heading(const Quaternion& rotation)
    {
        const Float3 forward = Float3::Transform(Float3::Forward, rotation);
        return Math::Atan2(forward.X, forward.Z) * RadiansToDegrees;
    }

    float HeadingDelta(const Quaternion& from, const Quaternion& to)
    {
        float delta = Heading(to) - Heading(from);
        while (delta > 180.0f)
            delta -= 360.0f;
        while (delta < -180.0f)
            delta += 360.0f;
        return delta;
    }

    bool SameRotation(const Quaternion& a, const Quaternion& b)
    {
        return Math::Abs(Quaternion::Dot(a, b)) > 1.0f - 1e-5f;
    }

    bool SamePosition(const Vector3& a, const Vector3& b)
    {
        return Vector3::Distance(a, b) < 1e-2f;
    }

    Transform Turned(float angle, const Transform& modelTransform)
    {
        Quaternion turn;
        Quaternion::RotationAxis(Float3::UnitY, angle * DegreesToRadians, turn);
        Transform result = Transform(Vector3::Zero, turn).LocalToWorld(modelTransform);
        result.Orientation.Normalize();
        return result;
    }

    OrientationWarp::Settings LyraLikeSettings()
    {
        OrientationWarp::Settings settings;
        settings.BodyRoot = Pelvis;
        settings.IKFootRoot = IKFootRoot;
        settings.SpineStart = Spine1;
        settings.SpineEnd = Chest;
        settings.Distribution = 0.75f;
        return settings;
    }
}

TEST_CASE("OrientationWarp")
{
    Array<SkeletonNode> nodes;
    SetupSkeleton(nodes);
    Transform input[Count];
    SetupPose(input);
    Transform before[Count];
    for (int32 i = 0; i < Count; i++)
        before[i] = ModelTransform(nodes, input, i);

    SECTION("Angle 0 returns the input pose")
    {
        Transform pose[Count];
        SetupPose(pose);
        OrientationWarp::Warp(nodes.Get(), Count, LyraLikeSettings(), 0.0f, pose);
        for (int32 i = 0; i < Count; i++)
        {
            CHECK(pose[i].Translation == input[i].Translation);
            CHECK(pose[i].Orientation == input[i].Orientation);
            CHECK(pose[i].Scale == input[i].Scale);
        }
    }

    SECTION("Feet turn by the angle and the chest keeps facing")
    {
        for (const float angle : { 30.0f, -30.0f, 75.0f })
        {
            Transform pose[Count];
            SetupPose(pose);
            OrientationWarp::Warp(nodes.Get(), Count, LyraLikeSettings(), angle, pose);
            Transform after[Count];
            for (int32 i = 0; i < Count; i++)
                after[i] = ModelTransform(nodes, pose, i);

            // FK and IK feet (and the legs) turn about the up axis through the origin
            for (const int32 i : { Pelvis, Thigh, Foot, IKFoot })
            {
                const Transform expected = Turned(angle, before[i]);
                CHECK(SameRotation(after[i].Orientation, expected.Orientation));
                CHECK(SamePosition(after[i].Translation, expected.Translation));
            }
            CHECK(Math::Abs(Math::Abs(HeadingDelta(before[Foot].Orientation, after[Foot].Orientation)) - Math::Abs(angle)) < 0.1f);
            CHECK(Math::Abs(Math::Abs(HeadingDelta(before[IKFoot].Orientation, after[IKFoot].Orientation)) - Math::Abs(angle)) < 0.1f);

            // The chest (and the hand below it) keeps its model space rotation
            CHECK(SameRotation(after[Chest].Orientation, before[Chest].Orientation));
            CHECK(SameRotation(after[Hand].Orientation, before[Hand].Orientation));
            CHECK(Math::Abs(HeadingDelta(before[Chest].Orientation, after[Chest].Orientation)) < 0.1f);

            // Lower spine nodes keep the part of the turn they don't take back
            const float share1 = OrientationWarp::GetSpineShare(0, 3, 0.75f);
            const float share2 = OrientationWarp::GetSpineShare(1, 3, 0.75f);
            CHECK(SameRotation(after[Spine1].Orientation, Turned(angle * (1.0f - share1), before[Spine1]).Orientation));
            CHECK(SameRotation(after[Spine2].Orientation, Turned(angle * (1.0f - share2), before[Spine2]).Orientation));

            // Local translations and scales of the spine are kept, and nodes outside the warp are untouched
            for (const int32 i : { Spine1, Spine2, Chest, Hand, Thigh, Foot, IKFoot })
            {
                CHECK(SamePosition(pose[i].Translation, input[i].Translation));
                CHECK(Float3::NearEqual(pose[i].Scale, input[i].Scale));
            }
            CHECK(pose[Root].Translation == input[Root].Translation);
            CHECK(pose[Root].Orientation == input[Root].Orientation);
            CHECK(pose[IKHandRoot].Translation == input[IKHandRoot].Translation);
            CHECK(pose[IKHandRoot].Orientation == input[IKHandRoot].Orientation);
        }
    }

    SECTION("Spine share")
    {
        // Distribution 0: the first spine node takes all of the turn back
        CHECK(Math::NearEqual(OrientationWarp::GetSpineShare(0, 5, 0.0f), 1.0f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpineShare(3, 5, 0.0f), 1.0f));

        // Distribution 1: even shares
        CHECK(Math::NearEqual(OrientationWarp::GetSpineShare(0, 5, 1.0f), 0.2f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpineShare(2, 5, 1.0f), 0.6f));

        // The last node always ends at the full turn back
        CHECK(Math::NearEqual(OrientationWarp::GetSpineShare(4, 5, 0.75f), 1.0f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpineShare(0, 5, 0.75f), 0.4f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpineShare(0, 1, 0.3f), 1.0f));
    }

    SECTION("Distribution 0 puts the whole turn back on the first spine node")
    {
        auto settings = LyraLikeSettings();
        settings.Distribution = 0.0f;
        Transform pose[Count];
        SetupPose(pose);
        OrientationWarp::Warp(nodes.Get(), Count, settings, 30.0f, pose);
        CHECK(SameRotation(ModelTransform(nodes, pose, Spine1).Orientation, before[Spine1].Orientation));
        CHECK(SameRotation(pose[Spine2].Orientation, input[Spine2].Orientation));
        CHECK(SameRotation(pose[Chest].Orientation, input[Chest].Orientation));
    }

    SECTION("Root as body root with a kept hand IK root")
    {
        OrientationWarp::Settings settings;
        settings.BodyRoot = Root;
        settings.SpineStart = Spine1;
        settings.SpineEnd = Chest;
        settings.KeepNode = IKHandRoot;
        Transform pose[Count];
        SetupPose(pose);
        OrientationWarp::Warp(nodes.Get(), Count, settings, 30.0f, pose);

        // Everything below the root turns, IK feet included
        const Transform foot = ModelTransform(nodes, pose, IKFoot);
        CHECK(SameRotation(foot.Orientation, Turned(30.0f, before[IKFoot]).Orientation));
        CHECK(SamePosition(foot.Translation, Turned(30.0f, before[IKFoot]).Translation));

        // The chest and the kept node hold their model space facing (and the kept node its position)
        CHECK(SameRotation(ModelTransform(nodes, pose, Chest).Orientation, before[Chest].Orientation));
        const Transform handRoot = ModelTransform(nodes, pose, IKHandRoot);
        CHECK(SameRotation(handRoot.Orientation, before[IKHandRoot].Orientation));
        CHECK(SamePosition(handRoot.Translation, before[IKHandRoot].Translation));
    }

    SECTION("Spine start outside the chain falls back to the spine end alone")
    {
        auto settings = LyraLikeSettings();
        settings.SpineStart = Thigh;
        Transform pose[Count];
        SetupPose(pose);
        OrientationWarp::Warp(nodes.Get(), Count, settings, 30.0f, pose);
        CHECK(SameRotation(pose[Spine1].Orientation, input[Spine1].Orientation));
        CHECK(SameRotation(pose[Spine2].Orientation, input[Spine2].Orientation));
        CHECK(SameRotation(ModelTransform(nodes, pose, Chest).Orientation, before[Chest].Orientation));
    }

    SECTION("Spine start above the body root falls back to the spine end alone")
    {
        auto settings = LyraLikeSettings();
        settings.SpineStart = Root;
        Transform pose[Count];
        SetupPose(pose);
        OrientationWarp::Warp(nodes.Get(), Count, settings, 30.0f, pose);

        // The body root keeps its full turn
        const Transform foot = ModelTransform(nodes, pose, Foot);
        CHECK(SameRotation(foot.Orientation, Turned(30.0f, before[Foot]).Orientation));
        CHECK(SamePosition(foot.Translation, Turned(30.0f, before[Foot]).Translation));
        CHECK(SameRotation(ModelTransform(nodes, pose, Chest).Orientation, before[Chest].Orientation));
    }

    SECTION("Turn direction")
    {
        // A positive angle about Y turns +X toward -Z
        Array<SkeletonNode> simple;
        simple.Resize(2);
        simple[0].ParentIndex = -1;
        simple[1].ParentIndex = 0;
        Transform pose[2] = { Transform(Vector3::Zero), Transform(Vector3(10, 0, 0)) };
        OrientationWarp::Settings settings;
        settings.BodyRoot = 1;
        OrientationWarp::Warp(simple.Get(), 2, settings, 90.0f, pose);
        CHECK(SamePosition(pose[1].Translation, Vector3(0, 0, -10)));
    }

    SECTION("Speed weight")
    {
        CHECK(Math::NearEqual(OrientationWarp::GetSpeedWeight(0.0f, 0.0f), 1.0f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpeedWeight(50.0f, 80.0f), 0.0f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpeedWeight(80.0f, 80.0f), 0.0f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpeedWeight(90.0f, 80.0f), 0.5f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpeedWeight(100.0f, 80.0f), 1.0f));
        CHECK(Math::NearEqual(OrientationWarp::GetSpeedWeight(500.0f, 80.0f), 1.0f));
    }

    SECTION("Clamp angle")
    {
        CHECK(Math::NearEqual(OrientationWarp::ClampAngle(100.0f, 0.0f), 100.0f));
        CHECK(Math::NearEqual(OrientationWarp::ClampAngle(100.0f, 90.0f), 90.0f));
        CHECK(Math::NearEqual(OrientationWarp::ClampAngle(-100.0f, 90.0f), -90.0f));
        CHECK(Math::NearEqual(OrientationWarp::ClampAngle(45.0f, 180.0f), 45.0f));
    }

    SECTION("Interp angle")
    {
        // No speed: straight to the target
        CHECK(Math::NearEqual(OrientationWarp::InterpAngle(0.0f, 40.0f, 0.0f, 0.1f), 40.0f));
        // A tenth of the way at speed 10 over a hundredth of a second
        CHECK(Math::NearEqual(OrientationWarp::InterpAngle(0.0f, 40.0f, 10.0f, 0.01f), 4.0f));
        // Never past the target
        CHECK(Math::NearEqual(OrientationWarp::InterpAngle(0.0f, 40.0f, 10.0f, 1.0f), 40.0f));
        // The short way round across the back: 170 to -170 is 20 degrees, half of it here
        CHECK(Math::NearEqual(Math::Abs(OrientationWarp::InterpAngle(170.0f, -170.0f, 10.0f, 0.05f)), 180.0f));
        float angle = 170.0f;
        float travelled = 0.0f;
        for (int32 i = 0; i < 120; i++)
        {
            const float next = OrientationWarp::InterpAngle(angle, -170.0f, 10.0f, 1.0f / 60.0f);
            travelled += Math::Abs(Math::UnwindDegrees(next - angle));
            angle = next;
        }
        CHECK(Math::NearEqual(angle, -170.0f, 0.1f));
        CHECK(Math::NearEqual(travelled, 20.0f, 0.1f));

        // Half way round, either way is as short: it unwinds toward zero, whatever the rounding
        CHECK(OrientationWarp::InterpAngle(10.0f, -170.0f, 10.0f, 0.01f) < 10.0f);
        CHECK(OrientationWarp::InterpAngle(-10.0f, 170.0f, 10.0f, 0.01f) > -10.0f);
        CHECK(OrientationWarp::InterpAngle(10.0f, -170.0001f, 10.0f, 0.01f) < 10.0f);
        CHECK(OrientationWarp::InterpAngle(10.0f, -169.9999f, 10.0f, 0.01f) < 10.0f);

        // Out to a half turn and straight back again: it comes back the way it went, never round a full turn
        angle = 0.0f;
        float turned = 0.0f;
        for (int32 i = 0; i < 60; i++)
        {
            const float next = OrientationWarp::InterpAngle(angle, 180.0f, 10.0f, 1.0f / 60.0f);
            turned += Math::UnwindDegrees(next - angle);
            angle = next;
        }
        for (int32 i = 0; i < 120; i++)
        {
            const float next = OrientationWarp::InterpAngle(angle, 0.0f, 10.0f, 1.0f / 60.0f);
            turned += Math::UnwindDegrees(next - angle);
            angle = next;
        }
        CHECK(Math::Abs(turned) < 1.0f);
    }

    SECTION("Unset nodes leave the pose alone")
    {
        Transform pose[Count];
        SetupPose(pose);
        OrientationWarp::Warp(nodes.Get(), Count, OrientationWarp::Settings(), 30.0f, pose);
        for (int32 i = 0; i < Count; i++)
        {
            CHECK(pose[i].Translation == input[i].Translation);
            CHECK(pose[i].Orientation == input[i].Orientation);
        }
    }
}
