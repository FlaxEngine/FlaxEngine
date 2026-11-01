// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Animations/StrideWarp.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Graphics/Models/SkeletonData.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    enum
    {
        Root,
        Pelvis,
        ThighL,
        CalfL,
        FootL,
        ThighR,
        CalfR,
        FootR,
        IKFootRoot,
        IKFootL,
        IKFootR,
        Count
    };

    void SetupSkeleton(Array<SkeletonNode>& nodes)
    {
        const int32 parents[Count] = { -1, Root, Pelvis, ThighL, CalfL, Pelvis, ThighR, CalfR, Root, IKFootRoot, IKFootRoot };
        nodes.Resize(Count);
        for (int32 i = 0; i < Count; i++)
        {
            nodes[i].ParentIndex = parents[i];
            nodes[i].LocalTransform = Transform::Identity;
        }
    }

    // A mid-stride pose: left foot ahead, right foot behind, both on the ground with bent knees and the IK feet on the feet
    void SetupPose(Transform* pose)
    {
        pose[Root] = Transform(Vector3::Zero);
        pose[Pelvis] = Transform(Vector3(0, 95, 0), Quaternion::Euler(0, 4, 2));
        pose[ThighL] = Transform(Vector3(10, -5, 0), Quaternion::Euler(-20, 0, 0));
        pose[CalfL] = Transform(Vector3(0, -45, 5), Quaternion::Euler(15, 0, 0));
        pose[FootL] = Transform(Vector3(0, -45, 25), Quaternion::Euler(10, 5, 0));
        pose[ThighR] = Transform(Vector3(-10, -5, 0), Quaternion::Euler(25, 0, 0));
        pose[CalfR] = Transform(Vector3(0, -45, -2), Quaternion::Euler(10, 0, 0));
        pose[FootR] = Transform(Vector3(0, -45, -23), Quaternion::Euler(-15, -5, 0));
        pose[IKFootRoot] = Transform(Vector3::Zero);
        pose[IKFootL] = Transform::Identity;
        pose[IKFootR] = Transform::Identity;
    }

    Transform ModelTransform(const Array<SkeletonNode>& nodes, const Transform* pose, int32 index)
    {
        Transform result = pose[index];
        for (int32 i = nodes[index].ParentIndex; i != -1; i = nodes[i].ParentIndex)
            result = pose[i].LocalToWorld(result);
        result.Orientation.Normalize();
        return result;
    }

    // Puts the IK feet on the FK feet (as in an animation with baked IK feet)
    void MatchIKFeet(const Array<SkeletonNode>& nodes, Transform* pose)
    {
        pose[IKFootL] = ModelTransform(nodes, pose, FootL);
        pose[IKFootR] = ModelTransform(nodes, pose, FootR);
    }

    bool SameRotation(const Quaternion& a, const Quaternion& b)
    {
        return Math::Abs(Quaternion::Dot(a, b)) > 1.0f - 1e-5f;
    }

    bool SamePosition(const Vector3& a, const Vector3& b)
    {
        return Vector3::Distance(a, b) < 1e-2f;
    }

    StrideWarp::Settings MannequinSettings()
    {
        StrideWarp::Settings settings;
        settings.Pelvis = Pelvis;
        settings.Legs[0].IKFoot = IKFootL;
        settings.Legs[0].Foot = FootL;
        settings.Legs[0].Thigh = ThighL;
        settings.Legs[1].IKFoot = IKFootR;
        settings.Legs[1].Foot = FootR;
        settings.Legs[1].Thigh = ThighR;
        return settings;
    }

    Real Along(const Vector3& position, const Vector3& pelvis, const Vector3& direction)
    {
        return Vector3::Dot(position - pelvis, direction);
    }
}

