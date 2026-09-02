// Copyright (c) Wojciech Figat. All rights reserved.

#include "HZBOcclusionCulling.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderContext.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerGPU.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Engine/Engine.h"

#define ResultValue uint32
#define ResultType PixelFormat::R32_UInt

HZBOcclusionCulling::HZBOcclusionCulling(const SpawnParams& params)
    : ScriptingObject(params)
    , _shader(Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Utils/Culling")))
    , _boundsBuffer(0, PixelFormat::R32G32B32_Float, false, TEXT("HZB.Bounds"))
{
    _boundsBuffer.Usage = GPUResourceUsage::Dynamic;
}

HZBOcclusionCulling::~HZBOcclusionCulling()
{
    SAFE_DELETE_GPU_RESOURCE(_resultsBuffer);
    SAFE_DELETE_GPU_RESOURCES(_readbackBuffers);
}

bool HZBOcclusionCulling::IsSupported()
{
    const GPULimits& limits = GPUDevice::Instance->Limits;
    return limits.HasCompute;
}

void HZBOcclusionCulling::BeginFrame(const RenderContext& renderContext)
{
    PROFILE_CPU();

    // Read settings
    auto settings = GraphicsSettings::Get();
    auto framesCount = Math::Clamp(settings->OcclusionBufferedFrames, 1, MaxFrames);
    if (_framesCount != framesCount)
    {
        // Reset state
        for (int32 i = framesCount; i < _framesCount; i++)
            SAFE_DELETE_GPU_RESOURCE(_readbackBuffers[i]);
        for (int32 i = _framesCount; i < framesCount; i++)
            _readbackBuffers[i] = GPUDevice::Instance->CreateBuffer(TEXT("HZB.Readback"));
        if (!_resultsBuffer)
            _resultsBuffer = GPUDevice::Instance->CreateBuffer(TEXT("HZB.Results"));
        else if (_resultsBuffer->IsAllocated())
        {
            auto desc = _resultsBuffer->GetDescription().ToStagingReadback();
            for (int32 i = _framesCount; i < framesCount; i++)
                _readbackBuffers[i]->Init(desc);
        }
        _framesCount = framesCount;
        for (auto& item : _items)
        {
            item.Occluded = false;
            Platform::MemoryClear(item.Frames, sizeof(item.Frames));
        }
        Platform::MemoryClear(_readbackFrames, sizeof(_readbackFrames));
    }
    _boundsScale = Math::Max(settings->OcclusionBoundsScale, 1.01f);

    // Handle origin-relative rendering
    _forceUpdateBounds = renderContext.View.Origin != _origin;
    _origin = renderContext.View.Origin;
    _viewPos = renderContext.View.WorldPosition;

    // Skip reading occlusion results on camera cuts
    bool forceVisible = renderContext.Task->IsCameraCut;

    // Skip reading when view was not rendered for some time
    uint64 engineFrame = Engine::FrameCount;
    forceVisible |= (int32)(engineFrame - _lastEngineFrameUsed) >= framesCount;
    _lastEngineFrameUsed = engineFrame;

    int32 frame = _frameCounter, bufferedFrame = _frameCounter % framesCount;
    if (forceVisible || _submitFailed)
    {
        // Reset
        _submitFailed = false;
        for (auto& item : _items)
            item.Occluded = false;
    }
    else if (_readbackFrames[bufferedFrame])
    {
        // Read results from the last frame
        int32 itemsCount = _items.Count(), itemsUsed = 0;
        auto* readback = (const ResultValue*)_readbackBuffers[bufferedFrame]->Map(GPUResourceMapMode::Read);
        if (readback)
        {
            auto* items = _items.Get();
            int32 frameItemsCount = Math::Min(_readbackCounts[bufferedFrame], itemsCount);
            ASSERT(_readbackBuffers[bufferedFrame]->GetSize() >= frameItemsCount * sizeof(ResultValue));
            for (int32 i = 0; i < frameItemsCount; i++)
            {
                auto& item = items[i];
                int32 itemFrame = item.Frames[bufferedFrame];
                int32 lag = frame - itemFrame;
                if (itemFrame && lag <= framesCount)
                {
                    // Clear frame
                    item.Frames[bufferedFrame] = 0;

                    // Read result
                    ResultValue result = readback[i];
                    item.Occluded = result == 0;
                    itemsUsed++;
                }
            }
            _readbackBuffers[bufferedFrame]->Unmap();
        }
        else
        {
            for (auto& item : _items)
                item.Occluded = false;
        }
        ZoneValue(itemsUsed);
        _readbackFrames[bufferedFrame] = 0;
    }

    _items.BeginFrame();

    // Resize buffers to store items culling results
    int32 itemsCapacity = Math::RoundUpToPowerOf2(_items.Count());
    if (_resultsBuffer->GetSize() < (uint32)itemsCapacity * sizeof(ResultValue))
    {
        itemsCapacity = Math::Max(itemsCapacity, 512);
        auto desc = GPUBufferDescription::Buffer(itemsCapacity * sizeof(ResultValue), GPUBufferFlags::UnorderedAccess, ResultType, nullptr, sizeof(ResultValue));
        _resultsBuffer->Init(desc);
        desc = desc.ToStagingReadback();
        for (int32 i = 0; i < _framesCount; i++)
            _readbackBuffers[i]->Init(desc);
    }

    // Prepare buffer to write geometry bounds in async during drawing
    bool init = _boundsBuffer.Data.IsEmpty();
    _boundsBuffer.Data.Resize(_items.Count() * 2 * sizeof(Float3), true);
    if (init && _boundsBuffer.Data.HasItems())
        Platform::MemoryClear(_boundsBuffer.Data.Get(), sizeof(Float3) * 2); // Clear first unused item
    _dirtyBounds = 0;
    _submitFailed = false;
}

void HZBOcclusionCulling::EndFrame(const RenderContext& renderContext)
{
    // Move to the next frame
    _frameCounter++;
}

void HZBOcclusionCulling::Submit(const RenderContext& renderContext)
{
    if (_items.IsEmpty() || !_shader || !_shader->IsLoaded())
        return;
    PROFILE_CPU();
    PROFILE_GPU("HZB Occlusion Culling");
    GPUContext* context = GPUDevice::Instance->GetMainContext();

    // Update objects to cull
    auto frustum = renderContext.View.Frustum;
    auto* items = _items.Get();
    int32 frame = _frameCounter, bufferedFrame = _frameCounter % _framesCount, itemsCount = _items.Count(), itemsEnd = 0;
    for (int32 i = 0; i < itemsCount; i++)
    {
        auto& item = items[i];
        if (item.LastUsedFrame != frame || frustum.Contains(item.Bounds) == ContainmentType::Disjoint)
            continue;
        itemsEnd = i + 1;

        // Mark as used in this HZB frame (to read results later)
        item.Frames[bufferedFrame] = frame;
    }
    ZoneValue(itemsEnd);
    _readbackCounts[bufferedFrame] = itemsEnd;
    if (itemsEnd == 0)
        return;

    // Build HZB with furthest depths (full mip chain) for the current frame
    context->ResetRenderTarget();
    GPUTexture* hzb = renderContext.Buffers->RequestHiZ(context, false, 0, false, true);
    if (!hzb)
    {
        _submitFailed = true;
        return;
    }

    // Upload object bounds data
    if (_dirtyBounds)
        _boundsBuffer.Flush(context);
    ASSERT(_boundsBuffer.GetBuffer()->GetSize() >= sizeof(Float3) * 2 * itemsCount);
    ASSERT(_resultsBuffer->GetSize() >= itemsCount * sizeof(ResultValue));
    ASSERT(_readbackBuffers[bufferedFrame]->GetSize() >= itemsCount * sizeof(ResultValue));

    // Test all object bounds against current frame HZB
    auto cs = _shader->GPU->GetCS("CS_HZBCull");
    auto cb = _shader->GPU->GetCB(0);
    {
        OcclusionCullingData data;
        Matrix::Transpose(renderContext.View.ViewProjection(), data.ViewProjectionMatrix);
        data.RTSizeX = (float)hzb->Width();
        data.RTSizeY = (float)hzb->Height();
        data.MaxMipLevel = (float)hzb->MipLevels();
        data.CullCount = itemsEnd;
        context->UpdateCB(cb, &data);
    }
    context->BindCB(0, cb);
    context->BindUA(0, _resultsBuffer->View());
    context->BindSR(0, _boundsBuffer.GetBuffer()->View());
    context->BindSR(1, hzb->View());
    context->Dispatch(cs, Math::DivideAndRoundUp(itemsEnd, 64));

    // Copy results to the readback buffer
    context->CopyBuffer(_readbackBuffers[bufferedFrame], _resultsBuffer, itemsEnd * sizeof(ResultValue));

    // Mark the readback buffer has a valid frame data
    _readbackFrames[bufferedFrame] = frame;

    // Restore state
    context->ResetSR();
    context->SetViewportAndScissors(renderContext.Buffers->GetViewport());
}

bool HZBOcclusionCulling::IsVisible(const BoundingBox& bounds, uint32& cullingId)
{
    return IsVisible(bounds, cullingId, nullptr);
}

bool HZBOcclusionCulling::IsVisible(const BoundingBox& bounds, GeometryDrawState& drawState)
{
    return IsVisible(bounds, drawState.CullingId, &drawState);
}

bool HZBOcclusionCulling::IsVisible(BoundingBox bounds, uint32& cullingId, GeometryDrawState* drawState)
{
    // Enlarge bounds to reduce popping
    bounds = BoundingBox::MakeScaled(bounds, _boundsScale);
    // TODO: use camera motion to enlarge bounds
    // TODO: use object motion (from prev frame world matrix) to enlarge bounds in the direction of movement to reduce popping

    // Assume visible when view is right inside the bounds
    if (bounds.Contains(_viewPos) == ContainmentType::Contains)
        return true;

    // Check id
    if (_items.GetCullingId(cullingId))
        return true;

    // Update item bounds
    auto& item = _items.Get()[cullingId];
    if (item.Bounds != bounds || _forceUpdateBounds)
    {
        // Update bounds
        item.Bounds = bounds;

        // Write to the bounds buffer
        Float3 boxMin = bounds.Minimum - _origin;
        Float3 boxMax = bounds.Maximum - _origin;
        ASSERT_LOW_LAYER(_boundsBuffer.Data.Count() >= (2 * sizeof(Float3)) * (cullingId + 1));
        Float3* boundsPtr = (Float3*)(_boundsBuffer.Data.Get() + (2 * sizeof(Float3)) * cullingId);
        boundsPtr[0] = boxMin;
        boundsPtr[1] = boxMax;
        Platform::InterlockedIncrement(&_dirtyBounds);
    }

    // Force visible when object was not rendered last frame (eg. outside the frustum)
    if (_frameCounter - item.LastUsedFrame > 1)
    {
        item.Occluded = false;
    }
    item.LastUsedFrame = _frameCounter;

    // Read occlusion result
    return !item.Occluded;
}
