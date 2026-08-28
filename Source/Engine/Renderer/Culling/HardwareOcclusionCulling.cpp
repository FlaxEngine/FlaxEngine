// Copyright (c) Wojciech Figat. All rights reserved.

#include "HardwareOcclusionCulling.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUPass.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderBuffers.h"
#include "Engine/Graphics/RenderContext.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerGPU.h"
#include "Engine/Core/Config/GraphicsSettings.h"
#include "Engine/Engine/Engine.h"

HardwareOcclusionCulling::HardwareOcclusionCulling(const SpawnParams& params)
    : ScriptingObject(params)
    , _vertexBuffer(0, sizeof(Float3), TEXT("HardwareOcclusionCulling.VB"), GPUVertexLayout::Get({ { VertexElement::Types::Position, 0, 0, 0, PixelFormat::R32G32B32_Float } }))
    , _shader(Content::LoadAsyncInternal<Shader>(TEXT("Shaders/Utils/Culling")))
{
}

HardwareOcclusionCulling::~HardwareOcclusionCulling()
{
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);
}

void HardwareOcclusionCulling::BeginFrame(const RenderContext& renderContext)
{
    PROFILE_CPU();

    // Read settings
    auto settings = GraphicsSettings::Get();
    auto framesCount = Math::Clamp(settings->OcclusionBufferedFrames, 1, MaxFrames);
    if (_framesCount != framesCount)
    {
        // Reset state (graphics backend recycles stale queries)
        _framesCount = framesCount;
        for (auto& e : _items)
        {
            Platform::MemoryClear(e.Queries, sizeof(e.Queries) + sizeof(e.Frames));
        }
    }
    _boundsScale = Math::Max(settings->OcclusionBoundsScale, 1.01f);

    // Handle origin-relative rendering
    _forceUpdateBounds = renderContext.View.Origin != _origin;
    _origin = renderContext.View.Origin;
    _viewPos = renderContext.View.WorldPosition;

    // Skip reading occlusion results on camera cuts (but issue queries for the next frame)
    bool forceVisible = renderContext.Task->IsCameraCut;

    // Skip reading when view was not rendered for some time (queries might expire)
    uint64 engineFrame = Engine::FrameCount;
    forceVisible |= (int32)(engineFrame - _lastEngineFrameUsed) >= framesCount;
    _lastEngineFrameUsed = engineFrame;

    if (forceVisible)
    {
        // Reset visibility
        for (auto& item : _items)
        {
            item.Occluded = false;
            Platform::MemoryClear(item.Queries, sizeof(item.Queries) + sizeof(item.Frames));
        }
    }
    else if (_frameCounter > 0)
    {
        // Resolve the last buffered frame results (with wait)
        PROFILE_CPU_NAMED("Wait for Occlusion Queries");
        ZoneColor(TracyWaitZoneColor);
        int32 frame = _frameCounter, bufferedFrame = _frameCounter % framesCount, itemsUsed = 0;
        auto device = GPUDevice::Instance;
        for (auto& item : _items)
        {
            uint64 query = item.Queries[bufferedFrame];
            int32 lag = frame - item.Frames[bufferedFrame];
            if (query && lag <= framesCount)
            {
                // Clear query
                item.Queries[bufferedFrame] = 0;

                // Read result (occluded object didn't pass any depth test, assume visible if query failed)
                uint64 result = 1;
                device->GetQueryResult(query, result, true);
                item.Occluded = result == 0;
                itemsUsed++;
            }
            else
            {
                // Maintain state if no new query has been issued (eg. object goes outside frustum or gets hidden)
            }
        }
        ZoneValue(itemsUsed);
    }
    
    // Remove used free items
    _freeItems.Resize(Math::Max((int32)_freeItemsCount, 0));

#if 0 // TODO: find a different way as there might be some invisible object with CullingId assigned and drawing it later will overlap with reused IDs
    // Trim history
    constexpr int32 frameTTL = 20;
    if (_frameCounter % 10 == 0 && _frameCounter > frameTTL)
    {
        const int32 lastFrame = _frameCounter - frameTTL;
        for (int32 i = 0; i < _items.Count(); i++)
        {
            auto& item = _items.Get()[i];
            if (item.LastUsedFrame && item.LastUsedFrame < lastFrame)
            {
                Platform::MemoryClear(&item, sizeof(item));
                _freeItems.Add(i);
            }
        }
    }
#endif

    // Allocate new items (as requested during the previous frame)
    if (_newItemsCount > 0)
    {
        int32 itemsStart = _items.Count(), count = (int32)_newItemsCount, freeStart = _freeItems.Count();
        if (itemsStart == 0)
            count++; // 0 is invalid for cullingId
        _items.AddZeroed(count);
        _freeItems.AddUninitialized(count);
        for (int32 i = 0; i < count; i++)
            _freeItems.Get()[freeStart + i] = itemsStart + i;
        if (itemsStart == 0)
            _freeItems.RemoveAt(0); // 0 is invalid for cullingId
        _newItemsCount = 0;
    }
    _freeItemsCount = _freeItems.Count();

    // Prepare vertex buffer to build geometry bound meshes in async during drawing
    _vertexBuffer.Data.Resize(_items.Count() * 8 * sizeof(Float3), true);
    _dirtyBounds = 0;
}

