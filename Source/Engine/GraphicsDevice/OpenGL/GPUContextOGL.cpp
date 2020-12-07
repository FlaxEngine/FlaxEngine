// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GPUContextOGL.h"

#if GRAPHICS_API_OPENGL

#include "Engine/Core/Math/Viewport.h"
#include "Engine/Profiler/RenderStats.h"
#include "Engine/Threading/Threading.h"
#include "PipelineStateOGL.h"
#include "BufferOGL.h"
#include "TextureOGL.h"
#include "GPUTextureViewOGL.h"
#include "GPUDeviceOGL.h"
#include "GPULimitsOGL.h"
#include "IShaderResourceOGL.h"
#include "Shaders/GPUConstantBufferOGL.h"
#include "Shaders/GPUShaderProgramOGL.h"
#include "feature/PixelFormatExtensions.h"

GPUContextOGL::GPUContextOGL(GPUDeviceOGL* device)
	: GPUContext(device)
	, _device(device)
	, _omDirtyFlag(false)
	, _rtCount(0)
	, _rtDepth(nullptr)
	, _uaOutput(nullptr)
	, _srDirtyFlag(false)
	, _uaDirtyFlag(false)
	, _cbDirtyFlag(false)
	, _activeTextureUnit(-1)
{
}

GPUContextOGL::~GPUContextOGL()
{
}

void GPUContextOGL::FrameBegin()
{
	// Base
	GPUContext::FrameBegin();

	// Setup
	_omDirtyFlag = false;
	_uaDirtyFlag = false;
	_srDirtyFlag = false;
	_cbDirtyFlag = false;
	_rtCount = 0;
	_currentState = nullptr;
	_rtDepth = nullptr;
	_uaOutput = nullptr;
	Platform::MemoryClear(_rtHandles, sizeof(_rtHandles));
	Platform::MemoryClear(_srHandles, sizeof(_srHandles));
	Platform::MemoryClear(_uaHandles, sizeof(_uaHandles));
	Platform::MemoryClear(_cbHandles, sizeof(_cbHandles));
	Platform::MemoryClear(_vbHandles, sizeof(_vbHandles));
	Platform::MemoryClear(_vbStrides, sizeof(_vbStrides));
	_ibHandle = nullptr;

	// TODO: dont setup this state every frame
	m_DepthEnableState = true;
	glEnable(GL_DEPTH_TEST);
	m_DepthWritesEnableState = true;
	m_DepthCmpFunc = ComparisonFunc::Less;
	glDepthFunc(RenderToolsOGL::ComparisonFuncToOGL(m_DepthCmpFunc));
	glDepthMask(1);
	Writeframe = false;
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	CullMode = ::CullMode::TwoSided;
	glDisable(GL_CULL_FACE);
	DepthClampEnable = true;
	glEnable(GL_DEPTH_CLAMP);
	BlendMode = BlendingMode::Opaque;
	glDisable(GL_BLEND);
}

void GPUContextOGL::Clear(GPUTextureView* rt, const Color& color)
{
	auto rtOGL = reinterpret_cast<GPUTextureViewOGL*>(rt);

	int32 rtIndex = -1;
	for (int32 i = 0; i < _rtCount; i++)
	{
		if (_rtHandles[i] == rtOGL)
		{
			rtIndex = i;
			break;
		}
	}
	if (rtIndex == -1)
	{
		// Check if not render targets is binded
		if (_rtCount == 0)
		{
			// Bind target just for clear
			SetRenderTarget(rtOGL);
			flushOM();
			glClearBufferfv(GL_COLOR, 0, color.Raw);
			VALIDATE_OPENGL_RESULT();
			SetRenderTarget();
			return;
		}

		LOG(Fatal, "Failed to clear the render target. On OpenGL, it must be binded to the pipeline first.");
	}

	flushOM();

	glClearBufferfv(GL_COLOR, rtIndex, color.Raw);
	VALIDATE_OPENGL_RESULT();
}

