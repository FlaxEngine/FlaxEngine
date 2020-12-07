// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Serialization/ReadStream.h"
#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "../RenderToolsOGL.h"
#include "../GPUDeviceOGL.h"

#if GRAPHICS_API_OPENGL

/// <summary>
/// Shaders base class for OpenGL
/// </summary>
template<typename BaseType>
class GPUShaderProgramOGL : public BaseType
{
protected:

	GLuint _handle;
	GLenum _shaderType;
	Array<byte> _shader;
#if GPU_OGL_KEEP_SHADER_SRC
	const char* ShaderSource = nullptr;
#endif

public:

	/// <summary>
	/// Init
	/// </summary>
	/// <param name="shaderType">Type of the OpenGL shader</param>
	/// <param name="shaderBytes">Bytes with an compiled shader</param>
	/// <param name="shaderBytesSize">Size in bytes of the cached shader</param>
	/// <param name="stream">Stream with data to read</param>
	/// <param name="name">Shader function name</param>
	GPUShaderProgramOGL(GLenum shaderType, byte* shaderBytes, uint32 shaderBytesSize, ReadStream& stream, const StringAnsi& name)
		: _handle(0)
		, _shaderType(shaderType)
	{
		_name = name;

		// Cache shader source (it cannot be compiled on content loading thread) - because OpenGL sucks
		// TODO: use shared context and compile shaders on a content threads instead of main thread
		_shader.Set(shaderBytes, shaderBytesSize);

		// Load meta
		stream.ReadUint32(&_instructionsCount);
		stream.ReadUint32(&_usedCBsMask);
		stream.ReadUint32(&_usedSRsMask);
		stream.ReadUint32(&_usedUAsMask);
	}

	/// <summary>
	/// Destructor
	/// </summary>
	~GPUShaderProgramOGL()
	{
#if GPU_OGL_KEEP_SHADER_SRC
        Allocator::Free(ShaderSource);
#endif
		if (_handle != 0)
		{
			glDeleteProgram(_handle);
			VALIDATE_OPENGL_RESULT();
		}
	}

private:

	static GLuint glCreateGPUShaderProgram(GLenum type, const char* source)
	{
		const GLuint shader = glCreateShader(type);
		if (shader)
		{
			glShaderSource(shader, 1, &source, NULL);
			glCompileShader(shader);
			const GLuint program = glCreateProgram();
			if (program)
			{
				GLint compiled = GL_FALSE;
				glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
				glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
				if (compiled)
				{
					glAttachShader(program, shader);
					glLinkProgram(program);
					glDetachShader(program, shader);
				}
			}
			glDeleteShader(shader);
			return program;
		}

		return 0;
	}

public:

	// Gets the OpenGL shader object handle. Performs compilation if shader has not been created yet.
	GLuint GetHandle()
	{
		if (_handle == 0)
		{
			// Compile the shader
			_handle = glCreateGPUShaderProgram(_shaderType, (const char*)_shader.Get());
			VALIDATE_OPENGL_RESULT();

			// Check the status and the messages
			{
				GLint linkCompileSuccess = 0;
				glGetProgramiv(_handle, GL_LINK_STATUS, &linkCompileSuccess);
				VALIDATE_OPENGL_RESULT();

				if (!linkCompileSuccess && _handle > 0)
				{
					GLint infologLength = 0;
					glGetProgramiv(_handle, GL_INFO_LOG_LENGTH, &infologLength);
					VALIDATE_OPENGL_RESULT();

					if (infologLength > 0)
					{
						GLint charsWritten = 0;

						Array<GLchar> infoLog;
						infoLog.Resize(infologLength);
						glGetProgramInfoLog(_handle, infologLength, &charsWritten, infoLog.Get());
						VALIDATE_OPENGL_RESULT();

						LOG(Warning, "Compile and linker info log:");
						LOG_STR(Warning, String(infoLog.Get()));
					}
				}
			}

#if GPU_OGL_KEEP_SHADER_SRC
			ShaderSource = Allocator::Allocate(_shader.Count());
			Platform::MemoryCopy((void*)ShaderSource, (void*)_shader.Get(), _shader.Count() * sizeof(char));
#endif
			// Cleanup source buffer
			_shader.Resize(0);
		}

		return _handle;
	}

public:

