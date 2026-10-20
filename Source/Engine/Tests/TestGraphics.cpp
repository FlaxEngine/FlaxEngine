// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Platform/Platform.h"
#if GRAPHICS_API_DIRECTX12
#include "Engine/GraphicsDevice/DirectX/DX12/GPUPipelineStateDX12.h"
#endif
#include <ThirdParty/catch2/catch.hpp>

#if GRAPHICS_API_DIRECTX12

namespace
{
    // Fills the key fields as GPUPipelineStateDX12::GetState does, over the bytes the memory held before.
    void SetKey(GPUPipelineStateKeyDX12& key)
    {
        key.RTsCount = 1;
        key.MSAA = MSAALevel::None;
        key.VertexLayout = nullptr;
        key.DepthFormat = PixelFormat::D24_UNorm_S8_UInt;
        for (int32 i = 0; i < GPU_MAX_RT_BINDED; i++)
            key.RTVsFormats[i] = i == 0 ? PixelFormat::R8G8B8A8_UNorm : PixelFormat::Unknown;
    }
}

TEST_CASE("GPUPipelineStateDX12")
{
    SECTION("Test key compare")
    {
        // Two keys with the same fields, over memory that held different bytes (the key is built on the stack, and
        // the struct has padding bytes that no field writes)
        alignas(GPUPipelineStateKeyDX12) byte memoryA[sizeof(GPUPipelineStateKeyDX12)];
        alignas(GPUPipelineStateKeyDX12) byte memoryB[sizeof(GPUPipelineStateKeyDX12)];
        Platform::MemoryClear(memoryA, sizeof(memoryA));
        Platform::MemorySet(memoryB, sizeof(memoryB), 0xAB);
        auto& keyA = *(GPUPipelineStateKeyDX12*)memoryA;
        auto& keyB = *(GPUPipelineStateKeyDX12*)memoryB;
        SetKey(keyA);
        SetKey(keyB);
        CHECK(GetHash(keyA) == GetHash(keyB));
        CHECK(keyA == keyB);

        // A different field still makes a different key
        keyB.DepthFormat = PixelFormat::Unknown;
        CHECK(!(keyA == keyB));
    }
}

#endif