TEST_CASE("StrideWarp")
{
    Array<SkeletonNode> nodes;
    SetupSkeleton(nodes);
    Transform input[Count];
    SetupPose(input);
    MatchIKFeet(nodes, input);
    Transform before[Count];
    for (int32 i = 0; i < Count; i++)
        before[i] = ModelTransform(nodes, input, i);
    const auto settings = MannequinSettings();
    const Float3 forward(0, 0, 1);

    SECTION("Scale 1 returns the input pose")
    {
        Transform pose[Count];
        Platform::MemoryCopy(pose, input, sizeof(input));
        const float drop = StrideWarp::GetPelvisDrop(nodes.Get(), Count, settings, forward, 1.0f, pose);
        CHECK(drop == 0.0f);
        StrideWarp::Warp(nodes.Get(), Count, settings, forward, 1.0f, drop, pose);
        for (int32 i = 0; i < Count; i++)
        {
            CHECK(pose[i].Translation == input[i].Translation);
            CHECK(pose[i].Orientation == input[i].Orientation);
            CHECK(pose[i].Scale == input[i].Scale);
        }
    }

    SECTION("Scale 1.5 puts the feet 1.5 times as far ahead and behind, on their IK targets")
    {
        for (const Float3 direction : { Float3(0, 0, 1), Float3(1, 0, 2), Float3(0, 0.5f, 1) })
        {
            Transform pose[Count];
            Platform::MemoryCopy(pose, input, sizeof(input));
            const float drop = StrideWarp::GetPelvisDrop(nodes.Get(), Count, settings, direction, 1.5f, pose);
            CHECK(drop > 0.0f);
            CHECK(drop < settings.MaxPelvisDrop);
            StrideWarp::Warp(nodes.Get(), Count, settings, direction, 1.5f, drop, pose);
            Transform after[Count];
            for (int32 i = 0; i < Count; i++)
                after[i] = ModelTransform(nodes, pose, i);
            const Vector3 dir = Vector3(direction.X, 0, direction.Z).GetNormalized();

            // The pelvis only lowers
            CHECK(SamePosition(after[Pelvis].Translation, before[Pelvis].Translation - Vector3(0, drop, 0)));
            CHECK(SameRotation(after[Pelvis].Orientation, before[Pelvis].Orientation));

            for (const int32 foot : { FootL, FootR })
            {
                const int32 ikFoot = foot == FootL ? IKFootL : IKFootR;
                const int32 thigh = foot == FootL ? ThighL : ThighR;
                const int32 calf = foot == FootL ? CalfL : CalfR;

                // IK feet scale their offset from the pelvis along the direction, and nothing else
                const Real alongBefore = Along(before[ikFoot].Translation, before[Pelvis].Translation, dir);
                const Real alongAfter = Along(after[ikFoot].Translation, before[Pelvis].Translation, dir);
                CHECK(Math::Abs(alongAfter - alongBefore * 1.5f) < 1e-2f);
                CHECK(SamePosition(after[ikFoot].Translation, before[ikFoot].Translation + dir * (alongBefore * 0.5f)));
                CHECK(SameRotation(after[ikFoot].Orientation, before[ikFoot].Orientation));

                // FK feet land on their IK feet, keeping their model space rotation
                CHECK(SamePosition(after[foot].Translation, after[ikFoot].Translation));
                CHECK(SameRotation(after[foot].Orientation, before[foot].Orientation));

                // Legs keep their bone lengths and reach no further than in the input pose
                const Real upperBefore = Vector3::Distance(before[calf].Translation, before[thigh].Translation);
                const Real lowerBefore = Vector3::Distance(before[foot].Translation, before[calf].Translation);
                CHECK(Math::Abs(Vector3::Distance(after[calf].Translation, after[thigh].Translation) - upperBefore) < 1e-2f);
                CHECK(Math::Abs(Vector3::Distance(after[foot].Translation, after[calf].Translation) - lowerBefore) < 1e-2f);
                CHECK(Vector3::Distance(after[foot].Translation, after[thigh].Translation) <= Vector3::Distance(before[foot].Translation, before[thigh].Translation) + 1e-2f);

                // Local scales are kept
                CHECK(Float3::NearEqual(pose[thigh].Scale, input[thigh].Scale));
                CHECK(Float3::NearEqual(pose[foot].Scale, input[foot].Scale));
            }
        }
    }

    SECTION("Scale below 1 shortens the stride without moving the pelvis")
    {
        Transform pose[Count];
        Platform::MemoryCopy(pose, input, sizeof(input));
        const float drop = StrideWarp::GetPelvisDrop(nodes.Get(), Count, settings, forward, 0.5f, pose);
        CHECK(drop == 0.0f);
        StrideWarp::Warp(nodes.Get(), Count, settings, forward, 0.5f, drop, pose);
        CHECK(pose[Pelvis].Translation == input[Pelvis].Translation);
        for (const int32 foot : { FootL, FootR })
        {
            const int32 ikFoot = foot == FootL ? IKFootL : IKFootR;
            const Transform after = ModelTransform(nodes, pose, foot);
            const Real alongBefore = Along(before[ikFoot].Translation, before[Pelvis].Translation, Vector3::UnitZ);
            CHECK(Math::Abs(Along(after.Translation, before[Pelvis].Translation, Vector3::UnitZ) - alongBefore * 0.5f) < 1e-2f);
            CHECK(SamePosition(after.Translation, ModelTransform(nodes, pose, ikFoot).Translation));
        }
    }

    SECTION("Without a pelvis drop, out of reach feet stop short without stretching")
    {
        auto noDrop = settings;
        noDrop.MaxPelvisDrop = 0.0f;
        Transform pose[Count];
        Platform::MemoryCopy(pose, input, sizeof(input));
        const float drop = StrideWarp::GetPelvisDrop(nodes.Get(), Count, noDrop, forward, 2.0f, pose);
        CHECK(drop == 0.0f);
        StrideWarp::Warp(nodes.Get(), Count, noDrop, forward, 2.0f, drop, pose);
        CHECK(pose[Pelvis].Translation == input[Pelvis].Translation);
        const Transform thigh = ModelTransform(nodes, pose, ThighL);
        const Transform calf = ModelTransform(nodes, pose, CalfL);
        const Transform foot = ModelTransform(nodes, pose, FootL);
        const Real upper = Vector3::Distance(before[CalfL].Translation, before[ThighL].Translation);
        const Real lower = Vector3::Distance(before[FootL].Translation, before[CalfL].Translation);
        CHECK(Math::Abs(Vector3::Distance(calf.Translation, thigh.Translation) - upper) < 1e-2f);
        CHECK(Math::Abs(Vector3::Distance(foot.Translation, calf.Translation) - lower) < 1e-2f);
        CHECK(Vector3::Distance(foot.Translation, ModelTransform(nodes, pose, IKFootL).Translation) > 1.0f);
    }

    SECTION("Pelvis drop is limited")
    {
        auto limited = settings;
        limited.MaxPelvisDrop = 2.0f;
        CHECK(Math::NearEqual(StrideWarp::GetPelvisDrop(nodes.Get(), Count, limited, forward, 1.5f, input), 2.0f));
        CHECK(StrideWarp::GetPelvisDrop(nodes.Get(), Count, settings, forward, 1.5f, input) > 2.0f);
    }

    SECTION("Legs without an IK foot use the foot for the stride")
    {
        auto fkOnly = settings;
        fkOnly.Legs[0].IKFoot = -1;
        fkOnly.Legs[1].IKFoot = -1;
        Transform pose[Count];
        Platform::MemoryCopy(pose, input, sizeof(input));
        const float drop = StrideWarp::GetPelvisDrop(nodes.Get(), Count, fkOnly, forward, 1.5f, pose);
        StrideWarp::Warp(nodes.Get(), Count, fkOnly, forward, 1.5f, drop, pose);
        const Transform foot = ModelTransform(nodes, pose, FootL);
        CHECK(Math::Abs(Along(foot.Translation, before[Pelvis].Translation, Vector3::UnitZ) - Along(before[FootL].Translation, before[Pelvis].Translation, Vector3::UnitZ) * 1.5f) < 1e-2f);
        CHECK(pose[IKFootL].Translation == input[IKFootL].Translation);
    }

    SECTION("Broken leg chains and missing nodes leave the pose alone")
    {
        auto broken = settings;
        broken.Legs[0].Thigh = CalfL; // Not the foot's grandparent
        broken.Legs[1].Thigh = -1;
        Transform pose[Count];
        Platform::MemoryCopy(pose, input, sizeof(input));
        StrideWarp::Warp(nodes.Get(), Count, broken, forward, 1.5f, 5.0f, pose);
        for (int32 i = 0; i < Count; i++)
            CHECK(pose[i].Translation == input[i].Translation);

        auto noPelvis = settings;
        noPelvis.Pelvis = -1;
        StrideWarp::Warp(nodes.Get(), Count, noPelvis, forward, 1.5f, 5.0f, pose);
        StrideWarp::Warp(nodes.Get(), Count, settings, Float3::UnitY, 1.5f, 5.0f, pose); // No direction on the ground
        for (int32 i = 0; i < Count; i++)
            CHECK(pose[i].Translation == input[i].Translation);
    }

    SECTION("Foot offset")
    {
        const Vector3 dir = Vector3::UnitZ;
        CHECK(SamePosition(StrideWarp::GetFootOffset(Vector3(10, 0, 30), Vector3(0, 95, 0), dir, 1.5f), Vector3(0, 0, 15)));
        CHECK(SamePosition(StrideWarp::GetFootOffset(Vector3(-10, 0, -20), Vector3(0, 95, 0), dir, 1.5f), Vector3(0, 0, -10)));
        CHECK(SamePosition(StrideWarp::GetFootOffset(Vector3(10, 0, 30), Vector3(0, 95, 0), dir, 1.0f), Vector3::Zero));
        CHECK(SamePosition(StrideWarp::GetFootOffset(Vector3(10, 0, 30), Vector3(0, 95, 0), dir, 0.5f), Vector3(0, 0, -15)));
    }

    SECTION("Ease")
    {
        CHECK(StrideWarp::Ease(1.0f, 1.5f, 0.0f, 0.016f) == 1.5f);
        CHECK(StrideWarp::Ease(1.0f, 1.5f, 10.0f, 0.0f) == 1.0f);
        CHECK(Math::NearEqual(StrideWarp::Ease(1.0f, 2.0f, 10.0f, 0.1f), 2.0f - Math::Exp(-1.0f), 1e-4f));

        // Frame rate independent: two half steps match one full step
        const float half = StrideWarp::Ease(StrideWarp::Ease(1.0f, 2.0f, 8.0f, 0.05f), 2.0f, 8.0f, 0.05f);
        CHECK(Math::NearEqual(half, StrideWarp::Ease(1.0f, 2.0f, 8.0f, 0.1f), 1e-4f));
    }
}
