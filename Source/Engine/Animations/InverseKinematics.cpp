// Copyright (c) Wojciech Figat. All rights reserved.

#include "InverseKinematics.h"

void InverseKinematics::SolveAimIK(const Transform& node, const Vector3& target, Quaternion& outNodeCorrection)
{
    Vector3 toTarget = target - node.Translation;
    toTarget.Normalize();
    const Vector3 fromNode = Vector3::Forward;
    Quaternion::FindBetween(fromNode, toTarget, outNodeCorrection);
}

void InverseKinematics::SolveTwoBoneIK(Transform& rootTransform, Transform& midJointTransform, Transform& endEffectorTransform, const Vector3& targetPosition, const Vector3& poleVector, bool allowStretching, float maxStretchScale)
{
    // Calculate limb segment lengths
    Real lowerLimbLength = (endEffectorTransform.Translation - midJointTransform.Translation).Length();
    Real upperLimbLength = (midJointTransform.Translation - rootTransform.Translation).Length();
    Vector3 midJointPos = midJointTransform.Translation;

    // Calculate the direction and length towards the target
    Vector3 toTargetVector = targetPosition - rootTransform.Translation;
    Real toTargetLength = toTargetVector.Length();
    Real totalLimbLength = lowerLimbLength + upperLimbLength;

    // Normalize the direction vector or set a default direction if too small
    Vector3 toTargetDir;
    if (toTargetLength < ZeroTolerance)
    {
        toTargetLength = ZeroTolerance;
        toTargetDir = Vector3(1, 0, 0);
    }
    else
    {
        toTargetDir = toTargetVector.GetNormalized();
    }

    // Calculate the pole vector direction
    Vector3 poleVectorDelta = poleVector - rootTransform.Translation;
    const Real poleVectorLengthSqr = poleVectorDelta.LengthSquared();

    Vector3 jointPlaneNormal, bendDirection;
    if (poleVectorLengthSqr < ZeroTolerance * ZeroTolerance)
    {
        bendDirection = Vector3::Forward;
        jointPlaneNormal = Vector3::Up;
    }
    else
    {
        jointPlaneNormal = toTargetDir ^ poleVectorDelta;
        if (jointPlaneNormal.LengthSquared() < ZeroTolerance * ZeroTolerance)
        {
            toTargetDir.FindBestAxisVectors(jointPlaneNormal, bendDirection);
        }
        else
        {
            jointPlaneNormal.Normalize();
            bendDirection = poleVectorDelta - (poleVectorDelta | toTargetDir) * toTargetDir;
            bendDirection.Normalize();
        }
    }

    // Handle limb stretching if allowed
    if (allowStretching)
    {
        const Real initialStretchRatio = 1.0f;
        const Real stretchRange = maxStretchScale - initialStretchRatio;
        if (stretchRange > ZeroTolerance && totalLimbLength > ZeroTolerance)
        {
            const Real reachRatio = toTargetLength / totalLimbLength;
            const Real scalingFactor = (maxStretchScale - 1.0f) * Math::Saturate((reachRatio - initialStretchRatio) / stretchRange);
            if (scalingFactor > ZeroTolerance)
            {
                lowerLimbLength *= 1.0f + scalingFactor;
                upperLimbLength *= 1.0f + scalingFactor;
                totalLimbLength *= 1.0f + scalingFactor;
            }
        }
    }

    // Calculate new positions for joint and end effector
    Vector3 newEndEffectorPos = targetPosition;
    Vector3 newMidJointPos = midJointPos;
    if (toTargetLength >= totalLimbLength)
    {
        // Target is beyond the reach of the limb
        Vector3 rootToEnd = (targetPosition - rootTransform.Translation).GetNormalized();

        // Calculate the slight offset towards the pole vector
        Vector3 rootToPole = (poleVector - rootTransform.Translation).GetNormalized();
        Vector3 slightBendDirection = Vector3::Cross(rootToEnd, rootToPole);
        if (slightBendDirection.LengthSquared() < ZeroTolerance * ZeroTolerance)
            slightBendDirection = Vector3::Up;
        else
            slightBendDirection.Normalize();

        // Calculate the direction from root to mid joint with a slight offset towards the pole vector
        Vector3 midJointDirection = Vector3::Cross(slightBendDirection, rootToEnd).GetNormalized();
        Real slightOffset = upperLimbLength * 0.01f; // Small percentage of the limb length for slight offset
        newMidJointPos = rootTransform.Translation + rootToEnd * (upperLimbLength - slightOffset) + midJointDirection * slightOffset;
    }
    else
    {
        // Target is within reach, calculate joint position
        const Real twoAb = 2.0f * upperLimbLength * toTargetLength;
        const Real cosAngle = twoAb > ZeroTolerance ? (upperLimbLength * upperLimbLength + toTargetLength * toTargetLength - lowerLimbLength * lowerLimbLength) / twoAb : 0.0f;
        const bool reverseUpperBone = cosAngle < 0.0f;
        const Real angle = Math::Acos(cosAngle);
        const Real jointLineDist = upperLimbLength * Math::Sin(angle);
        const Real projJointDistSqr = upperLimbLength * upperLimbLength - jointLineDist * jointLineDist;
        Real projJointDist = projJointDistSqr > 0.0f ? Math::Sqrt(projJointDistSqr) : 0.0f;
        if (reverseUpperBone)
            projJointDist *= -1.0f;
        newMidJointPos = rootTransform.Translation + projJointDist * toTargetDir + jointLineDist * bendDirection;
    }
    // TODO: fix the new IK impl (https://github.com/FlaxEngine/FlaxEngine/pull/2421) to properly work for character from https://github.com/PrecisionRender/CharacterControllerPro
#define OLD 1
    // Update root joint orientation
    {
#if OLD
        const Vector3 oldDir = (midJointPos - rootTransform.Translation).GetNormalized();
        const Vector3 newDir = (newMidJointPos - rootTransform.Translation).GetNormalized();
        const Quaternion deltaRotation = Quaternion::FindBetween(oldDir, newDir);
        rootTransform.Orientation = deltaRotation * rootTransform.Orientation;
#else
        // Vector from root joint to mid joint (local Y-axis direction)
        Vector3 localY = (newMidJointPos - rootTransform.Translation).GetNormalized();

        // Vector from mid joint to end effector (used to calculate plane normal)
        Vector3 midToEnd = (newEndEffectorPos - newMidJointPos).GetNormalized();

        // Calculate the plane normal (local Z-axis direction)
        Vector3 localZ = Vector3::Cross(localY, midToEnd).GetNormalized();

        // Calculate the local X-axis direction, should be perpendicular to the Y and Z axes
        Vector3 localX = Vector3::Cross(localY, localZ).GetNormalized();

        // Correct the local Z-axis direction based on the cross product of X and Y to ensure orthogonality
        localZ = Vector3::Cross(localX, localY).GetNormalized();

        // Construct a rotation from the orthogonal basis vectors
        rootTransform.Orientation = Quaternion::LookRotation(localZ, localY);
#endif
    }

    // Update mid joint orientation to point Y-axis towards the end effector and Z-axis perpendicular to the IK plane
    {
#if OLD
        const Vector3 oldDir = (endEffectorTransform.Translation - midJointPos).GetNormalized();
        const Vector3 newDir = (newEndEffectorPos - newMidJointPos).GetNormalized();
        const Quaternion deltaRotation = Quaternion::FindBetween(oldDir, newDir);
        midJointTransform.Orientation = deltaRotation * midJointTransform.Orientation;
#else
        // Vector from mid joint to end effector (local Y-axis direction after rotation)
        Vector3 midToEnd = (newEndEffectorPos - newMidJointPos).GetNormalized();

        // Calculate the plane normal using the root, mid joint, and end effector positions (will be the local Z-axis direction)
        Vector3 rootToMid = (newMidJointPos - rootTransform.Translation).GetNormalized();

        // Vector from mid joint to end effector (local Y-axis direction)
        Vector3 localY = (newEndEffectorPos - newMidJointPos).GetNormalized();

        // Calculate the plane normal using the root, mid joint, and end effector positions (local Z-axis direction)
        Vector3 localZ = Vector3::Cross(rootToMid, localY).GetNormalized();

        // Calculate the local X-axis direction, should be perpendicular to the Y and Z axes
        Vector3 localX = Vector3::Cross(localY, localZ).GetNormalized();

        // Correct the local Z-axis direction based on the cross product of X and Y to ensure orthogonality
        localZ = Vector3::Cross(localX, localY).GetNormalized();

        // Construct a rotation from the orthogonal basis vectors
        midJointTransform.Orientation = Quaternion::LookRotation(localZ, localY);
#endif
    }

    // Update mid and end locations
    midJointTransform.Translation = newMidJointPos;
    endEffectorTransform.Translation = newEndEffectorPos;
}