void HardwareOcclusionCulling::EndFrame(const RenderContext& renderContext)
{
    // Move to the next frame
    _frameCounter++;
}

void HardwareOcclusionCulling::Submit(const RenderContext& renderContext)
{
    if (_items.IsEmpty() || !_shader || !_shader->IsLoaded())
        return;
    PROFILE_CPU();
    PROFILE_GPU("Occlusion Culling");
    GPUContext* context = GPUDevice::Instance->GetMainContext();

    // Setup vertex and index buffers
    if (!_indexBuffer)
    {
        const uint16 cubeIndices[12 * 3] =
        {
            0, 2, 3,
            0, 3, 1,
            4, 5, 7,
            4, 7, 6,
            0, 1, 5,
            0, 5, 4,
            2, 6, 7,
            2, 7, 3,
            0, 4, 6,
            0, 6, 2,
            1, 3, 7,
            1, 7, 5,
        };
        auto desc = GPUBufferDescription::Index(sizeof(uint16), ARRAY_COUNT(cubeIndices), cubeIndices);
        _indexBuffer = GPUDevice::Instance->CreateBuffer(TEXT("HardwareOcclusionCulling.IB"));
        _indexBuffer->Init(desc);
    }
    if (_dirtyBounds)
        _vertexBuffer.Flush(context);
    auto vb = _vertexBuffer.GetBuffer();

    // Use depth-only for testing visibility
    GPUDrawPass pass(context, *renderContext.Buffers->DepthBuffer, GPUDrawPassAction::Load, Span<GPUTextureView*>(), Span<GPUDrawPassAction>());
    if (!_pso)
    {
        _pso = GPUDevice::Instance->CreatePipelineState();
        auto desc = GPUPipelineState::Description::Default;
        desc.DepthWriteEnable = false;
        desc.DepthClipEnable = false;
        desc.DepthFunc = ComparisonFunc::DefaultEqual;
        desc.StencilEnable = false;
        desc.CullMode = CullMode::Inverted;
        desc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::None;
        desc.VS = _shader->GPU->GetVS("VS_HardwareOcclusionCulling");
        if (_pso->Init(desc))
            return;
#if COMPILE_WITH_DEV_ENV
        _shader->Reloading.Bind<HardwareOcclusionCulling, &HardwareOcclusionCulling::OnShaderReloading>(this);
#endif
    }
    auto cb = _shader->GPU->GetCB(0);
    {
        Matrix viewProjectionMatrix;
        Matrix::Transpose(renderContext.View.ViewProjection(), viewProjectionMatrix);
        context->UpdateCB(cb, &viewProjectionMatrix);
    }
    context->BindCB(0, cb);
    context->BindIB(_indexBuffer);
    context->BindVB(Span<GPUBuffer*>(&vb, 1));
    context->SetState(_pso);
#if COMPILE_WITH_PROFILER
    auto stats = RenderStatsData::Counter;
#endif

    // Issue occlusion queries
    int32 frame = _frameCounter, bufferedFrame = _frameCounter % _framesCount, itemsCount = _items.Count(), itemsUsed = 0;
    auto frustum = renderContext.View.Frustum;
    auto* items = _items.Get();
    for (int32 i = 0; i < itemsCount; i++)
    {
        auto& item = items[i];
        if (item.LastUsedFrame != frame || frustum.Contains(item.Bounds) == ContainmentType::Disjoint)
            continue;
        itemsUsed++;

        // Begin occlusion query for this object index
        uint64 query = context->BeginQuery(GPUQueryType::BinaryOcclusion);
        item.Queries[bufferedFrame] = query;
        item.Frames[bufferedFrame] = frame;

        // Draw the low-poly bounds of that object
        context->DrawIndexed(12 * 3, i * 8);

        // End occlusion query
        context->EndQuery(query);
    }
    ZoneValue(itemsUsed);

#if COMPILE_WITH_PROFILER
    // Cancel-out any draw stats from profiler (hidden draws)
    RenderStatsData::Counter = stats;
#endif
}

