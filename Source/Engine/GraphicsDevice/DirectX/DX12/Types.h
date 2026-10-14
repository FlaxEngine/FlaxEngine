// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_DX_SHADER_COMPILER || GRAPHICS_API_DIRECTX12

#include "../IncludeDirectXHeaders.h"

struct DxShaderHeader
{
    /// <summary>
    /// Shader bytecode hash. Used for PSO lookups.
    /// </summary>
    uint64 Hash;

	/// <summary>
	/// The SRV dimensions per-slot.
	/// </summary>
	byte SrDimensions[32];

	/// <summary>
	/// The UAV dimensions per-slot.
	/// </summary>
	byte UaDimensions[4];
};

struct DxShaderHash
{
    static uint64 Generate(const void* data, uint64 size)
    {
        // TODO: improve hashing (eg. cityhash)
        uint64 hash = 14695981039346656037ULL;
        for (uint64 i = 0; i < size; i++)
        {
            hash ^= ((const byte*)data)[i];
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    static void Combine(uint64& hash, uint64 next)
    {
        hash ^= next;
        hash *= 1099511628211ULL;
    }
};


struct DxShaderHashName
{
    Char Buffer[32];

    DxShaderHashName(uint64 hash)
    {
        swprintf_s(Buffer, L"%016llX", (unsigned long long)hash);
    }

    operator const Char*() const
    {
        return Buffer;
    }
};

#endif
