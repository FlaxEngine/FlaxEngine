/*
 * XscC.h
 * 
 * This file is part of the XShaderCompiler project (Copyright (c) 2014-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef XSC_XSC_C_H
#define XSC_XSC_C_H


#include <Xsc/Export.h>
#include <Xsc/Version.h>
#include "TargetsC.h"
#include "LogC.h"
#include "IncludeHandlerC.h"
#include "ReflectionC.h"
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


//! Compiler warning flags.
enum XscWarnings
{
    XscWarnBasic                    = (1 <<  0), //!< Warning for basic issues (control path, disabled code etc.).
    XscWarnSyntax                   = (1 <<  1), //!< Warning for syntactic issues.
    XscWarnPreProcessor             = (1 <<  2), //!< Warning for pre-processor issues.
    XscWarnUnusedVariables          = (1 <<  3), //!< Warning for unused variables.
    XscWarnEmptyStatementBody       = (1 <<  4), //!< Warning for statements with empty body.
    XscWarnImplicitTypeConversions  = (1 <<  5), //!< Warning for specific implicit type conversions.
    XscWarnDeclarationShadowing     = (1 <<  6), //!< Warning for declarations that shadow a previous local (e.g. for-loops or variables in class hierarchy).
    XscWarnUnlocatedObjects         = (1 <<  7), //!< Warning for optional objects that where not found.
    XscWarnRequiredExtensions       = (1 <<  8), //!< Warning for required extensions in the output code.
    XscWarnCodeReflection           = (1 <<  9), //!< Warning for issues during code reflection.
    XscWarnIndexBoundary            = (1 << 10), //!< Warning for index boundary violations.

    XscWarnAll                      = (~0u),     //!< All warnings.
};

/**
\brief Language extension flags.
\remakrs This is only supported, if the compiler was build with the 'XSC_ENABLE_LANGUAGE_EXT' macro.
*/
enum XscExtensions
{
    XscExtLayoutAttribute   = (1 << 0), //!< Enables the 'layout' attribute (e.g. "[layout(rgba8)]").
    XscExtSpaceAttribute    = (1 << 1), //!< Enables the 'space' attribute extension for a stronger type system (e.g. "[space(OBJECT, MODEL)]").

    XscExtAll               = (~0u)     //!< All extensions.
};

//! Formatting descriptor structure for the output shader.
struct XscFormatting
{
    //! If true, scopes are always written in braces. By default false.
    bool        alwaysBracedScopes;

    //! If true, blank lines are allowed. By default true.
    bool        blanks;

    //! If true, wrapper functions for special intrinsics are written in a compact formatting (i.e. all in one line). By default false.
    bool        compactWrappers;

    //! Indentation string for code generation. By default 4 spaces.
    const char* indent;

    //! If true, line marks are allowed. By default false.
    bool        lineMarks;

    //! If true, auto-formatting of line separation is allowed. By default true.
    bool        lineSeparation;

    //! If true, the '{'-braces for an open scope gets its own line. If false, braces are written like in Java coding conventions. By default true.
    bool        newLineOpenScope;
};

//! Structure for additional translation options.
struct XscOptions
{
    //! If true, the shader output may contain GLSL extensions, if the target shader version is too low. By default false.
    bool    allowExtensions;

    /**
    \brief If true, binding slots for all buffer types will be generated sequentially, starting with index at 'autoBindingStartSlot'. By default false.
    \remarks This will also enable 'explicitBinding'.
    */
    bool    autoBinding;

    //! Index to start generating binding slots from. Only relevant if 'autoBinding' is enabled. By default 0.
    int     autoBindingStartSlot;

    //! If true, explicit binding slots are enabled. By default false.
    bool    explicitBinding;

    //! If true, code obfuscation is performed. By default false.
    bool    obfuscate;

    //! If true, little code optimizations are performed. By default false.
    bool    optimize;

    //! If true, intrinsics are prefered to be implemented as wrappers (instead of inlining). By default false.
    bool    preferWrappers;

    //! If true, only the preprocessed source code will be written out. By default false.
    bool    preprocessOnly;

    //! If true, commentaries are preserved for each statement. By default false.
    bool    preserveComments;

    //! If true, matrices have row-major alignment. Otherwise the matrices have column-major alignment. By default false.
    bool    rowMajorAlignment;

    //! If true, generated GLSL code will contain separate sampler and texture objects when supported. By default true.
    bool    separateSamplers;

    //! If true, generated GLSL code will support the 'ARB_separate_shader_objects' extension. By default false.
    bool    separateShaders;

    //! If true, the AST (Abstract Syntax Tree) will be written to the log output. By default false.
    bool    showAST;

    //! If true, the timings of the different compilation processes are written to the log output. By default false.
    bool    showTimes;

    //! If true, array initializations will be unrolled. By default false.
    bool    unrollArrayInitializers;

    //! If true, the source code is only validated, but no output code will be generated. By default false.
    bool    validateOnly;
};

//! Name mangling descriptor structure for shader input/output variables (also referred to as "varyings"), temporary variables, and reserved keywords.
struct XscNameMangling
{
    /**
    \brief Name mangling prefix for shader input variables. By default "xsv_".
    \remarks This can also be empty or equal to "outputPrefix".
    */
    const char* inputPrefix;

    /**
    \brief Name mangling prefix for shader output variables. By default "xsv_".
    \remarks This can also be empty or equal to "inputPrefix".
    */
    const char* outputPrefix;

