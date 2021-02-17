// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUContextDX11.h"
#include "GPUShaderDX11.h"
#include "GPUShaderProgramDX11.h"
#include "GPUPipelineStateDX11.h"
#include "GPUTextureDX11.h"
#include "GPUBufferDX11.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Profiler/RenderStats.h"

#define DX11_CLEAR_SR_ON_STAGE_DISABLE 0

#if DX11_CLEAR_SR_ON_STAGE_DISABLE
ID3D11ShaderResourceView* EmptySRHandles[GPU_MAX_SR_BINDED] = {};
#endif

// Ensure to match the indirect commands arguments layout
static_assert(sizeof(GPUDispatchIndirectArgs) == sizeof(uint32) * 3, "Wrong size of GPUDrawIndirectArgs.");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountX) == sizeof(uint32) * 0, "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountX");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountY) == sizeof(uint32) * 1,"Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountY");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountZ) == sizeof(uint32) * 2, "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountZ");
//
static_assert(sizeof(GPUDrawIndirectArgs) == sizeof(D3D11_DRAW_INSTANCED_INDIRECT_ARGS), "Wrong size of GPUDrawIndirectArgs.");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, VerticesCount) == OFFSET_OF(D3D11_DRAW_INSTANCED_INDIRECT_ARGS, VertexCountPerInstance), "Wrong offset for GPUDrawIndirectArgs::VerticesCount");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, InstanceCount) == OFFSET_OF(D3D11_DRAW_INSTANCED_INDIRECT_ARGS, InstanceCount),"Wrong offset for GPUDrawIndirectArgs::InstanceCount");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, StartVertex) == OFFSET_OF(D3D11_DRAW_INSTANCED_INDIRECT_ARGS, StartVertexLocation), "Wrong offset for GPUDrawIndirectArgs::StartVertex");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, StartInstance) == OFFSET_OF(D3D11_DRAW_INSTANCED_INDIRECT_ARGS, StartInstanceLocation), "Wrong offset for GPUDrawIndirectArgs::StartInstance");
//
static_assert(sizeof(GPUDrawIndexedIndirectArgs) == sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS), "Wrong size of GPUDrawIndexedIndirectArgs.");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, IndicesCount) == OFFSET_OF(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS, IndexCountPerInstance), "Wrong offset for GPUDrawIndexedIndirectArgs::IndicesCount");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, InstanceCount) == OFFSET_OF(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS, InstanceCount),"Wrong offset for GPUDrawIndexedIndirectArgs::InstanceCount");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartIndex) == OFFSET_OF(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS, StartIndexLocation), "Wrong offset for GPUDrawIndexedIndirectArgs::StartIndex");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartVertex) == OFFSET_OF(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS, BaseVertexLocation), "Wrong offset for GPUDrawIndexedIndirectArgs::StartVertex");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartInstance) == OFFSET_OF(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS, StartInstanceLocation), "Wrong offset for GPUDrawIndexedIndirectArgs::StartInstance");

GPUContextDX11::GPUContextDX11(GPUDeviceDX11* device, ID3D11DeviceContext* context)
    : GPUContext(device)
    , _device(device)
    , _context(context)
#if GPU_ALLOW_PROFILE_EVENTS
    , _userDefinedAnnotations(nullptr)
#endif
    , _omDirtyFlag(false)
    , _rtCount(0)
    , _rtDepth(nullptr)
    , _uaOutput(nullptr)
    , _srDirtyFlag(false)
    , _uaDirtyFlag(false)
    , _cbDirtyFlag(false)
    , _currentState(nullptr)
{
    ASSERT(_context);
#if GPU_ALLOW_PROFILE_EVENTS
    _context->QueryInterface(IID_PPV_ARGS(&_userDefinedAnnotations));
#endif

    // Only DirectX 11 supports more than 1 UAV
    _maxUASlotsForCS = GPU_MAX_UA_BINDED;
    if (_device->GetRendererType() != RendererType::DirectX11)
        _maxUASlotsForCS = 1;
}

