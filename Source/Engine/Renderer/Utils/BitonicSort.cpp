// Copyright (c) Wojciech Figat. All rights reserved.

#include "BitonicSort.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPULimits.h"

#define INDIRECT_ARGS_STRIDE 12

// The sorting keys buffer item structure template. Matches the shader type.
struct Item
{
    float Key;
    uint32 Value;
};

GPU_CB_STRUCT(Data {
    Item NullItem;
    uint32 CounterOffset;
    uint32 MaxIterations;
    uint32 LoopK;
    float KeySign;
    uint32 LoopJ;
    float Dummy0;
    });

String BitonicSort::ToString() const
{
    return TEXT("BitonicSort");
}

bool BitonicSort::Init()
{
    // Draw indirect and compute shaders support is required for this implementation
    const auto& limits = GPUDevice::Instance->Limits;
    if (!limits.HasDrawIndirect || !limits.HasCompute)
        return false;

    // Create indirect dispatch arguments buffer
    _dispatchArgsBuffer = GPUDevice::Instance->CreateBuffer(TEXT("BitonicSortDispatchArgs"));
    if (_dispatchArgsBuffer->Init(GPUBufferDescription::Raw(22 * 23 / 2 * INDIRECT_ARGS_STRIDE, GPUBufferFlags::Argument | GPUBufferFlags::UnorderedAccess)))
        return true;

    // Load asset
    _shader = Content::LoadAsyncInternal<Shader>(TEXT("Shaders/BitonicSort"));
    if (_shader == nullptr)
        return true;
#if COMPILE_WITH_DEV_ENV
    _shader.Get()->OnReloading.Bind<BitonicSort, &BitonicSort::OnShaderReloading>(this);
#endif

    return false;
}

bool BitonicSort::setupResources()
{
    // Check if shader has not been loaded
    if (!_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();

    // Validate shader constant buffer size
    _cb = shader->GetCB(0);
    if (_cb->GetSize() != sizeof(Data))
    {
        REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
        return true;
    }

    // Cache compute shaders
    _indirectArgsCS = shader->GetCS("CS_IndirectArgs");
    _preSortCS = shader->GetCS("CS_PreSort");
    _innerSortCS = shader->GetCS("CS_InnerSort");
    _outerSortCS = shader->GetCS("CS_OuterSort");
    _copyIndicesCS = shader->GetCS("CS_CopyIndices");

    return false;
}

void BitonicSort::Dispose()
{
    // Base
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_dispatchArgsBuffer);
    _cb = nullptr;
    _indirectArgsCS = nullptr;
    _preSortCS = nullptr;
    _innerSortCS = nullptr;
    _outerSortCS = nullptr;
    _copyIndicesCS = nullptr;
    _shader = nullptr;
}

void BitonicSort::Sort(GPUContext* context, GPUBuffer* sortingKeysBuffer, GPUBuffer* countBuffer, uint32 counterOffset, bool sortAscending, GPUBuffer* sortedIndicesBuffer)
{
    ASSERT(context && sortingKeysBuffer && countBuffer);

    PROFILE_GPU_CPU("Bitonic Sort");

    // Check if has missing resources
    if (checkIfSkipPass())
    {
        return;
    }

    // Prepare
    const uint32 elementSizeBytes = sizeof(uint64);
    const uint32 maxNumElements = sortingKeysBuffer->GetSize() / elementSizeBytes;
    const uint32 alignedMaxNumElements = Math::RoundUpToPowerOf2(maxNumElements);
    const uint32 maxIterations = (uint32)Math::Log2((float)Math::Max(2048u, alignedMaxNumElements)) - 10;

    // Setup constants buffer
    Data data;
    data.CounterOffset = counterOffset;
    data.NullItem.Key = sortAscending ? MAX_float : -MAX_float;
    data.NullItem.Value = 0;
    data.KeySign = sortAscending ? -1.0f : 1.0f;
    data.MaxIterations = maxIterations;
    data.LoopK = 0;
    data.LoopJ = 0;
    context->UpdateCB(_cb, &data);
    context->BindCB(0, _cb);

    // Generate execute indirect arguments
    context->BindSR(0, countBuffer->View());
    context->BindUA(0, _dispatchArgsBuffer->View());
    context->Dispatch(_indirectArgsCS, 1, 1, 1);

    // Pre-Sort the buffer up to k = 2048 (this also pads the list with invalid indices that will drift to the end of the sorted list)
    context->BindUA(0, sortingKeysBuffer->View());
    context->DispatchIndirect(_preSortCS, _dispatchArgsBuffer, 0);

    // We have already pre-sorted up through k = 2048 when first writing our list, so we continue sorting with k = 4096
    // For really large values of k, these indirect dispatches will be skipped over with thread counts of 0
    uint32 indirectArgsOffset = INDIRECT_ARGS_STRIDE;
    for (uint32 k = 4096; k <= alignedMaxNumElements; k *= 2)
    {
        for (uint32 j = k / 2; j >= 2048; j /= 2)
        {
            data.LoopK = k;
            data.LoopJ = j;
            context->UpdateCB(_cb, &data);
            context->BindCB(0, _cb);

            context->DispatchIndirect(_outerSortCS, _dispatchArgsBuffer, indirectArgsOffset);
            indirectArgsOffset += INDIRECT_ARGS_STRIDE;
        }

        context->DispatchIndirect(_innerSortCS, _dispatchArgsBuffer, indirectArgsOffset);
        indirectArgsOffset += INDIRECT_ARGS_STRIDE;
    }

    context->ResetUA();

    if (sortedIndicesBuffer)
    {
        // Copy indices to another buffer
#if !BUILD_RELEASE
        switch (sortedIndicesBuffer->GetDescription().Format)
        {
        case PixelFormat::R32_UInt:
        case PixelFormat::R16_UInt:
        case PixelFormat::R8_UInt:
            break;
        default:
            LOG(Warning, "Invalid format {0} of sortedIndicesBuffer for BitonicSort. It needs to be UInt type.", (int32)sortedIndicesBuffer->GetDescription().Format);
        }
#endif
        context->BindSR(1, sortingKeysBuffer->View());
        context->BindUA(0, sortedIndicesBuffer->View());
        // TODO: use indirect dispatch to match the items count for copy
        context->Dispatch(_copyIndicesCS, (alignedMaxNumElements + 1023) / 1024, 1, 1);
    }

    context->ResetUA();
    context->ResetSR();
}