    /**
    \brief Name mangling prefix for reserved words (such as "texture", "main", "sin" etc.). By default "xsr_".
    \remarks This must not be equal to any of the other prefixes and it must not be empty.
    */
    const char* reservedWordPrefix;

    /**
    \brief Name mangling prefix for temporary variables. By default "xst_".
    \remarks This must not be equal to any of the other prefixes and it must not be empty.
    */
    const char* temporaryPrefix;

    /**
    \brief Name mangling prefix for namespaces like structures or classes. By default "xsn_".
    \remarks This can also be empty, but if it's not empty it must not be equal to any of the other prefixes.
    */
    const char* namespacePrefix;

    /**
    If true, shader input/output variables are always renamed to their semantics,
    even for vertex input and fragment output. Otherwise, their original identifiers are used. By default false.
    */
    bool        useAlwaysSemantics;

    /**
    \brief If true, the data fields of a 'buffer'-objects is renamed rather than the outer identifier. By default false.
    \remarks This can be useful for external diagnostic tools, to access the original identifier.
    */
    bool        renameBufferFields;
};

//! Shader input descriptor structure.
struct XscShaderInput
{
    //! Specifies the filename of the input shader code. This is an optional attribute, and only a hint to the compiler. By default NULL.
    const char*                     filename;

    //! Specifies the input source code. This must not be null when passed to the "XscCompileShader" function!
    const char*                     sourceCode;

    //! Specifies the input shader version (e.g. XscEInputHLSL5 for "HLSL 5"). By default XscEInputHLSL5.
    enum XscInputShaderVersion      shaderVersion;
    
    //! Specifies the target shader (Vertex, Fragment etc.). By default XscUndefinedShader.
    enum XscShaderTarget            shaderTarget;

    //! Specifies the HLSL shader entry point. By default "main".
    const char*                     entryPoint;

    /**
    \brief Specifies the secondary HLSL shader entry point. By default NULL.
    \remarks This is only used for a Tessellation-Control Shader (alias Hull Shader) entry point,
    when a Tessellation-Control Shader (alias Domain Shader) is the output target.
    This is required to translate all Tessellation-Control attributes (i.e. "partitioning" and "outputtopology")
    to the Tessellation-Evaluation output shader. If this is empty, the default values for these attributes are used.
    */
    const char*                     secondaryEntryPoint;

    /**
    \brief Compiler warning flags. This can be a bitwise OR combination of the "XscWarnings" enumeration entries. By default 0.
    \see XscWarnings
    */
    unsigned int                    warnings;

    /**
    \brief Language extension flags. This can be a bitwise OR combination of the "Extensions" enumeration entries. By default 0.
    \remarks This is ignored, if the compiler was not build with the 'XSC_ENABLE_LANGUAGE_EXT' macro.
    \see Extensions
    */
    unsigned int                    extensions;

    //! Include handler member which contains a function pointer to handle '#include'-directives.
    struct XscIncludeHandler        includeHandler;
};

//! Vertex shader semantic (or rather attribute) layout structure.
struct XscVertexSemantic
{
    //! Specifies the shader semantic (or rather attribute).
    const char* semantic;

    //! Specifies the binding location.
    int         location;
};

//! Shader output descriptor structure.
struct XscShaderOutput
{
    //! Specifies the filename of the output shader code. This is an optional attribute, and only a hint to the compiler.
    const char*                     filename;

    //! Specifies the output source code. This will contain the output code. This must not be null when passed to the "XscCompileShader" function!
    const char**                    sourceCode;

    //! Specifies the output shader version. By default XscEOutputGLSL (to auto-detect minimum required version).
    enum XscOutputShaderVersion     shaderVersion;

    //! Optional list of vertex semantic layouts, to bind a vertex attribute (semantic name) to a location index (only used when 'explicitBinding' is true). By default NULL.
    const struct XscVertexSemantic* vertexSemantics;

    //! Number of elements the 'vertexSemantics' member points to. By default 0.
    size_t                          vertexSemanticsCount;

    //! Additional options to configure the code generation.
    struct XscOptions               options;

    //! Output code formatting descriptor.
    struct XscFormatting            formatting;
    
    //! Specifies the options for name mangling.
    struct XscNameMangling          nameMangling;
};

/**
\brief Initializes the specified descriptor structures to their default values.
\param[out] inputDesc Input shader code descriptor. If NULL, this structure is not initialized.
\param[out] outputDesc Output shader code descriptor. If NULL, this structure is not initialized.
*/
XSC_EXPORT void XscInitialize(struct XscShaderInput* inputDesc, struct XscShaderOutput* outputDesc);

/**
\brief Cross compiles the shader code from the specified input stream into the specified output shader code.
\param[in] inputDesc Input shader code descriptor.
\param[in] outputDesc Output shader code descriptor.
\param[in] log Optional pointer to an output log. This can be NULL (to ignore log) or XSC_DEFAULT_LOG (to use the default log).
\param[out] reflectionData Optional pointer to a code reflection data structure. If NULL, no reflection data is written out.
\return True if the code has been translated successfully.
\see ShaderInput
\see ShaderOutput
\see Log
\see ReflectionData
*/
XSC_EXPORT bool XscCompileShader(
    const struct XscShaderInput*    inputDesc,
    const struct XscShaderOutput*   outputDesc,
    const struct XscLog*            log,
    struct XscReflectionData*       reflectionData
);


#ifdef __cplusplus
} // /extern "C"
#endif


#endif



// ================================================================================