// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "GPUDeviceDX12.h"

/// <summary>
/// Vertex layout object for DirectX 12 backend.
/// </summary>
class GPUVertexLayoutDX12 : public GPUResourceDX12<GPUVertexLayout>
{
public:
	GPUVertexLayoutDX12(GPUDeviceDX12* device, const Elements& elements, bool explicitOffsets);

	uint32 InputElementsCount;
	D3D12_INPUT_ELEMENT_DESC InputElements[GPU_MAX_VS_ELEMENTS];
};

#endif
