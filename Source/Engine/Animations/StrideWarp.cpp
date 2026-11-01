// Copyright (c) Wojciech Figat. All rights reserved.

#include "StrideWarp.h"
#include "InverseKinematics.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/Models/SkeletonData.h"

namespace
{
    // Squared sine of the angle below which two directions are taken as parallel
    constexpr Real MinBendSineSq = 1e-6f;

    // Sine of the angle between the thigh and the hip-to-foot line: at or below StraightLegSine the leg's bend plane is noise and the hinge across the hips takes over, at or above BentLegSine the input bend plane is used
    constexpr float StraightLegSine = 0.05f;
    constexpr float BentLegSine = 0.3f;

    // The most the pelvis lowers per unit a foot is out of reach. It keeps the drop from jumping as a foot nears the edge of what lowering can reach, the foot stops short instead.
    constexpr Real MaxDropPerStretch = 2.0f;

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

    // Gets a node's model space transformation by walking up its ancestors (only the few nodes the warp needs, not the whole skeleton)
    Transform GetModelTransform(const SkeletonNode* nodes, const Transform* pose, int32 index)
    {
        Transform result = pose[index];
        for (int32 parent = nodes[index].ParentIndex; parent >= 0 && parent < index; parent = nodes[parent].ParentIndex)
        {
            result = pose[parent].LocalToWorld(result);
            index = parent;
        }
        return result;
    }

    Transform GetParentModelTransform(const SkeletonNode* nodes, const Transform* pose, int32 index)
    {
        const int32 parent = nodes[index].ParentIndex;
        return parent >= 0 && parent < index ? GetModelTransform(nodes, pose, parent) : Transform::Identity;
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
        const Real limit = (Math::Sqrt(lengthSq) - reach) * MaxDropPerStretch;
        if (groundSq >= reach * reach)
            return Math::Min(-up, limit); // Can't reach from any height, get as close as possible (hip level with the foot)
        return Math::Min(-up - Math::Sqrt(reach * reach - groundSq), limit);
    }

    // Gets how much a foot counts toward the pelvis drop: fully when near the lowest foot, not at all once raised well above it (a swing foot lands wherever the leg can reach, so it must not pull the pelvis down)
    float GetGroundedWeight(Real heightAboveLowest, Real legLength)
    {
        constexpr float GroundedHeight = 0.1f; // In leg lengths
        constexpr float RaisedHeight = 0.3f; // In leg lengths
        if (legLength < ZeroTolerance)
            return 0.0f;
        return 1.0f - Math::Saturate(((float)(heightAboveLowest / legLength) - GroundedHeight) / (RaisedHeight - GroundedHeight));
    }

    void SetModelTransform(int32 index, const Transform& parentModel, const Transform& target, Transform* pose)
    {
        const Float3 scale = pose[index].Scale;
        pose[index] = parentModel.WorldToLocal(target);
        pose[index].Orientation.Normalize();
        pose[index].Scale = scale;
    }
}

Vector3 StrideWarp::GetFootOffset(const Vector3& footPosition, const Vector3& pelvisPosition, const Vector3& direction, float strideScale)
{
    const Real along = Vector3::Dot(footPosition - pelvisPosition, direction);
    return direction * (along * (Real)(strideScale - 1.0f));
}

bool StrideWarp::CanWarp(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings)
{
    LegChain legs[2];
    return IsValid(settings.Pelvis, nodesCount) && GetLegs(nodes, nodesCount, settings, legs) != 0;
}

