/*
 * Reflection.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_REFLECTION_H
#define XSC_REFLECTION_H


#include "Export.h"
#include <limits>
#include <string>
#include <map>
#include <vector>
#include <ostream>


namespace Xsc
{

//! Shader code reflection namespace
namespace Reflection
{


/* ===== Public enumerations ===== */

//! Sampler filter enumeration (D3D11_FILTER).
enum class Filter
{
    MinMagMipPoint                          = 0,
    MinMagPointMipLinear                    = 0x1,
    MinPointMagLinearMipPoint               = 0x4,
    MinPointMagMipLinear                    = 0x5,
    MinLinearMagMipPoint                    = 0x10,
    MinLinearMagPointMipLinear              = 0x11,
    MinMagLinearMipPoint                    = 0x14,
    MinMagMipLinear                         = 0x15,
    Anisotropic                             = 0x55,
    ComparisonMinMagMipPoint                = 0x80,
    ComparisonMinMagPointMipLinear          = 0x81,
    ComparisonMinPointMagLinearMipPoint     = 0x84,
    ComparisonMinPointMagMipLinear          = 0x85,
    ComparisonMinLinearMagMipPoint          = 0x90,
    ComparisonMinLinearMagPointMipLinear    = 0x91,
    ComparisonMinMagLinearMipPoint          = 0x94,
    ComparisonMinMagMipLinear               = 0x95,
    ComparisonAnisotropic                   = 0xd5,
    MinimumMinMagMipPoint                   = 0x100,
    MinimumMinMagPointMipLinear             = 0x101,
    MinimumMinPointMagLinearMipPoint        = 0x104,
    MinimumMinPointMagMipLinear             = 0x105,
    MinimumMinLinearMagMipPoint             = 0x110,
    MinimumMinLinearMagPointMipLinear       = 0x111,
    MinimumMinMagLinearMipPoint             = 0x114,
    MinimumMinMagMipLinear                  = 0x115,
    MinimumAnisotropic                      = 0x155,
    MaximumMinMagMipPoint                   = 0x180,
    MaximumMinMagPointMipLinear             = 0x181,
    MaximumMinPointMagLinearMipPoint        = 0x184,
    MaximumMinPointMagMipLinear             = 0x185,
    MaximumMinLinearMagMipPoint             = 0x190,
    MaximumMinLinearMagPointMipLinear       = 0x191,
    MaximumMinMagLinearMipPoint             = 0x194,
    MaximumMinMagMipLinear                  = 0x195,
    MaximumAnisotropic                      = 0x1d5,
};

//! Texture address mode enumeration (D3D11_TEXTURE_ADDRESS_MODE).
enum class TextureAddressMode
{
    Wrap        = 1,
    Mirror      = 2,
    Clamp       = 3,
    Border      = 4,
    MirrorOnce  = 5,
};

//! Sample comparison function enumeration (D3D11_COMPARISON_FUNC).
enum class ComparisonFunc
{
    Never           = 1,
    Less            = 2,
    Equal           = 3,
    LessEqual       = 4,
    Greater         = 5,
    NotEqual        = 6,
    GreaterEqual    = 7,
    Always          = 8,
};


/* ===== Public structures ===== */

/**
\brief Static sampler state descriptor structure (D3D11_SAMPLER_DESC).
\remarks All members and enumerations have the same values like the one in the "D3D11_SAMPLER_DESC" structure respectively.
Thus, they can all be statically casted from and to the original D3D11 values.
\see https://msdn.microsoft.com/en-us/library/windows/desktop/ff476207(v=vs.85).aspx
*/
struct SamplerState
{
    Filter              filter          = Filter::MinMagMipLinear;
    TextureAddressMode  addressU        = TextureAddressMode::Clamp;
    TextureAddressMode  addressV        = TextureAddressMode::Clamp;
    TextureAddressMode  addressW        = TextureAddressMode::Clamp;
    float               mipLODBias      = 0.0f;
    unsigned int        maxAnisotropy   = 1u;
    ComparisonFunc      comparisonFunc  = ComparisonFunc::Never;
    float               borderColor[4]  = { 0.0f, 0.0f, 0.0f, 0.0f };
    float               minLOD          = -std::numeric_limits<float>::max();
    float               maxLOD          = std::numeric_limits<float>::max();
};

//! Binding slot of textures, constant buffers, and fragment targets.
struct BindingSlot
{
    //! Identifier of the binding point.
    std::string ident;

    //! Zero based binding point or location. If this is -1, the location has not been set.
    int         location;
};

enum class UniformType
{
    Buffer,
    UniformBuffer,
    Sampler,
    Variable,
    Struct
};

enum class BufferType
{
    Undefined,

    Buffer,
    StructuredBuffer,
    ByteAddressBuffer,

    RWBuffer,
    RWStructuredBuffer,
    RWByteAddressBuffer,
    AppendStructuredBuffer,
    ConsumeStructuredBuffer,

    RWTexture1D,
    RWTexture1DArray,
    RWTexture2D,
    RWTexture2DArray,
    RWTexture3D,

    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
    Texture2DMS,
    Texture2DMSArray,
};

enum class DataType
{
    Undefined,

    // String types,
    String,

    // Scalar types
    Bool,
    Int,
    UInt,
    Half,
    Float,
    Double,