GPUContextDX11::~GPUContextDX11()
{
#if GPU_ALLOW_PROFILE_EVENTS
    SAFE_RELEASE(_userDefinedAnnotations);
#endif
}

void GPUContextDX11::FrameBegin()
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
    Platform::MemoryClear(_vbOffsets, sizeof(_vbOffsets));
    _ibHandle = nullptr;
    CurrentBlendState = nullptr;
    CurrentRasterizerState = nullptr;
    CurrentDepthStencilState = nullptr;
    CurrentVS = nullptr;
    CurrentHS = nullptr;
    CurrentDS = nullptr;
    CurrentGS = nullptr;
    CurrentPS = nullptr;
    CurrentCS = nullptr;
    CurrentPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

    // Bind static samplers
    ID3D11SamplerState* samplers[] =
    {
        _device->_samplerLinearClamp,
        _device->_samplerPointClamp,
        _device->_samplerLinearWrap,
        _device->_samplerPointWrap,
        _device->_samplerShadow,
        _device->_samplerShadowPCF
    };
    _context->VSSetSamplers(0, ARRAY_COUNT(samplers), samplers);
    _context->DSSetSamplers(0, ARRAY_COUNT(samplers), samplers);
    _context->PSSetSamplers(0, ARRAY_COUNT(samplers), samplers);
    _context->CSSetSamplers(0, ARRAY_COUNT(samplers), samplers); // TODO: maybe we don't want to bind those static sampler always?
}

#if GPU_ALLOW_PROFILE_EVENTS

void GPUContextDX11::EventBegin(const Char* name)
{
    if (_userDefinedAnnotations)
        _userDefinedAnnotations->BeginEvent(name);
}

void GPUContextDX11::EventEnd()
{
    if (_userDefinedAnnotations)
        _userDefinedAnnotations->EndEvent();
}

#endif

void* GPUContextDX11::GetNativePtr() const
{
    return (void*)_context;
}

bool GPUContextDX11::IsDepthBufferBinded()
{
    return _rtDepth != nullptr;
}

void GPUContextDX11::Clear(GPUTextureView* rt, const Color& color)
{
    auto rtDX11 = static_cast<GPUTextureViewDX11*>(rt);

    if (rtDX11)
    {
        _context->ClearRenderTargetView(rtDX11->RTV(), color.Raw);
    }
}

void GPUContextDX11::ClearDepth(GPUTextureView* depthBuffer, float depthValue)
{
    auto depthBufferDX11 = static_cast<GPUTextureViewDX11*>(depthBuffer);

    if (depthBufferDX11)
    {
        ASSERT(depthBufferDX11->DSV());
        _context->ClearDepthStencilView(depthBufferDX11->DSV(), D3D11_CLEAR_DEPTH, depthValue, 0xff);
    }
}

void GPUContextDX11::ClearUA(GPUBuffer* buf, const Vector4& value)
{
    ASSERT(buf != nullptr && buf->IsUnorderedAccess());

    auto uav = ((GPUBufferViewDX11*)buf->View())->UAV();

    _context->ClearUnorderedAccessViewFloat(uav, value.Raw);
}

void GPUContextDX11::ResetRenderTarget()
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

