// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/PixelFormat.h"

struct GPUPipelineStateKeyDX12
{
    int32 RTsCount;
    MSAALevel MSAA;
    class GPUVertexLayoutDX12* VertexLayout;
    PixelFormat DepthFormat;
    PixelFormat RTVsFormats[GPU_MAX_RT_BINDED];

    bool operator==(const GPUPipelineStateKeyDX12& other) const
    {
        return Platform::MemoryCompare((void*)this, &other, sizeof(GPUPipelineStateKeyDX12)) == 0;
    }

    void Init(PixelFormat depthFormat, int32 rtCount, const PixelFormat* rtFormats, MSAALevel msaa, GPUVertexLayoutDX12* vertexLayout)
    {
        memset(this, 0, sizeof(GPUPipelineStateKeyDX12));
        RTsCount = rtCount;
        VertexLayout = vertexLayout;
        DepthFormat = depthFormat;
        MSAA = msaa;
        for (int32 i = 0; i < rtCount; i++)
            RTVsFormats[i] = rtFormats[i];
    }

#if GPU_ENABLE_DEVELOPMENT
    bool IsRenderFormatMatch(GPUPipelineStateKeyDX12 other) const
    {
        other.VertexLayout = VertexLayout;
        return Platform::MemoryCompare((void*)this, &other, sizeof(GPUPipelineStateKeyDX12)) == 0;
    }

    String ToString() const;
#endif

    friend uint32 GetHash(const GPUPipelineStateKeyDX12& key);
};

#endif
