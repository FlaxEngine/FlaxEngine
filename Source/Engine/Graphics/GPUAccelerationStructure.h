// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Matrix.h"
#include "Engine/Graphics/PixelFormat.h"

class GPUBuffer;
class GPUResourceView;

/// <summary>
/// Describes a single triangle-mesh instance used to build a hardware ray tracing acceleration structure.
/// Geometry is read directly from existing GPU vertex/index buffers (no CPU copy).
/// </summary>
struct GPUAccelerationStructureGeometry
{
    /// <summary>Vertex buffer holding the triangle positions.</summary>
    GPUBuffer* VertexBuffer = nullptr;
    /// <summary>Byte offset into the vertex buffer where the first position element starts (element offset within the vertex).</summary>
    uint32 VertexBufferOffset = 0;
    /// <summary>Stride (in bytes) between consecutive vertices in the vertex buffer.</summary>
    uint32 VertexStride = 0;
    /// <summary>Format of the position data in the vertex buffer (must be a ray-tracing-supported vertex format, e.g. R32G32B32_Float or R16G16B16A16_Float).</summary>
    PixelFormat VertexFormat = PixelFormat::R32G32B32_Float;
    /// <summary>Number of vertices referenced by the index buffer.</summary>
    uint32 VertexCount = 0;
    /// <summary>Index buffer (16-bit or 32-bit indices, matching the buffer stride).</summary>
    GPUBuffer* IndexBuffer = nullptr;
    /// <summary>Byte offset into the index buffer where the indices start.</summary>
    uint32 IndexBufferOffset = 0;
    /// <summary>Number of indices to use (triangle count * 3).</summary>
    uint32 IndexCount = 0;
    /// <summary>World transform applied to the instance inside the top-level structure (row-vector convention, same as Flax Matrix).</summary>
    Matrix Transform = Matrix::Identity;
};

/// <summary>
/// Hardware ray tracing acceleration structure (bottom-level + single-instance top-level) that can be used
/// in shaders via inline ray queries (HLSL RayQuery). Only available when GPUDevice::Limits.HasRayTracing is true
/// (DirectX 12 and Vulkan backends). Build it with GPUContext::BuildAccelerationStructure and bind the view
/// returned by GetView to a RaytracingAccelerationStructure shader resource slot via GPUContext::BindSR.
/// </summary>
class FLAXENGINE_API GPUAccelerationStructure
{
public:
    virtual ~GPUAccelerationStructure()
    {
    }

    /// <summary>
    /// Sets the geometry used to build this acceleration structure. Must be called before BuildAccelerationStructure.
    /// </summary>
    virtual void SetGeometry(const GPUAccelerationStructureGeometry& geometry) = 0;

    /// <summary>
    /// Returns true if the acceleration structure has been built and is ready to bind to shaders.
    /// </summary>
    virtual bool IsBuilt() const = 0;

    /// <summary>
    /// Gets the shader resource view of the top-level acceleration structure for binding via GPUContext::BindSR.
    /// </summary>
    virtual GPUResourceView* GetView() const = 0;

    /// <summary>
    /// Releases the GPU resources owned by the acceleration structure and deletes the object.
    /// </summary>
    virtual void DeleteObjectNow() = 0;
};
