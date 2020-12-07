// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUTimerQuery.h"
#include "GPUResourceOGL.h"

#if GRAPHICS_API_OPENGL

/// <summary>
/// GPU timer query object for OpenGL
/// </summary>
class GPUTimerQueryOGL : public GPUResourceOGL<GPUTimerQuery>
{
private:

	bool _finalized = false;
	bool _endCalled = false;
	float _timeDelta = 0.0f;

	GLuint _startQuery;
	GLuint _endQuery;

public:

	/// <summary>
	/// Initializes a new instance of the <see cref="GPUTimerQueryOGL"/> class.
	/// </summary>
	/// <param name="device">The graphics device.</param>
	GPUTimerQueryOGL(GPUDeviceOGL* device);

	/// <summary>
	/// Finalizes an instance of the <see cref="GPUTimerQueryOGL"/> class.
	/// </summary>
	~GPUTimerQueryOGL();

public:

	// [GPUResourceOGL]
	void ReleaseGPU() final override;

	// [GPUTimerQuery]
	void Begin() override;
	void End() override;
	bool HasResult() override;
	float GetResult() override;
};

#endif