void GPUContextOGL::ClearDepth(GPUTextureView* depthBuffer, float depthValue)
{
	auto depthBufferOGL = static_cast<GPUTextureViewOGL*>(depthBuffer);
	if (depthBufferOGL != _rtDepth)
	{
		LOG(Fatal, "Failed to clear the depth buffer. On OpenGL, it must be binded to the pipeline first.");
	}

	glClearDepthf(depthValue);
	RENDER_STAT_DISPATCH_CALL();

	glDepthMask(1);
	VALIDATE_OPENGL_RESULT();

	glClear(GL_DEPTH_BUFFER_BIT);
	VALIDATE_OPENGL_RESULT();
}

void GPUContextOGL::ClearUA(Buffer* buf, const Vector4& value)
{
	ASSERT(buf != nullptr && buf->IsUnorderedAccess());

	auto bufOGL = reinterpret_cast<BufferOGL*>(buf);

	MISSING_CODE("GPUContextOGL::ClearUA");
	/*GL.BindBuffer(bufOGL->BufferTarget, bufOGL->BufferId);
	GL.ClearBufferData(bufOGL->BufferTarget, buffer.TextureInternalFormat, buffer.TextureFormat, All.UnsignedInt8888, value);
	GL.BindBuffer(buffer.BufferTarget, 0);*/
}

void GPUContextOGL::ResetRenderTarget()
{
	if (_rtCount > 0 || _uaOutput || _rtDepth)
	{
		_omDirtyFlag = true;
		_rtCount = 0;
		_rtDepth = nullptr;
		_uaOutput = nullptr;

		Platform::MemoryClear(_rtHandles, sizeof(_rtHandles));

		flushOM();
	}
}

void GPUContextOGL::SetRenderTarget(GPUTextureView* rt)
{
	auto rtOGL = reinterpret_cast<GPUTextureViewOGL*>(rt);

	GPUTextureViewOGL* rtv = rtOGL;
	int32 newRtCount = rtv ? 1 : 0;

	if (_rtCount != newRtCount || _rtHandles[0] != rtv || _rtDepth != nullptr || _uaOutput)
	{
		_omDirtyFlag = true;
		_rtCount = newRtCount;
		_rtDepth = nullptr;
		_rtHandles[0] = rtv;
		_uaOutput = nullptr;
	}
}

void GPUContextOGL::SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt)
{
	auto rtOGL = reinterpret_cast<GPUTextureViewOGL*>(rt);
	auto depthBufferOGL = static_cast<GPUTextureViewOGL*>(depthBuffer);

	GPUTextureViewOGL* rtv = rtOGL;
	GPUTextureViewOGL* dsv = depthBufferOGL && depthBufferOGL->GetTexture()->IsDepthStencil() ? depthBufferOGL : nullptr;
	int32 newRtCount = rtv ? 1 : 0;

	if (_rtCount != newRtCount || _rtHandles[0] != rtv || _rtDepth != dsv || _uaOutput)
	{
		_omDirtyFlag = true;
		_rtCount = newRtCount;
		_rtDepth = dsv;
		_rtHandles[0] = rtv;
		_uaOutput = nullptr;
	}
}

void GPUContextOGL::SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts)
{
	ASSERT(Math::IsInRange(rtsCount, 1, GPU_MAX_RT_BINDED));

	auto depthBufferOGL = static_cast<GPUTextureViewOGL*>(depthBuffer);
	GPUTextureViewOGL* dsv = depthBufferOGL && depthBufferOGL->GetTexture()->IsDepthStencil() ? depthBufferOGL : nullptr;

	GPUTextureViewOGL* rtvs[GPU_MAX_RT_BINDED];
	for (int32 i = 0; i < rtsCount; i++)
	{
		auto rtOGL = reinterpret_cast<GPUTextureViewOGL*>(rts[i]);
		rtvs[i] = rtOGL;
	}
	int32 rtvsSize = sizeof(GPUTextureViewOGL*) * rtsCount;

	if (_rtCount != rtsCount || _rtDepth != dsv || _uaOutput || Platform::MemoryCompare(_rtHandles, rtvs, rtvsSize) != 0)
	{
		_omDirtyFlag = true;
		_rtCount = rtsCount;
		_rtDepth = dsv;
		_uaOutput = nullptr;
		Platform::MemoryCopy(_rtHandles, rtvs, rtvsSize);
	}
}

