// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/SimpleHeapAllocation.h"

class RendererAllocation : public SimpleHeapAllocation<RendererAllocation, 64>
{
public:
    static FLAXENGINE_API void* Allocate(uintptr size);
    static FLAXENGINE_API void Free(void* ptr, uintptr size);
};
