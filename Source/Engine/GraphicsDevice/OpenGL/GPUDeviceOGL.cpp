// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Engine/Graphics/Config.h"

#if GRAPHICS_API_OPENGL

#include "GPUDeviceOGL.h"
#include "GPUResourcesFactoryOGL.h"
#include "GPULimitsOGL.h"
#include "ContextOGL.h"
#include "Win32/Win32ContextOGL.h"
#include "Engine/Core/Log.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Graphics/RenderTask.h"
#include <regex>

GPUDeviceOGL::BlendDesc GPUDeviceOGL::BlendModes[5] =
{
	// Opaque rendering (default)
	{
		false,
		GL_ONE, GL_ZERO, GL_FUNC_ADD,
		GL_ONE, GL_ZERO, GL_FUNC_ADD,
	},

	// Additive rendering
	{
		true,
		GL_SRC_ALPHA, GL_ONE, GL_FUNC_ADD,
		GL_SRC_ALPHA, GL_ONE, GL_FUNC_ADD,
	},

	// Alpha blended rendering
	{
		true,
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_FUNC_ADD,
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_FUNC_ADD,
	},

	// Add color rendering
	{
		true,
		GL_ONE, GL_ONE, GL_FUNC_ADD,
		GL_ONE, GL_ONE, GL_FUNC_ADD,
	},

	// Multiply output color with texture color
	{
		true,
		GL_ZERO, GL_SRC_COLOR, GL_FUNC_ADD,
		GL_ZERO, GL_SRC_ALPHA, GL_FUNC_ADD,
	},
};

#define DEFINE_GL_ENTRYPOINTS(Type,Func) Type Func = NULL;
ENUM_GL_ENTRYPOINTS_ALL(DEFINE_GL_ENTRYPOINTS);

GPUDeviceOGL::GPUDeviceOGL(RendererType type, ShaderProfile profile, AdapterOGL* adapter, GPULimits* limits)
	: GPUDevice(type, profile, limits, New<GPUResourcesFactoryOGL>(this))
	, _adapter(adapter)
{
}

GPUDeviceOGL* GPUDeviceOGL::Create()
{
#define CHECK_NULL(obj) if (obj == nullptr) { return nullptr; }

	bool GRunningUnderRenderDoc = false;

	// Create a dummy context so that wglCreateContextAttribsARB can be initialized
	Win32ContextOGL::Data DummyContext;
	Win32ContextOGL::CreateDummyGLWindow(&DummyContext);
	DummyContext.OpenGLContext = wglCreateContext(DummyContext.DeviceContext);
	CHECK_NULL(DummyContext.OpenGLContext);
	ASSERT(DummyContext.OpenGLContext);
	Win32ContextOGL::ContextMakeCurrent(DummyContext.DeviceContext, DummyContext.OpenGLContext);
	Win32ContextOGL::wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetGLFuncAddress("wglCreateContextAttribsARB");
	CHECK_NULL(DummyContext.OpenGLContext);
	Win32ContextOGL::ContextMakeCurrent(NULL, NULL);
	wglDeleteContext(DummyContext.OpenGLContext);

#if GRAPHICS_API_OPENGLES
	todo_support-opengles
#else
	// Try to create OpenGL 4.4 context
	Win32ContextOGL::PlatformCreateOpenGLContextCore(&DummyContext, 4, 4, NULL);
	if (DummyContext.OpenGLContext == nullptr)
	{
		// Try to create OpenGL 4.1 context
		Win32ContextOGL::PlatformCreateOpenGLContextCore(&DummyContext, 4, 1, NULL);
		if (DummyContext.OpenGLContext == nullptr)
		{
			LOG(Error, "OpenGL 4.1 is not supported by the driver.");
			return nullptr;
		}
	}
#endif
	Win32ContextOGL::ContextMakeCurrent(DummyContext.DeviceContext, DummyContext.OpenGLContext);

	// Get all OpenGL functions from the OpenGL DLL
	{
		// Retrieve the OpenGL DLL
		void* OpenGLDLL = Platform::GetDllHandle(TEXT("opengl32.dll"));
		if (!OpenGLDLL)
		{
			LOG(Error, "Couldn't load opengl32.dll");
			return nullptr;
		}

		// Initialize entry points required from opengl32.dll
#define GET_GL_ENTRYPOINTS_DLL(Type,Func) Func = (Type)Platform::GetDllExport(OpenGLDLL, #Func);
		ENUM_GL_ENTRYPOINTS_DLL(GET_GL_ENTRYPOINTS_DLL);

		// Release the OpenGL DLL
		Platform::FreeDllHandle(OpenGLDLL);

		// Initialize all entry points required by Flax
#define GET_GL_ENTRYPOINTS(Type,Func) Func = (Type)wglGetProcAddress(#Func);
		ENUM_GL_ENTRYPOINTS(GET_GL_ENTRYPOINTS);
		ENUM_GL_ENTRYPOINTS_OPTIONAL(GET_GL_ENTRYPOINTS);

		// Check that all of the entry points have been initialized
		bool isMissing = false;
#define CHECK_GL_ENTRYPOINTS(Type,Func) if (Func == NULL) { isMissing = true; LOG(Warning, "Failed to find entry point for {0}", TEXT(#Func)); }
		ENUM_GL_ENTRYPOINTS_DLL(CHECK_GL_ENTRYPOINTS);
		ENUM_GL_ENTRYPOINTS(CHECK_GL_ENTRYPOINTS);
		if (isMissing)
		{
			LOG(Error, "Failed to find all OpenGL entry points.");
			return nullptr;
		}
	}

	// Create adapter
	auto adapter = New<AdapterOGL>();
	if (adapter->Init(DummyContext.DeviceContext))
	{
        Delete(adapter);
		LOG(Error, "Failed to init OpenGL adapter.");
		return nullptr;
	}

	// Create device
	GPUDeviceOGL* device = nullptr;
#if GRAPHICS_API_OPENGLES
	{
		todo_support_opengles
		//device = New<GPUDeviceOGLES3>(adapter);
	}
#else
	{
		if (adapter->Version >= 440)
			device = New<GPUDeviceOGL4_4>(adapter);
		else
			device = New<GPUDeviceOGL4_1>(adapter);
	}
#endif
	if (device->Init())
	{
		LOG(Warning, "Graphics Device init failed");
		Delete(device);
		return nullptr;
	}

	return device;

#undef CHECK_NULL
}

