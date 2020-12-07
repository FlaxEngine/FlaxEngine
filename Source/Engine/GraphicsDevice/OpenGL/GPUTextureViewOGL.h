// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUTextureView.h"
#include "IncludeOpenGLHeaders.h"

#if GRAPHICS_API_OPENGL

#include "IShaderResourceOGL.h"

class TextureOGL;

/// <summary>
/// Render target handle for OpenGL
/// </summary>
class GPUTextureViewOGL : public GPUTextureView, public IShaderResourceOGL
{
public:

	/// <summary>
	/// The OpenGL texture view target.
	/// </summary>
	GLuint ViewTarget;

	/// <summary>
	/// The OpenGL texture view target.
	/// </summary>
	GLuint ViewID;

public:

	GPUTextureViewOGL()
	{
		ViewTarget = 0;
		ViewID = 0;
		Texture = nullptr;
	}

	~GPUTextureViewOGL();

public:

	TextureOGL* Texture;
	bool IsFullView;
	int32 MipLevels;
	int32 FirstArraySlice;
	int32 NumArraySlices;
	int32 MostDetailedMip;

public:

	bool IsBackbuffer() const
	{
		// Render output backbuffers are used as a special case for rendering in OpenGL
		return IsFullView && Texture == nullptr;
	}

	TextureOGL* GetTexture() const
	{
		return Texture;
	}

	void InitAsBackbuffer(GPUResource* parent, PixelFormat format);

	void InitAsFull(TextureOGL* parent);

	void InitAsFull(TextureOGL* parent, PixelFormat format);

	enum class ViewType
	{
		Texture1D,
		Texture1D_Array,
		Texture2D,
		Texture2D_Array,
		Texture3D,
		TextureCube,
		TextureCube_Array,
	};

	void InitAsView(TextureOGL* parent, PixelFormat format, ViewType type, int32 mipLevels, int32 firstArraySlice, int32 numArraySlices, int32 mostDetailedMipIndex = 0);

	void AttachToFramebuffer(GLenum attachmentPoint);

	void Release();

public:

	// [IShaderResourceOGL]
	void Bind(int32 slotIndex) override;
};

#endif
