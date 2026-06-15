// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPURayTracingDX12.h"

#if GPU_DX12_ALLOW_RAYTRACING

#include "GPUContextDX12.h"
#include "GPUBufferDX12.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Graphics/GPUBufferDescription.h"
#include "Engine/Platform/Platform.h"
#include <ThirdParty/D3D12MemoryAllocator/D3D12MemAlloc.h>

GPUAccelerationStructureViewDX12::GPUAccelerationStructureViewDX12()
    : GPUResourceView(SpawnParams(Guid::New(), GPUResourceView::TypeInitializer))
{
    SrvDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
}

void GPUAccelerationStructureViewDX12::Init(GPUDeviceDX12* device, GPUResource* parent, D3D12_GPU_VIRTUAL_ADDRESS tlasLocation)
{
    _device = device;
    _parent = parent;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = tlasLocation;
    // Note: a ray tracing acceleration structure SRV must be created with a null resource (the structure is referenced via its GPU virtual address)
    _srv.CreateSRV(device, nullptr, &srvDesc);
}

void GPUAccelerationStructureViewDX12::Release()
{
    _srv.Release();
}

ID3D12Resource* GPUAccelerationStructureDX12::CreateBuffer(uint64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags, D3D12MA::Allocation** allocation)
{
    if (size == 0)
        return nullptr;
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = flags;
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = heapType;
    ID3D12Resource* resource = nullptr;
    const HRESULT result = _device->Allocator->CreateResource(&allocationDesc, &desc, initialState, nullptr, allocation, IID_PPV_ARGS(&resource));
    if (FAILED(result))
    {
        LOG(Error, "Failed to allocate ray tracing acceleration structure buffer ({0} bytes).", size);
        return nullptr;
    }
    return resource;
}

