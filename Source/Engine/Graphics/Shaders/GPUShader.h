// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUShaderProgram.h"
#include "Engine/Graphics/GPUResource.h"
#include "Engine/Core/Collections/Dictionary.h"

class GPUConstantBuffer;
class GPUShaderProgram;

/// <summary>
/// The runtime version of the shaders cache supported by the all graphics back-ends. The same for all the shader cache formats (easier to sync and validate).
/// </summary>
#define GPU_SHADER_CACHE_VERSION 12

/// <summary>
/// The GPU resource with shader programs that can run on the GPU and are able to perform rendering calculation using textures, vertices and other resources.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API GPUShader : public GPUResource
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUShader);

protected:
    Dictionary<uint32, GPUShaderProgram*> _shaders;
    GPUConstantBuffer* _constantBuffers[GPU_MAX_CB_BINDED];

    GPUShader();

public:
    /// <summary>
    /// Creates the shader resource and loads its data from the bytes.
    /// <param name="stream">The stream with compiled shader data.</param>
    /// <returns>True if cannot create state, otherwise false.</returns>
    virtual bool Create(class MemoryReadStream& stream);

public:
    /// <summary>
    /// Gets the vertex shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>The shader object.</returns>
    API_FUNCTION() FORCE_INLINE GPUShaderProgramVS* GetVS(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
        return static_cast<GPUShaderProgramVS*>(GetShader(ShaderStage::Vertex, name, permutationIndex));
    }

    /// <summary>
    /// Gets the hull shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>The shader object.</returns>
    API_FUNCTION() FORCE_INLINE GPUShaderProgramHS* GetHS(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
#if GPU_ALLOW_TESSELLATION_SHADERS
        return static_cast<GPUShaderProgramHS*>(GetShader(ShaderStage::Hull, name, permutationIndex));
#else
        return nullptr;
#endif
    }

    /// <summary>
    /// Gets domain shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>The shader object.</returns>
    API_FUNCTION() FORCE_INLINE GPUShaderProgramDS* GetDS(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
#if GPU_ALLOW_TESSELLATION_SHADERS
        return static_cast<GPUShaderProgramDS*>(GetShader(ShaderStage::Domain, name, permutationIndex));
#else
        return nullptr;
#endif
    }

    /// <summary>
    /// Gets the geometry shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>The shader object.</returns>
    API_FUNCTION() FORCE_INLINE GPUShaderProgramGS* GetGS(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
#if GPU_ALLOW_GEOMETRY_SHADERS
        return static_cast<GPUShaderProgramGS*>(GetShader(ShaderStage::Geometry, name, permutationIndex));
#else
        return nullptr;
#endif
    }

    /// <summary>
    /// Gets the pixel shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>The shader object.</returns>
    API_FUNCTION() FORCE_INLINE GPUShaderProgramPS* GetPS(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
        return static_cast<GPUShaderProgramPS*>(GetShader(ShaderStage::Pixel, name, permutationIndex));
    }

    /// <summary>
    /// Gets the compute shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>The shader object.</returns>
    API_FUNCTION() FORCE_INLINE GPUShaderProgramCS* GetCS(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
        return static_cast<GPUShaderProgramCS*>(GetShader(ShaderStage::Compute, name, permutationIndex));
    }

    /// <summary>
    /// Gets the constant buffer.
    /// </summary>
    /// <param name="slot">The buffer slot index.</param>
    /// <returns>The Constant Buffer object.</returns>
    API_FUNCTION() FORCE_INLINE GPUConstantBuffer* GetCB(int32 slot) const
    {
        ASSERT_LOW_LAYER(slot >= 0 && slot < ARRAY_COUNT(_constantBuffers));
        return _constantBuffers[slot];
    }

    /// <summary>
    /// Determines whether the specified shader program is in the shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns><c>true</c> if the shader is valid; otherwise, <c>false</c>.</returns>
    bool HasShader(const StringAnsiView& name, int32 permutationIndex = 0) const;

protected:
    GPUShaderProgram* GetShader(ShaderStage stage, const StringAnsiView& name, int32 permutationIndex) const;
    virtual GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, Span<byte> bytecode, MemoryReadStream& stream) = 0;
    static void ReadVertexLayout(MemoryReadStream& stream, GPUVertexLayout*& inputLayout, GPUVertexLayout*& vertexLayout);

public:
    // [GPUResource]
    GPUResourceType GetResourceType() const final override;

protected:
    // [GPUResource]
    void OnReleaseGPU() override;
};
