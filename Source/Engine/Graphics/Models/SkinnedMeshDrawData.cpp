// Copyright (c) Wojciech Figat. All rights reserved.

#include "SkinnedMeshDrawData.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Animations/Config.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Matrix3x4.h"

SkinnedMeshDrawData::SkinnedMeshDrawData()
{
}

SkinnedMeshDrawData::~SkinnedMeshDrawData()
{
    SAFE_DELETE_GPU_RESOURCE(BoneMatrices);
    SAFE_DELETE_GPU_RESOURCE(PrevBoneMatrices);
}

void SkinnedMeshDrawData::Setup(int32 bonesCount)
{
    if (BoneMatrices == nullptr)
    {
        BoneMatrices = GPUDevice::Instance->CreateBuffer(TEXT("BoneMatrices"));
    }

    const int32 elementsCount = bonesCount * 3; // 3 * float4 per bone
    if (BoneMatrices->Init(GPUBufferDescription::Typed(elementsCount, PixelFormat::R32G32B32A32_Float, false, GPUResourceUsage::Dynamic)))
    {
        LOG(Error, "Failed to initialize the skinned mesh bones buffer");
        return;
    }

    BonesCount = bonesCount;
    _hasValidData = false;
    _isDirty = false;
    Data.Resize(BoneMatrices->GetSize());
    SAFE_DELETE_GPU_RESOURCE(PrevBoneMatrices);
}

void SkinnedMeshDrawData::SetData(const Matrix* bones, bool dropHistory)
{
    if (!bones)
        return;
    ANIM_GRAPH_PROFILE_EVENT("SetSkinnedMeshData");

    // Copy bones to the buffer
    const int32 count = BonesCount;
    const int32 preFetchStride = 2;
    const Matrix* input = bones;
    const auto output = (Matrix3x4*)Data.Get();
    ASSERT(Data.Count() == count * sizeof(Matrix3x4));
    for (int32 i = 0; i < count; i++)
    {
        Matrix3x4* bone = output + i;
        Platform::Prefetch(bone + preFetchStride);
        Platform::Prefetch((byte*)(bone + preFetchStride) + PLATFORM_CACHE_LINE_SIZE);
        bone->SetMatrixTranspose(input[i]);
    }

    OnDataChanged(dropHistory);
}

void SkinnedMeshDrawData::OnDataChanged(bool dropHistory)
{
    // Setup previous frame bone matrices if needed
    if (_hasValidData && !dropHistory)
    {
        ASSERT(BoneMatrices);
        if (PrevBoneMatrices == nullptr)
        {
            PrevBoneMatrices = GPUDevice::Instance->CreateBuffer(TEXT("BoneMatrices"));
            if (PrevBoneMatrices->Init(BoneMatrices->GetDescription()))
            {
                LOG(Fatal, "Failed to initialize the skinned mesh bones buffer");
            }
        }
        Swap(PrevBoneMatrices, BoneMatrices);
    }
    else
    {
        SAFE_DELETE_GPU_RESOURCE(PrevBoneMatrices);
    }

    _isDirty = true;
    _hasValidData = true;
}