void GPUContextDX11::SetRenderTarget(GPUTextureView* rt)
{
    auto rtDX11 = reinterpret_cast<GPUTextureViewDX11*>(rt);

    ID3D11RenderTargetView* rtv = rtDX11 ? rtDX11->RTV() : nullptr;
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

void GPUContextDX11::SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt)
{
    auto rtDX11 = reinterpret_cast<GPUTextureViewDX11*>(rt);
    auto depthBufferDX11 = static_cast<GPUTextureViewDX11*>(depthBuffer);

    ID3D11RenderTargetView* rtv = rtDX11 ? rtDX11->RTV() : nullptr;
    ID3D11DepthStencilView* dsv = depthBufferDX11 ? depthBufferDX11->DSV() : nullptr;
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

void GPUContextDX11::SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts)
{
    ASSERT(Math::IsInRange(rts.Length(), 1, GPU_MAX_RT_BINDED));

    auto depthBufferDX11 = static_cast<GPUTextureViewDX11*>(depthBuffer);
    ID3D11DepthStencilView* dsv = depthBufferDX11 ? depthBufferDX11->DSV() : nullptr;

    ID3D11RenderTargetView* rtvs[GPU_MAX_RT_BINDED];
    for (int32 i = 0; i < rts.Length(); i++)
    {
        auto rtDX11 = reinterpret_cast<GPUTextureViewDX11*>(rts[i]);
        rtvs[i] = rtDX11 ? rtDX11->RTV() : nullptr;
    }
    int32 rtvsSize = sizeof(ID3D11RenderTargetView*) * rts.Length();

    if (_rtCount != rts.Length() || _rtDepth != dsv || _uaOutput || Platform::MemoryCompare(_rtHandles, rtvs, rtvsSize) != 0)
    {
        _omDirtyFlag = true;
        _rtCount = rts.Length();
        _rtDepth = dsv;
        _uaOutput = nullptr;
        Platform::MemoryCopy(_rtHandles, rtvs, rtvsSize);
    }
}

void GPUContextDX11::SetRenderTarget(GPUTextureView* rt, GPUBuffer* uaOutput)
{
    auto rtDX11 = reinterpret_cast<GPUTextureViewDX11*>(rt);
    auto uaOutputDX11 = reinterpret_cast<GPUBufferDX11*>(uaOutput);

    ID3D11RenderTargetView* rtv = rtDX11 ? rtDX11->RTV() : nullptr;
    ID3D11UnorderedAccessView* uav = uaOutputDX11 ? ((GPUBufferViewDX11*)uaOutputDX11->View())->UAV() : nullptr;
    int32 newRtCount = rtv ? 1 : 0;

    if (_rtCount != newRtCount || _rtHandles[0] != rtv || _rtDepth != nullptr || _uaOutput != uav)
    {
        _omDirtyFlag = true;
        _rtCount = newRtCount;
        _rtDepth = nullptr;
        _rtHandles[0] = rtv;
        _uaOutput = uav;
    }
}

void GPUContextDX11::ResetSR()
{
    _srDirtyFlag = false;
    Platform::MemoryClear(_srHandles, sizeof(_srHandles));

    _context->VSSetShaderResources(0, ARRAY_COUNT(_srHandles), _srHandles);
    _context->HSSetShaderResources(0, ARRAY_COUNT(_srHandles), _srHandles);
    _context->DSSetShaderResources(0, ARRAY_COUNT(_srHandles), _srHandles);
    _context->GSSetShaderResources(0, ARRAY_COUNT(_srHandles), _srHandles);
    _context->PSSetShaderResources(0, ARRAY_COUNT(_srHandles), _srHandles);
    _context->CSSetShaderResources(0, ARRAY_COUNT(_srHandles), _srHandles);
}

void GPUContextDX11::ResetUA()
{
    _uaDirtyFlag = false;
    Platform::MemoryClear(_uaHandles, sizeof(_uaHandles));

    _context->CSSetUnorderedAccessViews(0, _maxUASlotsForCS, _uaHandles, nullptr);
}

void GPUContextDX11::ResetCB()
{
    _cbDirtyFlag = false;
    Platform::MemoryClear(_cbHandles, sizeof(_cbHandles));

    _context->VSSetConstantBuffers(0, ARRAY_COUNT(_cbHandles), _cbHandles);
    _context->HSSetConstantBuffers(0, ARRAY_COUNT(_cbHandles), _cbHandles);
    _context->DSSetConstantBuffers(0, ARRAY_COUNT(_cbHandles), _cbHandles);
    _context->GSSetConstantBuffers(0, ARRAY_COUNT(_cbHandles), _cbHandles);
    _context->PSSetConstantBuffers(0, ARRAY_COUNT(_cbHandles), _cbHandles);
    _context->CSSetConstantBuffers(0, ARRAY_COUNT(_cbHandles), _cbHandles);
}

