// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "GPUDeviceVulkan.h"

/// <summary>
/// Vertex layout object for Vulkan backend.
/// </summary>
class GPUVertexLayoutVulkan : public GPUResourceVulkan<GPUVertexLayout>
{
public:
	GPUVertexLayoutVulkan(GPUDeviceVulkan* device, const Elements& elements, bool explicitOffsets);

	int32 MaxSlot;
};

#endif
