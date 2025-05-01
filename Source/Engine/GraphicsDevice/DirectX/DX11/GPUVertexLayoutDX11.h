// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX11

#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "GPUDeviceDX11.h"

/// <summary>
/// Vertex layout object for DirectX 11 backend.
/// </summary>
class GPUVertexLayoutDX11 : public GPUResourceBase<GPUDeviceDX11, GPUVertexLayout>
{
public:
	GPUVertexLayoutDX11(GPUDeviceDX11* device, const Elements& elements, bool explicitOffsets);

	uint32 InputElementsCount;
	D3D11_INPUT_ELEMENT_DESC InputElements[GPU_MAX_VS_ELEMENTS];
};

#endif
