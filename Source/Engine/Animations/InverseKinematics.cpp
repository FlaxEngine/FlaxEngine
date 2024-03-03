// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "InverseKinematics.h"

void InverseKinematics::SolveAimIK(const Transform& node, const Vector3& target, Quaternion& outNodeCorrection)
{
    Vector3 toTarget = target - node.Translation;
    toTarget.Normalize();
    const Vector3 fromNode = Vector3::Forward;
    Quaternion::FindBetween(fromNode, toTarget, outNodeCorrection);
}

void InverseKinematics::SolveTwoBoneIK(Transform& rootNode, Transform& jointNode, Transform& targetNode, const Vector3& target, const Vector3& jointTarget, bool allowStretching, float maxStretchScale)
{
    Real lowerLimbLength = (targetNode.Translation - jointNode.Translation).Length();
    Real upperLimbLength = (jointNode.Translation - rootNode.Translation).Length();
    Vector3 jointPos = jointNode.Translation;
    Vector3 desiredDelta = target - rootNode.Translation;
    Real desiredLength = desiredDelta.Length();
    Real limbLengthLimit = lowerLimbLength + upperLimbLength;

    Vector3 desiredDir;
    if (desiredLength < ZeroTolerance)
    {
        desiredLength = ZeroTolerance;
        desiredDir = Vector3(1, 0, 0);
    }
    else
    {
        desiredDir = desiredDelta.GetNormalized();
    }

    Vector3 jointTargetDelta = jointTarget - rootNode.Translation;
    const Real jointTargetLengthSqr = jointTargetDelta.LengthSquared();

    Vector3 jointPlaneNormal, jointBendDir;
    if (jointTargetLengthSqr < ZeroTolerance * ZeroTolerance)
    {
        jointBendDir = Vector3::Forward;
        jointPlaneNormal = Vector3::Up;
    }
    else
    {
        jointPlaneNormal = desiredDir ^ jointTargetDelta;
        if (jointPlaneNormal.LengthSquared() < ZeroTolerance * ZeroTolerance)
        {
            desiredDir.FindBestAxisVectors(jointPlaneNormal, jointBendDir);
        }
        else
        {
            jointPlaneNormal.Normalize();
            jointBendDir = jointTargetDelta - (jointTargetDelta | desiredDir) * desiredDir;
            jointBendDir.Normalize();
        }
    }

    if (allowStretching)
    {
        const Real initialStretchRatio = 1.0f;
        const Real range = maxStretchScale - initialStretchRatio;
        if (range > ZeroTolerance && limbLengthLimit > ZeroTolerance)
        {
            const Real reachRatio = desiredLength / limbLengthLimit;
            const Real scalingFactor = (maxStretchScale - 1.0f) * Math::Saturate((reachRatio - initialStretchRatio) / range);
            if (scalingFactor > ZeroTolerance)
            {
                lowerLimbLength *= 1.0f + scalingFactor;
                upperLimbLength *= 1.0f + scalingFactor;
                limbLengthLimit *= 1.0f + scalingFactor;
            }
        }
    }

    Vector3 resultEndPos = target;
    Vector3 resultJointPos = jointPos;

    if (desiredLength >= limbLengthLimit)
    {
        resultEndPos = rootNode.Translation + limbLengthLimit * desiredDir;
        resultJointPos = rootNode.Translation + upperLimbLength * desiredDir;
    }
    else
    {
        const Real twoAb = 2.0f * upperLimbLength * desiredLength;
        const Real cosAngle = twoAb > ZeroTolerance ? (upperLimbLength * upperLimbLength + desiredLength * desiredLength - lowerLimbLength * lowerLimbLength) / twoAb : 0.0f;
        const bool reverseUpperBone = cosAngle < 0.0f;
        const Real angle = Math::Acos(cosAngle);
        const Real jointLineDist = upperLimbLength * Math::Sin(angle);
        const Real projJointDistSqr = upperLimbLength * upperLimbLength - jointLineDist * jointLineDist;
        Real projJointDist = projJointDistSqr > 0.0f ? Math::Sqrt(projJointDistSqr) : 0.0f;
        if (reverseUpperBone)
            projJointDist *= -1.0f;
        resultJointPos = rootNode.Translation + projJointDist * desiredDir + jointLineDist * jointBendDir;
    }

    {
        const Vector3 oldDir = (jointPos - rootNode.Translation).GetNormalized();
        const Vector3 newDir = (resultJointPos - rootNode.Translation).GetNormalized();
        const Quaternion deltaRotation = Quaternion::FindBetween(oldDir, newDir);
        rootNode.Orientation = deltaRotation * rootNode.Orientation;
    }

    {
        const Vector3 oldDir = (targetNode.Translation - jointPos).GetNormalized();
        const Vector3 newDir = (resultEndPos - resultJointPos).GetNormalized();
        const Quaternion deltaRotation = Quaternion::FindBetween(oldDir, newDir);
        jointNode.Orientation = deltaRotation * jointNode.Orientation;
        jointNode.Translation = resultJointPos;
    }

    targetNode.Translation = resultEndPos;
}
