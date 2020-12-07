// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Common.h"
#include "Config.h"

#if GRAPHICS_API_OPENGL

#include "IncludeOpenGLHeaders.h"

class GPUTextureViewOGL;
class TextureOGL;

class FBOCache
{
private:

	struct Key
	{
		uint32 Hash;
		uint32 RTCount;
		GPUTextureViewOGL* DepthStencil;
		GPUTextureViewOGL* RT[GPU_MAX_RT_BINDED];

		Key()
			: Hash(0)
			, RTCount(0)
			, DepthStencil(nullptr)
		{
			for (int32 rt = 0; rt < GPU_MAX_RT_BINDED; rt++)
				RT[rt] = nullptr;
		}

		Key(uint32 rtCount, GPUTextureViewOGL* depthStencil, GPUTextureViewOGL* rts[]);

		bool HasReference(TextureOGL* texture);

		bool operator== (const Key& other) const;

		static uint32 HashFunction(const Key& key)
		{
			return key.Hash;
		}
	};

	Dictionary<Key, GLuint> Table;

public:

	FBOCache();

	~FBOCache();

public:

	GLuint GetFBO(uint32 rtCount, GPUTextureViewOGL* depthStencil, GPUTextureViewOGL* rts[]);
	void OnTextureRelease(TextureOGL* texture);
	void Dispose();
};

#endif