void GPUContextOGL::SetRenderTarget(GPUTextureView* rt, Buffer* uaOutput)
{
	auto rtOGL = reinterpret_cast<GPUTextureViewOGL*>(rt);
	auto uaOutputOGL = reinterpret_cast<BufferOGL*>(uaOutput);

	MISSING_CODE("GPUContextOGL::SetRenderTarget with UAV");
	/*GPUTextureViewOGL* rtv = rtOGL ? rtOGL : nullptr;
	GPUTextureViewOGL* uav = uaOutputOGL ? uaOutputOGL->GetUAV() : nullptr;
	int32 newRtCount = rtv ? 1 : 0;

	if (_rtCount != newRtCount || _rtHandles[0] != rtv || _rtDepth != nullptr || _uaOutput != uav)
	{
		_omDirtyFlag = true;
		_rtCount = newRtCount;
		_rtDepth = nullptr;
		_rtHandles[0] = rtv;
		_uaOutput = uav;
	}*/
}

void GPUContextOGL::ResetSR()
{
	// TODO: check if need to set dirty flag always to true?
	_srDirtyFlag = true;
	Platform::MemoryClear(_srHandles, sizeof(_srHandles));
}

void GPUContextOGL::ResetUA()
{
	// TODO: check if need to set dirty flag always to true?
	_uaDirtyFlag = true;
	Platform::MemoryClear(_uaHandles, sizeof(_uaHandles));
}

void GPUContextOGL::ResetCB()
{
	// TODO: check if need to set dirty flag always to true?
	_cbDirtyFlag = true;
	Platform::MemoryClear(_cbHandles, sizeof(_cbHandles));
}

void GPUContextOGL::BindCB(int32 slot, ConstantBuffer* cb)
{
	ASSERT(slot >= 0 && slot < GPU_MAX_CB_BINDED);

	auto cbOGL = static_cast<GPUConstantBufferOGL*>(cb);

	if (_cbHandles[slot] != cbOGL)
	{
		_cbDirtyFlag = true;
		_cbHandles[slot] = cbOGL;
	}
}

void GPUContextOGL::BindSR(int32 slot, GPUTextureView* rt)
{
	ASSERT(slot >= 0 && slot < GPU_MAX_SR_BINDED);
	
	auto rtOGL = reinterpret_cast<GPUTextureViewOGL*>(rt);

	if (_srHandles[slot] != rtOGL)
	{
		_srDirtyFlag = true;
		_srHandles[slot] = rtOGL;
	}
}

void GPUContextOGL::BindSR(int32 slot, Buffer* buf)
{
	ASSERT(slot >= 0 && slot < GPU_MAX_SR_BINDED);
	ASSERT(buf == nullptr || buf->IsShaderResource());

	auto bufOGL = reinterpret_cast<BufferOGL*>(buf);

	if (_srHandles[slot] != bufOGL)
	{
		_srDirtyFlag = true;
		_srHandles[slot] = bufOGL;
	}
}

void GPUContextOGL::BindUA(int32 slot, Buffer* buf)
{
	MISSING_CODE("GPUContextOGL::BindUA");
}

void GPUContextOGL::BindUA(int32 slot, RenderTarget* rt)
{
	MISSING_CODE("GPUContextOGL::BindUA");
}

void GPUContextOGL::Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
	ASSERT(shader);

	// Bind compute shader
	glUseProgram((GLuint)shader->GetBufferHandle());
	VALIDATE_OPENGL_RESULT();

	// Flush
	flushCBs();
	flushSRVs();
	flushUAVs();
	flushOM();

	// Dispatch
	glDispatchCompute(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	VALIDATE_OPENGL_RESULT();
	RENDER_STAT_DISPATCH_CALL();
}

void GPUContextOGL::ResolveMultisample(Texture* sourceMultisampleTexture, Texture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format)
{
	MISSING_CODE("GPUContextOGL::ResolveMultisample");
}

GLint getGLDrawMode(PipelineStateOGL* state)
{
	GLint primType;
	switch (state->PrimitiveTopologyType)
	{
	case PrimitiveTopologyType::Point:
		primType = GL_POINTS;
		break;
	case PrimitiveTopologyType::Line:
		primType = GL_LINES;
		break;
	case PrimitiveTopologyType::Triangle:
		primType = GL_TRIANGLES;
		break;
	}

	return primType;
}

