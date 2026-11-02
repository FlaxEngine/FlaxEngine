// Copyright (c) Wojciech Figat. All rights reserved.

#include "BlendProfile.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Serialization/ReadStream.h"

float BlendProfile::ScaleAlpha(float alpha, float scale)
{
    alpha = Math::Saturate(alpha);
    if (scale >= 1.0f)
        return alpha;
    if (scale <= ZeroTolerance)
        return alpha > 0.0f ? 1.0f : 0.0f;
    return Math::Saturate(alpha / scale);
}

void BlendProfile::ComputeWeights(float alpha, AlphaBlendMode mode, const BitArray<>* mask, float scale, int32 nodesCount, float* outWeights)
{
    if (mask && mask->Count() < nodesCount)
        mask = nullptr;
    const float baseWeight = AlphaBlend::Process(Math::Saturate(alpha), mode);
    const float maskedWeight = AlphaBlend::Process(ScaleAlpha(alpha, scale), mode);
    for (int32 i = 0; i < nodesCount; i++)
        outWeights[i] = mask && mask->Get(i) ? maskedWeight : baseWeight;
}

void BlendProfile::Blend(const Transform* poseA, const Transform* poseB, const float* weights, int32 nodesCount, Transform* outPose)
{
    for (int32 i = 0; i < nodesCount; i++)
        Transform::Lerp(poseA[i], poseB[i], weights[i], outPose[i]);
}

void BlendProfile::ReadTransitionData(ReadStream& stream, int32 version, Guid& maskId, float& scale)
{
    maskId = Guid::Empty;
    scale = DefaultScale;
    if (version >= 2)
    {
        stream.Read(maskId);
        stream.ReadFloat(&scale);
    }
}
