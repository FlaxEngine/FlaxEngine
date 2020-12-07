// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_OPENGL

#include "Engine/Graphics/Shaders/GPUShader.h"
#include "GPUDeviceOGL.h"
#include "GPUResourceOGL.h"

/// <summary>
/// Shader for OpenGL
/// </summary>
class GPUShaderOGL : public GPUResourceOGL<Shader>
{
public:

	/// <summary>
	/// Initializes a new instance of the <see cref="ShaderOGL"/> class.
	/// </summary>
	/// <param name="device">The device.</param>
	/// <param name="name">The name.</param>
	ShaderOGL(GPUDeviceOGL* device, const String& name);

	/// <summary>
	/// Destructor
	/// </summary>
	~ShaderOGL();

public:

	// [GPUResourceOGL]
	void ReleaseGPU() final override;
	
	// [GPUShader]
	bool Create(MemoryReadStream& stream) override;
};

#endif