void GPUAccelerationStructureDX12::Build(GPUContextDX12* context)
{
    auto device5 = _device->GetDevice5();
    auto cmd = context->GetCommandList4();
    if (device5 == nullptr || cmd == nullptr)
    {
        LOG(Warning, "Ray tracing is not available on this DirectX 12 device.");
        return;
    }
    if (_geometry.VertexBuffer == nullptr || _geometry.IndexBuffer == nullptr || _geometry.VertexCount == 0 || _geometry.IndexCount == 0)
    {
        LOG(Warning, "Cannot build acceleration structure without geometry.");
        return;
    }

    // Rebuild fresh
    ReleaseGPUResources();

    // Mesh vertex/index buffers are created without shader-resource access (DENY_SHADER_RESOURCE), which cannot be used
    // as acceleration structure build inputs, so copy the geometry into raw shader-resource buffers first.
    const uint32 vbSize = _geometry.VertexBuffer->GetSize();
    const uint32 ibSize = _geometry.IndexBuffer->GetSize();
    const uint32 ibStride = _geometry.IndexBuffer->GetStride() == 4 ? 4 : 2;
    _vbGeom = _device->CreateBuffer(TEXT("RT.GeometryVB"));
    _ibGeom = _device->CreateBuffer(TEXT("RT.GeometryIB"));
    if (_vbGeom->Init(GPUBufferDescription::Raw(Math::AlignUp<uint32>(vbSize, 4), GPUBufferFlags::ShaderResource)) ||
        _ibGeom->Init(GPUBufferDescription::Raw(Math::AlignUp<uint32>(ibSize, 4), GPUBufferFlags::ShaderResource)))
    {
        LOG(Error, "Failed to create ray tracing geometry buffers.");
        return;
    }
    context->CopyBuffer(_vbGeom, _geometry.VertexBuffer, vbSize, 0, 0);
    context->CopyBuffer(_ibGeom, _geometry.IndexBuffer, ibSize, 0, 0);
    context->SetResourceState((GPUBufferDX12*)_vbGeom, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    context->SetResourceState((GPUBufferDX12*)_ibGeom, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    context->FlushResourceBarriers();

    // Map the engine pixel format of the position data to a ray-tracing-supported vertex format. DXR only
    // accepts a small set of vertex position formats; meshes commonly store positions compressed as
    // R16G16B16A16_Float (8-byte stride) rather than full R32G32B32_Float, so honor the actual format.
    DXGI_FORMAT vertexFormat;
    switch (_geometry.VertexFormat)
    {
    case PixelFormat::R32G32B32_Float:
        vertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        break;
    case PixelFormat::R32G32_Float:
        vertexFormat = DXGI_FORMAT_R32G32_FLOAT;
        break;
    case PixelFormat::R16G16B16A16_Float:
        vertexFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        break;
    case PixelFormat::R16G16_Float:
        vertexFormat = DXGI_FORMAT_R16G16_FLOAT;
        break;
    case PixelFormat::R16G16B16A16_SNorm:
        vertexFormat = DXGI_FORMAT_R16G16B16A16_SNORM;
        break;
    case PixelFormat::R16G16B16A16_UNorm:
        vertexFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
        break;
    default:
        LOG(Warning, "Ray tracing acceleration structure: unsupported vertex position format {0}, falling back to R32G32B32_Float.", (int32)_geometry.VertexFormat);
        vertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        break;
    }

    // Bottom-level acceleration structure (the triangle geometry)
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.IndexFormat = ibStride == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    geometryDesc.Triangles.VertexFormat = vertexFormat;
    geometryDesc.Triangles.IndexCount = _geometry.IndexCount;
    geometryDesc.Triangles.VertexCount = _geometry.VertexCount;
    geometryDesc.Triangles.IndexBuffer = ((GPUBufferDX12*)_ibGeom)->GetLocation() + _geometry.IndexBufferOffset;
    geometryDesc.Triangles.VertexBuffer.StartAddress = ((GPUBufferDX12*)_vbGeom)->GetLocation() + _geometry.VertexBufferOffset;
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = _geometry.VertexStride;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInputs = {};
    blasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    blasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    blasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    blasInputs.NumDescs = 1;
    blasInputs.pGeometryDescs = &geometryDesc;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blasPrebuild = {};
    device5->GetRaytracingAccelerationStructurePrebuildInfo(&blasInputs, &blasPrebuild);

    D3D12MA::Allocation* blasScratchAllocation = nullptr;
    _blas = CreateBuffer(blasPrebuild.ResultDataMaxSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &_blasAllocation);
    ID3D12Resource* blasScratch = CreateBuffer(blasPrebuild.ScratchDataSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &blasScratchAllocation);
    if (_blas == nullptr || blasScratch == nullptr)
        return;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasBuild = {};
    blasBuild.Inputs = blasInputs;
    blasBuild.DestAccelerationStructureData = _blas->GetGPUVirtualAddress();
    blasBuild.ScratchAccelerationStructureData = blasScratch->GetGPUVirtualAddress();
    cmd->BuildRaytracingAccelerationStructure(&blasBuild, 0, nullptr);

    // Barrier so the top-level build sees the completed bottom-level structure
    context->AddUAVBarrier();
    context->FlushResourceBarriers();

    // Top-level acceleration structure with a single instance referencing the bottom-level structure
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    const Matrix& m = _geometry.Transform;
    // Flax matrices use row-vector convention (p' = p * M); DXR uses a 3x4 row-major transform applied as M * p (column-vector), so transpose
    instanceDesc.Transform[0][0] = m.M11; instanceDesc.Transform[0][1] = m.M21; instanceDesc.Transform[0][2] = m.M31; instanceDesc.Transform[0][3] = m.M41;
    instanceDesc.Transform[1][0] = m.M12; instanceDesc.Transform[1][1] = m.M22; instanceDesc.Transform[1][2] = m.M32; instanceDesc.Transform[1][3] = m.M42;
    instanceDesc.Transform[2][0] = m.M13; instanceDesc.Transform[2][1] = m.M23; instanceDesc.Transform[2][2] = m.M33; instanceDesc.Transform[2][3] = m.M43;
    instanceDesc.InstanceID = 0;
    instanceDesc.InstanceMask = 0xFF;
    instanceDesc.InstanceContributionToHitGroupIndex = 0;
    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDesc.AccelerationStructure = _blas->GetGPUVirtualAddress();

    D3D12MA::Allocation* instanceAllocation = nullptr;
    ID3D12Resource* instanceBuffer = CreateBuffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE, &instanceAllocation);
    if (instanceBuffer == nullptr)
        return;
    void* mapped = nullptr;
    instanceBuffer->Map(0, nullptr, &mapped);
    Platform::MemoryCopy(mapped, &instanceDesc, sizeof(instanceDesc));
    instanceBuffer->Unmap(0, nullptr);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
    tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    tlasInputs.NumDescs = 1;
    tlasInputs.InstanceDescs = instanceBuffer->GetGPUVirtualAddress();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuild = {};
    device5->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &tlasPrebuild);

    D3D12MA::Allocation* tlasScratchAllocation = nullptr;
    _tlas = CreateBuffer(tlasPrebuild.ResultDataMaxSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &_tlasAllocation);
    ID3D12Resource* tlasScratch = CreateBuffer(tlasPrebuild.ScratchDataSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &tlasScratchAllocation);
    if (_tlas == nullptr || tlasScratch == nullptr)
        return;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasBuild = {};
    tlasBuild.Inputs = tlasInputs;
    tlasBuild.DestAccelerationStructureData = _tlas->GetGPUVirtualAddress();
    tlasBuild.ScratchAccelerationStructureData = tlasScratch->GetGPUVirtualAddress();
    cmd->BuildRaytracingAccelerationStructure(&tlasBuild, 0, nullptr);

    context->AddUAVBarrier();
    context->FlushResourceBarriers();

    // Create the top-level structure shader resource view for binding
    _view.Init(_device, nullptr, _tlas->GetGPUVirtualAddress());

    // The scratch and instance upload buffers are only needed during the GPU build; release them once frames in flight have finished
    _device->AddResourceToLateRelease((IGraphicsUnknown*)blasScratch, blasScratchAllocation);
    _device->AddResourceToLateRelease((IGraphicsUnknown*)tlasScratch, tlasScratchAllocation);
    _device->AddResourceToLateRelease((IGraphicsUnknown*)instanceBuffer, instanceAllocation);

    _built = true;
}