void GPUContextDX11::BindCB(int32 slot, GPUConstantBuffer* cb)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_CB_BINDED);

    ID3D11Buffer* buffer = nullptr;
    if (cb && cb->GetSize() > 0)
    {
        auto cbDX11 = static_cast<GPUConstantBufferDX11*>(cb);
        buffer = cbDX11->GetBuffer();
    }

    if (_cbHandles[slot] != buffer)
    {
        _cbDirtyFlag = true;
        _cbHandles[slot] = buffer;
    }
}

void GPUContextDX11::BindSR(int32 slot, GPUResourceView* view)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_SR_BINDED);

    auto handle = view ? ((IShaderResourceDX11*)view->GetNativePtr())->SRV() : nullptr;

    if (_srHandles[slot] != handle)
    {
        _srDirtyFlag = true;
        _srHandles[slot] = handle;
    }
}

void GPUContextDX11::BindUA(int32 slot, GPUResourceView* view)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_UA_BINDED);

    auto handle = view ? ((IShaderResourceDX11*)view->GetNativePtr())->UAV() : nullptr;

    if (_uaHandles[slot] != handle)
    {
        _uaDirtyFlag = true;
        _uaHandles[slot] = handle;
    }
}

void GPUContextDX11::BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets)
{
    ASSERT(vertexBuffers.Length() >= 0 && vertexBuffers.Length() <= GPU_MAX_VB_BINDED);

    bool vbEdited = false;
    for (int32 i = 0; i < vertexBuffers.Length(); i++)
    {
        const auto vbDX11 = static_cast<GPUBufferDX11*>(vertexBuffers[i]);
        const auto vb = vbDX11 ? vbDX11->GetBuffer() : nullptr;
        vbEdited |= vb != _vbHandles[i];
        _vbHandles[i] = vb;
        const UINT stride = vbDX11 ? vbDX11->GetStride() : 0;
        vbEdited |= stride != _vbStrides[i];
        _vbStrides[i] = stride;
        const UINT offset = vertexBuffersOffsets ? vertexBuffersOffsets[i] : 0;
        vbEdited |= offset != _vbOffsets[i];
        _vbOffsets[i] = offset;
    }
    if (vbEdited)
    {
        _context->IASetVertexBuffers(0, vertexBuffers.Length(), _vbHandles, _vbStrides, _vbOffsets);
    }
}

void GPUContextDX11::BindIB(GPUBuffer* indexBuffer)
{
    const auto ibDX11 = static_cast<GPUBufferDX11*>(indexBuffer);
    if (ibDX11 != _ibHandle)
    {
        _ibHandle = ibDX11;
        _context->IASetIndexBuffer(ibDX11->GetBuffer(), RenderToolsDX::ToDxgiFormat(ibDX11->GetFormat()), 0);
    }
}

void GPUContextDX11::UpdateCB(GPUConstantBuffer* cb, const void* data)
{
    ASSERT(data && cb);
    auto cbDX11 = static_cast<GPUConstantBufferDX11*>(cb);
    const uint32 size = cbDX11->GetSize();
    if (size == 0)
        return;

    _context->UpdateSubresource(cbDX11->GetBuffer(), 0, nullptr, data, size, 1);
}

void GPUContextDX11::Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
    CurrentCS = (GPUShaderProgramCSDX11*)shader;

    // Flush
    flushCBs();
    flushSRVs();
    flushUAVs();
    flushOM();

    // Dispatch
    _context->CSSetShader((ID3D11ComputeShader*)shader->GetBufferHandle(), nullptr, 0);
    _context->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    RENDER_STAT_DISPATCH_CALL();

    CurrentCS = nullptr;
}

