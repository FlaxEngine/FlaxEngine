// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_OPENGL

#include "GPUBufferOGL.h"
#include "GPUDeviceOGL.h"
#include "RenderToolsOGL.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"

GPUBufferOGL::GPUBufferOGL(GPUDeviceOGL* device, const String& name)
	: GPUResourceOGL<Buffer>(device, name)
{
}

GPUBufferOGL::~GPUBufferOGL()
{
}

bool GPUBufferOGL::init()
{
	ASSERT(IsInMainThread());

	// Pick a buffer usage mode
	GLenum usage;
	switch (_desc.Usage)
	{
	case GPUResourceUsage::Default:
	case GPUResourceUsage::Immutable: usage = GL_STATIC_DRAW; break;
	case GPUResourceUsage::Staging:
	case GPUResourceUsage::Dynamic: usage = GL_DYNAMIC_DRAW; break;
	}

	// Pick a buffer target
	GLenum target = GL_ARRAY_BUFFER;
	if (_desc.Flags & GPUBufferFlags::VertexBuffer)
		target = GL_ARRAY_BUFFER;
	else if (_desc.Flags & GPUBufferFlags::IndexBuffer)
		target = GL_ELEMENT_ARRAY_BUFFER;
	else if (_desc.Flags & GPUBufferFlags::UnorderedAccess)
		target = GL_SHADER_STORAGE_BUFFER;
	else if (_desc.Flags & GPUBufferFlags::Argument)
		target = GL_DRAW_INDIRECT_BUFFER;
	else if (_desc.Flags & GPUBufferFlags::ShaderResource)
		target = GL_TEXTURE_BUFFER;
	else if (_desc.Usage == GPUResourceUsage::Staging)
		target = GL_PIXEL_UNPACK_BUFFER;
	BufferTarget = target;

	// Create a buffer
	glGenBuffers(1, &BufferId);
	VALIDATE_OPENGL_RESULT();
	if (!BufferId)
	{
		LOG(Warning, "Cannot create OpenGL buffer");
		return true;
	}

	// Initialize
	if (_desc.InitData)
	{
		glBindBuffer(target, BufferId);
		VALIDATE_OPENGL_RESULT();
		glBufferData(target, _desc.Size, _desc.InitData, usage);
		VALIDATE_OPENGL_RESULT();
		glBindBuffer(target, 0);
	}

	if (_desc.Flags & GPUBufferFlags::ShaderResource)
	{
		MISSING_CODE("Shader resource OpenGL GPU buffer");
		/*TextureTarget = TextureTarget.TextureBuffer;
		glGenTextures(1, &_textureId);
		VALIDATE_OPENGL_RESULT();
		glBindTexture(TextureTarget, _textureId);
		glTexBuffer(TextureBufferTarget::TextureBuffer, (SizedInternalFormat)TextureInternalFormat, BufferId);
		glBindTexture(TextureTarget, 0);
		*/
	}

	return false;
}

void GPUBufferOGL::release()
{
	_device->VAOCache.OnObjectRelease(this);

	// Release resource
	if (BufferId != 0)
	{
		glDeleteBuffers(1, &BufferId);
		VALIDATE_OPENGL_RESULT();
	}
	BufferId = 0;
	BufferTarget = 0;
	_memoryUsage = 0;

	// Base
	GPUBuffer::release();
}

bool GPUBufferOGL::SetData(const void* data, uint64 size)
{
	// Validate input and buffer state
	if (size == 0 || data == nullptr)
	{
		Log::ArgumentNullException(TEXT("Buffer.SetData"));
		return true;
	}
	if (size > GetSize())
	{
		Log::ArgumentOutOfRangeException(TEXT("Buffer.SetData"));
		return true;
	}
	if (!IsDynamic() && !IsStaging())
	{
		Log::InvalidOperationException(TEXT("Buffer.SetData"));
		return true;
	}
	if (BufferId == 0)
	{
		return true;
	}

	GPUDeviceLock lock(_device);

	// Map the staging resource for reading
	GLenum access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	glBindBuffer(BufferTarget, BufferId);
	VALIDATE_OPENGL_RESULT();
	glBufferData(BufferTarget, size, 0, GL_DYNAMIC_DRAW);
	VALIDATE_OPENGL_RESULT();
	void* buffer = glMapBufferRange(BufferTarget, 0, size, access);
	VALIDATE_OPENGL_RESULT();
	if (buffer == nullptr)
	{
		LOG(Warning, "Cannot map OpenGL buffer.");
		return true;
	}

	// Copy memory
	Platform::MemoryCopy(buffer, data, size);

	// Unmap resource
	if (!glUnmapBuffer(BufferTarget))
	{
		VALIDATE_OPENGL_RESULT();
		LOG(Warning, "OpenGL buffer data corrupted");
		return true;
	}
	glBindBuffer(BufferTarget, 0);
	VALIDATE_OPENGL_RESULT();

	return false;
}

bool GPUBufferOGL::GetData(BytesContainer& data)
{
	// Validate input and buffer state
	if (!IsDynamic() && !IsStaging())
	{
		Log::InvalidOperationException(TEXT("Buffer.GetData"));
		return true;
	}
	if (BufferId == 0)
	{
		return true;
	}
	auto size = GetSize();

	GPUDeviceLock lock(_device);

	// Map the staging resource for reading
	GLenum access = GL_MAP_READ_BIT;
	glBindBuffer(BufferTarget, BufferId);
	VALIDATE_OPENGL_RESULT();
	void* buffer = glMapBufferRange(BufferTarget, 0, size, access);
	VALIDATE_OPENGL_RESULT();
	if (buffer == nullptr)
	{
		LOG(Warning, "Cannot map OpenGL buffer.");
		return true;
	}

	// Copy memory
	data.Copy((byte*)buffer, size);

	// Unmap resource
	glBindBuffer(BufferTarget, BufferId);
	VALIDATE_OPENGL_RESULT();
	if (!glUnmapBuffer(BufferTarget))
	{
		VALIDATE_OPENGL_RESULT();
		LOG(Warning, "OpenGL buffer data corrupted");
		return true;
	}

	return false;
}

void GPUBufferOGL::Bind(int32 slotIndex)
{
	// TODO: finish this
	CRASH;
}

#endif
