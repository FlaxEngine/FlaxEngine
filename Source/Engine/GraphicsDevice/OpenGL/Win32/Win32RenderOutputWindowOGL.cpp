// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Win32GPUSwapChainOGL.h"

#if GRAPHICS_API_OPENGL && PLATFORM_WINDOWS

#include "Engine/Threading/Threading.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/Windows/WindowsWindow.h"
#include "Win32ContextOGL.h"
#include "../RenderToolsOGL.h"

Win32GPUSwapChainOGL* Win32GPUSwapChainOGL::Create(GPUDeviceOGL* device, const String& name, Window* parent)
{
	auto winWindow = (WindowsWindow*)parent;
	auto hwnd = winWindow->GetHWND();
	auto hdc = GetDC(hwnd);
	
	Win32ContextOGL::PlatformInitPixelFormatForDevice(hdc);

	return New<Win32GPUSwapChainOGL>(device, name, parent, hwnd, hdc);
}

Win32GPUSwapChainOGL::Win32GPUSwapChainOGL(GPUDeviceOGL* device, const String& name, Window* parent, HWND hwnd, HDC hdc)
	: GPUSwapChainOGL(device, name, parent)
	, _hwnd(hwnd)
	, _hdc(hdc)
	, _context(0)
	, _isDisposing(false)
{
}

Win32GPUSwapChainOGL::~Win32GPUSwapChainOGL()
{
	ReleaseDC(_hwnd, _hdc);
}

void Win32GPUSwapChainOGL::ReleaseGPU()
{
	if (_isDisposing || _memoryUsage == 0)
		return;
	_isDisposing = true;

	// TODO: disable fullscreenmode

	// Release data
	_backBufferHandle.Release();
	if (_context)
	{
		// Check if the main context is going to be closed
		if (_context == Win32ContextOGL::OpenGLContext)
		{
			// Release all GPU resources
			_device->Dispose();

			// Release all the child windows
			while (Win32ContextOGL::ChildWindows.HasItems())
			{
				Win32ContextOGL::ChildWindows[0]->ReleaseGPU();
			}

			Win32ContextOGL::OpenGLContext = 0;
			Win32ContextOGL::OpenGLContextWin = 0;
		}
		else
		{
			Win32ContextOGL::ChildWindows.Remove(this);
		}
		Win32ContextOGL::ContextMakeCurrent(NULL, NULL);
		wglDeleteContext(_context);
		_context = nullptr;
	}
	_memoryUsage = 0;
	_width = _height = 0;

	// Restore the main context if still valid
	if (Win32ContextOGL::OpenGLContext)
	{
		Win32ContextOGL::ContextMakeCurrent(Win32ContextOGL::OpenGLContextWin, Win32ContextOGL::OpenGLContext);
	}

	_isDisposing = false;
}

bool Win32GPUSwapChainOGL::IsFullscreen()
{
	// TODO: support fullscreen on OpenGL
	return false;
}

void Win32GPUSwapChainOGL::SetFullscreen(bool isFullscreen)
{
	// TODO: support fullscreen on OpenGL
	LOG(Warning, "TODO: support fullscreen on OpenGL/Windows");
}

bool Win32GPUSwapChainOGL::Resize(int32 width, int32 height)
{
	// Check if size won't change
	if (width == _width && height == _height)
	{
		// Back
		return false;
	}

	// Wait for GPU to flush commands
	WaitForGPU();

	// Lock device
	GPUDeviceLock lock(_device);

	// Check if has no context
	if (_context == 0)
	{
		// Create the context
		Win32ContextOGL::PlatformCreateOpenGLContextCore(&_context, _hdc, Win32ContextOGL::OpenGLContext);
		if (_context == 0)
		{
			LOG(Error, "Failed to create OpenGL device context");
			return true;
		}

		// Check if the main context is missing
		if (Win32ContextOGL::OpenGLContext == 0)
		{
			// Be a master context
			Win32ContextOGL::OpenGLContext = _context;
			Win32ContextOGL::OpenGLContextWin = _hdc;

			// Init device
			_device->InitWithMainContext();
		}
		else
		{
			// Be a child context
			Win32ContextOGL::ChildWindows.Add(this);
		}

		// Use the main context
		Win32ContextOGL::ContextMakeCurrent(Win32ContextOGL::OpenGLContextWin, Win32ContextOGL::OpenGLContext);
	}
	else
	{
		// TODO: need to resize the backuffer? win32 window gets resized by the platform backend
	}

	// Init back buffer handle
	_backBufferHandle.InitAsBackbuffer(this, GPU_BACK_BUFFER_PIXEL_FORMAT);

	// Cache data
	_width = width;
	_height = height;

	// Calculate memory usage
	_memoryUsage = CalculateTextureMemoryUsage(PixelFormat::R8G8B8A8_UNorm, _width, _height, 1) * 2;

	return false;
}

void Win32GPUSwapChainOGL::Present(bool vsync)
{
	// TODO: vsync on OpenGL
//wglSwapIntervalEXT(vsync ? 1 : 0);
//VALIDATE_OPENGL_RESULT();

	// Present frame
	SwapBuffers(_hdc);

	// Base
	GPUSwapChain::Present(vsync);
}

bool Win32GPUSwapChainOGL::DownloadData(TextureData* result)
{
	MISSING_CODE("Win32GPUSwapChainOGL::DownloadData");
	return true;
}

Task* Win32GPUSwapChainOGL::DownloadDataAsync(TextureData* result)
{
	MISSING_CODE("Win32GPUSwapChainOGL::DownloadDataAsync");
	return nullptr;
}

#endif