void GPUContextDX11::DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    CurrentCS = (GPUShaderProgramCSDX11*)shader;

    auto bufferForArgsDX11 = (GPUBufferDX11*)bufferForArgs;

    // Flush
    flushCBs();
    flushSRVs();
    flushUAVs();
    flushOM();

    // Dispatch
    _context->CSSetShader((ID3D11ComputeShader*)shader->GetBufferHandle(), nullptr, 0);
    _context->DispatchIndirect(bufferForArgsDX11->GetBuffer(), offsetForArgs);
    RENDER_STAT_DISPATCH_CALL();

    CurrentCS = nullptr;
}

void GPUContextDX11::ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format)
{
    ASSERT(sourceMultisampleTexture && sourceMultisampleTexture->IsMultiSample());
    ASSERT(destTexture && !destTexture->IsMultiSample());

    auto dstResourceDX11 = static_cast<GPUTextureDX11*>(destTexture);
    auto srcResourceDX11 = static_cast<GPUTextureDX11*>(sourceMultisampleTexture);

    const auto formatDXGI = RenderToolsDX::ToDxgiFormat(format == PixelFormat::Unknown ? destTexture->Format() : format);
    _context->ResolveSubresource(dstResourceDX11->GetResource(), destSubResource, srcResourceDX11->GetResource(), destSubResource, formatDXGI);
}

void GPUContextDX11::DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance, int32 startVertex)
{
    onDrawCall();
    if (instanceCount > 1)
        _context->DrawInstanced(verticesCount, instanceCount, startVertex, startInstance);
    else
        _context->Draw(verticesCount, startVertex);
    RENDER_STAT_DRAW_CALL(verticesCount * instanceCount, verticesCount * instanceCount / 3);
}

void GPUContextDX11::DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex)
{
    onDrawCall();
    if (instanceCount > 1)
        _context->DrawIndexedInstanced(indicesCount, instanceCount, startIndex, startVertex, startInstance);
    else
        _context->DrawIndexed(indicesCount, startIndex, startVertex);
    RENDER_STAT_DRAW_CALL(0, indicesCount / 3 * instanceCount);
}

void GPUContextDX11::DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(bufferForArgs && bufferForArgs->GetFlags() & GPUBufferFlags::Argument);

    const auto bufferForArgsDX11 = static_cast<GPUBufferDX11*>(bufferForArgs);

    onDrawCall();
    _context->DrawInstancedIndirect(bufferForArgsDX11->GetBuffer(), offsetForArgs);
    RENDER_STAT_DRAW_CALL(0, 0);
}

void GPUContextDX11::DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(bufferForArgs && bufferForArgs->GetFlags() & GPUBufferFlags::Argument);

    const auto bufferForArgsDX11 = static_cast<GPUBufferDX11*>(bufferForArgs);

    onDrawCall();
    _context->DrawIndexedInstancedIndirect(bufferForArgsDX11->GetBuffer(), offsetForArgs);
    RENDER_STAT_DRAW_CALL(0, 0);
}

void GPUContextDX11::SetViewport(const Viewport& viewport)
{
    _context->RSSetViewports(1, (D3D11_VIEWPORT*)&viewport);
}

void GPUContextDX11::SetScissor(const Rectangle& scissorRect)
{
    D3D11_RECT rect;
    rect.left = (LONG)scissorRect.GetLeft();
    rect.right = (LONG)scissorRect.GetRight();
    rect.top = (LONG)scissorRect.GetTop();
    rect.bottom = (LONG)scissorRect.GetBottom();
    _context->RSSetScissorRects(1, &rect);
}

GPUPipelineState* GPUContextDX11::GetState() const
{
    return _currentState;
}