void GPUContextOGL::Draw(Buffer** vertexBuffers, int32 vertexBuffersCount, uint32 startVertex, uint32 verticesCount)
{
	ASSERT(_currentState && vertexBuffers && vertexBuffers[0] && vertexBuffersCount > 0 && vertexBuffersCount <= GPU_MAX_VB_BINDED);

	VAOCache::StreamData streams[GPU_MAX_VB_BINDED];
	for (int32 i = 0; i < vertexBuffersCount; i++)
	{
		auto bufferOGL = (BufferOGL*)vertexBuffers[i];
		ASSERT(bufferOGL->BufferId != 0);
		streams[i].Buffer = bufferOGL;
		streams[i].Offset = 0;
		streams[i].Stride = bufferOGL ? bufferOGL->GetStride() : 0;
	}
	GLuint vao = _device->VAOCache.GetVAO(_currentState->VS, nullptr, vertexBuffersCount, streams);
	GLint primType = getGLDrawMode(_currentState);

	// Draw
	onDrawCall();
	glBindVertexArray(vao);
	VALIDATE_OPENGL_RESULT();
	glDrawArrays(primType, startIndex, vertices);
	VALIDATE_OPENGL_RESULT();
	RENDER_STAT_DRAW_CALL(vertices, vertices / 3);
}

void GPUContextOGL::DrawIndexed(Buffer** vertexBuffers, int32 vertexBuffersCount, Buffer* indexBuffer, uint32 indicesCount, int32 startVertex, int32 startIndex)
{
	// TODO: implement VAO binding
	MISSING_CODE("GPUContextOGL::DrawIndexed");
}

void GPUContextOGL::DrawIndexedInstanced(Buffer** vertexBuffers, int32 vertexBuffersCount, Buffer* indexBuffer, uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex)
{
	MISSING_CODE("GPUContextOGL::DrawIndexedInstanced");
}

void GPUContextOGL::DrawInstancedIndirect(Buffer* bufferForArgs, uint32 offsetForArgs)
{
	MISSING_CODE("GPUContextOGL::DrawInstancedIndirect");
}

void GPUContextOGL::SetViewport(int width, int height)
{
	// TODO: cache viewport depth to reduce api calls
	glDepthRangef(0.0, 1.0);
	glViewport(0, 0, width, height);
	VALIDATE_OPENGL_RESULT();
}

void GPUContextOGL::SetViewport(const Viewport& viewport)
{
	// TODO: cache viewport depth to reduce api calls
	glDepthRangef(viewport.MinDepth, viewport.MaxDepth);
	glViewport((GLsizei)viewport.X, (GLsizei)viewport.Y, (GLsizei)viewport.Width, (GLsizei)viewport.Height);
	VALIDATE_OPENGL_RESULT();
}

