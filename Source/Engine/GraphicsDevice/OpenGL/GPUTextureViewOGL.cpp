// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GPUTextureViewOGL.h"

#if GRAPHICS_API_OPENGL

#include "TextureOGL.h"
#include "GPUDeviceOGL.h"
#include "GPULimitsOGL.h"
#include "RenderToolsOGL.h"

GPUTextureViewOGL::~GPUTextureViewOGL()
{
	if (ViewID != 0)
	{
		glDeleteTextures(1, &ViewID);
		VALIDATE_OPENGL_RESULT();
	}
}

void GPUTextureViewOGL::InitAsBackbuffer(GPUResource* parent, PixelFormat format)
{
	ASSERT(ViewID == 0);

	ViewTarget = 0;
	Texture = nullptr;
	IsFullView = true;
	MipLevels = 1;
	FirstArraySlice = 0;
	NumArraySlices = 1;
	MostDetailedMip = 0;

	Init(parent, format);
}

void GPUTextureViewOGL::InitAsFull(TextureOGL* parent)
{
	InitAsFull(parent, parent->Format());
}

void GPUTextureViewOGL::InitAsFull(TextureOGL* parent, PixelFormat format)
{
	ASSERT(ViewID == 0);

	ViewTarget = parent->Target;
	Texture = parent;
	IsFullView = true;
	MipLevels = parent->MipLevels();
	FirstArraySlice = 0;
	NumArraySlices = parent->ArraySize();
	MostDetailedMip = 0;

	Init(parent, format);
}

void GPUTextureViewOGL::InitAsView(TextureOGL* parent, PixelFormat format, ViewType type, int32 mipLevels, int32 firstArraySlice, int32 numArraySlices, int32 mostDetailedMipIndex)
{
	ASSERT(ViewID == 0);

	switch (type)
	{
	case ViewType::Texture1D:
		ViewTarget = GL_TEXTURE_1D;
		break;

	case ViewType::Texture1D_Array:
		ViewTarget = GL_TEXTURE_1D_ARRAY;
		break;

	case ViewType::Texture2D:
		ViewTarget = parent->IsMultiSample() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		break;

	case ViewType::Texture2D_Array:
		ViewTarget = parent->IsMultiSample() ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;
		break;

	case ViewType::Texture3D:
		ViewTarget = GL_TEXTURE_3D;
		break;

	case ViewType::TextureCube:
		ViewTarget = GL_TEXTURE_CUBE_MAP;
		break;

	case ViewType::TextureCube_Array:
		ViewTarget = GL_TEXTURE_CUBE_MAP_ARRAY;
		break;
	}

	glGenTextures(1, &ViewID);
	VALIDATE_OPENGL_RESULT();

	auto internalFormat = parent->GetDevice()->GetLimits()->GetInternalTextureFormat(format);
	glTextureView(ViewID, ViewTarget, parent->TextureID, internalFormat, mostDetailedMipIndex, mipLevels, firstArraySlice, numArraySlices);
	VALIDATE_OPENGL_RESULT();

	Texture = parent;
	IsFullView = false;
	MipLevels = mipLevels;
	FirstArraySlice = firstArraySlice;
	NumArraySlices = numArraySlices;
	MostDetailedMip = mostDetailedMipIndex;
}