void GPUContextDX11::SetState(GPUPipelineState* state)
{
    if (_currentState != state)
    {
        _currentState = static_cast<GPUPipelineStateDX11*>(state);

        ID3D11BlendState* blendState = nullptr;
        ID3D11RasterizerState* rasterizerState = nullptr;
        ID3D11DepthStencilState* depthStencilState = nullptr;
        GPUShaderProgramVSDX11* vs = nullptr;
        GPUShaderProgramHSDX11* hs = nullptr;
        GPUShaderProgramDSDX11* ds = nullptr;
        GPUShaderProgramGSDX11* gs = nullptr;
        GPUShaderProgramPSDX11* ps = nullptr;
        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

        if (state)
        {
            ASSERT(_currentState->IsValid());

            blendState = _currentState->BlendState;
            rasterizerState = _device->RasterizerStates[_currentState->RasterizerStateIndex];
            depthStencilState = _device->DepthStencilStates[_currentState->DepthStencilStateIndex];

            ASSERT(_currentState->VS != nullptr);
            vs = _currentState->VS;
            hs = _currentState->HS;
            ds = _currentState->DS;
            gs = _currentState->GS;
            ps = _currentState->PS;

            primitiveTopology = _currentState->PrimitiveTopology;
        }

        // Per pipeline stage state caching
        if (CurrentDepthStencilState != depthStencilState)
        {
            CurrentDepthStencilState = depthStencilState;
            _context->OMSetDepthStencilState(depthStencilState, 0);
        }
        if (CurrentRasterizerState != rasterizerState)
        {
            CurrentRasterizerState = rasterizerState;
            _context->RSSetState(rasterizerState);
        }
        if (CurrentBlendState != blendState)
        {
            CurrentBlendState = blendState;
            FLOAT blendFactor[4] = { 1, 1, 1, 1 };
            _context->OMSetBlendState(blendState, blendFactor, D3D11_DEFAULT_SAMPLE_MASK);
        }
        if (CurrentVS != vs)
        {
#if DX11_CLEAR_SR_ON_STAGE_DISABLE
			if (CurrentVS && !vs)
			{
				_context->VSSetShaderResources(0, ARRAY_COUNT(EmptySRHandles), EmptySRHandles);
			}
#endif
            CurrentVS = vs;
            _context->VSSetShader(vs ? vs->GetBufferHandleDX11() : nullptr, nullptr, 0);
            _context->IASetInputLayout(vs ? vs->GetInputLayoutDX11() : nullptr);
        }
        if (CurrentHS != hs)
        {
#if DX11_CLEAR_SR_ON_STAGE_DISABLE
			if (CurrentHS && !hs)
			{
				_context->HSSetShaderResources(0, ARRAY_COUNT(EmptySRHandles), EmptySRHandles);
			}
#endif
            CurrentHS = hs;
            _context->HSSetShader(hs ? hs->GetBufferHandleDX11() : nullptr, nullptr, 0);
        }
        if (CurrentDS != ds)
        {
#if DX11_CLEAR_SR_ON_STAGE_DISABLE
			if (CurrentDS && !ds)
			{
				_context->DSSetShaderResources(0, ARRAY_COUNT(EmptySRHandles), EmptySRHandles);
			}
#endif
            CurrentDS = ds;
            _context->DSSetShader(ds ? ds->GetBufferHandleDX11() : nullptr, nullptr, 0);
        }
        if (CurrentGS != gs)
        {
#if DX11_CLEAR_SR_ON_STAGE_DISABLE
			if (CurrentGS && !gs)
			{
				_context->GSSetShaderResources(0, ARRAY_COUNT(EmptySRHandles), EmptySRHandles);
			}
#endif
            CurrentGS = gs;
            _context->GSSetShader(gs ? gs->GetBufferHandleDX11() : nullptr, nullptr, 0);
        }
        if (CurrentPS != ps)
        {
#if DX11_CLEAR_SR_ON_STAGE_DISABLE
			if (CurrentPS && !ps)
			{
				_context->PSSetShaderResources(0, ARRAY_COUNT(EmptySRHandles), EmptySRHandles);
			}
#endif
            CurrentPS = ps;
            _context->PSSetShader(ps ? ps->GetBufferHandleDX11() : nullptr, nullptr, 0);
        }
        if (CurrentPrimitiveTopology != primitiveTopology)
        {
            CurrentPrimitiveTopology = primitiveTopology;
            _context->IASetPrimitiveTopology(primitiveTopology);
        }

        RENDER_STAT_PS_STATE_CHANGE();
    }
}

