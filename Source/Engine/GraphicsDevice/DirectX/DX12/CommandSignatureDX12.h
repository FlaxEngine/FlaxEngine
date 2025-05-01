// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "GPUDeviceDX12.h"
#include "../IncludeDirectXHeaders.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"

#if GRAPHICS_API_DIRECTX12

class CommandSignatureDX12;

class IndirectParameterDX12
{
    friend CommandSignatureDX12;

protected:

    D3D12_INDIRECT_ARGUMENT_DESC _parameter;

public:

    IndirectParameterDX12()
    {
        _parameter.Type = (D3D12_INDIRECT_ARGUMENT_TYPE)0xFFFFFFFF;
    }

    void Draw()
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
    }

    void DrawIndexed()
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    }

    void Dispatch()
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    }

    void VertexBufferView(UINT slot)
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
        _parameter.VertexBuffer.Slot = slot;
    }

    void IndexBufferView()
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
    }

    void Constant(UINT rootParameterIndex, UINT destOffsetIn32BitValues, UINT num32BitValuesToSet)
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        _parameter.Constant.RootParameterIndex = rootParameterIndex;
        _parameter.Constant.DestOffsetIn32BitValues = destOffsetIn32BitValues;
        _parameter.Constant.Num32BitValuesToSet = num32BitValuesToSet;
    }

    void ConstantBufferView(UINT rootParameterIndex)
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
        _parameter.ConstantBufferView.RootParameterIndex = rootParameterIndex;
    }

    void ShaderResourceView(UINT rootParameterIndex)
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
        _parameter.ShaderResourceView.RootParameterIndex = rootParameterIndex;
    }

    void UnorderedAccessView(UINT rootParameterIndex)
    {
        _parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
        _parameter.UnorderedAccessView.RootParameterIndex = rootParameterIndex;
    }

    const D3D12_INDIRECT_ARGUMENT_DESC& GetDesc() const
    {
        return _parameter;
    }
};

class CommandSignatureDX12 : public GPUResourceDX12<GPUResource>
{
protected:

    ID3D12CommandSignature* _signature;
    Array<IndirectParameterDX12, FixedAllocation<4>> _parameters;

public:

    CommandSignatureDX12(GPUDeviceDX12* device, int32 numParams)
        : GPUResourceDX12<GPUResource>(device, TEXT("CommandSignatureDX12"))
        , _signature(nullptr)
    {
        _parameters.Resize(numParams);
    }

public:

    FORCE_INLINE IndirectParameterDX12& At(int32 entryIndex)
    {
        return _parameters[entryIndex];
    }

    FORCE_INLINE IndirectParameterDX12& operator[](int32 entryIndex)
    {
        return _parameters[entryIndex];
    }

    FORCE_INLINE const IndirectParameterDX12& operator[](int32 entryIndex) const
    {
        return _parameters[entryIndex];
    }

    FORCE_INLINE ID3D12CommandSignature* GetSignature() const
    {
        return _signature;
    }

    void Finalize(ID3D12RootSignature* rootSignature = nullptr)
    {
        if (_signature != nullptr)
            return;

        UINT byteStride = 0;
        bool requiresRootSignature = false;
        const int32 numParameters = _parameters.Count();

        for (int32 i = 0; i < numParameters; i++)
        {
            switch (_parameters[i].GetDesc().Type)
            {
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
                byteStride += sizeof(D3D12_DRAW_ARGUMENTS);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
                byteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
                byteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
                byteStride += _parameters[i].GetDesc().Constant.Num32BitValuesToSet * 4;
                requiresRootSignature = true;
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
                byteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
                byteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
                byteStride += 8;
                requiresRootSignature = true;
                break;
            }
        }

        D3D12_COMMAND_SIGNATURE_DESC desc;
        desc.ByteStride = byteStride;
        desc.NumArgumentDescs = numParameters;
        desc.pArgumentDescs = (const D3D12_INDIRECT_ARGUMENT_DESC*)_parameters.Get();
        desc.NodeMask = 1;

        ID3D12RootSignature* rootSig = rootSignature;
        if (requiresRootSignature)
        {
            ASSERT(rootSig != nullptr);
        }
        else
        {
            rootSig = nullptr;
        }

        const auto result = _device->GetDevice()->CreateCommandSignature(&desc, rootSig, IID_PPV_ARGS(&_signature));
        LOG_DIRECTX_RESULT(result);

        _signature->SetName(L"CommandSignature");

        _memoryUsage = 100;
    }

public:

    // [GPUResourceDX12]
    GPUResourceType GetResourceType() const override
    {
        return GPUResourceType::Descriptor;
    }

protected:

    // [GPUResourceDX12]
    void OnReleaseGPU() override
    {
        DX_SAFE_RELEASE_CHECK(_signature, 0);
        _parameters.Resize(0);
    }
};

#endif
