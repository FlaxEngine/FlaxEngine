// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_OPENGL

#include "GPUTimerQueryOGL.h"
#include "RenderToolsOGL.h"

GPUTimerQueryOGL::GPUTimerQueryOGL(GPUDeviceOGL* device)
	: GPUResourceOGL<GPUTimerQuery>(device, String::Empty)
{
	GLuint queries[2];
	glGenQueries(2, queries);
	VALIDATE_OPENGL_RESULT();

	_startQuery = queries[0];
	_endQuery = queries[1];

	// Set non-zero mem usage (fake)
	_memoryUsage = sizeof(queries);
}

GPUTimerQueryOGL::~GPUTimerQueryOGL()
{
	ReleaseGPU();
}

void GPUTimerQueryOGL::ReleaseGPU()
{
	if (_memoryUsage == 0)
		return;

	GLuint queries[2];
	queries[0] = _startQuery;
	queries[1] = _endQuery;

	glDeleteQueries(2, queries);
	VALIDATE_OPENGL_RESULT();

	_memoryUsage = 0;
}

void GPUTimerQueryOGL::Begin()
{
	glQueryCounter(_startQuery, GL_TIMESTAMP);
	VALIDATE_OPENGL_RESULT();

	_endCalled = false;
}

void GPUTimerQueryOGL::End()
{
	if (_endCalled)
		return;

	glQueryCounter(_endQuery, GL_TIMESTAMP);
	VALIDATE_OPENGL_RESULT();

	_endCalled = true;
	_finalized = false;
}

bool GPUTimerQueryOGL::HasResult()
{
	if (!_endCalled)
		return false;

	GLint done = 0;
	glGetQueryObjectiv(_endQuery, GL_QUERY_RESULT_AVAILABLE, &done);
	VALIDATE_OPENGL_RESULT();

	return done == GL_TRUE;
}

float GPUTimerQueryOGL::GetResult()
{
	if (!_finalized)
	{
#if BUILD_DEBUG
		ASSERT(HasResult());
#endif

		GLuint64 timeStart;
		GLuint64 timeEnd;

		glGetQueryObjectui64v(_startQuery, GL_QUERY_RESULT, &timeStart);
		VALIDATE_OPENGL_RESULT();

		glGetQueryObjectui64v(_endQuery, GL_QUERY_RESULT, &timeEnd);
		VALIDATE_OPENGL_RESULT();

		_timeDelta = (timeEnd - timeStart) / 1000000.0f;

		_finalized = true;
	}
	return _timeDelta;
}

#endif
