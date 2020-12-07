// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUSwapChain.h"
#include "GPUDeviceOGL.h"
#include "GPUResourceOGL.h"
#include "GPUTextureViewOGL.h"
#include "IncludeOpenGLHeaders.h"

#if GRAPHICS_API_OPENGL

/// <summary>
/// Graphics Device rendering output for OpenGL
/// </summary>
class GPUSwapChainOGL : public GPUResourceOGL<GPUSwapChain>
{
protected:

	GPUTextureViewOGL _backBufferHandle;

	GPUSwapChainOGL(GPUDeviceOGL* device, Window* parent);

public:

	// Init
	static GPUSwapChainOGL* Create(GPUDeviceOGL* device, Window* parent, int32 width, int32 height, bool fullscreen);

public:

	// [GPUSwapChain]
	GPUTextureView* GetBackBufferView() const override
	{
		return (GPUTextureView*)&_backBufferHandle;
	}
};

#endif
