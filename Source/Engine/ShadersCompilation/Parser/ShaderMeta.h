// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_SHADER_COMPILER

#include "Engine/Core/Config.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Config.h"

struct ShaderPermutationEntry
{
    StringAnsi Name;
    StringAnsi Value;
};

struct ShaderPermutation
{
    // TODO: maybe we could use macro SHADER_PERMUTATIONS_MAX_PARAMS_COUNT and reduce amount of dynamic allocations for permutations
    Array<ShaderPermutationEntry> Entries;

    Array<char> DebugData;
};

/// <summary>
/// Shader function metadata
/// </summary>
class ShaderFunctionMeta
{
public:
    /// <summary>
    /// Virtual destructor
    /// </summary>
    virtual ~ShaderFunctionMeta() = default;

public:
    /// <summary>
    /// Function name
    /// </summary>
    StringAnsi Name;

    /// <summary>
    /// Function flags.
    /// </summary>
    ShaderFlags Flags;

    /// <summary>
    /// The minimum graphics platform feature level to support this shader.
    /// </summary>
    FeatureLevel MinFeatureLevel;

    /// <summary>
    /// All possible values for the permutations and it's values to generate different permutation of this function
    /// </summary>
    Array<ShaderPermutation> Permutations;

public:
    /// <summary>
    /// Checks if definition name has been added to the given permutation
    /// </summary>
    /// <param name="permutationIndex">Shader permutation index</param>
    /// <param name="defineName">Name of the definition to check</param>
    /// <returns>True if definition of given name has been registered for the permutations, otherwise false</returns>
    bool HasDefinition(int32 permutationIndex, const StringAnsi& defineName) const
    {
        ASSERT(Math::IsInRange(permutationIndex, 0, Permutations.Count() - 1));

        auto& permutation = Permutations[permutationIndex];
        for (int32 i = 0; i < permutation.Entries.Count(); i++)
        {
            if (permutation.Entries[i].Name == defineName)
                return true;
        }

        return false;
    }

    /// <summary>
    /// Checks if definition name has been added to the permutations collection
    /// </summary>
    /// <param name="defineName">Name of the definition to check</param>
    /// <returns>True if definition of given name has been registered for the permutations, otherwise false</returns>
    bool HasDefinition(const StringAnsi& defineName) const
    {
        for (int32 permutationIndex = 0; permutationIndex < Permutations.Count(); permutationIndex++)
        {
            if (HasDefinition(permutationIndex, defineName))
                return true;
        }

        return false;
    }

    /// <summary>
    /// Gets all macros for given shader permutation
    /// </summary>
    /// <param name="permutationIndex">Shader permutation index</param>
    /// <param name="macros">Output array with permutation macros</param>
    void GetDefinitionsForPermutation(int32 permutationIndex, Array<ShaderMacro>& macros) const
    {
        ASSERT(Math::IsInRange(permutationIndex, 0, Permutations.Count() - 1));

        auto& permutation = Permutations[permutationIndex];
        for (int32 i = 0; i < permutation.Entries.Count(); i++)
        {
            auto& e = permutation.Entries[i];
            macros.Add({
                e.Name.Get(),
                e.Value.Get()
            });
        }
    }

public:
    /// <summary>
    /// Gets shader function meta stage type.
    /// </summary>
    virtual ShaderStage GetStage() const = 0;
};

/// <summary>
/// Vertex shader function meta
/// </summary>
class VertexShaderMeta : public ShaderFunctionMeta
{
public:
    /// <summary>
    /// Input element type
    /// [Deprecated in v1.10]
    /// </summary>
    enum class InputType : byte
    {
        Invalid = 0,
        POSITION = 1,
        COLOR = 2,
        TEXCOORD = 3,
        NORMAL = 4,
        TANGENT = 5,
        BITANGENT = 6,
        ATTRIBUTE = 7,
        BLENDINDICES = 8,
        BLENDWEIGHTS = 9,
        // [Deprecated in v1.10]
        BLENDWEIGHT = BLENDWEIGHTS,
    };

    /// <summary>
    /// Input element
    /// [Deprecated in v1.10]
    /// </summary>
    struct InputElement
    {
        /// <summary>
        /// Semantic type
        /// </summary>
        InputType Type;

        /// <summary>
        /// Semantic index
        /// </summary>
        byte Index;

        /// <summary>
        /// Element data format
        /// </summary>
        PixelFormat Format;

        /// <summary>
        /// An integer value that identifies the input-assembler
        /// </summary>
        byte InputSlot;

