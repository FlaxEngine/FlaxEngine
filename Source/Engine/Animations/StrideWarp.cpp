// Copyright (c) Wojciech Figat. All rights reserved.

#include "StrideWarp.h"
#include "InverseKinematics.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/Models/SkeletonData.h"

namespace
{
    typedef Array<Transform, InlinedAllocation<96>> ModelPose;

    struct LegChain
    {
        int32 StrideNode; // The IK foot, or the foot when the leg has no IK foot
        int32 IKFoot;
        int32 Foot;
        int32 Calf;
        int32 Thigh;
    };

    bool IsValid(int32 index, int32 nodesCount)
    {
        return index >= 0 && index < nodesCount;
    }

    // Gets the travel direction on the ground plane (model Y up), false when it has no length there
    bool GetGroundDirection(const Float3& direction, Vector3& result)
    {
        result = Vector3((Real)direction.X, 0, (Real)direction.Z);
        const Real length = result.Length();
        if (length < ZeroTolerance)
            return false;
        result /= length;
        return true;
    }

    void ComputeModelPose(const SkeletonNode* nodes, int32 nodesCount, const Transform* pose, ModelPose& model)
    {
        model.Resize(nodesCount, false);
        for (int32 i = 0; i < nodesCount; i++)
        {
            const int32 parent = nodes[i].ParentIndex;
            model[i] = parent >= 0 && parent < i ? model[parent].LocalToWorld(pose[i]) : pose[i];
        }
    }

    // Gets the legs that can be solved (the foot's parent is the calf and the calf's parent is the thigh)
    int32 GetLegs(const SkeletonNode* nodes, int32 nodesCount, const StrideWarp::Settings& settings, LegChain legs[2])
    {
        int32 count = 0;
        for (const StrideWarp::Leg& leg : settings.Legs)
        {
            if (!IsValid(leg.Foot, nodesCount) || !IsValid(leg.Thigh, nodesCount))
                continue;
            const int32 calf = nodes[leg.Foot].ParentIndex;
            if (!IsValid(calf, nodesCount) || nodes[calf].ParentIndex != leg.Thigh || leg.Thigh == settings.Pelvis)
                continue;
            LegChain& chain = legs[count++];
            chain.IKFoot = IsValid(leg.IKFoot, nodesCount) ? leg.IKFoot : -1;
            chain.StrideNode = chain.IKFoot != -1 ? chain.IKFoot : leg.Foot;
            chain.Foot = leg.Foot;
            chain.Calf = calf;
            chain.Thigh = leg.Thigh;
        }
        return count;
    }

    // Gets how far the hip has to lower so it is no further than the reach from the foot (only lowering helps, and only when the foot is below the hip)
    Real GetRequiredDrop(const Vector3& hipToFoot, Real reach)
    {
        const Real lengthSq = hipToFoot.LengthSquared();
        if (lengthSq <= reach * reach)
            return 0;
        const Real up = hipToFoot.Y;
        if (up >= 0)
            return 0;
        const Real groundSq = Math::Max<Real>(lengthSq - up * up, 0);
        if (groundSq >= reach * reach)
            return -up; // Can't reach from any height, get as close as possible (hip level with the foot)
        return -up - Math::Sqrt(reach * reach - groundSq);
    }

    void SetModelTransform(const SkeletonNode* nodes, int32 index, const Transform* model, const Transform& target, Transform* pose)
    {
        const int32 parent = nodes[index].ParentIndex;
        const Float3 scale = pose[index].Scale;
        pose[index] = parent >= 0 && parent < index ? model[parent].WorldToLocal(target) : target;
        pose[index].Orientation.Normalize();
        pose[index].Scale = scale;
    }
}

Vector3 StrideWarp::GetFootOffset(const Vector3& footPosition, const Vector3& pelvisPosition, const Vector3& direction, float strideScale)
{
    const Real along = Vector3::Dot(footPosition - pelvisPosition, direction);
    return direction * (along * (Real)(strideScale - 1.0f));
}

