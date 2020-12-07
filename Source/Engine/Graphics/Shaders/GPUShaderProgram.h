// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
#include "Config.h"

/// <summary>
/// The shader program metadata container. Contains description about resources used by the shader.
/// </summary>
struct FLAXENGINE_API ShaderBindings
{
    uint32 InstructionsCount;
    uint32 UsedCBsMask;
    uint32 UsedSRsMask;
    uint32 UsedUAsMask;

    bool IsUsingCB(uint32 slotIndex) const
    {
        return (UsedCBsMask & (1u << slotIndex)) != 0u;
    }

    bool IsUsingSR(uint32 slotIndex) const
    {
        return (UsedSRsMask & (1u << slotIndex)) != 0u;
    }

    bool IsUsingUA(uint32 slotIndex) const
    {
        return (UsedUAsMask & (1u << slotIndex)) != 0u;
    }
};

struct GPUShaderProgramInitializer
{
    StringAnsi Name;
    ShaderBindings Bindings;
    ShaderFlags Flags;
};

/// <summary>
/// Mini program that can run on the GPU.
/// </summary>
class FLAXENGINE_API GPUShaderProgram
{
protected:

    StringAnsi _name;
    ShaderBindings _bindings;
    ShaderFlags _flags;

    void Init(const GPUShaderProgramInitializer& initializer)
    {
        _name = initializer.Name;
        _bindings = initializer.Bindings;
        _flags = initializer.Flags;
    }

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUShaderProgram"/> class.
    /// </summary>
    virtual ~GPUShaderProgram()
    {
    }

public:

    /// <summary>
    /// Gets name of the shader program
    /// </summary>
    /// <returns>Name</returns>
    FORCE_INLINE const StringAnsi& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Gets the shader resource bindings.
    /// </summary>
    /// <returns>The bindings.</returns>
    FORCE_INLINE const ShaderBindings& GetBindings() const
    {
        return _bindings;
    }

    /// <summary>
    /// Gets the shader flags.
    /// </summary>
    /// <returns>The flags.</returns>
    FORCE_INLINE ShaderFlags GetFlags() const
    {
        return _flags;
    }

public:

    /// <summary>
    /// Gets shader program stage type
    /// </summary>
    /// <returns>Shader Stage type</returns>
    virtual ShaderStage GetStage() const = 0;

    /// <summary>
    /// Gets buffer handle (platform dependent)
    /// </summary>
    /// <returns>Handle</returns>
    virtual void* GetBufferHandle() const = 0;

    /// <summary>
    /// Gets buffer size (in bytes)
    /// </summary>
    /// <returns>Size of the buffer in bytes</returns>
    virtual uint32 GetBufferSize() const = 0;
};

/// <summary>
/// Vertex Shader program.
/// </summary>
class GPUShaderProgramVS : public GPUShaderProgram
{
public:

    /// <summary>
    /// Gets input layout description handle (platform dependent)
    /// </summary>
    /// <returns>Input layout</returns>
    virtual void* GetInputLayout() const = 0;

    /// <summary>
    /// Gets input layout description size (in bytes)
    /// </summary>
    /// <returns>Input layout description size in bytes</returns>
    virtual byte GetInputLayoutSize() const = 0;

public:

    // [GPUShaderProgram]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Vertex;
    }
};

/// <summary>
/// Geometry Shader program.
/// </summary>
class GPUShaderProgramGS : public GPUShaderProgram
{
public:

    // [GPUShaderProgram]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Geometry;
    }
};

/// <summary>
/// Hull Shader program.
/// </summary>
class GPUShaderProgramHS : public GPUShaderProgram
{
protected:

    int32 _controlPointsCount;

public:

    /// <summary>
    /// Gets the input control points count (valid range: 1-32).
    /// </summary>
    FORCE_INLINE int32 GetControlPointsCount() const
    {
        return _controlPointsCount;
    }

public:

    // [GPUShaderProgram]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Hull;
    }
};

/// <summary>
/// Domain Shader program.
/// </summary>
class GPUShaderProgramDS : public GPUShaderProgram
{
public:

    // [GPUShaderProgram]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Domain;
    }
};

/// <summary>
/// Pixel Shader program.
/// </summary>
class GPUShaderProgramPS : public GPUShaderProgram
{
public:

    // [GPUShaderProgram]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Pixel;
    }
};

/// <summary>
/// Compute Shader program.
/// </summary>
class GPUShaderProgramCS : public GPUShaderProgram
{
public:

    // [GPUShaderProgram]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Compute;
    }
};
