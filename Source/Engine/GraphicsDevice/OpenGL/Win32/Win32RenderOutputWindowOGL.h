// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "../GPUSwapChainOGL.h"

#if GRAPHICS_API_OPENGL && PLATFORM_WINDOWS

#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"

/// <summary>
/// Graphics Device rendering output for OpenGL on Windows
/// </summary>
class Win32GPUSwapChainOGL : public GPUSwapChainOGL
{
private:

	HWND _hwnd;
	HDC	_hdc;
	HGLRC _context;
	bool _isDisposing;

	Win32GPUSwapChainOGL(GPUDeviceOGL* device, const String& name, Window* parent, HWND hwnd, HDC hdc);
	~Win32GPUSwapChainOGL();

public:

	// Init
	static Win32GPUSwapChainOGL* Create(GPUDeviceOGL* device, const String& name, Window* parent);

public:

	// [GPUResourceOGL]
	void ReleaseGPU() override;

	// [GPUSwapChainOGL]
	bool IsFullscreen() override;
	void SetFullscreen(bool isFullscreen) override;
	bool DownloadData(TextureData* result) override;
	Task* DownloadDataAsync(TextureData* result) override;
	void Present(bool vsync) override;
	bool Resize(int32 width, int32 height) override;
};

#endif