void GPUTextureViewOGL::AttachToFramebuffer(GLenum attachmentPoint)
{
	auto desc = GetTexture()->GetDescription();
	auto textureId = GetTexture()->TextureID;
	switch (desc.Dimensions)
	{
	case TextureDimensions::Texture:
	{
		if (desc.IsArray())
		{
			if (NumArraySlices == desc.ArraySize)
			{
				glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip);
				VALIDATE_OPENGL_RESULT();
				glFramebufferTexture(GL_READ_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip);
				VALIDATE_OPENGL_RESULT();
			}
			else if (NumArraySlices == 1)
			{
				// Texture name must either be zero or the name of an existing 3D texture, 1D or 2D array texture, 
				// cube map array texture, or multisample array texture.
				glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip, FirstArraySlice);
				VALIDATE_OPENGL_RESULT();
				glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip, FirstArraySlice);
				VALIDATE_OPENGL_RESULT();
			}
			else
			{
				LOG(Fatal, "Only one slice or the entire texture array can be attached to a framebuffer");
			}
		}
		else
		{
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachmentPoint, GetTexture()->Target, textureId, MostDetailedMip);
			VALIDATE_OPENGL_RESULT();
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, attachmentPoint, GetTexture()->Target, textureId, MostDetailedMip);
			VALIDATE_OPENGL_RESULT();
		}
	}
	break;
	case TextureDimensions::CubeTexture:
	{
		if (desc.IsArray())
		{
			// Every OpenGL API call that operates on cubemap array textures takes layer-faces, not array layers. 
			// So the parameters that represent the Z component are layer-faces. 
			if (NumArraySlices == desc.ArraySize)
			{
				// glFramebufferTexture() attaches the given mipmap levelas a layered image with the number of layers that the given texture has.
				glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip);
				VALIDATE_OPENGL_RESULT();
				glFramebufferTexture(GL_READ_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip);
				VALIDATE_OPENGL_RESULT();
			}
			else if (NumArraySlices == 1)
			{
				// Texture name must either be zero or the name of an existing 3D texture, 1D or 2D array texture, 
				// cube map array texture, or multisample array texture.
				glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip, FirstArraySlice);
				VALIDATE_OPENGL_RESULT();
				glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip, FirstArraySlice);
				VALIDATE_OPENGL_RESULT();
			}
			else
			{
				LOG(Fatal, "Only one slice or the entire cubemap array can be attached to a framebuffer");
			}
		}
		else
		{
			if (NumArraySlices == desc.ArraySize)
			{
				glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip);
				VALIDATE_OPENGL_RESULT();
				glFramebufferTexture(GL_READ_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip);
				VALIDATE_OPENGL_RESULT();
			}
			else if (NumArraySlices == 1)
			{
				// Texture name must either be zero or the name of an existing 3D texture, 1D or 2D array texture, 
				// cube map array texture, or multisample array texture.
				static const GLenum CubeMapFaces[6] =
				{
					GL_TEXTURE_CUBE_MAP_POSITIVE_X,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
					GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
					GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
					GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
				};
				auto CubeMapFaceBindTarget = CubeMapFaces[FirstArraySlice];

				// For glFramebufferTexture2D, if texture is not zero, textarget must be one of GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE, 
// GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 
// GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 
				// or GL_TEXTURE_2D_MULTISAMPLE.
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachmentPoint, textureId, CubeMapFaceBindTarget, MostDetailedMip);
				VALIDATE_OPENGL_RESULT();
				glFramebufferTexture2D(GL_READ_FRAMEBUFFER, attachmentPoint, textureId, CubeMapFaceBindTarget, MostDetailedMip);
				VALIDATE_OPENGL_RESULT();
			}
			else
			{
				LOG(Fatal, "Only one slice or the entire cubemap can be attached to a framebuffer");
			}
		}
	}
	break;
	case TextureDimensions::VolumeTexture:
	{
		if (NumArraySlices == desc.ArraySize)
		{
			glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip);
			VALIDATE_OPENGL_RESULT();
			glFramebufferTexture(GL_READ_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip);
			VALIDATE_OPENGL_RESULT();
		}
		else if (NumArraySlices == 1)
		{
			glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip, FirstArraySlice);
			VALIDATE_OPENGL_RESULT();
			glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, attachmentPoint, textureId, MostDetailedMip, FirstArraySlice);
			VALIDATE_OPENGL_RESULT();
		}
		else
		{
			LOG(Fatal, "Only one slice or the entire 3D texture can be attached to a framebuffer");
		}
	}
	break;
	}
}

void GPUTextureViewOGL::Release()
{
	if (ViewID != 0)
	{
		glDeleteTextures(1, &ViewID);
		VALIDATE_OPENGL_RESULT();
		ViewID = 0;
	}
	ViewTarget = 0;
}

void GPUTextureViewOGL::Bind(int32 slotIndex)
{
	if (IsFullView)
	{
		glBindTexture(Texture->Target, Texture->TextureID);
		VALIDATE_OPENGL_RESULT();
	}
	else
	{
		// TODO: bind image with glBindImageTexture
		CRASH;
	}
}

#endif