void GPUContextOGL::SetState(PipelineState* state)
{
	// Check if state will change
	if (_currentState != state)
	{
		// Set new state
		_currentState = (PipelineStateOGL*)state;

		// Invalidate pipeline state
		// TODO: maybe don't flush all or only on draw call?
		_cbDirtyFlag = true;
		_srDirtyFlag = true;
		_uaDirtyFlag = true;

		if (_currentState)
		{
			_currentState->OnBind();

			glUseProgram(0);
			VALIDATE_OPENGL_RESULT();

			glBindProgramPipeline(_currentState->ProgramPipeline);
			VALIDATE_OPENGL_RESULT();

			if (m_DepthEnableState != _currentState->DepthTestEnable)
			{
				m_DepthEnableState = _currentState->DepthTestEnable;
				if (m_DepthEnableState)
				{
					glEnable(GL_DEPTH_TEST);
				}
				else
				{
					glDisable(GL_DEPTH_TEST);
				}
			}

			if (m_DepthWritesEnableState != _currentState->EnableDepthWrite)
			{
				m_DepthWritesEnableState = _currentState->EnableDepthWrite;
				glDepthMask(m_DepthWritesEnableState ? 1 : 0);
			}

			if (m_DepthCmpFunc != _currentState->DepthFunc)
			{
				m_DepthCmpFunc = _currentState->DepthFunc;
				glDepthFunc(RenderToolsOGL::ComparisonFuncToOGL(m_DepthCmpFunc));
			}

			if (Writeframe != _currentState->Wireframe)
			{
				Writeframe = _currentState->Wireframe;
				auto PolygonMode = Writeframe ? GL_LINE : GL_FILL;
				glPolygonMode(GL_FRONT_AND_BACK, PolygonMode);
			}

			if (CullMode != _currentState->CullMode)
			{
				CullMode = _currentState->CullMode;
				if (CullMode == CullMode::TwoSided)
				{
					glDisable(GL_CULL_FACE);
				}
				else
				{
					glEnable(GL_CULL_FACE);
					auto CullFace = CullMode == CullMode::Normal ? GL_BACK : GL_FRONT;
					glCullFace(CullFace);
				}
			}

			if (DepthClampEnable != _currentState->DepthClipEnable)
			{
				DepthClampEnable = _currentState->DepthClipEnable;
				if (DepthClampEnable)
				{
					glEnable(GL_DEPTH_CLAMP);
				}
				else
				{
					glDisable(GL_DEPTH_CLAMP);
				}
			}

			if (BlendMode != _currentState->BlendMode)
			{
				BlendMode = _currentState->BlendMode;
				todo_update_pipeline_state_for_opengl_to_match_new_blend_state
				const auto& desc = GPUDeviceOGL::BlendModes[(int32)BlendMode];
				if (desc.BlendEnable)
				{
					glBlendFuncSeparate(desc.SrcBlend, desc.DestBlend, desc.SrcBlendAlpha, desc.DestBlendAlpha);
					VALIDATE_OPENGL_RESULT();

					glBlendEquationSeparate(desc.BlendOp, desc.BlendOpAlpha);
					VALIDATE_OPENGL_RESULT();
				}
				else
				{
					glDisable(GL_BLEND);
				}
			}
		}
		else
		{
			// TODO: what to do when no state is binded?
		}

		RENDER_STAT_PS_STATE_CHANGE();
	}
}

void GPUContextOGL::ClearState()
{
	ResetRenderTarget();
	ResetSR();
	ResetUA();
	SetState(nullptr);

	FlushState();
}

void GPUContextOGL::FlushState()
{
	// Flush
	flushCBs();
	flushSRVs();
	flushUAVs();
	flushOM();
}

void GPUContextOGL::Flush()
{
	glFinish();
}

void GPUContextOGL::UpdateSubresource(Texture* texture, int32 arrayIndex, int32 mipIndex, void* data, uint32 rowPitch, uint32 slicePitch)
{
	ASSERT(IsInMainThread());
	ASSERT(texture && texture->IsAllocated() && data && !texture->IsMultiSample());

	auto textureOGL = static_cast<TextureOGL*>(texture);
	auto target = textureOGL->Target;
	auto format = textureOGL->FormatGL;
	bool isCompressed = PixelFormatExtensions::IsCompressed(texture->Format());
	int formatSize = PixelFormatExtensions::SizeInBytes(texture->Format());

	SetActiveTextureUnit(0);

	glBindTexture(target, textureOGL->TextureID);
	VALIDATE_OPENGL_RESULT();

	// TODO: maybe don't bind it?
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	int32 mipWidth, mipHeight, mipDepth;
	texture->GetMipSize(mipIndex, mipWidth, mipHeight, mipDepth);

	// Determine the opengl read unpack alignment
	auto expectedRowPitch = mipWidth * formatSize;
	auto packAlignment = 1;
	if ((rowPitch & 1) != 0)
	{
		if (rowPitch == expectedRowPitch)
			packAlignment = 1;
	}
	else if ((rowPitch & 2) != 0)
	{
		auto diff = rowPitch - expectedRowPitch;
		if (diff >= 0 && diff < 2)
			packAlignment = 2;
	}
	else if ((rowPitch & 4) != 0)
	{
		auto diff = rowPitch - expectedRowPitch;
		if (diff >= 0 && diff < 4)
			packAlignment = 4;
	}
	else if ((rowPitch & 8) != 0)
	{
		auto diff = rowPitch - expectedRowPitch;
		if (diff >= 0 && diff < 8)
			packAlignment = 8;
	}
	else if (rowPitch == expectedRowPitch)
	{
		packAlignment = 4;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, packAlignment);
	VALIDATE_OPENGL_RESULT();

	// Just to be clear: OpenGL sucks and is very shitty graphics API

	switch (texture->GetDescription().Dimensions)
	{
	case TextureDimensions::Texture:
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, mipWidth);
		VALIDATE_OPENGL_RESULT();
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

		if (texture->IsArray())
		{
			CRASH;
		}
		else
		{
			ASSERT(arrayIndex == 0);

			if (isCompressed)
			{
				glCompressedTexSubImage2D(target, mipIndex, 0, 0, mipWidth, mipHeight, format, slicePitch, data);
				VALIDATE_OPENGL_RESULT();
			}
			else
			{
				auto formatInfo = _device->GetLimits()->TextureFormats[(int32)texture->Format()];
				glTexSubImage2D(target, mipIndex, 0, 0, mipWidth, mipHeight, formatInfo.Format, formatInfo.Type, data);
				VALIDATE_OPENGL_RESULT();
			}
		}
		break;
	}
	case TextureDimensions::CubeTexture:
	{
		if (texture->IsArray())
		{
			CRASH;
		}
		else
		{
			ASSERT(arrayIndex == 0);
			CRASH;
		}
		break;
	}
	case TextureDimensions::VolumeTexture:
	{
		CRASH;
		break;
	}
	}
}

