// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "TextureOGL.h"

#if GRAPHICS_API_OPENGL

#include "RenderToolsOGL.h"
#include "GPUDeviceOGL.h"
#include "GPULimitsOGL.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

TextureOGL::TextureOGL(GPUDeviceOGL* device, const String& name)
	: GPUResourceOGL(device, name)
{
}

bool TextureOGL::init()
{
	ASSERT(IsInMainThread());

	switch (_desc.Dimensions)
	{
	case TextureDimensions::Texture:
	{
		if (IsMultiSample())
		{
			Target = IsArray() ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_MULTISAMPLE;
		}
		else
		{
			Target = IsArray() ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
		}
	}
	break;
	case TextureDimensions::CubeTexture:
	{
		Target = IsArray() ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
	}
	break;
	case TextureDimensions::VolumeTexture:
	{
		Target = GL_TEXTURE_3D;
	}
	break;
	}

	// Generate texture handle
	glGenTextures(1, &TextureID);
	VALIDATE_OPENGL_RESULT();

	// Set texture type
	glBindTexture(Target, TextureID);
	VALIDATE_OPENGL_RESULT();
	
	glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, _desc.MipLevels - 1);
	VALIDATE_OPENGL_RESULT();

	// Allocate internal buffer so that glTexSubImageXD can be used
	FormatGL = _device->GetLimits()->GetInternalTextureFormat(_desc.Format, _desc.Flags);

	// Init storage info
	switch (_desc.Dimensions)
	{
	case TextureDimensions::Texture:
	{
		if (IsMultiSample())
		{
			if (IsArray())
			{
				CRASH;
				//glTexStorage3DMultisample(Target, (GLsizei)_desc.MultiSampleLevel, FormatGL, _desc.Width, _desc.Height, _desc.ArraySize, GL_TRUE);
			}
			else
			{
				glTexStorage2DMultisample(Target, (GLsizei)_desc.MultiSampleLevel, FormatGL, _desc.Width, _desc.Height, GL_TRUE);
			}
		}
		else
		{
			if (IsArray())
			{
				glTexStorage3D(Target, _desc.MipLevels, FormatGL, _desc.Width, _desc.Height, _desc.ArraySize);
			}
			else
			{
				glTexStorage2D(Target, _desc.MipLevels, FormatGL, _desc.Width, _desc.Height);
			}
		}
	}
	break;
	case TextureDimensions::CubeTexture:
	{
		if (IsArray())
		{
			glTexStorage3D(Target, _desc.MipLevels, FormatGL, _desc.Width, _desc.Height, _desc.ArraySize);
		}
		else
		{
			glTexStorage2D(Target, _desc.MipLevels, FormatGL, _desc.Width, _desc.Height);
		}
	}
	break;
	case TextureDimensions::VolumeTexture:
	{
		glTexStorage3D(Target, _desc.MipLevels, FormatGL, _desc.Width, _desc.Height, _desc.Depth);
	}
	break;
	}
	VALIDATE_OPENGL_RESULT();

	// Update memory usage
	_memoryUsage = calculateMemoryUsage();

	// Initialize handles to the resource
	if (IsRegularTexture())
	{
		// 'Regular' texture is using only one handle (texture/cubemap)
		_handlesPerSlice.Resize(1, false);
		_handlesPerSlice[0].InitAsFull(this);
	}
	else
	{
		// Create all handles
		initHandles();
	}

	return false;
}

void TextureOGL::onResidentMipsChanged()
{
	// TODO: change GL_TEXTURE_BASE_LEVEL and GL_TEXTURE_MAX_LEVEL to match D3D11 behaviour
}

void TextureOGL::release()
{
	if (IsRenderTarget() || IsUnorderedAccess())
		_device->FBOCache.OnTextureRelease(this);

	// Release views
	_handlesPerMip.Resize(0, false);
	_handlesPerSlice.Resize(0, false);
	_handleArray.Release();
	_handleVolume.Release();

	// Release texture
	glDeleteTextures(1, &TextureID);
	VALIDATE_OPENGL_RESULT();
	TextureID = 0;
	Target = 0;
	FormatGL = 0;
	_memoryUsage = 0;

	// Base
	Texture::release();
}

bool TextureOGL::GetData(int32 arrayOrDepthSliceIndex, int32 mipMapIndex, TextureData::MipData& data, uint32 mipRowPitch)
{
	MISSING_CODE("TextureOGL::GetData");

	return true;
}

#endif
