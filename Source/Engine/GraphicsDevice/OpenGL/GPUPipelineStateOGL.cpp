// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_OPENGL

#include "GPUPipelineStateOGL.h"
#include "GPUDeviceOGL.h"
#include "GPULimitsOGL.h"
#include "RenderToolsOGL.h"
#include "Shaders/GPUShaderProgramOGL.h"

GPUPipelineStateOGL::GPUPipelineStateOGL(GPUDeviceOGL* device)
	: GPUResourceOGL<PipelineState>(device, StringView::Empty)
{
}

GPUPipelineStateOGL::~GPUPipelineStateOGL()
{
	ReleaseGPU();
}

void GPUPipelineStateOGL::OnBind()
{
	if (!IsCreated)
	{
		ASSERT(ProgramPipeline == 0 && Program == 0);

		glGenProgramPipelines(1, &ProgramPipeline);
		VALIDATE_OPENGL_RESULT();

		_usedSRsMask = 0;

		if (VS != nullptr)
		{
			glUseProgramStages(ProgramPipeline, GL_VERTEX_SHADER_BIT, VS->GetHandle());
			VALIDATE_OPENGL_RESULT();
			_usedSRsMask |= VS->GetSRsMask();
		}

		if (PS != nullptr)
		{
			glUseProgramStages(ProgramPipeline, GL_FRAGMENT_SHADER_BIT, PS->GetHandle());
			VALIDATE_OPENGL_RESULT();
			_usedSRsMask |= PS->GetSRsMask();
		}

		if (GS != nullptr)
		{
			glUseProgramStages(ProgramPipeline, GL_GEOMETRY_SHADER_BIT, GS->GetHandle());
			VALIDATE_OPENGL_RESULT();
			_usedSRsMask |= GS->GetSRsMask();
		}

		IsCreated = true;
	}
}

void GPUPipelineStateOGL::ReleaseGPU()
{
	if (_memoryUsage == 0)
		return;

	if (Program != 0)
	{
		glDeleteProgram(Program);
		Program = 0;
	}
	if (ProgramPipeline != 0)
	{
		glDeleteProgramPipelines(1, &ProgramPipeline);
		ProgramPipeline = 0;
	}

	IsCreated = false;
	_memoryUsage = 0;
}

bool GPUPipelineStateOGL::IsValid() const
{
	return _memoryUsage != 0;
}

bool GPUPipelineStateOGL::Init(const Description& desc)
{
	// Cache state
	_memoryUsage = 1;
	EnableDepthWrite = desc.EnableDepthWrite;
	DepthTestEnable = desc.DepthTestEnable;
	DepthClipEnable = desc.DepthClipEnable;
	DepthFunc = desc.DepthFunc;
	VS = (GPUShaderProgramVSOGL*)desc.VS;
	GS = (GPUShaderProgramGSOGL*)desc.GS;
	PS = (GPUShaderProgramPSOGL*)desc.PS;
	PrimitiveTopologyType = desc.PrimitiveTopologyType;
	Wireframe = desc.Wireframe;
	CullMode = desc.CullMode;
	BlendMode = desc.BlendMode;

	return PipelineState::Create(desc);
}

#endif
