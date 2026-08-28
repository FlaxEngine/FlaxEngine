// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "IOcclusionCulling.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Content/AssetReference.h"

class GPUPipelineState;

/// <summary>
/// Occlusion culling system based on hardware occlusion queries.
/// Uses GPU query to determine if the object is visible (not occluded by other geometry) and can be drawn.
/// Results are readback with a few frames latency (objects can pop in fast motion).
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API HardwareOcclusionCulling : public ScriptingObject, public IOcclusionCulling
{
    DECLARE_SCRIPTING_TYPE(HardwareOcclusionCulling);
    ~HardwareOcclusionCulling();

    // Maximum number of frames to delay the visibility results readback from GPU (to avoid stalls). The higher value the more latency but less GPU stalls.
    constexpr static int32 MaxFrames = 4;

private:
    struct alignas(sizeof(uint64)) Item
    {
        BoundingBox Bounds; // Object bounds (world-space)
        uint64 Queries[MaxFrames]; // Buffered frames (ring-buffer)
        int32 Frames[MaxFrames]; // Frame counter for each query
        int32 LastUsedFrame; // Last frame object was drawn (incl. not frustum-culled or hidden)
        bool Occluded; // Result from the last frame
    };

    int32 _framesCount = 2, _frameCounter = 0;
    volatile int64 _freeItemsCount = 0;
    volatile int64 _newItemsCount = 0;
    volatile int64 _dirtyBounds = 0;
    uint64 _lastEngineFrameUsed = 0;
    float _boundsScale = 1.0f;
    Array<Item> _items;
    Array<uint32> _freeItems;
    GPUPipelineState* _pso = nullptr;
    DynamicVertexBuffer _vertexBuffer;
    GPUBuffer* _indexBuffer = nullptr;
    Vector3 _origin = Vector3::Zero;
    Vector3 _viewPos = Vector3::Zero;
    bool _forceUpdateBounds = true;
    AssetReference<class Shader> _shader;

public:
    // [IOcclusionCulling]
    void BeginFrame(const RenderContext& renderContext) override;
    void EndFrame(const RenderContext& renderContext) override;
    void Submit(const RenderContext& renderContext) override;
    bool IsVisible(const BoundingBox& bounds, uint32& cullingId) override;
    bool IsVisible(const BoundingBox& bounds, GeometryDrawState& drawState) override;

private:
    bool IsVisible(BoundingBox bounds, uint32& cullingId, GeometryDrawState* drawState);
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj);
#endif
};