        /// <summary>
        /// Optional. Offset (in bytes) between each element. Use INPUT_LAYOUT_ELEMENT_ALIGN for convenience to define the current element directly after the previous one, including any packing if necessary
        /// </summary>
        uint32 AlignedByteOffset;

        /// <summary>
        /// Identifies the input data class for a single input slot. INPUT_LAYOUT_ELEMENT_PER_VERTEX_DATA or INPUT_LAYOUT_ELEMENT_PER_INSTANCE_DATA
        /// </summary>
        byte InputSlotClass;

        /// <summary>
        /// The number of instances to draw using the same per-instance data before advancing in the buffer by one element. This value must be 0 for an element that contains per-vertex data
        /// </summary>
        uint32 InstanceDataStepRate;

        /// <summary>
        /// The visible flag expression. Allows to show/hide element from the input layout based on input macros (also from permutation macros). use empty value to skip this feature.
        /// </summary>
        StringAnsi VisibleFlag;
    };

public:
    /// <summary>
    /// Input layout description
    /// [Deprecated in v1.10]
    /// </summary>
    Array<InputElement> InputLayout;

public:
    // [ShaderFunctionMeta]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Vertex;
    }
};

/// <summary>
/// Hull (or tessellation control) shader function meta
/// </summary>
class HullShaderMeta : public ShaderFunctionMeta
{
public:
    /// <summary>
    /// The input control points count (valid range: 1-32).
    /// </summary>
    int32 ControlPointsCount;

public:
    // [ShaderFunctionMeta]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Hull;
    }
};

/// <summary>
/// Domain (or tessellation evaluation) shader function meta
/// </summary>
class DomainShaderMeta : public ShaderFunctionMeta
{
public:
    // [ShaderFunctionMeta]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Domain;
    }
};

/// <summary>
/// Geometry shader function meta
/// </summary>
class GeometryShaderMeta : public ShaderFunctionMeta
{
public:
    // [ShaderFunctionMeta]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Geometry;
    }
};

/// <summary>
/// Pixel shader function meta
/// </summary>
class PixelShaderMeta : public ShaderFunctionMeta
{
public:
    // [ShaderFunctionMeta]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Pixel;
    }
};

/// <summary>
/// Compute shader function meta
/// </summary>
class ComputeShaderMeta : public ShaderFunctionMeta
{
public:
    // [ShaderFunctionMeta]
    ShaderStage GetStage() const override
    {
        return ShaderStage::Compute;
    }
};

/// <summary>
/// Constant buffer meta
/// </summary>
struct ConstantBufferMeta
{
    /// <summary>
    /// Slot index
    /// </summary>
    byte Slot;

    /// <summary>
    /// Buffer name
    /// </summary>
    StringAnsi Name;
};

/// <summary>
/// Shader source metadata
/// </summary>
class ShaderMeta
{
public:
    /// <summary>
    /// Vertex Shaders
    /// </summary>
    Array<VertexShaderMeta> VS;

    /// <summary>
    /// Hull Shaders
    /// </summary>
    Array<HullShaderMeta> HS;

    /// <summary>
    /// Domain Shaders
    /// </summary>
    Array<DomainShaderMeta> DS;

    /// <summary>
    /// Geometry Shaders
    /// </summary>
    Array<GeometryShaderMeta> GS;

    /// <summary>
    /// Pixel Shaders
    /// </summary>
    Array<PixelShaderMeta> PS;

    /// <summary>
    /// Compute Shaders
    /// </summary>
    Array<ComputeShaderMeta> CS;

    /// <summary>
    /// Constant Buffers
    /// </summary>
    Array<ConstantBufferMeta> CB;

public:
    /// <summary>
    /// Gets amount of shaders attached (not counting permutations).
    /// </summary>
    int32 GetShadersCount() const
    {
        return VS.Count() + HS.Count() + DS.Count() + GS.Count() + PS.Count() + CS.Count();
    }

    /// <summary>
    /// Gets all shader functions (all types).
    /// </summary>
    /// <param name="functions">Output collections of functions</param>
    void GetShaders(Array<const ShaderFunctionMeta*>& functions) const
    {
#define PEEK_SHADERS(collection) for (int32 i = 0; i < collection.Count(); i++) functions.Add(dynamic_cast<const ShaderFunctionMeta*>(&(collection[i])));
        PEEK_SHADERS(VS);
        PEEK_SHADERS(HS);
        PEEK_SHADERS(DS);
        PEEK_SHADERS(GS);
        PEEK_SHADERS(PS);
        PEEK_SHADERS(CS);
#undef PEEK_SHADERS
    }
};

#endif
