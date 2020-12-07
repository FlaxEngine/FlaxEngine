// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Common.h"
#include "Config.h"

#if GRAPHICS_API_OPENGL

#include "IncludeOpenGLHeaders.h"

class GPUTextureViewOGL;
class PipelineStateOGL;
class GPUShaderProgramVSOGL;
class TextureOGL;
class BufferOGL;

class VAOCache
{
public:

	struct StreamData
	{
		BufferOGL* Buffer;
		uint32 Stride;
		uint32 Offset;

		StreamData()
			: Buffer(nullptr)
			, Stride(0)
			, Offset(0)
		{
		}

		bool operator== (const StreamData& other) const
		{
			return Buffer == other.Buffer && Stride == other.Stride && Offset == other.Offset;
		}
		
		bool operator!= (const StreamData& other) const
		{
			return !operator==(other);
		}
	};

private:

	struct Key
	{
		uint32 Hash;
		GPUShaderProgramVSOGL* VS;
		BufferOGL* IndexBuffer;
		uint32 StreamsCount;
		StreamData Streams[GPU_MAX_VB_BINDED];

		Key()
			: Hash(0)
			, StreamsCount(0)
			, VS(nullptr)
			, IndexBuffer(nullptr)
		{
		}

		Key(GPUShaderProgramVSOGL* vs, BufferOGL* indexBuffer, uint32 streamsCount, StreamData streams[]);

		bool HasReference(GPUShaderProgramVSOGL* obj);
		bool HasReference(BufferOGL* obj);

		bool operator== (const Key& other) const;

		static uint32 HashFunction(const Key& key)
		{
			return key.Hash;
		}
	};

	Dictionary<Key, GLuint> Table;

public:

	VAOCache();
	~VAOCache();

public:

	GLuint GetVAO(GPUShaderProgramVSOGL* vs, BufferOGL* indexBuffer, uint32 streamsCount, StreamData streams[]);
	void OnObjectRelease(GPUShaderProgramVSOGL* obj);
	void OnObjectRelease(BufferOGL* obj);
	void Dispose();
};

#endif
