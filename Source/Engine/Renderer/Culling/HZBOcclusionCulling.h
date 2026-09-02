// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "IOcclusionCulling.h"
#include "OcclusionCullingTools.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/DynamicBuffer.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// Occlusion culling system based on Hierarchical Z-Buffer visibility test with a readback.
/// It builds a mipmap chain from the depth buffer and runs a compute shader to test object bounding boxes against it to skip rendering unseen geometry.
/// Results are readback with a few frames latency (objects can pop in fast motion).
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API HZBOcclusionCulling : public ScriptingObject, public IOcclusionCulling
{
    DECLARE_SCRIPTING_TYPE(HZBOcclusionCulling);
    ~HZBOcclusionCulling();

    // Maximum number of frames to delay the visibility results readback from GPU (to avoid stalls). The higher value the more latency but less GPU stalls.
    constexpr static int32 MaxFrames = 4;

private:
    struct alignas(sizeof(uint64)) Item
    {
        BoundingBox Bounds; // Object bounds (world-space)
        int32 Frames[MaxFrames]; // Frame counter for each test (0 if not performed)
        int32 LastUsedFrame; // Last frame object was drawn (incl. not frustum-culled or hidden)
        bool Occluded; // Result from the last frame
    };

    int32 _framesCount = 0, _frameCounter = 0;
    volatile int64 _dirtyBounds = 0;
    uint64 _lastEngineFrameUsed = 0;
    Vector3 _origin = Vector3::Zero;
    Vector3 _viewPos = Vector3::Zero;
    AssetReference<class Shader> _shader;
    OcclusionCullingItems<Item> _items;
    DynamicTypedBuffer _boundsBuffer;
    GPUBuffer* _resultsBuffer = nullptr;
    GPUBuffer* _readbackBuffers[MaxFrames] = {};
    int32 _readbackFrames[MaxFrames] = {};
    int32 _readbackCounts[MaxFrames] = {};
    float _boundsScale = 1.0f;
    bool _forceUpdateBounds = true;
    bool _submitFailed = false;

public:
    // [IOcclusionCulling]
    bool IsSupported() override;
    void BeginFrame(const RenderContext& renderContext) override;
    void EndFrame(const RenderContext& renderContext) override;
    void Submit(const RenderContext& renderContext) override;
    bool IsVisible(const BoundingBox& bounds, uint32& cullingId) override;
    bool IsVisible(const BoundingBox& bounds, GeometryDrawState& drawState) override;

private:
    bool IsVisible(BoundingBox bounds, uint32& cullingId, GeometryDrawState* drawState);
};