bool HardwareOcclusionCulling::IsVisible(const BoundingBox& bounds, uint32& cullingId)
{
    return IsVisible(bounds, cullingId, nullptr);
}

bool HardwareOcclusionCulling::IsVisible(const BoundingBox& bounds, GeometryDrawState& drawState)
{
    return IsVisible(bounds, drawState.CullingId, &drawState);
}

bool HardwareOcclusionCulling::IsVisible(BoundingBox bounds, uint32& cullingId, GeometryDrawState* drawState)
{
    // Enlarge bounds to reduce popping
    bounds = BoundingBox::MakeScaled(bounds, _boundsScale);
    // TODO: use camera motion to enlarge bounds
    // TODO: use object motion (from prev frame world matrix) to enlarge bounds in the direction of movement to reduce popping

    // Assume visible when view is right inside the bounds
    if (bounds.Contains(_viewPos) == ContainmentType::Contains)
        return true;

    // Check if object doesn't have ID assigned yet
    if (cullingId == 0 || cullingId >= (uint32)_items.Count())
    {
        int64 freeIndex = Platform::InterlockedDecrement(&_freeItemsCount);
        if (freeIndex >= 0)
        {
            // Use the ID from the free list
            ASSERT_LOW_LAYER(freeIndex < _freeItems.Count());
            cullingId = _freeItems.Get()[freeIndex];
        }
        else
        {
            // Count space needed to contain all objects (for the next frame)
            Platform::InterlockedIncrement(&_newItemsCount);
            return true;
        }
    }

    // Update item bounds
    auto& item = _items.Get()[cullingId];
    if (item.Bounds != bounds || _forceUpdateBounds)
    {
        // Update bounds
        item.Bounds = bounds;

        // Write to the vertex buffer
        Float3 boxMin = bounds.Minimum - _origin;
        Float3 boxMax = bounds.Maximum - _origin;
        ASSERT_LOW_LAYER(_vertexBuffer.Data.Count() >= (8 * sizeof(Float3)) * (cullingId + 1));
        Float3* vertices = (Float3*)(_vertexBuffer.Data.Get() + (8 * sizeof(Float3)) * cullingId);
        vertices[0] = boxMin;
        vertices[1] = Float3(boxMin.X, boxMin.Y, boxMax.Z);
        vertices[2] = Float3(boxMin.X, boxMax.Y, boxMin.Z);
        vertices[3] = Float3(boxMin.X, boxMax.Y, boxMax.Z);
        vertices[4] = Float3(boxMax.X, boxMin.Y, boxMin.Z);
        vertices[5] = Float3(boxMax.X, boxMin.Y, boxMax.Z);
        vertices[6] = Float3(boxMax.X, boxMax.Y, boxMin.Z);
        vertices[7] = boxMax;
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

#if COMPILE_WITH_DEV_ENV

void HardwareOcclusionCulling::OnShaderReloading(Asset* obj)
{
    SAFE_DELETE_GPU_RESOURCE(_pso);
}

#endif
