// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GPUConstantBufferOGL.h"

#if GRAPHICS_API_OPENGL

#include "../RenderToolsOGL.h"
#include "Engine/Threading/Threading.h"

GPUConstantBufferOGL::GPUConstantBufferOGL(GPUDeviceOGL* device, const String& name, uint32 size)
	: GPUResourceOGL(device, name)
{
    _size = size;
}

GPUConstantBufferOGL::~GPUConstantBufferOGL()
{
	ReleaseGPU();
}

GLuint GPUConstantBufferOGL::GetHandle()
{
	// Create buffer if missing
	if (_handle == 0)
	{
		glGenBuffers(1, &_handle);
		VALIDATE_OPENGL_RESULT();

		glBindBuffer(GL_UNIFORM_BUFFER, _handle);
		VALIDATE_OPENGL_RESULT();

		glBufferData(GL_UNIFORM_BUFFER, _size, nullptr, GL_DYNAMIC_DRAW);
		VALIDATE_OPENGL_RESULT();

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		VALIDATE_OPENGL_RESULT();
	}

	return _handle;
}

void GPUConstantBufferOGL::ReleaseGPU()
{
	if (_handle != 0)
	{
		glDeleteBuffers(1, &_handle);
		VALIDATE_OPENGL_RESULT();
		_handle = 0;
	}
	_memoryUsage = 0;
}

#endif
