// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Animations/LayeredBlend.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Graphics/Models/SkeletonData.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // Skeleton: 0 root, 1 pelvis, 2 spine, 3 chest, 4 hand, 5 thigh (child of pelvis)
    enum
    {
        Root,
        Pelvis,
        Spine,
        Chest,
        Hand,
        Thigh,
        Count
    };

    void SetupSkeleton(Array<SkeletonNode>& nodes)
    {
        const int32 parents[Count] = { -1, Root, Pelvis, Spine, Chest, Pelvis };
        nodes.Resize(Count);
        for (int32 i = 0; i < Count; i++)
        {
            nodes[i].ParentIndex = parents[i];
            nodes[i].LocalTransform = Transform(Vector3(0, 10, 0));
        }
    }

    void SetupPose(Transform* pose)
    {
        for (int32 i = 0; i < Count; i++)
            pose[i] = Transform(Vector3(0, 10, 0));
    }

    Quaternion ModelRotation(const Array<SkeletonNode>& nodes, const Transform* pose, int32 index)
    {
        Quaternion result = pose[index].Orientation;
        for (int32 i = nodes[index].ParentIndex; i != -1; i = nodes[i].ParentIndex)
            result = pose[i].Orientation * result;
        result.Normalize();
        return result;
    }

    bool SameRotation(const Quaternion& a, const Quaternion& b)
    {
        return Math::Abs(Quaternion::Dot(a, b)) > 1.0f - 1e-4f;
    }
}

TEST_CASE("LayeredBlend")
{
    Array<SkeletonNode> nodes;
    SetupSkeleton(nodes);

    // Base: the walk twists the pelvis and the spine
    Transform basePose[Count];
    SetupPose(basePose);
    basePose[Pelvis].Orientation = Quaternion::Euler(0, 40, 5);
    basePose[Spine].Orientation = Quaternion::Euler(0, -15, 0);
    basePose[Chest].Orientation = Quaternion::Euler(3, 10, 0);
    basePose[Thigh].Orientation = Quaternion::Euler(30, 0, 0);

    // Layer: a still upper body hold
    Transform layerPose[Count];
    SetupPose(layerPose);
    layerPose[Chest].Orientation = Quaternion::Euler(20, 0, 0);
    layerPose[Hand].Orientation = Quaternion::Euler(0, 0, 45);
    layerPose[Chest].Translation = Vector3(0, 12, 0);

    SECTION("Weights")
    {
        float weights[Count];

        // Branch root with a full weight
        LayeredBlend::ComputeWeights(nodes.Get(), Count, Spine, 0, nullptr, weights);
        CHECK(weights[Root] == 0.0f);
        CHECK(weights[Pelvis] == 0.0f);
        CHECK(weights[Spine] == 1.0f);
        CHECK(weights[Chest] == 1.0f);
        CHECK(weights[Hand] == 1.0f);
        CHECK(weights[Thigh] == 0.0f);

        // Branch root with a ramp up the chain
        LayeredBlend::ComputeWeights(nodes.Get(), Count, Spine, 4, nullptr, weights);
        CHECK(Math::NearEqual(weights[Spine], 0.25f));
        CHECK(Math::NearEqual(weights[Chest], 0.5f));
        CHECK(Math::NearEqual(weights[Hand], 0.75f));
        CHECK(weights[Thigh] == 0.0f);

        // Mask limits the branch
        BitArray<> mask;
        mask.Resize(Count);
        mask.SetAll(true);
        mask.Set(Hand, false);
        LayeredBlend::ComputeWeights(nodes.Get(), Count, Spine, 0, &mask, weights);
        CHECK(weights[Chest] == 1.0f);
        CHECK(weights[Hand] == 0.0f);

        // Mask alone
        mask.SetAll(false);
        mask.Set(Chest, true);
        LayeredBlend::ComputeWeights(nodes.Get(), Count, -1, 0, &mask, weights);
        CHECK(weights[Chest] == 1.0f);
        CHECK(weights[Spine] == 0.0f);
        CHECK(weights[Thigh] == 0.0f);

        // Neither
        LayeredBlend::ComputeWeights(nodes.Get(), Count, -1, 0, nullptr, weights);
        for (int32 i = 0; i < Count; i++)
            CHECK(weights[i] == 1.0f);
    }

    SECTION("Mesh Space Rotation")
    {
        float weights[Count];
        Transform result[Count];
        LayeredBlend::ComputeWeights(nodes.Get(), Count, Spine, 0, nullptr, weights);
        LayeredBlend::Blend(nodes.Get(), Count, basePose, layerPose, weights, 1.0f, true, result);

        // Layered nodes keep the layer's model space rotation despite the twisted pelvis
        CHECK(SameRotation(ModelRotation(nodes, result, Spine), ModelRotation(nodes, layerPose, Spine)));
        CHECK(SameRotation(ModelRotation(nodes, result, Chest), ModelRotation(nodes, layerPose, Chest)));
        CHECK(SameRotation(ModelRotation(nodes, result, Hand), ModelRotation(nodes, layerPose, Hand)));
        CHECK(Vector3::NearEqual(result[Chest].Translation, layerPose[Chest].Translation));

        // Nodes outside the branch keep the base pose
        CHECK(result[Pelvis] == basePose[Pelvis]);
        CHECK(result[Thigh] == basePose[Thigh]);
        CHECK(SameRotation(ModelRotation(nodes, result, Pelvis), ModelRotation(nodes, basePose, Pelvis)));

        // Partial alpha blends the model space rotation
        LayeredBlend::Blend(nodes.Get(), Count, basePose, layerPose, weights, 0.3f, true, result);
        Quaternion expected;
        Quaternion::Slerp(ModelRotation(nodes, basePose, Chest), ModelRotation(nodes, layerPose, Chest), 0.3f, expected);
        expected.Normalize();
        CHECK(SameRotation(ModelRotation(nodes, result, Chest), expected));

        // Zero alpha gives the base pose
        LayeredBlend::Blend(nodes.Get(), Count, basePose, layerPose, weights, 0.0f, true, result);
        for (int32 i = 0; i < Count; i++)
            CHECK(result[i] == basePose[i]);
    }

    SECTION("Local Space Matches Blend With Mask")
    {
        BitArray<> mask;
        mask.Resize(Count);
        mask.SetAll(false);
        mask.Set(Spine, true);
        mask.Set(Chest, true);
        mask.Set(Hand, true);
        float weights[Count];
        Transform result[Count];
        LayeredBlend::ComputeWeights(nodes.Get(), Count, -1, 0, &mask, weights);
        const float alpha = 0.6f;
        LayeredBlend::Blend(nodes.Get(), Count, basePose, layerPose, weights, alpha, false, result);
        for (int32 i = 0; i < Count; i++)
        {
            // Same math as the Blend with Mask node
            Transform expected = basePose[i];
            if (mask[i])
                Transform::Lerp(basePose[i], layerPose[i], alpha, expected);
            CHECK(result[i] == expected);
        }

        // Local space blend inherits the pelvis twist, so the chest doesn't match the layer
        LayeredBlend::Blend(nodes.Get(), Count, basePose, layerPose, weights, 1.0f, false, result);
        CHECK(!SameRotation(ModelRotation(nodes, result, Chest), ModelRotation(nodes, layerPose, Chest)));
    }
}