	// [BaseType]
	uint32 GetBufferSize() const override
	{
		return 0;
	}
	void* GetBufferHandle() const override
	{
		return (void*)_handle;
	}
};

/// <summary>
/// Vertex Shader for OpenGL
/// </summary>
class GPUShaderProgramVSOGL : public GPUShaderProgramOGL<GPUShaderProgramVS>
{
public:

	struct LayoutElement
	{
		int32 BufferSlot;
		PixelFormat Format;
		int32 Size;
		int32 InstanceDataStepRate;
		int32 InputIndex;
		int32 RelativeOffset;
		int32 TypeCount;
		GLboolean Normalized;
		GLenum GlType;
		bool IsInteger;
	};

private:

	GPUDeviceOGL* _device;

public:

	/// <summary>
	/// Init
	/// </summary>
	/// <param name="device">The graphics device.</param>
	/// <param name="shaderBytes">Bytes with an compiled shader</param>
	/// <param name="shaderBytesSize">Size in bytes of the cached shader</param>
	/// <param name="stream">Stream with data to read</param>
	/// <param name="name">Shader function name</param>
	GPUShaderProgramVSOGL(GPUDeviceOGL* device, byte* shaderBytes, uint32 shaderBytesSize, ReadStream& stream, const StringAnsi& name);

	/// <summary>
	/// Destructor
	/// </summary>
	~GPUShaderProgramVSOGL()
	{
		_device->VAOCache.OnObjectRelease(this);
	}

public:

	Array<LayoutElement, FixedAllocation<VERTEX_SHADER_MAX_INPUT_ELEMENTS>> Layout;

public:

	// [GPUShaderProgramOGL]
	void* GetInputLayout() const override
	{
		return (void*)nullptr;
	}
	byte GetInputLayoutSize() const override
	{
		return Layout.Count();
	}
};

/// <summary>
/// Geometry Shader for OpenGL
/// </summary>
class GPUShaderProgramGSOGL : public GPUShaderProgramOGL<GPUShaderProgramGS>
{
public:

	/// <summary>
	/// Init
	/// </summary>
	/// <param name="shaderBytes">Bytes with an compiled shader</param>
	/// <param name="shaderBytesSize">Size in bytes of the cached shader</param>
	/// <param name="stream">Stream with data to read</param>
	/// <param name="name">Shader function name</param>
	GPUShaderProgramGSOGL(byte* shaderBytes, uint32 shaderBytesSize, ReadStream& stream, const StringAnsi& name)
		: GPUShaderProgramOGL(GL_GEOMETRY_SHADER, shaderBytes, shaderBytesSize, stream, name)
	{
	}
};

/// <summary>
/// Pixel Shader for OpenGL
/// </summary>
class GPUShaderProgramPSOGL : public GPUShaderProgramOGL<GPUShaderProgramPS>
{
public:

	/// <summary>
	/// Init
	/// </summary>
	/// <param name="shaderBytes">Bytes with an compiled shader</param>
	/// <param name="shaderBytesSize">Size in bytes of the cached shader</param>
	/// <param name="stream">Stream with data to read</param>
	/// <param name="name">Shader function name</param>
	GPUShaderProgramPSOGL(byte* shaderBytes, uint32 shaderBytesSize, ReadStream& stream, const StringAnsi& name)
		: GPUShaderProgramOGL(GL_FRAGMENT_SHADER, shaderBytes, shaderBytesSize, stream, name)
	{
	}
};

/// <summary>
/// Compute Shader for OpenGL
/// </summary>
class GPUShaderProgramCSOGL : public GPUShaderProgramOGL<GPUShaderProgramCS>
{
public:

	/// <summary>
	/// Init
	/// </summary>
	/// <param name="shaderBytes">Bytes with an compiled shader</param>
	/// <param name="shaderBytesSize">Size in bytes of the cached shader</param>
	/// <param name="stream">Stream with data to read</param>
	/// <param name="name">Shader function name</param>
	GPUShaderProgramCSOGL(byte* shaderBytes, uint32 shaderBytesSize, ReadStream& stream, const StringAnsi& name)
		: GPUShaderProgramOGL(GL_COMPUTE_SHADER, shaderBytes, shaderBytesSize, stream, name)
	{
	}
};

#endif
