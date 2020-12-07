// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "VAOCache.h"

#if GRAPHICS_API_OPENGL

#include "TextureOGL.h"
#include "BufferOGL.h"
#include "RenderToolsOGL.h"
#include "Shaders/GPUShaderProgramOGL.h"

VAOCache::VAOCache()
	: Table(2048)
{
}

VAOCache::~VAOCache()
{
	Dispose();
}

GLuint VAOCache::GetVAO(GPUShaderProgramVSOGL* vs, BufferOGL* indexBuffer, uint32 streamsCount, StreamData streams[])
{
	Key key(vs, indexBuffer, streamsCount, streams);
	GLuint vao;

	if (!Table.TryGet(key, vao))
	{
		// Create new VAO
		glGenVertexArrays(1, &vao);
		VALIDATE_OPENGL_RESULT();

		// Bind VAO
		glBindVertexArray(vao);
		VALIDATE_OPENGL_RESULT();
		
		// Initialize VAO
		for (int32 i = 0; i < vs->Layout.Count(); i++)
		{
			auto& item = vs->Layout[i];

			auto bufferSlot = item.BufferSlot;
			if (bufferSlot >= streamsCount)
			{
				LOG(Error, "Incorrect input buffer slot");
				continue;
			}

			auto& stream = streams[bufferSlot];
			ASSERT(stream.Buffer != nullptr && stream.Buffer->BufferId != 0);

			glBindBuffer(GL_ARRAY_BUFFER, stream.Buffer->BufferId);
			VALIDATE_OPENGL_RESULT();
			
			GLvoid* DataStartOffset = reinterpret_cast<GLvoid*>(static_cast<size_t>(stream.Offset + item.RelativeOffset));
			if (item.IsInteger)
				glVertexAttribIPointer(i, item.TypeCount, item.GlType, stream.Stride, DataStartOffset);
			else
				glVertexAttribPointer(i, item.TypeCount, item.GlType, item.Normalized, stream.Stride, DataStartOffset);
			VALIDATE_OPENGL_RESULT();

			glVertexAttribDivisor(i, item.InstanceDataStepRate);
			VALIDATE_OPENGL_RESULT();

			glEnableVertexAttribArray(i);
			VALIDATE_OPENGL_RESULT();
		}
		if (indexBuffer)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->BufferId);
		}

		// Register in cache
		Table.Add(key, vao);
	}

	return vao;
}

void VAOCache::OnObjectRelease(GPUShaderProgramVSOGL* obj)
{
	for (auto i = Table.Begin(); i.IsNotEnd(); ++i)
	{
		if (i->Key.HasReference(obj))
		{
			GLuint fbo = i->Value;
			glDeleteVertexArrays(1, &fbo);
			Table.Remove(i);
		}
	}
}

void VAOCache::OnObjectRelease(BufferOGL* obj)
{
	for (auto i = Table.Begin(); i.IsNotEnd(); ++i)
	{
		if (i->Key.HasReference(obj))
		{
			GLuint fbo = i->Value;
			glDeleteVertexArrays(1, &fbo);
			Table.Remove(i);
		}
	}
}

void VAOCache::Dispose()
{
	for (auto i = Table.Begin(); i.IsNotEnd(); ++i)
	{
		GLuint fbo = i->Value;
		glDeleteVertexArrays(1, &fbo);
	}
	Table.Clear();
}

VAOCache::Key::Key(GPUShaderProgramVSOGL* vs, BufferOGL* indexBuffer, uint32 streamsCount, StreamData streams[])
{
	VS = vs;
	IndexBuffer = indexBuffer;
	StreamsCount = streamsCount;

	Hash = streamsCount * 371;
	HashCombinePointer(Hash, vs);
	HashCombinePointer(Hash, indexBuffer);

	for (int32 i = 0; i < streamsCount; i++)
	{
		Streams[i] = streams[i];
		HashCombinePointer(Hash, streams[i].Buffer);
		HashCombine(Hash, streams[i].Offset);
		HashCombine(Hash, streams[i].Stride);
	}
}

bool VAOCache::Key::HasReference(GPUShaderProgramVSOGL* obj)
{
	return obj == VS;
}

bool VAOCache::Key::HasReference(BufferOGL* obj)
{
	for (int32 i = 0; i < ARRAY_COUNT(Streams); i++)
	{
		if (Streams[i].Buffer == obj)
			return true;
	}
	return IndexBuffer == obj;
}

bool VAOCache::Key::operator==(const Key & other) const
{
	if (Hash != 0 && other.Hash != 0 && Hash != other.Hash)
		return false;

	if (StreamsCount != other.StreamsCount || IndexBuffer != other.IndexBuffer)
		return false;

	for (int32 rt = 0; rt < StreamsCount; rt++)
	{
		if (Streams[rt] != other.Streams[rt])
			return false;
	}

	return true;
}

#endif