void GPUAccelerationStructureDX12::ReleaseGPUResources()
{
    _built = false;
    _view.Release();
    if (_blas)
    {
        _device->AddResourceToLateRelease((IGraphicsUnknown*)_blas, _blasAllocation);
        _blas = nullptr;
        _blasAllocation = nullptr;
    }
    if (_tlas)
    {
        _device->AddResourceToLateRelease((IGraphicsUnknown*)_tlas, _tlasAllocation);
        _tlas = nullptr;
        _tlasAllocation = nullptr;
    }
    SAFE_DELETE_GPU_RESOURCE(_vbGeom);
    SAFE_DELETE_GPU_RESOURCE(_ibGeom);
}

void GPUAccelerationStructureDX12::DeleteObjectNow()
{
    ReleaseGPUResources();
    Delete(this);
}

GPUAccelerationStructure* GPUDeviceDX12::CreateAccelerationStructure(const StringView& name)
{
    if (!Limits.HasRayTracing)
        return nullptr;
    return New<GPUAccelerationStructureDX12>(this);
}

void GPUContextDX12::BuildAccelerationStructure(GPUAccelerationStructure* accelerationStructure)
{
    if (accelerationStructure)
        ((GPUAccelerationStructureDX12*)accelerationStructure)->Build(this);
}

#else

#include "GPUDeviceDX12.h"
#include "GPUContextDX12.h"
#include "Engine/Graphics/GPUAccelerationStructure.h"

GPUAccelerationStructure* GPUDeviceDX12::CreateAccelerationStructure(const StringView& name)
{
    return nullptr;
}

void GPUContextDX12::BuildAccelerationStructure(GPUAccelerationStructure* accelerationStructure)
{
}

#endif

#endif