void GPUContextOGL::CopyTexture(Texture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, Texture* srcResource, uint32 srcSubresource)
{
	MISSING_CODE("GPUContextOGL::CopyTexture");
}

void GPUContextOGL::ResetCounter(Buffer* buffer, uint32 alignedByteOffset)
{
	MISSING_CODE("GPUContextOGL::ResetCounter");
}

void GPUContextOGL::CopyCounter(Buffer* dstBuffer, uint32 dstAlignedByteOffset, Buffer* srcBuffer)
{
	MISSING_CODE("GPUContextOGL::CopyCounter");
}

void GPUContextOGL::CopyResource(GPUResource* dstResource, GPUResource* srcResource)
{
	MISSING_CODE("GPUContextOGL::CopyResource");

	/*glBindBuffer(GL_COPY_READ_BUFFER, mBufferId);
	VALIDATE_OPENGL_RESULT();

	glBindBuffer(GL_COPY_WRITE_BUFFER, dstBuffer.getGLBufferId());
	VALIDATE_OPENGL_RESULT();

	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcOffset, dstOffset, length);
	VALIDATE_OPENGL_RESULT();*/

	/*VERIFY_GL_SCOPE();
	ASSERT(FOpenGL::SupportsCopyBuffer());
	FOpenGLVertexBuffer* SourceBuffer = ResourceCast(SourceBufferRHI);
	FOpenGLVertexBuffer* DestBuffer = ResourceCast(DestBufferRHI);
	ASSERT(SourceBuffer->GetSize() == DestBuffer->GetSize());

	glBindBuffer(GL_COPY_READ_BUFFER, SourceBuffer->Resource);
	glBindBuffer(GL_COPY_WRITE_BUFFER, DestBuffer->Resource);
	FOpenGL::CopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, SourceBuffer->GetSize());
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);*/
}

void GPUContextOGL::CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource)
{
	MISSING_CODE("GPUContextOGL::CopySubresource");
}

void GPUContextOGL::SetActiveTextureUnit(int32 unit)
{
	if (_activeTextureUnit != unit)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		VALIDATE_OPENGL_RESULT();

		_activeTextureUnit = unit;
	}
}

void GPUContextOGL::flushSRVs()
{
	// Check if need to flush shader resources
	if (_srDirtyFlag)
	{
		// Clear flag
		_srDirtyFlag = false;

		// TODO: din't interate over all slots, only the used ones by the given pipeline state (cache it)

		int32 shaderStagesCount = ShaderStage_Count;
		for (int32 slot = 0; slot < GPU_MAX_SR_BINDED; slot++)
		{
			const auto srOGL = _srHandles[slot];

			if (srOGL == nullptr || !_currentState->IsUsingSR(slot))
				continue;

			SetActiveTextureUnit(slot);

			//for (int32 stageIndex = 0; stageIndex < shaderStagesCount; stageIndex++)
			{
				//const auto stage = (ShaderStage)stageIndex;

				srOGL->Bind(slot);

				//glProgramUniform1i(GLProgramObj, it->Location + ArrInd, slot);
// TODO: don't use glUniform1i all the time? maybe bind only on shader init?
//glUniform1i(slot, slot);
				//VALIDATE_OPENGL_RESULT();
			}
		}
	}
}

