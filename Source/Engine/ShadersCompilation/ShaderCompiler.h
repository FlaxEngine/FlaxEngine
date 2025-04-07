// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_SHADER_COMPILER

#include "ShaderCompilationContext.h"
#include "Parser/ShaderMeta.h"
#include "Engine/Graphics/Shaders/GPUShaderProgram.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"

/// <summary>
/// Base class for the objects that can compile shaders source code.
/// </summary>
class ShaderCompiler
{
public:
    struct ShaderResourceBuffer
    {
        byte Slot;
        bool IsUsed;
        uint32 Size;
    };

private:
    Array<char> _funcNameDefineBuffer;

protected:
    ShaderProfile _profile;
    ShaderCompilationContext* _context = nullptr;
    Array<ShaderMacro> _globalMacros;
    Array<ShaderMacro> _macros;
    Array<ShaderResourceBuffer> _constantBuffers;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderCompiler"/> class.
    /// </summary>
    /// <param name="profile">The profile.</param>
    ShaderCompiler(ShaderProfile profile)
        : _profile(profile)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ShaderCompiler"/> class.
    /// </summary>
    virtual ~ShaderCompiler() = default;

public:
    /// <summary>
    /// Gets shader profile supported by this compiler.
    /// </summary>
    /// <returns>The shader profile.</returns>
    FORCE_INLINE ShaderProfile GetProfile() const
    {
        return _profile;
    }

    /// <summary>
    /// Performs the shader compilation.
    /// </summary>
    /// <param name="context">The compilation context.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool Compile(ShaderCompilationContext* context);

    /// <summary>
    /// Gets the included file source code. Handles system includes and absolute includes. Method is thread-safe.
    /// </summary>
    /// <param name="context">The compilation context.</param>
    /// <param name="sourceFile">The source file that is being compiled.</param>
    /// <param name="includedFile">The included file name (absolute or relative).</param>
    /// <param name="source">The output source code of the file (null-terminated), null if failed to load.</param>
    /// <param name="sourceLength">The output source code length of the file (characters count), 0 if failed to load.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool GetIncludedFileSource(ShaderCompilationContext* context, const char* sourceFile, const char* includedFile, const char*& source, int32& sourceLength);

    /// <summary>
    /// Clears the cache used by the shader includes.
    /// </summary>
    static void DisposeIncludedFilesCache();

protected:
    // Input elements read from reflection after shader compilation. Rough approx or attributes without exact format nor bind slot (only semantics and value dimensions match).
    struct AdditionalDataVS
    {
        GPUVertexLayout::Elements Inputs;
    };

    typedef bool (*WritePermutationData)(ShaderCompilationContext*, ShaderFunctionMeta&, int32, const Array<ShaderMacro>&, void*);

    virtual bool CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite = nullptr) = 0;

    bool CompileShaders();

    virtual bool OnCompileBegin();
    virtual bool OnCompileEnd();

    static bool WriteShaderFunctionBegin(ShaderCompilationContext* context, ShaderFunctionMeta& meta);
    static bool WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* header, int32 headerSize, const void* cache, int32 cacheSize);
    static bool WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* cache, int32 cacheSize);
    static bool WriteShaderFunctionEnd(ShaderCompilationContext* context, ShaderFunctionMeta& meta);
    static bool WriteCustomDataVS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const Array<ShaderMacro>& macros, void* additionalData);
    static bool WriteCustomDataHS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const Array<ShaderMacro>& macros, void* additionalData);
    void GetDefineForFunction(ShaderFunctionMeta& meta, Array<ShaderMacro>& macros);

    static VertexElement::Types ParseVertexElementType(StringAnsiView semantic, uint32 index = 0);
};

#endif
