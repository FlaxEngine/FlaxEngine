// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Enums.h"

#if GRAPHICS_API_OPENGL

#include "Engine/Graphics/Textures/GPUTextureDescription.h"
#include "IncludeOpenGLHeaders.h"

#if GPU_ENABLE_ASSERTION

// OpenGL results validation
#define VALIDATE_OPENGL_RESULT() RenderToolsOGL::CheckError(__FILE__, __LINE__)

#else

#define VALIDATE_OPENGL_RESULT(x)

#endif

/// <summary>
/// Set of utilities for rendering on OpenGL platform.
/// </summary>
namespace RenderToolsOGL
{
	inline GLenum ComparisonFuncToOGL(ComparisonFunc func)
	{
		switch (func)
		{
		case ComparisonFunc::Never: return GL_NEVER;
		case ComparisonFunc::Less: return GL_LESS;
		case ComparisonFunc::Equal: return GL_EQUAL;
		case ComparisonFunc::LessEqual: return GL_LEQUAL;
		case ComparisonFunc::Grather: return GL_GREATER;
		case ComparisonFunc::NotEqual: return GL_NOTEQUAL;
		case ComparisonFunc::GratherEqual: return GL_GEQUAL;
		case ComparisonFunc::Always: return GL_ALWAYS;
		default: CRASH; return GL_ALWAYS;
		}
	}

	const Char* GetErrorString(GLenum errorCode);

	void CheckError(const char* file, uint32 line);

	void ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam);
};

#endif