void GPUContextOGL::flushUAVs()
{
	// Check if need to flush unordered access
	if (_uaDirtyFlag)
	{
		// Clear flag
		_uaDirtyFlag = false;

		if (_currentState == nullptr)
			return;

		// Set shader resources views table
		//_context->CSSetUnorderedAccessViews(0, ARRAY_COUNT(_uaHandles), _uaHandles, nullptr);
	}
}

void GPUContextOGL::flushCBs()
{
	// Check if need to flush constant buffers
	if (_cbDirtyFlag)
	{
		// Clear flag
		_cbDirtyFlag = false;

		if (_currentState == nullptr)
			return;

		for (int32 slot = 0; slot < MAX_CONSTANT_BUFFER_SLOTS; slot++)
		{
			auto cbOGL = _cbHandles[slot];
			GLuint handle = cbOGL ? cbOGL->GetHandle() : 0;

			if (handle == 0)
				continue;

			// Update buffer is need to
			if (cbOGL->IsDirty())
			{
				// Update buffer
				glBindBuffer(GL_UNIFORM_BUFFER, handle);
				VALIDATE_OPENGL_RESULT();
				glBufferSubData(GL_UNIFORM_BUFFER, 0, cbOGL->GetSize(), cbOGL->GetDataToUpload());
				VALIDATE_OPENGL_RESULT();
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
				VALIDATE_OPENGL_RESULT();

				// Register as flushed
				cbOGL->OnUploaded();
			}

			// Bind
			if (_currentState->VS && _currentState->VS->IsUsingCB(slot))
			{
				glUniformBlockBinding(_currentState->VS->GetHandle(), slot, slot);
				VALIDATE_OPENGL_RESULT();
			}
			if (_currentState->GS && _currentState->GS->IsUsingCB(slot))
			{
				glUniformBlockBinding(_currentState->GS->GetHandle(), slot, slot);
				VALIDATE_OPENGL_RESULT();
			}
			if (_currentState->PS && _currentState->PS->IsUsingCB(slot))
			{
				glUniformBlockBinding(_currentState->PS->GetHandle(), slot, slot);
				VALIDATE_OPENGL_RESULT();
			}
			// TODO: bind constant buffer to Compute Shader ???
			glBindBufferBase(GL_UNIFORM_BUFFER, slot, handle);
			VALIDATE_OPENGL_RESULT();
		}
	}
}

void GPUContextOGL::flushOM()
{
	// Check if need to flush output merger state or/and unorder access views
	if (_omDirtyFlag)
	{
#if _DEBUG
		// Validate binded render targets amount
		int32 rtCount = 0;
		for (int i = 0; i < ARRAY_COUNT(_rtHandles) && i < _rtCount; i++)
		{
			if (_rtHandles[i] != nullptr)
				rtCount++;
			else
				break;
		}
		ASSERT(rtCount == _rtCount);
#endif

		// Check if render to window backbuffer
		if (_rtCount == 1 && _rtHandles[0]->IsBackbuffer())
		{
			// On-screen rendering
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			VALIDATE_OPENGL_RESULT();
		}
		// Check if use UAV
		else if (_uaOutput)
		{

		}
		// Check if bind sth
		else if(_rtCount || _rtDepth)
		{
			// Get or create FBO
			GLuint fbo = _device->FBOCache.GetFBO(_rtCount, _rtDepth, _rtHandles);

			// Bind FBO
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
			VALIDATE_OPENGL_RESULT();
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
			VALIDATE_OPENGL_RESULT();
		}
		else
		{
			// Unbind FBO
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			VALIDATE_OPENGL_RESULT();
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			VALIDATE_OPENGL_RESULT();
		}

		// Clear flag
		_omDirtyFlag = false;
	}
}

void GPUContextOGL::onDrawCall()
{
	// Flush
	flushCBs();
	flushSRVs();
	flushUAVs();
	flushOM();
}

#endif
