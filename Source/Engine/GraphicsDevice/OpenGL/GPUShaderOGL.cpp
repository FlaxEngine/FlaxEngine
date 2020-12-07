// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_OPENGL

#include "GPUShaderOGL.h"
#include "Engine/Core/Memory/Allocator.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "GPUConstantBufferOGL.h"
#include "GPUShaderProgramOGL.h"
#include "Engine/Graphics/GPULimitsOGL.h"
#include <LZ4/lz4.h>

GPUShaderOGL::GPUShaderOGL(GPUDeviceOGL* device, const String& name)
	: GPUResourceOGL<Shader>(device, name)
{
}

GPUShaderOGL::~GPUShaderOGL()
{
}

void GPUShaderOGL::ReleaseGPU()
{
	// Base
	GPUResourceOGL::ReleaseGPU();
}

static bool SupportsSeparateShaderObjects()
{
	return ((GPULimitsOGL*)GPUDevice::Instance->Limits)->SupportsSeparateShaderObjects;
}

// Verify that an OpenGL program has linked successfully.
static bool VerifyLinkedProgram(GLuint program)
{
#if BUILD_DEBUG || GPU_OGL_DEBUG_SHADERS
	GLint linkStatus = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE)
	{
		GLint logLength;
		char defaultLog[] = "No log";
		char* compileLog = defaultLog;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 1)
		{
			compileLog = (char*)Allocator::Allocate(logLength);
			glGetProgramInfoLog(program, logLength, NULL, compileLog);
		}

		LOG(Error, "Failed to link program. Compile log: \n{0}", compileLog);

		if (logLength > 1)
		{
			Allocator::Free(compileLog);
		}

		return false;
	}
#endif
	return true;
}

// Verify that an OpenGL shader has compiled successfully.
static bool VerifyCompiledShader(GLuint shader, const char* glslCode)
{
#if BUILD_DEBUG || GPU_OGL_DEBUG_SHADERS
	if (SupportsSeparateShaderObjects() && glIsProgram(shader))
	{
		bool const compiledOK = VerifyLinkedProgram(shader);
#if GPU_OGL_DEBUG_SHADERS
		if (!compiledOK && glslCode)
		{
			LOG(Warning, "Shader: \n{0}", glslCode);
		}
#endif
		return compiledOK;
	}
	else
	{
		GLint compileStatus;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
		if (compileStatus != GL_TRUE)
		{
			GLint logLength;
			char defaultLog[] = "No log";
			char* compileLog = defaultLog;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
#if PLATFORM_ANDROID
			if (LogLength == 0)
			{
				// Make it big anyway
// There was a bug in android 2.2 where glGetShaderiv would return 0 even though there was a error message
				// https://code.google.com/p/android/issues/detail?id=9953
				LogLength = 4096;
			}
#endif
			if (logLength > 1)
			{
				compileLog = (char*)Allocator::Allocate(logLength);
				glGetShaderInfoLog(shader, logLength, NULL, compileLog);
			}

#if GPU_OGL_DEBUG_SHADERS
			if (glslCode)
			{
				LOG(Warning, "Shader: \n{0}", glslCode);
			}
#endif
			LOG(Fatal, "Failed to compile shader. Compile log: \n{0}", compileLog);

			if (logLength > 1)
			{
				Allocator::Free(compileLog);
			}
			return false;
		}
	}
#endif
	return true;
}

static bool VerifyProgramPipeline(GLuint Program)
{
	bool result = true;

	// Don't try and validate SSOs here - the draw state matters to SSOs and it definitely can't be guaranteed to be valid at this stage
	if (SupportsSeparateShaderObjects())
	{
#if GPU_OGL_DEBUG_SHADERS
		result = glIsProgramPipeline(Program) == GL_TRUE;
#endif
	}
	else
	{
		result = VerifyLinkedProgram(Program);
	}

	return result;
}

