// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Win32ContextOGL.h"

#if GRAPHICS_API_OPENGL && PLATFORM_WINDOWS

#include "Engine/Platform/Platform.h"
#include "Engine/Core/Log.h"
#include "../AdapterOGL.h"
#include "../GPUDeviceOGL.h"

PFNWGLCREATECONTEXTATTRIBSARBPROC Win32ContextOGL::wglCreateContextAttribsARB = NULL;

HGLRC Win32ContextOGL::OpenGLContext = 0;
HDC Win32ContextOGL::OpenGLContextWin = 0;
Array<Win32GPUSwapChainOGL*, FixedAllocation<32>> Win32ContextOGL::ChildWindows;

// A dummy window procedure (for WinAPI).
static LRESULT CALLBACK PlatformDummyGLWndproc(HWND hWnd, uint32 Message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, Message, wParam, lParam);
}

// Initializes a pixel format descriptor for the given window handle.
void Win32ContextOGL::PlatformInitPixelFormatForDevice(HDC context)
{
	// Pixel format descriptor for the context
	PIXELFORMATDESCRIPTOR PixelFormatDesc;
	Platform::MemoryClear(&PixelFormatDesc, sizeof(PixelFormatDesc));
	PixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	PixelFormatDesc.nVersion = 1;
	PixelFormatDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	PixelFormatDesc.iPixelType = PFD_TYPE_RGBA;
	PixelFormatDesc.cColorBits = 32;
	PixelFormatDesc.cDepthBits = 0;
	PixelFormatDesc.cStencilBits = 0;
	PixelFormatDesc.iLayerType = PFD_MAIN_PLANE;

	// Set the pixel format
	int32 PixelFormat = ChoosePixelFormat(context, &PixelFormatDesc);
	if (!PixelFormat || !SetPixelFormat(context, PixelFormat, &PixelFormatDesc))
	{
		LOG(Error, "Failed to set pixel format for device context.");
	}
}

// Creates a dummy window used to construct OpenGL contexts.
void Win32ContextOGL::CreateDummyGLWindow(Data* context)
{
	const Char* DummyWindowClassName = TEXT("DummyGLWindow");

	// Register a dummy window class
	{
		WNDCLASS wc;
		Platform::MemoryClear(&wc, sizeof(wc));
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = PlatformDummyGLWndproc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = NULL;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = (HBRUSH)(COLOR_MENUTEXT);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = DummyWindowClassName;
		ATOM ClassAtom = ::RegisterClass(&wc);
		ASSERT(ClassAtom);
	}

	// Create a dummy window
	context->WindowHandle = CreateWindowEx(
		WS_EX_WINDOWEDGE,
		DummyWindowClassName,
		NULL,
		WS_POPUP,
		0, 0, 1, 1,
		NULL, NULL, NULL, NULL);
	ASSERT(context->WindowHandle);
	context->bReleaseWindowOnDestroy = true;

	// Get the device context.
	context->DeviceContext = GetDC(context->WindowHandle);
	ASSERT(context->DeviceContext);
	PlatformInitPixelFormatForDevice(context->DeviceContext);
}

// Create a core profile OpenGL context.
void Win32ContextOGL::PlatformCreateOpenGLContextCore(Data* result, int majorVersion, int minorVersion, HGLRC parentContext)
{
	ASSERT(result);
	
	result->SyncInterval = -1;	// invalid value to enforce setup on first buffer swap
	result->ViewportFramebuffer = 0;

	PlatformCreateOpenGLContextCore(&result->OpenGLContext, result->DeviceContext, majorVersion, minorVersion, parentContext);
}

void Win32ContextOGL::PlatformCreateOpenGLContextCore(HGLRC* result, HDC deviceContext, int majorVersion, int minorVersion, HGLRC parentContext)
{
	ASSERT(wglCreateContextAttribsARB);
	ASSERT(result && deviceContext);
	
	int debugFlag = 0;
#if GPU_OGL_USE_DEBUG_LAYER
	debugFlag = WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

	int attributes[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, majorVersion,
		WGL_CONTEXT_MINOR_VERSION_ARB, minorVersion,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | debugFlag,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	*result = wglCreateContextAttribsARB(deviceContext, parentContext, attributes);
}

void Win32ContextOGL::PlatformCreateOpenGLContextCore(HGLRC* result, HDC deviceContext, HGLRC parentContext)
{
	auto adapter = (AdapterOGL*)GPUDevice::Instance->GetAdapter();
	PlatformCreateOpenGLContextCore(result, deviceContext, adapter->VersionMajor, adapter->VersionMinor, parentContext);
}

#endif
