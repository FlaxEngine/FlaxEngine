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

    // A mid-stride pose: left foot about 49 ahead of the hip, right foot about 44 behind, both near the ground with bent knees, and the IK feet on the feet
    void SetupPose(Transform* pose)
    {
        pose[Root] = Transform(Vector3::Zero);
        pose[Pelvis] = Transform(Vector3(0, 95, 0), Quaternion::Euler(0, 4, 2));
        pose[ThighL] = Transform(Vector3(10, -5, 0), Quaternion::Euler(-20, 0, 0));
        pose[CalfL] = Transform(Vector3(0, -45, 5), Quaternion::Euler(15, 0, 0));
        pose[FootL] = Transform(Vector3(0, -45, 25), Quaternion::Euler(10, 5, 0));
        pose[ThighR] = Transform(Vector3(-10, -5, 0), Quaternion::Euler(10, 0, 0));
        pose[CalfR] = Transform(Vector3(0, -45, -2), Quaternion::Euler(5, 0, 0));
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

    SECTION("Knees keep bending forward at long strides")
    {
        // Slightly bent legs (about 14 degrees at the knee) with identity rotations, so local translations are model space offsets
        const Vector3 pelvisPos(0, 90, 0);
        Transform pose[Count];
        for (int32 i = 0; i < Count; i++)
            pose[i] = Transform::Identity;
        pose[Pelvis].Translation = pelvisPos;
        Vector3 hips[2], knees[2], feet[2];
        for (int32 side = 0; side < 2; side++)
        {
            hips[side] = Vector3(side == 0 ? 10 : -10, 90, 0);
            feet[side] = Vector3(side == 0 ? 10 : -10, 5, side == 0 ? 20 : -20);
            const Vector3 line = feet[side] - hips[side];
            const Real half = line.Length() * 0.5f;
            const Vector3 bend = Vector3::Cross(line, Vector3::UnitX).GetNormalized(); // Forward and up, away from the hip-to-foot line
            knees[side] = hips[side] + line * 0.5f + bend * Math::Sqrt(45.0f * 45.0f - half * half);
            const int32 thigh = side == 0 ? ThighL : ThighR;
            pose[thigh].Translation = hips[side] - pelvisPos;
            pose[thigh + 1].Translation = knees[side] - hips[side];
            pose[thigh + 2].Translation = feet[side] - knees[side];
        }
        auto fkOnly = settings;
        fkOnly.Legs[0].IKFoot = -1;
        fkOnly.Legs[1].IKFoot = -1;
        for (const float scale : { 1.5f, 2.0f, 2.25f, 2.5f })
        {
            Transform warped[Count];
            Platform::MemoryCopy(warped, pose, sizeof(pose));
            const float drop = StrideWarp::GetPelvisDrop(nodes.Get(), Count, fkOnly, forward, scale, warped);
            CHECK(drop < fkOnly.MaxPelvisDrop);
            StrideWarp::Warp(nodes.Get(), Count, fkOnly, forward, scale, drop, warped);
            for (int32 side = 0; side < 2; side++)
            {
                const int32 thigh = side == 0 ? ThighL : ThighR;
                const Vector3 hip = ModelTransform(nodes, warped, thigh).Translation;
                const Vector3 knee = ModelTransform(nodes, warped, thigh + 1).Translation;
                const Vector3 foot = ModelTransform(nodes, warped, thigh + 2).Translation;
                const Vector3 target(feet[side].X, feet[side].Y, feet[side].Z * scale);
                CHECK(SamePosition(foot, target));

                // The knee stays on the same side of the hip-to-foot line (bent forward, not backward)
                const Real sideBefore = Vector3::Dot(Vector3::Cross(feet[side] - hips[side], knees[side] - hips[side]), Vector3::UnitX);
                const Real sideAfter = Vector3::Dot(Vector3::Cross(foot - hip, knee - hip), Vector3::UnitX);
                CHECK(sideBefore < 0);
                CHECK(sideAfter < 0);
                CHECK(knee.Z > (hip.Z + foot.Z) * 0.5f);
            }
        }
    }

    // A planted leg plus a swing leg with its foot raised behind the hip
    const auto setupSwingPose = [](Transform* pose, const Vector3& swingFoot)
    {
        const Vector3 pelvisPos(0, 90, 0);
        const Vector3 hip(-10, 90, 0);
        for (int32 i = 0; i < Count; i++)
            pose[i] = Transform::Identity;
        pose[Pelvis].Translation = pelvisPos;
        pose[ThighL].Translation = Vector3(10, 0, 0);
        pose[CalfL].Translation = Vector3(0, -42, 5);
        pose[FootL].Translation = Vector3(0, -42, -5);

        // The swing knee is forward of the hip-to-foot line, with 45 long bones
        const Vector3 line = swingFoot - hip;
        const Real half = line.Length() * 0.5f;
        const Vector3 knee = hip + line * 0.5f + Vector3::Cross(line, Vector3::UnitX).GetNormalized() * Math::Sqrt(45.0f * 45.0f - half * half);
        pose[ThighR].Translation = hip - pelvisPos;
        pose[CalfR].Translation = knee - hip;
        pose[FootR].Translation = swingFoot - knee;
    };

    SECTION("Raised feet don't drive the pelvis drop")
    {
        auto fkOnly = settings;
        fkOnly.Legs[0].IKFoot = -1;
        fkOnly.Legs[1].IKFoot = -1;

        // Near hip height behind the hip, the stride moves the foot further from the hip when the pelvis is ahead of it
        Transform pose[Count];
        setupSwingPose(pose, Vector3(-10, 70, -30));
        CHECK(StrideWarp::GetPelvisDrop(nodes.Get(), Count, fkOnly, forward, 1.3f, pose) == 0.0f);

        // A planted foot still does
        setupSwingPose(pose, Vector3(-10, 8, -30));
        CHECK(StrideWarp::GetPelvisDrop(nodes.Get(), Count, fkOnly, forward, 1.5f, pose) > 0.5f);

        // Lifting the foot fades the drop out without a jump
        float last = StrideWarp::GetPelvisDrop(nodes.Get(), Count, fkOnly, forward, 1.5f, pose);
        for (float height = 8.0f; height < 50.0f; height += 0.5f)
        {
            setupSwingPose(pose, Vector3(-10, height, -30));
            const float drop = StrideWarp::GetPelvisDrop(nodes.Get(), Count, fkOnly, forward, 1.5f, pose);
            CHECK(Math::Abs(drop - last) < 2.0f);
            last = drop;
        }
        CHECK(last == 0.0f);
    }

    SECTION("Knees don't snap sideways when the hip-to-foot line swings around")
    {
        // A foot raised next to the hip, whose target ends up on the other side of the hip: the line turns nearly all the way around, and which way it turns changes with the foot height
        auto fkOnly = settings;
        fkOnly.Legs[0].IKFoot = -1;
        fkOnly.Legs[1].IKFoot = -1;
        const Vector3 hip(-10, 90, -12);
        const Vector3 kneeOffset(0, -20, 40);
        Vector3 lastKnee;
        bool first = true;
        for (float height = -1.5f; height <= 0.5f; height += 0.05f)
        {
            Transform pose[Count];
            setupSwingPose(pose, Vector3(-10, 8, 0));
            pose[ThighR].Translation = hip - Vector3(0, 90, 0);
            pose[CalfR].Translation = kneeOffset;
            pose[FootR].Translation = Vector3(0.1f, height, -2.5f) - kneeOffset;
            StrideWarp::Warp(nodes.Get(), Count, fkOnly, forward, 0.65f, 0.0f, pose);
            const Vector3 knee = ModelTransform(nodes, pose, CalfR).Translation;
            CHECK(Vector3::Distance(ModelTransform(nodes, pose, FootR).Translation, Vector3(-9.9f, 90 + height, -9.425f)) < 1e-2f);
            if (!first)
                CHECK(Vector3::Distance(knee, lastKnee) < 5.0f);
            lastKnee = knee;
            first = false;
        }
    }

    SECTION("Straight legs bend the knee forward")
    {
        auto fkOnly = settings;
        fkOnly.Legs[0].IKFoot = -1;
        fkOnly.Legs[1].IKFoot = -1;
        Transform pose[Count];
        setupSwingPose(pose, Vector3(-10, 8, 0));
        const Vector3 line(0, -80, -30);
        pose[CalfR].Translation = line * 0.5f;
        pose[FootR].Translation = line * 0.5f;
        StrideWarp::Warp(nodes.Get(), Count, fkOnly, forward, 0.5f, 0.0f, pose);
        const Vector3 hip = ModelTransform(nodes, pose, ThighR).Translation;
        const Vector3 knee = ModelTransform(nodes, pose, CalfR).Translation;
        const Vector3 foot = ModelTransform(nodes, pose, FootR).Translation;
        CHECK(SamePosition(foot, Vector3(-10, 10, -15)));
        CHECK(knee.Z > (hip.Z + foot.Z) * 0.5f + 1.0f);
    }

    SECTION("With both feet raised the lower one still drives the pelvis drop")
    {
        auto fkOnly = settings;
        fkOnly.Legs[0].IKFoot = -1;
        fkOnly.Legs[1].IKFoot = -1;
        Transform pose[Count];
        setupSwingPose(pose, Vector3(-10, 50, -40));
        pose[CalfL].Translation = Vector3(0, -20, 5);
        pose[FootL].Translation = Vector3(0, -20, -5);
        CHECK(StrideWarp::GetPelvisDrop(nodes.Get(), Count, fkOnly, forward, 1.5f, pose) > 1.0f);
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