float StrideWarp::GetPelvisDrop(const SkeletonNode* nodes, int32 nodesCount, const Settings& settings, const Float3& direction, float strideScale, const Transform* pose)
{
    Vector3 dir;
    if (settings.MaxPelvisDrop <= 0.0f || !IsValid(settings.Pelvis, nodesCount) || !GetGroundDirection(direction, dir) || Math::NearEqual(strideScale, 1.0f))
        return 0.0f;
    LegChain legs[2];
    const int32 legsCount = GetLegs(nodes, nodesCount, settings, legs);

    // Each leg keeps reaching no further from its thigh than in the input pose, so it keeps its knee bend
    const Vector3 pelvis = GetModelTransform(nodes, pose, settings.Pelvis).Translation;
    Vector3 hips[2], feet[2], offsets[2];
    Real legLengths[2];
    Real lowest = MAX_Real;
    for (int32 i = 0; i < legsCount; i++)
    {
        const LegChain& leg = legs[i];
        const Transform thigh = GetModelTransform(nodes, pose, leg.Thigh);
        const Transform calf = thigh.LocalToWorld(pose[leg.Calf]);
        hips[i] = thigh.Translation;
        feet[i] = calf.LocalToWorld(pose[leg.Foot]).Translation;
        legLengths[i] = Vector3::Distance(calf.Translation, hips[i]) + Vector3::Distance(feet[i], calf.Translation);
        const Vector3 stridePosition = leg.IKFoot != -1 ? GetModelTransform(nodes, pose, leg.IKFoot).Translation : feet[i];
        offsets[i] = GetFootOffset(stridePosition, pelvis, dir, strideScale);
        lowest = Math::Min(lowest, feet[i].Y);
    }
    Real drop = 0;
    for (int32 i = 0; i < legsCount; i++)
    {
        const Real reach = Vector3::Distance(feet[i], hips[i]);
        const Real required = GetRequiredDrop(feet[i] + offsets[i] - hips[i], reach);
        drop = Math::Max(drop, required * GetGroundedWeight(feet[i].Y - lowest, legLengths[i]));
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

    // The input legs and the warped foot targets, fixed to the ground (they don't follow the pelvis down)
    const Transform pelvisModel = GetModelTransform(nodes, pose, settings.Pelvis);
    Transform ikTargets[2], footTargets[2];
    Vector3 inputHips[2], inputKnees[2], inputFeet[2];
    for (int32 i = 0; i < legsCount; i++)
    {
        const LegChain& leg = legs[i];
        const Transform thigh = GetModelTransform(nodes, pose, leg.Thigh);
        const Transform calf = thigh.LocalToWorld(pose[leg.Calf]);
        const Transform foot = calf.LocalToWorld(pose[leg.Foot]);
        inputHips[i] = thigh.Translation;
        inputKnees[i] = calf.Translation;
        inputFeet[i] = foot.Translation;
        Transform ikFoot = foot;
        if (leg.IKFoot != -1)
            ikFoot = GetModelTransform(nodes, pose, leg.IKFoot);
        const Vector3 offset = GetFootOffset(ikFoot.Translation, pelvisModel.Translation, dir, strideScale);
        ikTargets[i] = ikFoot;
        ikTargets[i].Translation += offset;
        footTargets[i] = foot;
        footTargets[i].Translation += offset;
    }

    // Lower the pelvis
    if (pelvisDrop > 0.0f)
    {
        Transform target = pelvisModel;
        target.Translation.Y -= pelvisDrop;
        SetModelTransform(settings.Pelvis, GetParentModelTransform(nodes, pose, settings.Pelvis), target, pose);
    }

    // Move the IK feet
    for (int32 i = 0; i < legsCount; i++)
    {
        if (legs[i].IKFoot != -1)
            SetModelTransform(legs[i].IKFoot, GetParentModelTransform(nodes, pose, legs[i].IKFoot), ikTargets[i], pose);
    }

    // The knee hinge for legs too straight to show their own: across the hips, with knees bending toward model forward
    Vector3 hipsHinge = legsCount == 2 ? inputHips[1] - inputHips[0] : Vector3::UnitX;
    hipsHinge.Y = 0;
    hipsHinge = hipsHinge.LengthSquared() > ZeroTolerance ? hipsHinge.GetNormalized() : Vector3::UnitX;
    if (hipsHinge.X > 0)
        hipsHinge = -hipsHinge;

    // Solve each leg onto its foot target
    for (int32 i = 0; i < legsCount; i++)
    {
        const LegChain& leg = legs[i];
        const Transform thighParent = GetParentModelTransform(nodes, pose, leg.Thigh);
        Transform thigh = thighParent.LocalToWorld(pose[leg.Thigh]);
        Transform calf = thigh.LocalToWorld(pose[leg.Calf]);
        Transform foot = calf.LocalToWorld(pose[leg.Foot]);
        Vector3 target = footTargets[i].Translation;

        // Stop short of targets out of reach, rather than stretching the leg
        const Real legLength = Vector3::Distance(calf.Translation, thigh.Translation) + Vector3::Distance(foot.Translation, calf.Translation);
        const Vector3 toTarget = target - thigh.Translation;
        const Real toTargetLength = toTarget.Length();
        const Real maxReach = legLength * 0.9999f;
        if (toTargetLength > maxReach)
            target = thigh.Translation + toTarget * (maxReach / toTargetLength);

        // Keep the knee bending the way it did: stay in the input bend plane, on the same side of the hip-to-foot line. Turning the knee along with the line instead would snap it sideways when the line swings a long way (a raised foot near the hip).
        const Vector3 inputKnee = inputKnees[i] - inputHips[i];
        const Vector3 inputLine = inputFeet[i] - inputHips[i];
        const Vector3 newLine = target - thigh.Translation;
        const Vector3 inputNormal = Vector3::Cross(inputLine, inputKnee);
        const Real inputNormalLength = inputNormal.Length();
        const Real inputLengths = inputLine.Length() * inputKnee.Length();
        const float bent = inputLengths > ZeroTolerance ? Math::Saturate(((float)(inputNormalLength / inputLengths) - StraightLegSine) / (BentLegSine - StraightLegSine)) : 0.0f;
        Vector3 bendNormal = hipsHinge * (Real)(1.0f - bent);
        if (bent > 0.0f)
            bendNormal += inputNormal * ((Real)bent / inputNormalLength);
        const Vector3 bend = Vector3::Cross(bendNormal, newLine);
        const Vector3 pole = thigh.Translation + (bend.LengthSquared() > MinBendSineSq * bendNormal.LengthSquared() * newLine.LengthSquared() ? bend : inputKnee);

        InverseKinematics::SolveTwoBoneIK(thigh, calf, foot, target, pole);
        thigh.Orientation.Normalize();
        calf.Orientation.Normalize();
        foot.Orientation = footTargets[i].Orientation;

        SetModelTransform(leg.Thigh, thighParent, thigh, pose);
        SetModelTransform(leg.Calf, thigh, calf, pose);
        SetModelTransform(leg.Foot, calf, foot, pose);
    }
}
