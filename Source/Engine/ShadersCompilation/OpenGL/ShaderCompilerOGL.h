// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_OGL_SHADER_COMPILER

#include "Engine/ShadersCompilation/ShaderCompiler.h"
#include "IncludeXShaderCompiler.h"
#include "../IncludeOpenGLHeaders.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include <sstream>

/// <summary>
/// Implementation of shaders compiler for OpenGL and OpenGL ES platforms
/// </summary>
class ShaderCompilerOGL : public ShaderCompiler
{
private:

	ShaderProfile _profile;

	ShaderCompilationContext* _context;
	Array<ShaderMacro> _globalMacros;
	Array<ShaderMacro> _macros;
	Array<ShaderResourceBuffer> _constantBuffers;
	Array<byte> _dataCompressedCache;
	Xsc::ShaderInput _inputDesc;
	Xsc::ShaderOutput _outputDesc;
	std::stringstream _sourceStream;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderCompilerOGL"/> class.
    /// </summary>
    /// <param name="profile">The profile.</param>
    ShaderCompilerOGL(ShaderProfile profile);

private:

	bool ProcessShader(Xsc::Reflection::ReflectionData& data, uint32& cbMask, uint32& srMask, uint32& uaMask)

protected:

	// [ShaderCompiler]
	bool CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite = nullptr) override;
    bool OnCompileBegin() override;
};

#endif
