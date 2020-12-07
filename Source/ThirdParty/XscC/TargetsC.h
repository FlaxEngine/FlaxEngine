/*
 * TargetsC.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_TARGETS_C_H
#define XSC_TARGETS_C_H


#include <Xsc/Export.h>
#include <stdbool.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


//! Shader target enumeration.
enum XscShaderTarget
{
    XscETargetUndefined,                    //!< Undefined shader target.

    XscETargetVertexShader,                 //!< Vertex shader.
    XscETargetTessellationControlShader,    //!< Tessellation-control (also Hull-) shader.
    XscETargetTessellationEvaluationShader, //!< Tessellation-evaluation (also Domain-) shader.
    XscETargetGeometryShader,               //!< Geometry shader.
    XscETargetFragmentShader,               //!< Fragment (also Pixel-) shader.
    XscETargetComputeShader,                //!< Compute shader.
};

//! Output shader version enumeration.
enum XscInputShaderVersion
{
    XscEInputCg     = 2,            //!< Cg (C for graphics) is a slightly extended HLSL3.

    XscEInputHLSL3  = 3,            //!< HLSL Shader Model 3.0 (DirectX 9).
    XscEInputHLSL4  = 4,            //!< HLSL Shader Model 4.0 (DirectX 10).
    XscEInputHLSL5  = 5,            //!< HLSL Shader Model 5.0 (DirectX 11).
    XscEInputHLSL6  = 6,            //!< HLSL Shader Model 6.0 (DirectX 12).

    XscEInputGLSL   = 0x0000ffff,   //!< GLSL (OpenGL).
    XscEInputESSL   = 0x0001ffff,   //!< GLSL (OpenGL ES).
    XscEInputVKSL   = 0x0002ffff,   //!< GLSL (Vulkan).
};

//! Output shader version enumeration.
enum XscOutputShaderVersion
{
    XscEOutputGLSL110   = 110,                 //!< GLSL 1.10 (OpenGL 2.0).
    XscEOutputGLSL120   = 120,                 //!< GLSL 1.20 (OpenGL 2.1).
    XscEOutputGLSL130   = 130,                 //!< GLSL 1.30 (OpenGL 3.0).
    XscEOutputGLSL140   = 140,                 //!< GLSL 1.40 (OpenGL 3.1).
    XscEOutputGLSL150   = 150,                 //!< GLSL 1.50 (OpenGL 3.2).
    XscEOutputGLSL330   = 330,                 //!< GLSL 3.30 (OpenGL 3.3).
    XscEOutputGLSL400   = 400,                 //!< GLSL 4.00 (OpenGL 4.0).
    XscEOutputGLSL410   = 410,                 //!< GLSL 4.10 (OpenGL 4.1).
    XscEOutputGLSL420   = 420,                 //!< GLSL 4.20 (OpenGL 4.2).
    XscEOutputGLSL430   = 430,                 //!< GLSL 4.30 (OpenGL 4.3).
    XscEOutputGLSL440   = 440,                 //!< GLSL 4.40 (OpenGL 4.4).
    XscEOutputGLSL450   = 450,                 //!< GLSL 4.50 (OpenGL 4.5).
    XscEOutputGLSL      = 0x0000ffff,          //!< Auto-detect minimal required GLSL version (for OpenGL 2+).

    XscEOutputESSL100   = (0x00010000 + 100),  //!< ESSL 1.00 (OpenGL ES 2.0). \note Currently not supported!
    XscEOutputESSL300   = (0x00010000 + 300),  //!< ESSL 3.00 (OpenGL ES 3.0). \note Currently not supported!
    XscEOutputESSL310   = (0x00010000 + 310),  //!< ESSL 3.10 (OpenGL ES 3.1). \note Currently not supported!
    XscEOutputESSL320   = (0x00010000 + 320),  //!< ESSL 3.20 (OpenGL ES 3.2). \note Currently not supported!
    XscEOutputESSL      = 0x0001ffff,          //!< Auto-detect minimum required ESSL version (for OpenGL ES 2+). \note Currently not supported!

    XscEOutputVKSL450   = (0x00020000 + 450),  //!< VKSL 4.50 (Vulkan 1.0).
    XscEOutputVKSL      = 0x0002ffff,           //!< Auto-detect minimum required VKSL version (for Vulkan/SPIR-V).
};


//! Returns the specified shader target as string.
XSC_EXPORT void XscShaderTargetToString(const enum XscShaderTarget target, char* str, size_t maxSize);

//! Returns the specified shader input version as string.
XSC_EXPORT void XscInputShaderVersionToString(const enum XscInputShaderVersion shaderVersion, char* str, size_t maxSize);

//! Returns the specified shader output version as string.
XSC_EXPORT void XscOutputShaderVersionToString(const enum XscOutputShaderVersion shaderVersion, char* str, size_t maxSize);

//! Returns true if the shader input version specifies HLSL (for DirectX).
XSC_EXPORT bool XscIsInputLanguageHLSL(const enum XscInputShaderVersion shaderVersion);

//! Returns true if the shader input version specifies GLSL (for OpenGL, OpenGL ES, and Vulkan).
XSC_EXPORT bool XscIsInputLanguageGLSL(const enum XscInputShaderVersion shaderVersion);

//! Returns true if the shader output version specifies GLSL (for OpenGL 2+).
XSC_EXPORT bool XscIsOutputLanguageGLSL(const enum XscOutputShaderVersion shaderVersion);

//! Returns true if the shader output version specifies ESSL (for OpenGL ES 2+).
XSC_EXPORT bool XscIsOutputLanguageESSL(const enum XscOutputShaderVersion shaderVersion);

//! Returns true if the shader output version specifies VKSL (for Vulkan).
XSC_EXPORT bool XscIsOutputLanguageVKSL(const enum XscOutputShaderVersion shaderVersion);

/**
\brief Returns the enumeration of all supported GLSL extensions as a map of extension name and version number.
\param[in] iterator Specifies the iterator. This must be NULL, to get the first element, or the value previously returned by this function.
\param[out] extension Specifies the output string of the extension name.
\param[in] maxSize Specifies the maximal size of the extension name string including the null terminator.
\param[out] version Specifies the output extension version.
\remarks Here is a usage example:
\code
char extension[256];
int version;

// Get first extension
void* iterator = XscGetGLSLExtensionEnumeration(NULL, extension, 256, &version);

while (iterator != NULL)
{
    // Print extension name and version
    printf("%s ( %d )\n", extension, version);
    
    // Get next extension
    iterator = XscGetGLSLExtensionEnumeration(iterator, extension, 256, &version);
}
\endcode
\note This can NOT be used in a multi-threaded environment!
*/
XSC_EXPORT void* XscGetGLSLExtensionEnumeration(void* iterator, char* extension, size_t maxSize, int* version);


#ifdef __cplusplus
} // /extern "C"
#endif


#endif



// ================================================================================