void GPUContextDX11::ClearState()
{
    if (!_context)
        return;

    ResetRenderTarget();
    ResetSR();
    ResetUA();
    ResetCB();
    SetState(nullptr);

    FlushState();

    //_context->ClearState();
}

void GPUContextDX11::FlushState()
{
    // Flush
    flushCBs();
    flushSRVs();
    flushUAVs();
    flushOM();
}

void GPUContextDX11::Flush()
{
    if (_context)
        _context->Flush();
}

void GPUContextDX11::UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset)
{
    ASSERT(data);
    ASSERT(buffer && buffer->GetSize() >= size);

    auto bufferDX11 = (GPUBufferDX11*)buffer;

    // Use map/unmap for dynamic buffers
    if (buffer->IsDynamic())
    {
        D3D11_MAPPED_SUBRESOURCE map;
        const HRESULT result = _context->Map(bufferDX11->GetResource(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
        if (FAILED(result))
        {
            LOG_DIRECTX_RESULT(result);
            return;
        }
        Platform::MemoryCopy(map.pData, data, size);
        _context->Unmap(bufferDX11->GetResource(), 0);
    }
    else
    {
        D3D11_BOX box;
        box.left = offset;
        box.right = offset + size;
        box.front = 0;
        box.back = 1;
        box.top = 0;
        box.bottom = 1;
        _context->UpdateSubresource(bufferDX11->GetResource(), 0, &box, data, size, 0);
    }
}

void GPUContextDX11::CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset)
{
    ASSERT(dstBuffer && srcBuffer);

    auto dstBufferDX11 = static_cast<GPUBufferDX11*>(dstBuffer);
    auto srcBufferDX11 = static_cast<GPUBufferDX11*>(srcBuffer);

    D3D11_BOX box;
    box.left = srcOffset;
    box.right = srcOffset + size;
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;
    _context->CopySubresourceRegion(dstBufferDX11->GetResource(), 0, dstOffset, 0, 0, srcBufferDX11->GetResource(), 0, &box);
}

void GPUContextDX11::UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch)
{
    ASSERT(texture && texture->IsAllocated() && data);

    auto textureDX11 = static_cast<GPUTextureDX11*>(texture);

    const int32 subresourceIndex = RenderToolsDX::CalcSubresourceIndex(mipIndex, arrayIndex, texture->MipLevels());
    _context->UpdateSubresource(textureDX11->GetResource(), subresourceIndex, nullptr, data, static_cast<UINT>(rowPitch), slicePitch);

    //D3D11_MAPPED_SUBRESOURCE mapped;
    //_device->GetIM()->Map(_resource, textureMipIndex, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    //Platform::MemoryCopy(mapped.pData, data, size);
    //_device->GetIM()->Unmap(_resource, textureMipIndex);
}

void GPUContextDX11::CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource)
{
    ASSERT(dstResource && srcResource);

    auto dstResourceDX11 = static_cast<GPUTextureDX11*>(dstResource);
    auto srcResourceDX11 = static_cast<GPUTextureDX11*>(srcResource);

    _context->CopySubresourceRegion(dstResourceDX11->GetResource(), dstSubresource, dstX, dstY, dstZ, srcResourceDX11->GetResource(), srcSubresource, nullptr);
}

void GPUContextDX11::ResetCounter(GPUBuffer* buffer)
{
}

void GPUContextDX11::CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer)
{
    ASSERT(dstBuffer && srcBuffer);

    auto dstBufferDX11 = static_cast<GPUBufferDX11*>(dstBuffer);
    auto srvUavView = ((GPUBufferViewDX11*)srcBuffer->View())->UAV();

    _context->CopyStructureCount(dstBufferDX11->GetBuffer(), dstOffset, srvUavView);
}

