/*
 * ReflectionC.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_REFLECTION_C_H
#define XSC_REFLECTION_C_H


#include <Xsc/Export.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


//! Sampler filter enumeration (D3D11_FILTER).
enum XscFilter
{
    XscEFilterMinMagMipPoint                        = 0,
    XscEFilterMinMagPointMipLinear                  = 0x1,
    XscEFilterMinPointMagLinearMipPoint             = 0x4,
    XscEFilterMinPointMagMipLinear                  = 0x5,
    XscEFilterMinLinearMagMipPoint                  = 0x10,
    XscEFilterMinLinearMagPointMipLinear            = 0x11,
    XscEFilterMinMagLinearMipPoint                  = 0x14,
    XscEFilterMinMagMipLinear                       = 0x15,
    XscEFilterAnisotropic                           = 0x55,
    XscEFilterComparisonMinMagMipPoint              = 0x80,
    XscEFilterComparisonMinMagPointMipLinear        = 0x81,
    XscEFilterComparisonMinPointMagLinearMipPoint   = 0x84,
    XscEFilterComparisonMinPointMagMipLinear        = 0x85,
    XscEFilterComparisonMinLinearMagMipPoint        = 0x90,
    XscEFilterComparisonMinLinearMagPointMipLinear  = 0x91,
    XscEFilterComparisonMinMagLinearMipPoint        = 0x94,
    XscEFilterComparisonMinMagMipLinear             = 0x95,
    XscEFilterComparisonAnisotropic                 = 0xd5,
    XscEFilterMinimumMinMagMipPoint                 = 0x100,
    XscEFilterMinimumMinMagPointMipLinear           = 0x101,
    XscEFilterMinimumMinPointMagLinearMipPoint      = 0x104,
    XscEFilterMinimumMinPointMagMipLinear           = 0x105,
    XscEFilterMinimumMinLinearMagMipPoint           = 0x110,
    XscEFilterMinimumMinLinearMagPointMipLinear     = 0x111,
    XscEFilterMinimumMinMagLinearMipPoint           = 0x114,
    XscEFilterMinimumMinMagMipLinear                = 0x115,
    XscEFilterMinimumAnisotropic                    = 0x155,
    XscEFilterMaximumMinMagMipPoint                 = 0x180,
    XscEFilterMaximumMinMagPointMipLinear           = 0x181,
    XscEFilterMaximumMinPointMagLinearMipPoint      = 0x184,
    XscEFilterMaximumMinPointMagMipLinear           = 0x185,
    XscEFilterMaximumMinLinearMagMipPoint           = 0x190,
    XscEFilterMaximumMinLinearMagPointMipLinear     = 0x191,
    XscEFilterMaximumMinMagLinearMipPoint           = 0x194,
    XscEFilterMaximumMinMagMipLinear                = 0x195,
    XscEFilterMaximumAnisotropic                    = 0x1d5,
};

//! Texture address mode enumeration (D3D11_TEXTURE_ADDRESS_MODE).
enum XscTextureAddressMode
{
    XscEAddressWrap         = 1,
    XscEAddressMirror       = 2,
    XscEAddressClamp        = 3,
    XscEAddressBorder       = 4,
    XscEAddressMirrorOnce   = 5,
};

//! Sample comparison function enumeration (D3D11_COMPARISON_FUNC).
enum XscComparisonFunc
{
    XscEComparisonNever         = 1,
    XscEComparisonLess          = 2,
    XscEComparisonEqual         = 3,
    XscEComparisonLessEqual     = 4,
    XscEComparisonGreater       = 5,
    XscEComparisonNotEqual      = 6,
    XscEComparisonGreaterEqual  = 7,
    XscEComparisonAlways        = 8,
};

/**
\brief Static sampler state descriptor structure (D3D11_SAMPLER_DESC).
\remarks All members and enumerations have the same values like the one in the "D3D11_SAMPLER_DESC" structure respectively.
Thus, they can all be statically casted from and to the original D3D11 values.
\see https://msdn.microsoft.com/en-us/library/windows/desktop/ff476207(v=vs.85).aspx
*/
struct XscSamplerState
{
    const char*                 ident;
    enum XscFilter              filter;
    enum XscTextureAddressMode  addressU;
    enum XscTextureAddressMode  addressV;
    enum XscTextureAddressMode  addressW;
    float                       mipLODBias;
    unsigned int                maxAnisotropy;
    enum XscComparisonFunc      comparisonFunc;
    float                       borderColor[4];
    float                       minLOD;
    float                       maxLOD;
};

//! Binding slot of textures, constant buffers, and fragment targets.
struct XscBindingSlot
{
    //! Identifier of the binding point.
    const char* ident;

    //! Zero based binding point or location. If this is -1, the location has not been set explicitly.
    int         location;
};

//! Number of threads within each work group of a compute shader.
struct XscNumThreads
{
    int x;
    int y;
    int z;
};

//! Structure for shader output statistics (e.g. texture/buffer binding points).
struct XscReflectionData
{
    //! All defined macros after pre-processing.
    const char**                    macros;

    //! Number of elements in 'macros'.
    size_t                          macrosCount;

    //! Single shader uniforms.
    const char**                    uniforms;

    //! Number of elements in 'uniforms'.
    size_t                          uniformsCount;

    //! Texture bindings.
    const struct XscBindingSlot*    textures;

    //! Number of elements in 'textures'.
    size_t                          texturesCount;

    //! Storage buffer bindings.
    const struct XscBindingSlot*    storageBuffers;

    //! Number of elements in 'storageBuffers'.
    size_t                          storageBuffersCount;

    //! Constant buffer bindings.
    const struct XscBindingSlot*    constantBuffers;

    //! Number of elements in 'constantBuffers'.
    size_t                          constantBufferCounts;

    //! Shader input attributes.
    const struct XscBindingSlot*    inputAttributes;

    //! Number of elements in 'inputAttributes'.
    size_t                          inputAttributesCount;

    //! Shader output attributes.
    const struct XscBindingSlot*    outputAttributes;

    //! Number of elements in 'outputAttributes'.
    size_t                          outputAttributesCount;

    //! Static sampler states (identifier, states).
    const struct XscSamplerState*   samplerStates;

    //! Number of elements in 'samplerStates'.
    size_t                          samplerStatesCount;

    //! 'numthreads' attribute of a compute shader.
    struct XscNumThreads            numThreads;
};


//! Returns the string representation of the specified 'SamplerState::Filter' type.
XSC_EXPORT void XscFilterToString(const enum XscFilter t, char* str, size_t maxSize);

//! Returns the string representation of the specified 'SamplerState::TextureAddressMode' type.
XSC_EXPORT void XscTextureAddressModeToString(const enum XscTextureAddressMode t, char* str, size_t maxSize);

//! Returns the string representation of the specified 'SamplerState::ComparisonFunc' type.
XSC_EXPORT void XscComparisonFuncToString(const enum XscComparisonFunc t, char* str, size_t maxSize);


#ifdef __cplusplus
} // /extern "C"
#endif


#endif



// ================================================================================