// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GPUSwapChainOGL.h"

#if GRAPHICS_API_OPENGL

#include "Engine/Threading/Threading.h"
#include "Engine/Platform/Window.h"
#include "RenderToolsOGL.h"
#if PLATFORM_WIN32
#include "Win32/Win32GPUSwapChainOGL.h"
#endif

GPUSwapChainOGL* GPUSwapChainOGL::Create(GPUDeviceOGL* device, Window* parent, int32 width, int32 height, bool fullscreen)
{
	// Create object
#if PLATFORM_WIN32
	Win32GPUSwapChainOGL* result = Win32GPUSwapChainOGL::Create(device, parent);
#else
#Error "The current platform does not support OpenGL backend."
#endif

	// Resize output
	result->Resize(width, height);

	// Check if enter fullscreen mode
	if (fullscreen)
		result->SetFullscreen(true);

	return result;
}

GPUSwapChainOGL::GPUSwapChainOGL(GPUDeviceOGL* device, Window* parent)
	: GPUResourceOGL(device, StringView::Empty)
{
	setParent(parent);
}

#endif
