// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "../IncludeOpenGLHeaders.h"

#if GRAPHICS_API_OPENGL && PLATFORM_WINDOWS

#include "Engine/Core/Common.h"

class Win32GPUSwapChainOGL;

class Win32ContextOGL
{
public:

	// Platform specific OpenGL context.
	struct Data
	{
		HWND WindowHandle;
		HDC DeviceContext;
		HGLRC OpenGLContext;
		bool bReleaseWindowOnDestroy;
		int32 SyncInterval;
		GLuint ViewportFramebuffer;
		GLuint VertexArrayObject;	// one has to be generated and set for each context (OpenGL 3.2 Core requirements)
		GLuint BackBufferResource; // TODO: use it
		GLenum BackBufferTarget; // TODO: use it

		~Data()
		{
			if (OpenGLContext)
			{
				ContextMakeCurrent(NULL, NULL);
				wglDeleteContext(OpenGLContext);
			}
			ReleaseDC(WindowHandle, DeviceContext);
			ASSERT(bReleaseWindowOnDestroy);
			DestroyWindow(WindowHandle);
		}
	};

	static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

public:

	static HGLRC OpenGLContext;
	static HDC OpenGLContextWin;
	static Array<Win32GPUSwapChainOGL*, FixedAllocation<32>> ChildWindows;

public:

	static bool IsReady()
	{
		return OpenGLContext != 0;
	}

	static void ContextMakeCurrent(HDC dc, HGLRC rc)
	{
		BOOL result = wglMakeCurrent(dc, rc);
		if (!result)
		{
			result = wglMakeCurrent(nullptr, nullptr);
		}
		ASSERT(result);
	}

	static HGLRC GetCurrentContext()
	{
		return wglGetCurrentContext();
	}

	static void PlatformInitPixelFormatForDevice(HDC context);
	static void CreateDummyGLWindow(Data* context);
	static void PlatformCreateOpenGLContextCore(Data* result, int majorVersion, int minorVersion, HGLRC parentContext);
	static void PlatformCreateOpenGLContextCore(HGLRC* result, HDC deviceContext, int majorVersion, int minorVersion, HGLRC parentContext);
	static void PlatformCreateOpenGLContextCore(HGLRC* result, HDC deviceContext, HGLRC parentContext);
};

#endif