float StrideWarp::GetPelvisDrop(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings, const Float3& direction, float strideScale, const Transform* pose)
{
    Vector3 dir;
    if (settings.MaxPelvisDrop <= 0.0f || !IsValid(settings.Pelvis, nodesCount) || !GetGroundDirection(direction, dir) || Math::NearEqual(strideScale, 1.0f))
        return 0.0f;
    LegChain legs[2];
    const int32 legsCount = GetLegs(nodes, nodesCount, settings, legs);
    if (legsCount == 0)
        return 0.0f;
    ModelPose model;
    ComputeModelPose(nodes, nodesCount, pose, model);

    // Each leg keeps reaching no further from its thigh than in the input pose, so it keeps its knee bend
    const Vector3 pelvis = model[settings.Pelvis].Translation;
    Real drop = 0;
    for (int32 i = 0; i < legsCount; i++)
    {
        const LegChain& leg = legs[i];
        const Vector3 hip = model[leg.Thigh].Translation;
        const Vector3 foot = model[leg.Foot].Translation;
        const Vector3 offset = GetFootOffset(model[leg.StrideNode].Translation, pelvis, dir, strideScale);
        const Real reach = Vector3::Distance(foot, hip);
        drop = Math::Max(drop, GetRequiredDrop(foot + offset - hip, reach));
    }
    return Math::Min((float)drop, settings.MaxPelvisDrop);
}

float StrideWarp::Ease(float current, float target, float speed, float deltaTime)
{
    if (speed <= 0.0f || deltaTime <= 0.0f)
        return speed <= 0.0f ? target : current;
    return Math::Lerp(current, target, 1.0f - Math::Exp(-speed * deltaTime));
}

void StrideWarp::Warp(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings, const Float3& direction, float strideScale, float pelvisDrop, Transform* pose)
{
    Vector3 dir;
    if (!IsValid(settings.Pelvis, nodesCount) || !GetGroundDirection(direction, dir))
        return;
    if (Math::NearEqual(strideScale, 1.0f) && pelvisDrop <= 0.0f)
        return;
    LegChain legs[2];
    const int32 legsCount = GetLegs(nodes, nodesCount, settings, legs);
    if (legsCount == 0)
        return;
    ModelPose model;
    ComputeModelPose(nodes, nodesCount, pose, model);

    // Warped foot targets, fixed to the ground (they don't follow the pelvis down)
    const Vector3 pelvis = model[settings.Pelvis].Translation;
    Transform ikTargets[2];
    Transform footTargets[2];
    for (int32 i = 0; i < legsCount; i++)
    {
        const LegChain& leg = legs[i];
        const Vector3 offset = GetFootOffset(model[leg.StrideNode].Translation, pelvis, dir, strideScale);
        if (leg.IKFoot != -1)
        {
            ikTargets[i] = model[leg.IKFoot];
            ikTargets[i].Translation += offset;
        }
        footTargets[i] = model[leg.Foot];
        footTargets[i].Translation += offset;
    }

    // Lower the pelvis
    if (pelvisDrop > 0.0f)
    {
        Transform target = model[settings.Pelvis];
        target.Translation.Y -= pelvisDrop;
        SetModelTransform(nodes, settings.Pelvis, model.Get(), target, pose);
        ComputeModelPose(nodes, nodesCount, pose, model);
    }

    // Move the IK feet
    bool movedIKFeet = false;
    for (int32 i = 0; i < legsCount; i++)
    {
        if (legs[i].IKFoot != -1)
        {
            SetModelTransform(nodes, legs[i].IKFoot, model.Get(), ikTargets[i], pose);
            movedIKFeet = true;
        }
    }
    if (movedIKFeet)
        ComputeModelPose(nodes, nodesCount, pose, model);

    // Solve each leg onto its foot target, keeping the knee bending the way it did
    for (int32 i = 0; i < legsCount; i++)
    {
        const LegChain& leg = legs[i];
        Transform thigh = model[leg.Thigh];
        Transform calf = model[leg.Calf];
        Transform foot = model[leg.Foot];
        Vector3 target = footTargets[i].Translation;

        // Stop short of targets out of reach, rather than stretching the leg
        const Real legLength = Vector3::Distance(calf.Translation, thigh.Translation) + Vector3::Distance(foot.Translation, calf.Translation);
        const Vector3 toTarget = target - thigh.Translation;
        const Real toTargetLength = toTarget.Length();
        const Real maxReach = legLength * 0.9999f;
        if (toTargetLength > maxReach)
            target = thigh.Translation + toTarget * (maxReach / toTargetLength);

        InverseKinematics::SolveTwoBoneIK(thigh, calf, foot, target, calf.Translation);
        thigh.Orientation.Normalize();
        calf.Orientation.Normalize();
        foot.Orientation = footTargets[i].Orientation;

        SetModelTransform(nodes, leg.Thigh, model.Get(), thigh, pose);
        model[leg.Thigh] = thigh;
        SetModelTransform(nodes, leg.Calf, model.Get(), calf, pose);
        model[leg.Calf] = calf;
        SetModelTransform(nodes, leg.Foot, model.Get(), foot, pose);
        model[leg.Foot] = foot;
    }
}
