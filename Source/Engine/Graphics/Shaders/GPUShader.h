// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "GPUShaderProgram.h"
#include "Engine/Graphics/GPUResource.h"
#include "Engine/Core/Collections/Dictionary.h"

class GPUConstantBuffer;
class GPUShaderProgram;

/// <summary>
/// The runtime version of the shaders cache supported by the all graphics back-ends. The same for all the shader cache formats (easier to sync and validate).
/// </summary>
#define GPU_SHADER_CACHE_VERSION 8

/// <summary>
/// Represents collection of shader programs with permutations and custom names.
/// </summary>
class GPUShaderProgramsContainer
{
private:

    Dictionary<int32, GPUShaderProgram*> _shaders;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShaderProgramsContainer"/> class.
    /// </summary>
    GPUShaderProgramsContainer();

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUShaderProgramsContainer"/> class.
    /// </summary>
    ~GPUShaderProgramsContainer();

public:

    /// <summary>
    /// Adds a new shader program to the collection.
    /// </summary>
    /// <param name="shader">The shader to store.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    void Add(GPUShaderProgram* shader, int32 permutationIndex);

    /// <summary>
    /// Gets a shader of given name and permutation index.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>Stored shader program or null if cannot find it.</returns>
    GPUShaderProgram* Get(const StringAnsiView& name, int32 permutationIndex) const;

    /// <summary>
    /// Clears collection (deletes all shaders).
    /// </summary>
    void Clear();

public:

    /// <summary>
    /// Calculates unique hash for given shader program name and its permutation index.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader program permutation index.</param>
    /// <returns>Calculated hash value.</returns>
    static uint32 CalculateHash(const StringAnsiView& name, int32 permutationIndex);
};

/// <summary>
/// The GPU resource with shader programs that can run on the GPU and are able to perform rendering calculation using textures, vertices and other resources.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API GPUShader : public GPUResource
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUShader);
protected:

    GPUShaderProgramsContainer _shaders;
    GPUConstantBuffer* _constantBuffers[MAX_CONSTANT_BUFFER_SLOTS];

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUShader"/> class.
    /// </summary>
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
        return static_cast<GPUShaderProgramHS*>(GetShader(ShaderStage::Hull, name, permutationIndex));
    }

    /// <summary>
    /// Gets domain shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>The shader object.</returns>
    API_FUNCTION() FORCE_INLINE GPUShaderProgramDS* GetDS(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
        return static_cast<GPUShaderProgramDS*>(GetShader(ShaderStage::Domain, name, permutationIndex));
    }

    /// <summary>
    /// Gets the geometry shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns>The shader object.</returns>
    API_FUNCTION() FORCE_INLINE GPUShaderProgramGS* GetGS(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
        return static_cast<GPUShaderProgramGS*>(GetShader(ShaderStage::Geometry, name, permutationIndex));
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

public:

    /// <summary>
    /// Determines whether the specified shader program is in the shader.
    /// </summary>
    /// <param name="name">The shader program name.</param>
    /// <param name="permutationIndex">The shader permutation index.</param>
    /// <returns><c>true</c> if the shader is valid; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool HasShader(const StringAnsiView& name, int32 permutationIndex = 0) const
    {
        return _shaders.Get(name, permutationIndex) != nullptr;
    }

protected:

    GPUShaderProgram* GetShader(ShaderStage stage, const StringAnsiView& name, int32 permutationIndex) const;
    virtual GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, byte* cacheBytes, uint32 cacheSize, MemoryReadStream& stream) = 0;
    virtual GPUConstantBuffer* CreateCB(const String& name, uint32 size, MemoryReadStream& stream) = 0;

public:

    // [GPUResource]
    ResourceType GetResourceType() const final override;

protected:

    // [GPUResource]
    void OnReleaseGPU() override;
};
