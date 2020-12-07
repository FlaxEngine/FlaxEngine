// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUPipelineState.h"
#include "GPUResourceOGL.h"

#if GRAPHICS_API_OPENGL

class GPUShaderProgramVSOGL;
class GPUShaderProgramGSOGL;
class GPUShaderProgramPSOGL;

/// <summary>
/// Graphics pipeline state object for OpenGL.
/// </summary>
class GPUPipelineStateOGL : public GPUResourceOGL<PipelineState>
{
private:

	int32 _usedSRsMask;

public:

	/// <summary>
	/// Initializes a new instance of the <see cref="GPUPipelineStateOGL"/> class.
	/// </summary>
	/// <param name="device">The graphics device.</param>
	GPUPipelineStateOGL(GPUDeviceOGL* device);

	/// <summary>
	/// Finalizes an instance of the <see cref="GPUPipelineStateOGL"/> class.
	/// </summary>
	~GPUPipelineStateOGL();

public:

	bool EnableDepthWrite;
	bool DepthTestEnable;
	bool DepthClipEnable;
	ComparisonFunc DepthFunc;
	GPUShaderProgramVSOGL* VS;
	GPUShaderProgramGSOGL* GS;
	GPUShaderProgramPSOGL* PS;
	PrimitiveTopologyType PrimitiveTopologyType;
	bool Wireframe;
	CullMode CullMode;
	BlendingMode BlendMode;

	bool IsCreated = false;
	GLuint Program = 0;
	GLuint ProgramPipeline = 0;

public:

	int32 GetSRsMask() const
	{
		return _usedSRsMask;
	}

	bool IsUsingSR(int32 slotIndex) const
	{
		return _usedSRsMask & (1 << slotIndex) != 0;
	}

public:

	void OnBind();

public:

	// [GPUResourceOGL]
	void ReleaseGPU() final override;

	// [GPUPipelineState]
	bool IsValid() const override;
	bool Init(const Description& desc) override;
};

#endif
