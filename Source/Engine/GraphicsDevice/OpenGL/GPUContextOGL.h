// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUContext.h"
#include "GPUDeviceOGL.h"
#include "GPUResourceOGL.h"
#include "PipelineStateOGL.h"
#include "RenderToolsOGL.h"

#if GRAPHICS_API_OPENGL

class GPUConstantBufferOGL;
class GPUTextureViewOGL;
class BufferOGL;
class IShaderResourceOGL;

/// <summary>
/// GPU Context for OpenGL
/// </summary>
class GPUContextOGL : public GPUContext
{
private:

	GPUDeviceOGL* _device;

	// Output Merger
	bool _omDirtyFlag;
	int32 _rtCount;
	GPUTextureViewOGL* _rtDepth;
	GPUTextureViewOGL* _rtHandles[GPU_MAX_RT_BINDED];
	GPUTextureViewOGL* _uaOutput;

	// Shader Resources
	bool _srDirtyFlag;
	IShaderResourceOGL* _srHandles[GPU_MAX_SR_BINDED];

	// Unordered Access
	bool _uaDirtyFlag;
	GPUResource* _uaHandles[GPU_MAX_UA_BINDED];

	// Constant Buffers
	bool _cbDirtyFlag;
	GPUConstantBufferOGL* _cbHandles[GPU_MAX_CB_BINDED];

	// Vertex Buffers
	BufferOGL* _ibHandle;
	BufferOGL* _vbHandles[GPU_MAX_VB_BINDED];
	uint32 _vbStrides[GPU_MAX_VB_BINDED];

	// Pipeline State
	PipelineStateOGL* _currentState;
	bool m_DepthEnableState;
	bool m_DepthWritesEnableState;
	ComparisonFunc m_DepthCmpFunc;
	/*bool m_StencilTestEnableState; // TODO: support stencil buffer usage
	uint16 m_StencilReadMask = 0xFFFF;
	uint16 m_StencilWriteMask = 0xFFFF;
	struct StencilOpState
	{
		COMPARISON_FUNCTION Func = COMPARISON_FUNC_UNKNOWN;
		STENCIL_OP StencilFailOp = STENCIL_OP_UNDEFINED;
		STENCIL_OP StencilDepthFailOp = STENCIL_OP_UNDEFINED;
		STENCIL_OP StencilPassOp = STENCIL_OP_UNDEFINED;
		int32 Ref = std::numeric_limits<Int32>::min();
		uint32 Mask = MAX_uint32;
	}m_StencilOpState[2];*/
	bool Writeframe;
	CullMode CullMode;
	bool DepthClampEnable;
	BlendingMode BlendMode;

	// OpenGL state
	int32 _activeTextureUnit;

public:

	/// <summary>
	/// Initializes a new instance of the <see cref="GPUContextOGL"/> class.
	/// </summary>
	/// <param name="device">The graphics device.</param>
	GPUContextOGL(GPUDeviceOGL* device);

	/// <summary>
	/// Finalizes an instance of the <see cref="GPUContextOGL"/> class.
	/// </summary>
	~GPUContextOGL();

private:

	void SetActiveTextureUnit(int32 unit);

	void flushSRVs();
	void flushUAVs();
	void flushCBs();
	void flushOM();
	void onDrawCall();

public:

	// [GPUContext]
	void FrameBegin() override;
	bool IsDepthBufferBinded() override
	{
		return _rtDepth != nullptr;
	}
	void Clear(GPUTextureView* rt, const Color& color) override;
	void ClearDepth(GPUTextureView* depthBuffer, float depthValue) override;
	void ClearUA(Buffer* buf, const Vector4& value) override;
	void ResetRenderTarget() override;
	void SetRenderTarget(GPUTextureView* rt) override;
	void SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt) override;
	void SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts) override;
	void SetRenderTarget(GPUTextureView* rt, Buffer* uaOutput) override;
	void ResetSR() override;
	void ResetUA() override;
	void ResetCB() override;
	void BindCB(int32 slot, ConstantBuffer* cb) override;
	void BindSR(int32 slot, GPUTextureView* rt) override;
	void BindSR(int32 slot, Buffer* buf) override;
	void BindUA(int32 slot, Buffer* buf) override;
	void BindUA(int32 slot, Texture* rt) override;
	void UpdateCB(ConstantBuffer* cb, const void* data) override;
	void UpdateBuffer(Buffer* buffer, const void* data, uint32 size) override;
	void Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override;
	void ResolveMultisample(Texture* sourceMultisampleTexture, Texture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format) override;
	void Draw(Buffer** vertexBuffers, int32 vertexBuffersCount, uint32 startVertex, uint32 verticesCount) override;
	void DrawInstanced(Buffer** vertexBuffers, int32 vertexBuffersCount, uint32 instanceCount, int32 startInstance, uint32 startVertex, uint32 verticesCount) override;
	void DrawIndexed(Buffer** vertexBuffers, int32 vertexBuffersCount, Buffer* indexBuffer, uint32 indicesCount, int32 startVertex, int32 startIndex) override;
	void DrawIndexedInstanced(Buffer** vertexBuffers, int32 vertexBuffersCount, Buffer* indexBuffer, uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex) override;
	void DrawInstancedIndirect(Buffer* bufferForArgs, uint32 offsetForArgs) override;
	void SetViewport(const Viewport& viewport) override;
	PipelineState* GetState() const override
	{
		return _currentState;
	}
	void SetState(PipelineState* state) override;
	void ClearState() override;
	void FlushState() override;
	void Flush() override;
	void UpdateSubresource(Texture* texture, int32 arrayIndex, int32 mipIndex, void* data, uint32 rowPitch, uint32 slicePitch) override;
	void CopyTexture(Texture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, Texture* srcResource, uint32 srcSubresource) override;
	void ResetCounter(Buffer* buffer, uint32 alignedByteOffset) override;
	void CopyCounter(Buffer* dstBuffer, uint32 dstAlignedByteOffset, Buffer* srcBuffer) override;
	void CopyResource(GPUResource* dstResource, GPUResource* srcResource) override;
	void CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource) override;
};

#endif
