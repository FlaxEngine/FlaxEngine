// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
#include "Config.h"

class GPUShader;

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
#if !BUILD_RELEASE
    GPUShader* Owner;
#endif
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
#if !BUILD_RELEASE
    GPUShader* _owner;
#endif

    void Init(const GPUShaderProgramInitializer& initializer)
    {
        _name = initializer.Name;
        _bindings = initializer.Bindings;
        _flags = initializer.Flags;
#if !BUILD_RELEASE
        _owner = initializer.Owner;
#endif
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
    /// Gets name of the shader program.
    /// </summary>
    FORCE_INLINE const StringAnsi& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Gets the shader resource bindings.
    /// </summary>
    FORCE_INLINE const ShaderBindings& GetBindings() const
    {
        return _bindings;
    }

    /// <summary>
    /// Gets the shader flags.
    /// </summary>
    FORCE_INLINE ShaderFlags GetFlags() const
    {
        return _flags;
    }

public:

    /// <summary>
    /// Gets shader program stage type.
    /// </summary>
    virtual ShaderStage GetStage() const = 0;

    /// <summary>
    /// Gets buffer handle (platform dependent).
    /// </summary>
    virtual void* GetBufferHandle() const = 0;

    /// <summary>
    /// Gets buffer size (in bytes).
    /// </summary>
    virtual uint32 GetBufferSize() const = 0;
};

/// <summary>
/// Vertex Shader program.
/// </summary>
class GPUShaderProgramVS : public GPUShaderProgram
{
public:

    /// <summary>
    /// Gets input layout description handle (platform dependent).
    /// </summary>
    virtual void* GetInputLayout() const = 0;

    /// <summary>
    /// Gets input layout description size (in bytes).
    /// </summary>
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
