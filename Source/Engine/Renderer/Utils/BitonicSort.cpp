// Copyright (c) Wojciech Figat. All rights reserved.

#include "BitonicSort.h"
#include "Engine/Content/Content.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPULimits.h"

GPU_CB_STRUCT(Data {
    float NullItemKey;
    uint32 NullItemIndex;
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
    if (_dispatchArgsBuffer->Init(GPUBufferDescription::Raw(22 * 23 / 2 * sizeof(GPUDispatchIndirectArgs), GPUBufferFlags::Argument | GPUBufferFlags::UnorderedAccess)))
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
    if (!_shader->IsLoaded())
        return true;
    const auto shader = _shader->GetShader();
    _cb = shader->GetCB(0);
    CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);

    // Cache compute shaders
    _indirectArgsCS = shader->GetCS("CS_IndirectArgs");
    _preSortCS.Get(shader, "CS_PreSort");
    _innerSortCS = shader->GetCS("CS_InnerSort");
    _outerSortCS = shader->GetCS("CS_OuterSort");

    return false;
}

void BitonicSort::Dispose()
{
    RendererPass::Dispose();

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(_dispatchArgsBuffer);
    _cb = nullptr;
    _indirectArgsCS = nullptr;
    _preSortCS.Clear();
    _innerSortCS = nullptr;
    _outerSortCS = nullptr;
    _shader = nullptr;
}

void BitonicSort::Sort(GPUContext* context, GPUBuffer* indicesBuffer, GPUBuffer* keysBuffer, GPUBuffer* countBuffer, uint32 counterOffset, bool sortAscending, int32 maxElements)
{
    ASSERT(context && indicesBuffer && keysBuffer && countBuffer);
    if (checkIfSkipPass())
        return;
    PROFILE_GPU_CPU("Bitonic Sort");
    int32 maxNumElements = (int32)indicesBuffer->GetElementsCount();
    if (maxElements > 0 && maxElements < maxNumElements)
        maxNumElements = maxElements;
    const uint32 alignedMaxNumElements = Math::RoundUpToPowerOf2(maxNumElements);
    const uint32 maxIterations = (uint32)Math::Log2((float)Math::Max(2048u, alignedMaxNumElements)) - 10;

    // Setup constants buffer
    Data data;
    data.CounterOffset = counterOffset;
    data.NullItemKey = sortAscending ? MAX_float : -MAX_float;
    data.NullItemIndex = 0;
    data.KeySign = sortAscending ? -1.0f : 1.0f;
    data.MaxIterations = maxIterations;
    data.LoopK = 0;
    data.LoopJ = 0;
    context->UpdateCB(_cb, &data);
    context->BindCB(0, _cb);
    context->BindSR(0, countBuffer->View());

    // If item count is small we can do only presorting within a single dispatch thread group
    if (maxNumElements <= 2048)
    {
        // Use pre-sort with smaller thread group size (eg. for small particle emitters sorting)
        const int32 permutation = maxNumElements < 128 ? 1 : 0;
        context->BindUA(0, indicesBuffer->View());
        context->BindUA(1, keysBuffer->View());
        context->Dispatch(_preSortCS.Get(permutation), 1, 1, 1);
    }
    else
    {
        // Generate execute indirect arguments
        context->BindUA(0, _dispatchArgsBuffer->View());
        context->Dispatch(_indirectArgsCS, 1, 1, 1);

        // Pre-Sort the buffer up to k = 2048 (this also pads the list with invalid indices that will drift to the end of the sorted list)
        context->BindUA(0, indicesBuffer->View());
        context->BindUA(1, keysBuffer->View());
        context->DispatchIndirect(_preSortCS.Get(0), _dispatchArgsBuffer, 0);

        // We have already pre-sorted up through k = 2048 when first writing our list, so we continue sorting with k = 4096
        // For really large values of k, these indirect dispatches will be skipped over with thread counts of 0
        uint32 indirectArgsOffset = sizeof(GPUDispatchIndirectArgs);
        for (uint32 k = 4096; k <= alignedMaxNumElements; k *= 2)
        {
            for (uint32 j = k / 2; j >= 2048; j /= 2)
            {
                data.LoopK = k;
                data.LoopJ = j;
                context->UpdateCB(_cb, &data);

                context->DispatchIndirect(_outerSortCS, _dispatchArgsBuffer, indirectArgsOffset);
                indirectArgsOffset += sizeof(GPUDispatchIndirectArgs);
            }

            context->DispatchIndirect(_innerSortCS, _dispatchArgsBuffer, indirectArgsOffset);
            indirectArgsOffset += sizeof(GPUDispatchIndirectArgs);
        }
    }

    context->ResetUA();
}