    // Vector types
    Bool2,
    Bool3,
    Bool4,
    Int2,
    Int3,
    Int4,
    UInt2,
    UInt3,
    UInt4,
    Half2,
    Half3,
    Half4,
    Float2,
    Float3,
    Float4,
    Double2,
    Double3,
    Double4,

    // Matrix types
    Bool2x2,
    Bool2x3,
    Bool2x4,
    Bool3x2,
    Bool3x3,
    Bool3x4,
    Bool4x2,
    Bool4x3,
    Bool4x4,
    Int2x2,
    Int2x3,
    Int2x4,
    Int3x2,
    Int3x3,
    Int3x4,
    Int4x2,
    Int4x3,
    Int4x4,
    UInt2x2,
    UInt2x3,
    UInt2x4,
    UInt3x2,
    UInt3x3,
    UInt3x4,
    UInt4x2,
    UInt4x3,
    UInt4x4,
    Half2x2,
    Half2x3,
    Half2x4,
    Half3x2,
    Half3x3,
    Half3x4,
    Half4x2,
    Half4x3,
    Half4x4,
    Float2x2,
    Float2x3,
    Float2x4,
    Float3x2,
    Float3x3,
    Float3x4,
    Float4x2,
    Float4x3,
    Float4x4,
    Double2x2,
    Double2x3,
    Double2x4,
    Double3x2,
    Double3x3,
    Double3x4,
    Double4x2,
    Double4x3,
    Double4x4,
};

enum class VarType
{
    Undefined,
    Void,

    // Scalar types
    Bool,
    Int,
    UInt,
    Half,
    Float,
    Double,

    // Vector types
    Bool2,
    Bool3,
    Bool4,
    Int2,
    Int3,
    Int4,
    UInt2,
    UInt3,
    UInt4,
    Half2,
    Half3,
    Half4,
    Float2,
    Float3,
    Float4,
    Double2,
    Double3,
    Double4,

    // Matrix types
    Bool2x2,
    Bool2x3,
    Bool2x4,
    Bool3x2,
    Bool3x3,
    Bool3x4,
    Bool4x2,
    Bool4x3,
    Bool4x4,
    Int2x2,
    Int2x3,
    Int2x4,
    Int3x2,
    Int3x3,
    Int3x4,
    Int4x2,
    Int4x3,
    Int4x4,
    UInt2x2,
    UInt2x3,
    UInt2x4,
    UInt3x2,
    UInt3x3,
    UInt3x4,
    UInt4x2,
    UInt4x3,
    UInt4x4,
    Half2x2,
    Half2x3,
    Half2x4,
    Half3x2,
    Half3x3,
    Half3x4,
    Half4x2,
    Half4x3,
    Half4x4,
    Float2x2,
    Float2x3,
    Float2x4,
    Float3x2,
    Float3x3,
    Float3x4,
    Float4x2,
    Float4x3,
    Float4x4,
    Double2x2,
    Double2x3,
    Double2x4,
    Double3x2,
    Double3x3,
    Double3x4,
    Double4x2,
    Double4x3,
    Double4x4,
};

//! A single element in a constant buffer or an opaque type
struct Uniform
{
    //! Identifier of the element.
    std::string ident;

    //! Data type of the element.
    UniformType type = UniformType::Variable;

    //! Determines actual type of the element. Contents depend on "type".
    int baseType = 0;

    //! Index of the uniform block this uniform belongs to. -1 if none.
    int uniformBlock = -1;
};

//! Number of threads within each work group of a compute shader.
struct NumThreads
{
    //! Number of shader compute threads in X dimension.
    int x = 0;

    //! Number of shader compute threads in Y dimension.
    int y = 0;

    //! Number of shader compute threads in Z dimension.
    int z = 0;
};

//! Structure for shader output statistics (e.g. texture/buffer binding points).
struct ReflectionData
{
    //! All defined macros after pre-processing.
    std::vector<std::string>            macros;

    //! Single shader uniforms.
    std::vector<Uniform>                uniforms;

    //! Texture bindings.
    std::vector<BindingSlot>            textures;

    //! Storage buffer bindings.
    std::vector<BindingSlot>            storageBuffers;

    //! Constant buffer bindings.
    std::vector<BindingSlot>            constantBuffers;

    //! Shader input attributes.
    std::vector<BindingSlot>            inputAttributes;

    //! Shader output attributes.
    std::vector<BindingSlot>            outputAttributes;

    //! Static sampler states (identifier, states).
    std::map<std::string, SamplerState> samplerStates;

    //! Number of local threads in a compute shader.
    NumThreads                          numThreads;
};


} // /namespace Reflection


/* ===== Public functions ===== */

//! Returns the string representation of the specified 'SamplerState::Filter' type.
XSC_EXPORT std::string ToString(const Reflection::Filter t);

//! Returns the string representation of the specified 'SamplerState::TextureAddressMode' type.
XSC_EXPORT std::string ToString(const Reflection::TextureAddressMode t);

//! Returns the string representation of the specified 'SamplerState::ComparisonFunc' type.
XSC_EXPORT std::string ToString(const Reflection::ComparisonFunc t);

//! Prints the reflection data into the output stream in a human readable format.
XSC_EXPORT void PrintReflection(std::ostream& stream, const Reflection::ReflectionData& reflectionData);


} // /namespace Xsc


#endif



// ================================================================================