// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "FBOCache.h"

#if GRAPHICS_API_OPENGL

#include "TextureOGL.h"
#include "RenderToolsOGL.h"

FBOCache::FBOCache()
	: Table(2048)
{
}

FBOCache::~FBOCache()
{
	Dispose();
}

GLuint FBOCache::GetFBO(uint32 rtCount, GPUTextureViewOGL* depthStencil, GPUTextureViewOGL* rts[])
{
	ASSERT(rtCount > 0 || depthStencil != nullptr);

	Key key(rtCount, depthStencil, rts);
	GLuint fbo;

	if (!Table.TryGet(key, fbo))
	{
		// Create new FBO
		glGenFramebuffers(1, &fbo);
		VALIDATE_OPENGL_RESULT();

		// Bind FBO
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		VALIDATE_OPENGL_RESULT();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		VALIDATE_OPENGL_RESULT();

		// Initialize FBO
		for (int32 i = 0; i < rtCount; i++)
		{
			rts[i]->AttachToFramebuffer(GL_COLOR_ATTACHMENT0 + i);
		}
		if(depthStencil)
		{
			GLenum attachmentPoint = PixelFormatExtensions::HasStencil(depthStencil->GetFormat()) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
			depthStencil->AttachToFramebuffer(attachmentPoint);
		}

		// We now need to set mapping between shader outputs and 
// color attachments. This largely redundant step is performed
		// by glDrawBuffers()
		static const GLenum DrawBuffers[] =
		{
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
			GL_COLOR_ATTACHMENT4,
			GL_COLOR_ATTACHMENT5,
			GL_COLOR_ATTACHMENT6,
			GL_COLOR_ATTACHMENT7,
			GL_COLOR_ATTACHMENT8,
			GL_COLOR_ATTACHMENT9,
			GL_COLOR_ATTACHMENT10,
			GL_COLOR_ATTACHMENT11,
			GL_COLOR_ATTACHMENT12,
			GL_COLOR_ATTACHMENT13,
			GL_COLOR_ATTACHMENT14,
			GL_COLOR_ATTACHMENT15
		};
		// The state set by glDrawBuffers() is part of the state of the framebuffer. 
		// So it can be set up once and left it set.
		glDrawBuffers(rtCount, DrawBuffers);
		VALIDATE_OPENGL_RESULT();

		// Validate
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			const Char* str = TEXT("Unknown");
			switch (status)
			{
#define STATUS_TO_STR(name) case name: str = TEXT(#name); break
				STATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
				STATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
				STATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
				STATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
				STATUS_TO_STR(GL_FRAMEBUFFER_UNSUPPORTED);
				STATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
				STATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
#undef STATUS_TO_STR
			}
			LOG(Error, "Framebuffer is incomplete. Status: {0}", str);
		}

		// Register in cache
		Table.Add(key, fbo);
	}

	return fbo;
}

void FBOCache::OnTextureRelease(TextureOGL* texture)
{
	for (auto i = Table.Begin(); i.IsNotEnd(); ++i)
	{
		if (i->Key.HasReference(texture))
		{
			GLuint fbo = i->Value;
			glDeleteFramebuffers(1, &fbo);
			Table.Remove(i);
		}
	}
}

void FBOCache::Dispose()
{
	for (auto i = Table.Begin(); i.IsNotEnd(); ++i)
	{
		GLuint fbo = i->Value;
		glDeleteFramebuffers(1, &fbo);
	}
	Table.Clear();
}

FBOCache::Key::Key(uint32 rtCount, GPUTextureViewOGL* depthStencil, GPUTextureViewOGL* rts[])
{
	Hash = rtCount * 371;
	HashCombinePointer(Hash, depthStencil);
	RTCount = rtCount;
	DepthStencil = depthStencil;
	for (int32 i = 0; i < rtCount; i++)
	{
		RT[i] = rts[i];
		HashCombinePointer(Hash, rts[i]);
	}
}

bool FBOCache::Key::HasReference(TextureOGL* texture)
{
	for (int32 i = 0; i < RTCount; i++)
	{
		if (RT[i] && RT[i]->GetTexture() == texture)
			return true;
	}
	return (DepthStencil && DepthStencil->GetTexture() == texture);
}

bool FBOCache::Key::operator==(const Key & other) const
{
	if (Hash != 0 && other.Hash != 0 && Hash != other.Hash)
		return false;

	if (RTCount != other.RTCount)
		return false;

	for (int32 rt = 0; rt < RTCount; rt++)
	{
		if (RT[rt] != other.RT[rt])
			return false;
	}

	return DepthStencil == other.DepthStencil;
}

#endif
