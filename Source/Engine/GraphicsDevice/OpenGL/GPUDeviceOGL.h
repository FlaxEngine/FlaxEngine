// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_OPENGL

#include "Engine/Graphics/Config.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "AdapterOGL.h"
#include "FBOCache.h"
#include "VAOCache.h"
#include "Engine/Graphics/GPUDevice.h"

class Engine;
class GPUContextOGL;
class GPULimitsOGL;
class GPUSwapChainOGL;

/// <summary>
/// Base for all OpenGL graphics devices.
/// </summary>
class GPUDeviceOGL : public GPUDevice
{
	friend GPUContextOGL;
	friend GPUSwapChainOGL;

public:

	typedef struct BlendDesc
	{
		bool BlendEnable;
		GLenum SrcBlend;
		GLenum DestBlend;
		GLenum BlendOp;
		GLenum SrcBlendAlpha;
		GLenum DestBlendAlpha;
		GLenum BlendOpAlpha;
	} BlendDesc;

	static BlendDesc BlendModes[5];

protected:

	GPUContextOGL* _mainContext;
	AdapterOGL* _adapter;

protected:

	GPUDeviceOGL(RendererType type, ShaderProfile profile, AdapterOGL* adapter, GPULimits* limits);

public:

	static GPUDeviceOGL* Create();

	~GPUDeviceOGL();

public:

	GPULimitsOGL* GetLimits()
	{
		return (GPULimitsOGL*)Limits;
	}

	/// <summary>
	/// The frame buffer objects cache.
	/// </summary>
	FBOCache FBOCache;

	/// <summary>
	/// The vertex array objects cache.
	/// </summary>
	VAOCache VAOCache;

	/// <summary>
	/// Performs graphics device initialization after OpenGL main context creation and assign.
	/// </summary>
	void InitWithMainContext();

public:

	// [GPUDevice]
	Adapter* GetAdapter() const override
	{
		return _adapter;
	}
	GPUContext* GetMainContext() override
	{
		return reinterpret_cast<GPUContext*>(_mainContext);
	}
	void* GetNativePtr() const override
	{
		return nullptr;
	}
	bool Init() override;
	bool CanDraw() override;
	void WaitForGPU() override;
	void Dispose() override;
    Texture* CreateTexture(const StringView& name) override
	{
		return New<GPUTextureOGL>(_device, name);
	}
	Shader* CreateShader(const StringView& name) override
	{
		return New<GPUShaderOGL>(_device, name);
	}
	PipelineState* CreatePipelineState() override
	{
		return New<GPUPipelineStateOGL>(_device);
	}
	GPUTimerQuery* CreateTimerQuery() override
	{
		return New<GPUTimerQueryOGL>(_device);
	}
	Buffer* CreateBuffer(const StringView& name) override
	{
		return New<GPUBufferOGL>(_device, name);
	}
	GPUSwapChain* CreateGPUSwapChain(Window* parent, int32 width, int32 height, bool fullscreen) override
	{
		return GPUSwapChainOGL::Create(_device, parent, width, height, fullscreen);
	}
};

#if GRAPHICS_API_OPENGLES

/// <summary>
/// Base for all OpenGL ES graphics devices
/// </summary>
class GPUDeviceOGLES : public GPUDeviceOGL
{
};

/// <summary>
/// Implementation of Graphics Device for OpenGL ES 3.0 (or higher) rendering system.
/// </summary>
class GPUDeviceOGLES3 : public GPUDeviceOGLES
{
public:

	GPUDeviceOGLES3(AdapterOGL* adapter);
};

/// <summary>
/// Implementation of Graphics Device for OpenGL ES 3.1 (or higher) rendering system.
/// </summary>
class GPUDeviceOGLES3_1 : public GPUDeviceOGLES
{
public:

	GPUDeviceOGLES3_1(AdapterOGL* adapter);
};

#else

/// <summary>
/// Implementation of Graphics Device for OpenGL 4.1 (or higher) rendering system.
/// </summary>
class GPUDeviceOGL4_1 : public GPUDeviceOGL
{
public:

	GPUDeviceOGL4_1(AdapterOGL* adapter);
};

/// <summary>
/// Implementation of Graphics Device for OpenGL 4.4 (or higher) rendering system.
/// </summary>
class GPUDeviceOGL4_4 : public GPUDeviceOGL
{
public:

	GPUDeviceOGL4_4(AdapterOGL* adapter);
};

#endif

#endif
