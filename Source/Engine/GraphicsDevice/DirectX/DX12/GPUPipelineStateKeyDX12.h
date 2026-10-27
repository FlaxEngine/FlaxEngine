// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/Enums.h"
#include "Engine/Graphics/PixelFormat.h"

struct GPUPipelineStateKeyDX12
{
    union
    {
        struct
        {
            uint32 RTsCount : 3; // Limited by GPU_MAX_RT_BINDED
            MSAALevel MSAA : 4;
            PixelFormat DepthFormat : 8;
        };

        uint32 Flags;
    };

    byte RTVsFormats[GPU_MAX_RT_BINDED];
    class GPUVertexLayoutDX12* VertexLayout;

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
            RTVsFormats[i] = (byte)rtFormats[i];
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
