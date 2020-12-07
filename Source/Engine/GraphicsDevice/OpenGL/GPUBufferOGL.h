// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once


#if GRAPHICS_API_OPENGL

#include "Engine/Graphics/GPUBuffer.h"
#include "GPUResourceOGL.h"
#include "IShaderResourceOGL.h"

/// <summary>
/// GPU buffer for OpenGL
/// </summary>
/// <seealso cref="GPUResourceOGL" />
class GPUBufferOGL : public GPUResourceOGL<GPUBuffer>, public IShaderResourceOGL
{
public:

	/// <summary>
	/// Initializes a new instance of the <see cref="GPUBufferOGL"/> class.
	/// </summary>
	/// <param name="device">The graphics device.</param>
	/// <param name="name">The resource name.</param>
	GPUBufferOGL(GPUDeviceOGL* device, const String& name);

	/// <summary>
	/// Finalizes an instance of the <see cref="GPUBufferOGL"/> class.
	/// </summary>
	~GPUBufferOGL();

public:

	GLenum BufferTarget = 0;
	GLuint BufferId = 0;

public:

	// [GPUBuffer]
	bool SetData(const void* data, uint64 size) override;
	bool GetData(BytesContainer& data) override;

	// [IShaderResourceOGL]
	void Bind(int32 slotIndex) override;

protected:

	// [GPUBuffer]
	bool OnInit() override;
	void OnReleaseGPU() override;
};

#endif
