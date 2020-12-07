// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "RenderToolsOGL.h"

#if GRAPHICS_API_OPENGL

#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Log.h"

const Char* RenderToolsOGL::GetErrorString(GLenum errorCode)
{
	switch (errorCode)
	{
#define OGL_ERROR(name) case name: return TEXT(#name)
		OGL_ERROR(GL_INVALID_ENUM);
		OGL_ERROR(GL_INVALID_VALUE);
		OGL_ERROR(GL_INVALID_OPERATION);
		OGL_ERROR(GL_OUT_OF_MEMORY);
		OGL_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
#undef OGL_ERROR
	}

	return nullptr;
}

void RenderToolsOGL::CheckError(const char* file, uint32 line)
{
	GLenum errorCode = glGetError();
	if (errorCode != GL_NO_ERROR)
	{
		StringBuilder sb;
		sb.AppendFormat(TEXT("OpenGL error at {0}:{1}"), file, line);

		const Char* errorString = GetErrorString(errorCode);
		if (errorString)
		{
			sb.AppendLine().Append(errorString);
		}

#if PLATFORM_WINDOWS
		{
			HGLRC context = wglGetCurrentContext();
			if (!context)
			{
				sb.AppendLine().Append(TEXT("No OpenGL context set!"));
			}
		}
#endif

		/*
		do
		{
			const Char* errorString = GetErrorString(errorCode);
			if (errorString)
			{
				sb.AppendLine().Append(errorString);
			}

			errorCode = glGetError();
		} while (errorCode != GL_NO_ERROR);
		*/

		LOG_STR(Fatal, sb.ToString());
	}
}

void RenderToolsOGL::ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	if (type != GL_DEBUG_TYPE_PERFORMANCE && type != GL_DEBUG_TYPE_OTHER)
	{
		LOG(Fatal, "OpenGL error: {0}", message);
	}
}

#endif