bool GPUShaderOGL::Create(MemoryReadStream& stream)
{
	ASSERT(_device);

	ReleaseGPU();

	// Version
	int32 version;
	stream.ReadInt32(&version);
	if (version != GPU_SHADER_CACHE_VERSION)
	{
		LOG(Warning, "Unsupported OpenGL shader version {0}.", version);
		return true;
	}

	Array<byte> cache;
	Array<byte> cache2;

	// Shaders count
	int32 shadersCount;
	stream.ReadInt32(&shadersCount);
	for (int32 i = 0; i < shadersCount; i++)
	{
		uint32 cacheSize;
		ShaderStage type = static_cast<ShaderStage>(stream.ReadByte());
		int32 permutationsCount = stream.ReadByte();
		ASSERT(Math::IsInRange(permutationsCount, 1, SHADER_PERMUTATIONS_MAX_COUNT));

		// Load shader name
		StringAnsi name;
		stream.ReadStringAnsi(&name, 11);

		for (int32 permutationIndex = 0; permutationIndex < permutationsCount; permutationIndex++)
		{
			// Load cache
			int32 cacheType;
			stream.ReadInt32(&cacheType);
			uint32 cacheOriginalSize;
			stream.ReadUint32(&cacheOriginalSize);
			stream.ReadUint32(&cacheSize);
			ASSERT(Math::IsInRange<uint32>(cacheSize, 1, 1024 * 1024));
			cache.EnsureCapacity(cacheSize);
			stream.ReadBytes(cache.Get(), cacheSize);

			// Check if unpack the data
			byte* shaderBytes = cache.Get();
			if (cacheType == SHADER_DATA_FORMAT_RAW)
			{
				ASSERT(cacheOriginalSize == cacheSize);
			}
			else if (cacheType == SHADER_DATA_FORMAT_LZ4)
			{
				// Decompress LZ4 data
				cache2.EnsureCapacity(cacheOriginalSize);
				int32 res = LZ4_decompress_safe((const char*)cache.Get(), (char*)cache2.Get(), cacheSize, cacheOriginalSize);
				if (res <= 0)
				{
					LOG(Warning, "Failed to decompress OpenGL shader data. Result: {0}.", res);
					return true;
				}
				cacheSize = cacheOriginalSize;
				shaderBytes = cache2.Get();
			}
			else
			{
				LOG(Warning, "Unknown OpenGL shader data format.");
				return true;
			}

			// Create Flax shader object
			GPUShaderProgram* shader;
			switch (type)
			{
			case ShaderStage::Vertex: shader = New<GPUShaderProgramVSOGL>(_device, shaderBytes, cacheSize, stream, name); break;
			case ShaderStage::Hull: shader = New<GPUShaderProgramHSOGL>(_device, shaderBytes, cacheSize, stream, name); break;
			case ShaderStage::Domain: shader = New<GPUShaderProgramDSOGL>(_device, shaderBytes, cacheSize, stream, name); break;
			case ShaderStage::Geometry: shader = New<GPUShaderProgramGSOGL>(shaderBytes, cacheSize, stream, name); break;
			case ShaderStage::Pixel: shader = New<GPUShaderProgramPSOGL>(shaderBytes, cacheSize, stream, name); break;
			case ShaderStage::Compute: shader = New<GPUShaderProgramCSOGL>(shaderBytes, cacheSize, stream, name); break;
			default: return true;
			}

			// Add to collection
			_shaders.Add(shader, permutationIndex);
		}
	}

	// Constant Buffers
	byte constantBuffersCount = stream.ReadByte();
	byte maximumConstantBufferSlot = stream.ReadByte();
	if (constantBuffersCount > 0)
	{
		ASSERT(maximumConstantBufferSlot < MAX_CONSTANT_BUFFER_SLOTS);

		for (int32 i = 0; i < constantBuffersCount; i++)
		{
			// Load info
			byte slotIndex = stream.ReadByte();
			uint32 size;
			stream.ReadUint32(&size);

			// Create CB
#if GPU_ENABLE_RESOURCE_NAMING
			String name = ToString() + TEXT(".CB") + i;
#else
			String name;
#endif
			auto cb = New<GPUConstantBufferOGL>(_device, name, size);
			ASSERT(_constantBuffers[slotIndex] == nullptr);
			_constantBuffers[slotIndex] = cb;
		}
	}

	return false;
}

#endif