bool GPUDeviceOGL::Init()
{
	// Base
	if (GPUDevice::Init())
		return true;

	_state = DeviceState::Created;

	// Init device limits
	if (Limits->Init())
	{
		LOG(Warning, "Cannot initialize device limits.");
		return true;
	}

	// Create main context
	_mainContext = New<GPUContextOGL>(this);

	// TODO: create vertex buffer for the quad drawing?

	// Finished
	_state = DeviceState::Inited;
	return false;
}

bool GPUDeviceOGL::CanDraw()
{
	return GPUDevice::CanDraw() && ContextOGL::IsReady();
}

GPUDeviceOGL::~GPUDeviceOGL()
{
	// Ensure to be disposed
	GPUDeviceOGL::Dispose();
}

#if GPU_OGL_USE_DEBUG_LAYER

void OpenGlErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	if (type != GL_DEBUG_TYPE_PERFORMANCE && type != GL_DEBUG_TYPE_OTHER)
	{
		LOG(Warning, "OpenGL error: {0}", (const char*)message);
	}
}

#endif

void GPUDeviceOGL::InitWithMainContext()
{
#if GPU_OGL_USE_DEBUG_LAYER
	if (_adapter->HasExtension("GL_ARB_debug_output"))
	{
		glDebugMessageCallbackARB(&OpenGlErrorCallback, 0);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}
#endif

	// Init some OpenGL states
	glFrontFace(GL_CW);

	// Intel HD4000 under <= 10.8.4 requires GL_DITHER disabled or dithering will occur on any channel < 8bits.
	// No other driver does this but we don't need GL_DITHER on anyway.
	glDisable(GL_DITHER);

	// Render targets with sRGB flag should do sRGB conversion like in D3D11
	glEnable(GL_FRAMEBUFFER_SRGB);

	// Engine always expects seamless cubemap, so enable it if available
	if (GetLimits()->SupportsSeamlessCubemap)
	{
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}

#if PLATFORM_WINDOWS || PLATFORM_LINUX
	if (GetLimits()->SupportsClipControl)
	{
		glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
	}
#endif
}

void GPUDeviceOGL::Dispose()
{
	GPUDeviceLock lock(this);

	// Check if has been disposed already
	if (_state == DeviceState::Disposed)
		return;

	// Set current state
	_state = DeviceState::Disposing;

	// Wait for rendering end
	WaitForGPU();

	// Pre dispose
	preDispose();

	// Clear device resources
	FBOCache.Dispose();
	VAOCache.Dispose();

	// Clear OpenGL stuff
	SAFE_DELETE(_mainContext);
	SAFE_DELETE(_adapter);
	
	// Base
	GPUDevice::Dispose();

	// Set current state
	_state = DeviceState::Disposed;
}

void GPUDeviceOGL::WaitForGPU()
{
	// Note: in OpenGL driver manages CPU/GPU work synchronization and work submission
}

#if GRAPHICS_API_OPENGLES

GPUDeviceOGLES3::GPUDeviceOGLES3(AdapterOGL* adapter)
	: GPUDeviceOGL(RendererType::OpenGLES3, ShaderProfile::Unknown, adapter, New<GPULimitsOGL>(this))
{
}

GPUDeviceOGLES3_1::GPUDeviceOGLES3_1(AdapterOGL* adapter)
	: GPUDeviceOGL(RendererType::OpenGLES3_1, ShaderProfile::Unknown, adapter, New<GPULimitsOGL>(this))
{
}

#else

GPUDeviceOGL4_1::GPUDeviceOGL4_1(AdapterOGL* adapter)
	: GPUDeviceOGL(RendererType::OpenGL4_1, ShaderProfile::GLSL_410, adapter, New<GPULimitsOGL>(this))
{
}

GPUDeviceOGL4_4::GPUDeviceOGL4_4(AdapterOGL* adapter)
	: GPUDeviceOGL(RendererType::OpenGL4_4, ShaderProfile::GLSL_440, adapter, New<GPULimitsOGL>(this))
{
}

#endif

#endif
