/*
 * Targets.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_TARGETS_H
#define XSC_TARGETS_H


#include "Export.h"
#include <string>
#include <map>


namespace Xsc
{


/* ===== Public enumerations ===== */

//! Shader target enumeration.
enum class ShaderTarget
{
    Undefined,                      //!< Undefined shader target.

    VertexShader,                   //!< Vertex shader.
    TessellationControlShader,      //!< Tessellation-control (also Hull-) shader.
    TessellationEvaluationShader,   //!< Tessellation-evaluation (also Domain-) shader.
    GeometryShader,                 //!< Geometry shader.
    FragmentShader,                 //!< Fragment (also Pixel-) shader.
    ComputeShader,                  //!< Compute shader.
};

//! Input shader version enumeration.
enum class InputShaderVersion
{
    Cg      = 2,            //!< Cg (C for graphics) is a slightly extended HLSL3.

    HLSL3   = 3,            //!< HLSL Shader Model 3.0 (DirectX 9).
    HLSL4   = 4,            //!< HLSL Shader Model 4.0 (DirectX 10).
    HLSL5   = 5,            //!< HLSL Shader Model 5.0 (DirectX 11).
    HLSL6   = 6,            //!< HLSL Shader Model 6.0 (DirectX 12).

    GLSL    = 0x0000ffff,   //!< GLSL (OpenGL).
    ESSL    = 0x0001ffff,   //!< GLSL (OpenGL ES).
    VKSL    = 0x0002ffff,   //!< GLSL (Vulkan).
};

//! Output shader version enumeration.
enum class OutputShaderVersion
{
    GLSL110 = 110,                  //!< GLSL 1.10 (OpenGL 2.0).
    GLSL120 = 120,                  //!< GLSL 1.20 (OpenGL 2.1).
    GLSL130 = 130,                  //!< GLSL 1.30 (OpenGL 3.0).
    GLSL140 = 140,                  //!< GLSL 1.40 (OpenGL 3.1).
    GLSL150 = 150,                  //!< GLSL 1.50 (OpenGL 3.2).
    GLSL330 = 330,                  //!< GLSL 3.30 (OpenGL 3.3).
    GLSL400 = 400,                  //!< GLSL 4.00 (OpenGL 4.0).
    GLSL410 = 410,                  //!< GLSL 4.10 (OpenGL 4.1).
    GLSL420 = 420,                  //!< GLSL 4.20 (OpenGL 4.2).
    GLSL430 = 430,                  //!< GLSL 4.30 (OpenGL 4.3).
    GLSL440 = 440,                  //!< GLSL 4.40 (OpenGL 4.4).
    GLSL450 = 450,                  //!< GLSL 4.50 (OpenGL 4.5).
    GLSL460 = 460,                  //!< GLSL 4.60 (OpenGL 4.6).
    GLSL    = 0x0000ffff,           //!< Auto-detect minimal required GLSL version (for OpenGL 2+).

    ESSL100 = (0x00010000 + 100),   //!< ESSL 1.00 (OpenGL ES 2.0). \note Currently not supported!
    ESSL300 = (0x00010000 + 300),   //!< ESSL 3.00 (OpenGL ES 3.0). \note Currently not supported!
    ESSL310 = (0x00010000 + 310),   //!< ESSL 3.10 (OpenGL ES 3.1). \note Currently not supported!
    ESSL320 = (0x00010000 + 320),   //!< ESSL 3.20 (OpenGL ES 3.2). \note Currently not supported!
    ESSL    = 0x0001ffff,           //!< Auto-detect minimum required ESSL version (for OpenGL ES 2+). \note Currently not supported!

    VKSL450 = (0x00020000 + 450),   //!< VKSL 4.50 (Vulkan 1.0).
    VKSL    = 0x0002ffff,           //!< Auto-detect minimum required VKSL version (for Vulkan/SPIR-V).
};

//! Intermediate language enumeration.
enum class IntermediateLanguage
{
    SPIRV, //!< SPIR-V.
};


/* ===== Public functions ===== */

//! Returns the specified shader target as string.
XSC_EXPORT std::string ToString(const ShaderTarget target);

//! Returns the specified shader input version as string.
XSC_EXPORT std::string ToString(const InputShaderVersion shaderVersion);

//! Returns the specified shader output version as string.
XSC_EXPORT std::string ToString(const OutputShaderVersion shaderVersion);

//! Returns the specified intermediate language as string.
XSC_EXPORT std::string ToString(const IntermediateLanguage language);

//! Returns true if the shader input version specifies HLSL (for DirectX) or Cg (handled as dialect or HLSL).
XSC_EXPORT bool IsLanguageHLSL(const InputShaderVersion shaderVersion);

//! Returns true if the shader input version specifies GLSL (for OpenGL, OpenGL ES, and Vulkan).
XSC_EXPORT bool IsLanguageGLSL(const InputShaderVersion shaderVersion);

//! Returns true if the shader output version specifies GLSL (for OpenGL 2+).
XSC_EXPORT bool IsLanguageGLSL(const OutputShaderVersion shaderVersion);

//! Returns true if the shader output version specifies ESSL (for OpenGL ES 2+).
XSC_EXPORT bool IsLanguageESSL(const OutputShaderVersion shaderVersion);

//! Returns true if the shader output version specifies VKSL (for Vulkan).
XSC_EXPORT bool IsLanguageVKSL(const OutputShaderVersion shaderVersion);

//! Returns the enumeration of all supported GLSL extensions as a map of extension name and version number.
XSC_EXPORT const std::map<std::string, int>& GetGLSLExtensionEnumeration();


} // /namespace Xsc


#endif



// ================================================================================