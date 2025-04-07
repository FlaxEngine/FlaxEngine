// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUContext.h"
#include "GPUDeviceDX11.h"
#include "GPUPipelineStateDX11.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX11

class GPUBufferDX11;
class GPUVertexLayoutDX11;

/// <summary>
/// GPU Context for DirectX 11 backend.
/// </summary>
class GPUContextDX11 : public GPUContext
{
private:

    GPUDeviceDX11* _device;
    ID3D11DeviceContext* _context;
#if GPU_ALLOW_PROFILE_EVENTS
    ID3DUserDefinedAnnotation* _userDefinedAnnotations;
#endif
    int32 _maxUASlots;

    // Output Merger
    bool _omDirtyFlag;
    int32 _rtCount;
    ID3D11DepthStencilView* _rtDepth;
    ID3D11RenderTargetView* _rtHandles[GPU_MAX_RT_BINDED];

    // Shader Resources
    uint32 _srMaskDirtyGraphics;
    uint32 _srMaskDirtyCompute;
    ID3D11ShaderResourceView* _srHandles[GPU_MAX_SR_BINDED];

    // Unordered Access
    bool _uaDirtyFlag;
    ID3D11UnorderedAccessView* _uaHandles[GPU_MAX_UA_BINDED];

    // Constant Buffers
    bool _cbDirtyFlag;
    ID3D11Buffer* _cbHandles[GPU_MAX_CB_BINDED];

    // Vertex Buffers
    GPUBufferDX11* _ibHandle;
    ID3D11Buffer* _vbHandles[GPU_MAX_VB_BINDED];
    UINT _vbStrides[GPU_MAX_VB_BINDED];
    UINT _vbOffsets[GPU_MAX_VB_BINDED];
    GPUVertexLayoutDX11* _vertexLayout;
    bool _iaInputLayoutDirtyFlag;

    // Pipeline State
    GPUPipelineStateDX11* _currentState;
    ID3D11BlendState* CurrentBlendState;
    ID3D11RasterizerState* CurrentRasterizerState;
    ID3D11DepthStencilState* CurrentDepthStencilState;
    GPUShaderProgramVSDX11* CurrentVS;
#if GPU_ALLOW_TESSELLATION_SHADERS
    GPUShaderProgramHSDX11* CurrentHS;
    GPUShaderProgramDSDX11* CurrentDS;
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    GPUShaderProgramGSDX11* CurrentGS;
#endif
    GPUShaderProgramPSDX11* CurrentPS;
    GPUShaderProgramCSDX11* CurrentCS;
    D3D11_PRIMITIVE_TOPOLOGY CurrentPrimitiveTopology;
    uint32 CurrentStencilRef;
    Float4 CurrentBlendFactor;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUContextDX11"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="context">The context.</param>
    GPUContextDX11(GPUDeviceDX11* device, ID3D11DeviceContext* context);

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUContextDX11"/> class.
    /// </summary>
    ~GPUContextDX11();

public:

    /// <summary>
    /// Gets DirectX 11 device context used by this context
    /// </summary>
    /// <returns>GPU context</returns>
    FORCE_INLINE ID3D11DeviceContext* GetContext() const
    {
        return _context;
    }

private:

    void flushSRVs();
    void flushUAVs();
    void flushCBs();
    void flushOM();
    void flushIA();
    void onDrawCall();

public:

    // [GPUContext]
    void FrameBegin() override;
#if GPU_ALLOW_PROFILE_EVENTS
    void EventBegin(const Char* name) override;
    void EventEnd() override;
#endif
    void* GetNativePtr() const override;
    bool IsDepthBufferBinded() override;
    void Clear(GPUTextureView* rt, const Color& color) override;
    void ClearDepth(GPUTextureView* depthBuffer, float depthValue, uint8 stencilValue) override;
    void ClearUA(GPUBuffer* buf, const Float4& value) override;
    void ClearUA(GPUBuffer* buf, const uint32 value[4]) override;
    void ClearUA(GPUTexture* texture, const uint32 value[4]) override;
    void ClearUA(GPUTexture* texture, const Float4& value) override;
    void ResetRenderTarget() override;
    void SetRenderTarget(GPUTextureView* rt) override;
    void SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt) override;
    void SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts) override;
    void SetBlendFactor(const Float4& value) override;
    void SetStencilRef(uint32 value) override;
    void ResetSR() override;
    void ResetUA() override;
    void ResetCB() override;
    void BindCB(int32 slot, GPUConstantBuffer* cb) override;
    void BindSR(int32 slot, GPUResourceView* view) override;
    void BindUA(int32 slot, GPUResourceView* view) override;
    void BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets = nullptr, GPUVertexLayout* vertexLayout = nullptr) override;
    void BindIB(GPUBuffer* indexBuffer) override;
    void BindSampler(int32 slot, GPUSampler* sampler) override;
    void UpdateCB(GPUConstantBuffer* cb, const void* data) override;
    void Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override;
    void DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;
    void ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format) override;
    void DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance, int32 startVertex) override;
    void DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex) override;
    void DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;
    void DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;
    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Rectangle& scissorRect) override;
    GPUPipelineState* GetState() const override;
    void SetState(GPUPipelineState* state) override;
    void ClearState() override;
    void FlushState() override;
    void Flush() override;
    void UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset) override;
    void CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset) override;
    void UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch) override;
    void CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource) override;
    void ResetCounter(GPUBuffer* buffer) override;
    void CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer) override;
    void CopyResource(GPUResource* dstResource, GPUResource* srcResource) override;
    void CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource) override;
};

#endif
