// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

// GPU resource access flags. Used to describe how resource can be accessed which allows GPU to optimize data layout and memory access.
enum class GPUResourceAccess
{
    None = 0,
    CopyRead = 1 << 0,
    CopyWrite = 1 << 1,
    CpuRead = 1 << 2,
    CpuWrite = 1 << 3,
    DepthRead = 1 << 4,
    DepthWrite = 1 << 5,
    DepthBuffer = DepthRead | DepthWrite,
    RenderTarget = 1 << 6,
    UnorderedAccess = 1 << 7,
    IndirectArgs = 1 << 8,
    ShaderReadCompute = 1 << 9,
    ShaderReadPixel = 1 << 10,
    ShaderReadNonPixel = 1 << 11,
    ShaderReadGraphics = ShaderReadPixel | ShaderReadNonPixel,
    Last,
    All = (Last << 1) - 1,
};

DECLARE_ENUM_OPERATORS(GPUResourceAccess);