void GPUContextDX11::CopyResource(GPUResource* dstResource, GPUResource* srcResource)
{
    ASSERT(dstResource && srcResource);

    auto dstResourceDX11 = dynamic_cast<IGPUResourceDX11*>(dstResource);
    auto srcResourceDX11 = dynamic_cast<IGPUResourceDX11*>(srcResource);

    _context->CopyResource(dstResourceDX11->GetResource(), srcResourceDX11->GetResource());
}

void GPUContextDX11::CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource)
{
    ASSERT(dstResource && srcResource);

    auto dstResourceDX11 = dynamic_cast<IGPUResourceDX11*>(dstResource);
    auto srcResourceDX11 = dynamic_cast<IGPUResourceDX11*>(srcResource);

    _context->CopySubresourceRegion(dstResourceDX11->GetResource(), dstSubresource, 0, 0, 0, srcResourceDX11->GetResource(), srcSubresource, nullptr);
}

void GPUContextDX11::flushSRVs()
{
    // Check if need to flush shader resources
    if (_srDirtyFlag)
    {
        // Clear flag
        _srDirtyFlag = false;

        // Flush with the driver
        // TODO: don't bind SRV to all stages and all slots (use mask for bind diff?)
#define FLUSH_STAGE(STAGE) \
		if (Current##STAGE) \
		{ \
			_context->STAGE##SetShaderResources(0, ARRAY_COUNT(_srHandles), _srHandles); \
		}
        FLUSH_STAGE(VS);
        FLUSH_STAGE(HS);
        FLUSH_STAGE(DS);
        FLUSH_STAGE(GS);
        FLUSH_STAGE(PS);
        FLUSH_STAGE(CS);
#undef FLUSH_STAGE
    }
}

void GPUContextDX11::flushUAVs()
{
    // Check if need to flush unordered access
    if (_uaDirtyFlag)
    {
        // Clear flag
        _uaDirtyFlag = false;

        // Flush with the driver
        uint32 initialCounts[GPU_MAX_UA_BINDED] = { 0 };
        _context->CSSetUnorderedAccessViews(0, _maxUASlotsForCS, _uaHandles, initialCounts);
    }
}

void GPUContextDX11::flushCBs()
{
    // Check if need to flush constant buffers
    if (_cbDirtyFlag)
    {
        // Clear flag
        _cbDirtyFlag = false;

        // Flush with the driver
        // TODO: don't bind CBV to all stages and all slots (use mask for bind diff? eg. cache mask from last flush and check if there is a diff + include mask from diff slots?)
#define FLUSH_STAGE(STAGE) \
		if (Current##STAGE) \
		{ \
			_context->STAGE##SetConstantBuffers(0, ARRAY_COUNT(_cbHandles), _cbHandles); \
		}
        FLUSH_STAGE(VS);
        FLUSH_STAGE(HS);
        FLUSH_STAGE(DS);
        FLUSH_STAGE(GS);
        FLUSH_STAGE(PS);
        FLUSH_STAGE(CS);
#undef FLUSH_STAGE
    }
}

void GPUContextDX11::flushOM()
{
    // Check if need to flush output merger state or/and unordered access views
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

        // Check if don't use UAVs and set output merger render targets table
        if (_uaOutput == nullptr)
        {
            _context->OMSetRenderTargets(_rtCount, _rtHandles, _rtDepth);
        }
        else
        {
            // Note: current dx11 content implementation assumes that there must be one or more render targets binded in order to use uav output
            ASSERT(_rtCount > 0);

            uint32 initialCounts[1] = { 0 };
            // TODO: set -1 if buffer had no ResetCounter call since last time
            _context->OMSetRenderTargetsAndUnorderedAccessViews(_rtCount, _rtHandles, _rtDepth, _rtCount, 1, &_uaOutput, initialCounts);
        }

        // Clear flag
        _omDirtyFlag = false;
    }
}

void GPUContextDX11::onDrawCall()
{
    ASSERT(_currentState);

    // Flush
    flushCBs();
    flushSRVs();
    flushUAVs();
    flushOM();
}

#endif
