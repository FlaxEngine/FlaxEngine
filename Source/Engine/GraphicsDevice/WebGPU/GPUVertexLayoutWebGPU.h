// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "GPUDeviceWebGPU.h"

/// <summary>
/// Vertex layout object for Web GPU backend.
/// </summary>
class GPUVertexLayoutWebGPU : public GPUResourceBase<GPUDeviceWebGPU, GPUVertexLayout>
{
public:
	GPUVertexLayoutWebGPU(GPUDeviceWebGPU* device, const Elements& elements, bool explicitOffsets);
};

#endif
