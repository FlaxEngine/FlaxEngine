// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
#include "Config.h"

class GPUShader;
class GPUVertexLayout;

/// <summary>
/// The shader program metadata container. Contains description about resources used by the shader.
/// </summary>
struct FLAXENGINE_API ShaderBindings
{
    uint32 InstructionsCount;
    uint32 UsedCBsMask;
    uint32 UsedSRsMask;
    uint32 UsedUAsMask;

    FORCE_INLINE bool IsUsingCB(uint32 slotIndex) const
    {
        return (UsedCBsMask & (1u << slotIndex)) != 0u;
    }

    FORCE_INLINE bool IsUsingSR(uint32 slotIndex) const
    {
        return (UsedSRsMask & (1u << slotIndex)) != 0u;
    }

    FORCE_INLINE bool IsUsingUA(uint32 slotIndex) const
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

    void Init(const GPUShaderProgramInitializer& initializer);

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
    // Input element run-time data (see VertexShaderMeta::InputElement for compile-time data)
    // [Deprecated in v1.10] Use VertexElement instead.
    PACK_STRUCT(struct InputElement
        {
        byte Type; // VertexShaderMeta::InputType
        byte Index;
        byte Format; // PixelFormat
        byte InputSlot;
        uint32 AlignedByteOffset; // Fixed value or INPUT_LAYOUT_ELEMENT_ALIGN if auto
        byte InputSlotClass; // INPUT_LAYOUT_ELEMENT_PER_VERTEX_DATA or INPUT_LAYOUT_ELEMENT_PER_INSTANCE_DATA
        uint32 InstanceDataStepRate; // 0 if per-vertex
        });

    // Vertex elements input layout defined explicitly in the shader.
    // It's optional as it's been deprecated in favor or layouts defined by vertex buffers to allow data customizations.
    // Can be overriden by the vertex buffers provided upon draw call.
    // (don't release it as it's managed by GPUVertexLayout::Get)
    // [Deprecated in v1.10]
    GPUVertexLayout* Layout = nullptr;

    // Vertex shader inputs layout. Used to ensure that binded vertex buffers provide all required elements.
    GPUVertexLayout* InputLayout = nullptr;

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
    int32 _controlPointsCount = 0;

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
