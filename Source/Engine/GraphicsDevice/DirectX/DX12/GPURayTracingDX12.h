// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/GPUAccelerationStructure.h"
#include "Engine/Graphics/GPUResource.h"
#include "GPUDeviceDX12.h"
#include "IShaderResourceDX12.h"
#include "DescriptorHeapDX12.h"
#include "../IncludeDirectXHeaders.h"

class GPUContextDX12;

#if defined(__ID3D12Device5_FWD_DEFINED__) && defined(__ID3D12GraphicsCommandList4_FWD_DEFINED__)
#define GPU_DX12_ALLOW_RAYTRACING 1
#else
#define GPU_DX12_ALLOW_RAYTRACING 0
#endif

#if GPU_DX12_ALLOW_RAYTRACING

/// <summary>
/// The top-level acceleration structure shader resource view for DirectX 12. Bound to a RaytracingAccelerationStructure shader slot.
/// </summary>
class GPUAccelerationStructureViewDX12 : public GPUResourceView, public IShaderResourceDX12
{
private:
    GPUDeviceDX12* _device = nullptr;
    DescriptorHeapWithSlotsDX12::Slot _srv;

public:
    GPUAccelerationStructureViewDX12();

    void Init(GPUDeviceDX12* device, GPUResource* parent, D3D12_GPU_VIRTUAL_ADDRESS tlasLocation);
    void Release();

public:
    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)(IShaderResourceDX12*)this;
    }

    // [IShaderResourceDX12]
    bool IsDepthStencilResource() const override
    {
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE SRV() const override
    {
        return _srv.CPU();
    }
    D3D12_CPU_DESCRIPTOR_HANDLE UAV() const override
    {
        return {};
    }
    ResourceOwnerDX12* GetResourceOwner() const override
    {
        // Acceleration structures stay permanently in D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        // so they must never be transitioned by the SRV flush path (which skips RTAS-dimension views).
        return nullptr;
    }
};

/// <summary>
/// DirectX 12 hardware ray tracing acceleration structure (bottom-level + single-instance top-level).
/// </summary>
class GPUAccelerationStructureDX12 : public GPUAccelerationStructure
{
private:
    GPUDeviceDX12* _device;
    GPUAccelerationStructureGeometry _geometry;
    bool _built = false;

    ID3D12Resource* _blas = nullptr;
    D3D12MA::Allocation* _blasAllocation = nullptr;
    ID3D12Resource* _tlas = nullptr;
    D3D12MA::Allocation* _tlasAllocation = nullptr;
    GPUAccelerationStructureViewDX12 _view;

    // Shader-readable copies of the source mesh geometry (the engine's mesh VB/IB deny shader resource access, which DXR build inputs require)
    GPUBuffer* _vbGeom = nullptr;
    GPUBuffer* _ibGeom = nullptr;

public:
    explicit GPUAccelerationStructureDX12(GPUDeviceDX12* device)
        : _device(device)
    {
    }

    ~GPUAccelerationStructureDX12()
    {
        ReleaseGPUResources();
    }

    // Records the bottom-level and top-level acceleration structure build commands.
    void Build(GPUContextDX12* context);

public:
    // [GPUAccelerationStructure]
    void SetGeometry(const GPUAccelerationStructureGeometry& geometry) override
    {
        _geometry = geometry;
        _built = false;
    }
    bool IsBuilt() const override
    {
        return _built;
    }
    GPUResourceView* GetView() const override
    {
        return _built ? (GPUResourceView*)&_view : nullptr;
    }
    void DeleteObjectNow() override;

private:
    ID3D12Resource* CreateBuffer(uint64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags, D3D12MA::Allocation** allocation);
    void ReleaseGPUResources();
};

#endif

#endif
