// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Viewport.h"
#include "PixelFormat.h"
#include "Config.h"

class GPUConstantBuffer;
class GPUShaderProgramCS;
class GPUBuffer;
class GPUPipelineState;
class GPUTexture;
class GPUSampler;
class GPUDevice;
class GPUResource;
class GPUResourceView;
class GPUTextureView;
class GPUBufferView;

// Gets the GPU texture view. Checks if pointer is not null and texture has one or more mip levels loaded.
#define GET_TEXTURE_VIEW_SAFE(t) (t && t->ResidentMipLevels() > 0 ? t->View() : nullptr)

/// <summary>
/// The GPU dispatch indirect command arguments data.
/// </summary>
API_STRUCT() struct GPUDispatchIndirectArgs
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUDispatchIndirectArgs);

    /// <summary>
    /// The X dimension of dispatch size.
    /// </summary>
    API_FIELD() uint32 ThreadGroupCountX;

    /// <summary>
    /// The Y dimension of dispatch size.
    /// </summary>
    API_FIELD() uint32 ThreadGroupCountY;

    /// <summary>
    /// The Z dimension of dispatch size.
    /// </summary>
    API_FIELD() uint32 ThreadGroupCountZ;
};

/// <summary>
/// The GPU draw indirect command arguments data.
/// </summary>
API_STRUCT() struct GPUDrawIndirectArgs
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUDrawIndirectArgs);

    /// <summary>
    /// The number of vertices to draw for each instance.
    /// </summary>
    API_FIELD() uint32 VerticesCount;

    /// <summary>
    /// The number of instances to draw.
    /// </summary>
    API_FIELD() uint32 InstanceCount;

    /// <summary>
    /// An offset added to each vertex index.
    /// </summary>
    API_FIELD() uint32 StartVertex;

    /// <summary>
    /// An offset added to each instance index.
    /// </summary>
    API_FIELD() uint32 StartInstance;
};

/// <summary>
/// The GPU draw indexed indirect command arguments data.
/// </summary>
API_STRUCT() struct GPUDrawIndexedIndirectArgs
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUDrawIndexedIndirectArgs);

    /// <summary>
    /// The number of indices to draw for each instance.
    /// </summary>
    API_FIELD() uint32 IndicesCount;

    /// <summary>
    /// The number of instances to draw.
    /// </summary>
    API_FIELD() uint32 InstanceCount;

    /// <summary>
    /// An offset into the index buffer where drawing should begin.
    /// </summary>
    API_FIELD() uint32 StartIndex;

    /// <summary>
    /// An offset added to each vertex index.
    /// </summary>
    API_FIELD() uint32 StartVertex;

    /// <summary>
    /// An offset added to each instance index.
    /// </summary>
    API_FIELD() uint32 StartInstance;
};

/// <summary>
/// Interface for GPU device context that can record and send graphics commands to the GPU in a sequence.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API GPUContext : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUContext);
private:
    GPUDevice* _device;

protected:
    double _lastRenderTime = -1;
    GPUContext(GPUDevice* device);

#if !BUILD_RELEASE
    enum class InvalidBindPoint { SRV, UAV, DSV, RTV };

    static void LogInvalidResourceUsage(int32 slot, const GPUResourceView* view, InvalidBindPoint bindPoint);
#endif

public:
    /// <summary>
    /// Gets the graphics device.
    /// </summary>
    FORCE_INLINE GPUDevice* GetDevice() const
    {
        return _device;
    }

public:
    /// <summary>
    /// Begins new frame and enters commands collecting mode.
    /// </summary>
    virtual void FrameBegin();

    /// <summary>
    /// Ends the current frame rendering.
    /// </summary>
    virtual void FrameEnd();

public:
#if GPU_ALLOW_PROFILE_EVENTS
    /// <summary>
    /// Begins the profile event.
    /// </summary>
    /// <param name="name">The name.</param>
    virtual void EventBegin(const Char* name)
    {
    }

    /// <summary>
    /// Ends the last profile event.
    /// </summary>
    virtual void EventEnd()
    {
    }
#endif

public:
    /// <summary>
    /// Gets the native pointer to the underlying graphics device context. It's a low-level platform-specific handle.
    /// </summary>
    API_PROPERTY() virtual void* GetNativePtr() const = 0;

    /// <summary>
    /// Determines whether depth buffer is binded to the pipeline.
    /// </summary>
    /// <returns><c>true</c> if  depth buffer is binded; otherwise, <c>false</c>.</returns>
    virtual bool IsDepthBufferBinded() = 0;

public:
    /// <summary>
    /// Clears texture surface with a color. Supports volumetric textures and texture arrays (including cube textures).
    /// </summary>
    /// <param name="rt">The target surface.</param>
    /// <param name="color">The clear color.</param>
    API_FUNCTION() virtual void Clear(GPUTextureView* rt, const Color& color) = 0;

    /// <summary>
    /// Clears depth buffer.
    /// </summary>
    /// <param name="depthBuffer">The depth buffer to clear.</param>
    /// <param name="depthValue">The clear depth value.</param>
    /// <param name="stencilValue">The clear stencil value.</param>
    API_FUNCTION() virtual void ClearDepth(GPUTextureView* depthBuffer, float depthValue = 1.0f, uint8 stencilValue = 0) = 0;

    /// <summary>
    /// Clears an unordered access buffer with a float value.
    /// </summary>
    /// <param name="buf">The buffer to clear.</param>
    /// <param name="value">The clear value.</param>
    API_FUNCTION() virtual void ClearUA(GPUBuffer* buf, const Float4& value) = 0;

    /// <summary>
    /// Clears an unordered access buffer with an unsigned value.
    /// </summary>
    /// <param name="buf">The buffer to clear.</param>
    /// <param name="value">The clear value.</param>
    virtual void ClearUA(GPUBuffer* buf, const uint32 value[4]) = 0;

    /// <summary>
    /// Clears an unordered access texture with an unsigned value.
    /// </summary>
    /// <param name="texture">The texture to clear.</param>
    /// <param name="value">The clear value.</param>
    virtual void ClearUA(GPUTexture* texture, const uint32 value[4]) = 0;

    /// <summary>
    /// Clears an unordered access texture with a float value.
    /// </summary>
    /// <param name="texture">The texture to clear.</param>
    /// <param name="value">The clear value.</param>
    virtual void ClearUA(GPUTexture* texture, const Float4& value) = 0;

public:
    /// <summary>
    /// Updates the buffer data.
    /// </summary>
    /// <param name="buffer">The destination buffer to write to.</param>
    /// <param name="data">The pointer to the data.</param>
    /// <param name="size">The data size (in bytes) to write.</param>
    /// <param name="offset">The offset (in bytes) from the buffer start to copy data to.</param>
    API_FUNCTION() virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset = 0) = 0;

    /// <summary>
    /// Copies the buffer data.
    /// </summary>
    /// <param name="dstBuffer">The destination buffer to write to.</param>
    /// <param name="srcBuffer">The source buffer to read from.</param>
    /// <param name="size">The size of data to copy (in bytes).</param>
    /// <param name="dstOffset">The offset (in bytes) from the destination buffer start to copy data to.</param>
    /// <param name="srcOffset">The offset (in bytes) from the source buffer start to copy data from.</param>
    API_FUNCTION() virtual void CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset = 0, uint32 srcOffset = 0) = 0;

    /// <summary>
    /// Updates the texture data.
    /// </summary>
    /// <param name="texture">The destination texture.</param>
    /// <param name="arrayIndex">The destination surface index in the texture array.</param>
    /// <param name="mipIndex">The absolute index of the mip map to update.</param>
    /// <param name="data">The pointer to the data.</param>
    /// <param name="rowPitch">The row pitch (in bytes) of the input data.</param>
    /// <param name="slicePitch">The slice pitch (in bytes) of the input data.</param>
    API_FUNCTION() virtual void UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch) = 0;

    /// <summary>
    /// Copies region of the texture.
    /// </summary>
    /// <param name="dstResource">The destination resource.</param>
    /// <param name="dstSubresource">The destination subresource index.</param>
    /// <param name="dstX">The x-coordinate of the upper left corner of the destination region.</param>
    /// <param name="dstY">The y-coordinate of the upper left corner of the destination region.</param>
    /// <param name="dstZ">The z-coordinate of the upper left corner of the destination region.</param>
    /// <param name="srcResource">The source resource.</param>
    /// <param name="srcSubresource">The source subresource index.</param>
    API_FUNCTION() virtual void CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource) = 0;

    /// <summary>
    /// Resets the counter buffer to zero (hidden by the driver).
    /// </summary>
    /// <param name="buffer">The buffer.</param>
    API_FUNCTION() virtual void ResetCounter(GPUBuffer* buffer) = 0;

    /// <summary>
    /// Copies the counter buffer value.
    /// </summary>
    /// <param name="dstBuffer">The destination buffer.</param>
    /// <param name="dstOffset">The destination aligned byte offset.</param>
    /// <param name="srcBuffer">The source buffer.</param>
    API_FUNCTION() virtual void CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer) = 0;

    /// <summary>
    /// Copies the resource data (whole resource).
    /// </summary>
    /// <param name="dstResource">The destination resource.</param>
    /// <param name="srcResource">The source resource.</param>
    API_FUNCTION() virtual void CopyResource(GPUResource* dstResource, GPUResource* srcResource) = 0;

    /// <summary>
    /// Copies the subresource data.
    /// </summary>
    /// <param name="dstResource">The destination resource.</param>
    /// <param name="dstSubresource">The destination subresource index.</param>
    /// <param name="srcResource">The source resource.</param>
    /// <param name="srcSubresource">The source subresource index.</param>
    API_FUNCTION() virtual void CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource) = 0;

public:
    /// <summary>
    /// Unbinds all the render targets and flushes the change with the driver (used to prevent driver detection of resource hazards, eg. when down-scaling the texture).
    /// </summary>
    API_FUNCTION() virtual void ResetRenderTarget() = 0;

    /// <summary>
    /// Sets the render target to the output.
    /// </summary>
    /// <param name="rt">The render target.</param>
    API_FUNCTION() virtual void SetRenderTarget(GPUTextureView* rt) = 0;

    /// <summary>
    /// Sets the render target and the depth buffer to the output.
    /// </summary>
    /// <param name="depthBuffer">The depth buffer.</param>
    /// <param name="rt">The render target.</param>
    API_FUNCTION() virtual void SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt) = 0;

    /// <summary>
    /// Sets the render targets and the depth buffer to the output.
    /// </summary>
    /// <param name="depthBuffer">The depth buffer (can be null).</param>
    /// <param name="rts">The array with render targets to bind.</param>
    API_FUNCTION() virtual void SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts) = 0;

    /// <summary>
    /// Sets the blend factor that modulate values for a pixel shader, render target, or both.
    /// </summary>
    /// <param name="value">Blend factors, one for each RGBA component.</param>
    API_FUNCTION() virtual void SetBlendFactor(const Float4& value) = 0;

    /// <summary>
    /// Sets the reference value for depth stencil tests.
    /// </summary>
    /// <param name="value">Reference value to perform against when doing a depth-stencil test.</param>
    API_FUNCTION() virtual void SetStencilRef(uint32 value) = 0;

public:
    /// <summary>
    /// Unbinds all shader resource slots and flushes the change with the driver (used to prevent driver detection of resource hazards, eg. when down-scaling the texture).
    /// </summary>
    API_FUNCTION() virtual void ResetSR() = 0;

    /// <summary>
    /// Unbinds all unordered access resource slots and flushes the change with the driver (used to prevent driver detection of resource hazards, eg. when down-scaling the texture).
    /// </summary>
    API_FUNCTION() virtual void ResetUA() = 0;

    /// <summary>
    /// Unbinds all constant buffer slots and flushes the change with the driver (used to prevent driver detection of resource hazards, eg. when down-scaling the texture).
    /// </summary>
    API_FUNCTION() virtual void ResetCB() = 0;

    /// <summary>
    /// Unbinds shader resource slot.
    /// </summary>
    /// <param name="slot">The slot index.</param>
    FORCE_INLINE void UnBindSR(int32 slot)
    {
        BindSR(slot, static_cast<GPUResourceView*>(nullptr));
    }

    /// <summary>
    /// Unbinds unordered access resource slot.
    /// </summary>
    /// <param name="slot">The slot index.</param>
    FORCE_INLINE void UnBindUA(int32 slot)
    {
        BindUA(slot, static_cast<GPUResourceView*>(nullptr));
    }

    /// <summary>
    /// Unbinds constant buffer slot.
    /// </summary>
    /// <param name="slot">The slot index.</param>
    FORCE_INLINE void UnBindCB(int32 slot)
    {
        BindCB(slot, nullptr);
    }

    /// <summary>
    /// Binds the texture to the shader resource slot.
    /// </summary>
    /// <param name="slot">The slot index.</param>
    /// <param name="t">The GPU texture.</param>
    API_FUNCTION() void BindSR(int32 slot, GPUTexture* t);

    /// <summary>
    /// Binds the resource view to the shader resource slot (texture view or buffer view).
    /// </summary>
    /// <param name="slot">The slot index.</param>
    /// <param name="view">The resource view.</param>
    API_FUNCTION() virtual void BindSR(int32 slot, GPUResourceView* view) = 0;

    /// <summary>
    /// Binds the resource view to the unordered access slot (texture view or buffer view).
    /// </summary>
    /// <param name="slot">The slot index.</param>
    /// <param name="view">The resource view.</param>
    API_FUNCTION() virtual void BindUA(int32 slot, GPUResourceView* view) = 0;

    /// <summary>
    /// Binds the constant buffer to the slot.
    /// </summary>
    /// <param name="slot">The slot index.</param>
    /// <param name="cb">The constant buffer.</param>
    API_FUNCTION() virtual void BindCB(int32 slot, GPUConstantBuffer* cb) = 0;

    /// <summary>
    /// Binds the vertex buffers to the pipeline.
    /// </summary>
    /// <param name="vertexBuffers">The array of vertex buffers to use.</param>
    /// <param name="vertexBuffersOffsets">The optional array of byte offsets from the vertex buffers begins. Can be used to offset the vertex data when reusing the same buffer allocation for multiple geometry objects.</param>
    API_FUNCTION() virtual void BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets = nullptr) = 0;

    /// <summary>
    /// Binds the index buffer to the pipeline.
    /// </summary>
    /// <param name="indexBuffer">The index buffer.</param>
    API_FUNCTION() virtual void BindIB(GPUBuffer* indexBuffer) = 0;

    /// <summary>
    /// Binds the texture sampler to the pipeline.
    /// </summary>
    /// <param name="slot">The slot index.</param>
    /// <param name="sampler">The sampler.</param>
    API_FUNCTION() virtual void BindSampler(int32 slot, GPUSampler* sampler) = 0;

public:
    /// <summary>
    /// Updates the constant buffer data.
    /// </summary>
    /// <param name="cb">The constant buffer.</param>
    /// <param name="data">The pointer to the data.</param>
    API_FUNCTION() virtual void UpdateCB(GPUConstantBuffer* cb, const void* data) = 0;

public:
    /// <summary>
    /// Executes a command list from a thread group.
    /// </summary>
    /// <param name="shader">The compute shader program to execute.</param>
    /// <param name="threadGroupCountX">The number of groups dispatched in the x direction.</param>
    /// <param name="threadGroupCountY">The number of groups dispatched in the y direction.</param>
    /// <param name="threadGroupCountZ">The number of groups dispatched in the z direction.</param>
    API_FUNCTION() virtual void Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) = 0;

    /// <summary>
    /// Executes a command list from a thread group. Buffer must contain GPUDispatchIndirectArgs.
    /// </summary>
    /// <param name="shader">The compute shader program to execute.</param>
    /// <param name="bufferForArgs">The buffer with drawing arguments.</param>
    /// <param name="offsetForArgs">The aligned byte offset for arguments.</param>
    API_FUNCTION() virtual void DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs) = 0;

    /// <summary>
    /// Resolves the multisampled texture by performing a copy of the resource into a non-multisampled resource.
    /// </summary>
    /// <param name="sourceMultisampleTexture">The source multisampled texture. Must be multisampled.</param>
    /// <param name="destTexture">The destination texture. Must be single-sampled.</param>
    /// <param name="sourceSubResource">The source sub-resource index.</param>
    /// <param name="destSubResource">The destination sub-resource index.</param>
    /// <param name="format">The format. Indicates how the multisampled resource will be resolved to a single-sampled resource.</param>
    API_FUNCTION() virtual void ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource = 0, int32 destSubResource = 0, PixelFormat format = PixelFormat::Unknown) = 0;

    /// <summary>
    /// Draws the fullscreen triangle (using single triangle). Use instance count parameter to render more than one instance of the triangle.
    /// </summary>
    /// <param name="instanceCount">The instance count. Use SV_InstanceID in vertex shader to detect volume slice plane index.</param>
    API_FUNCTION() void DrawFullscreenTriangle(int32 instanceCount = 1);

    /// <summary>
    /// Draws the specified source texture to destination render target (using fullscreen triangle). Copies contents with resizing and format conversion support. Uses linear texture sampling.
    /// </summary>
    /// <param name="dst">The destination texture.</param>
    /// <param name="src">The source texture.</param>
    API_FUNCTION() void Draw(GPUTexture* dst, GPUTexture* src);

    /// <summary>
    /// Draws the specified texture to render target (using fullscreen triangle). Copies contents with resizing and format conversion support. Uses linear texture sampling.
    /// </summary>
    /// <param name="rt">The texture.</param>
    API_FUNCTION() void Draw(GPUTexture* rt);

    /// <summary>
    /// Draws the specified texture to render target (using fullscreen triangle). Copies contents with resizing and format conversion support. Uses linear texture sampling.
    /// </summary>
    /// <param name="rt">The texture view.</param>
    API_FUNCTION() void Draw(GPUTextureView* rt);

    /// <summary>
    /// Draws non-indexed, non-instanced primitives.
    /// </summary>
    /// <param name="startVertex">A value added to each index before reading a vertex from the vertex buffer.</param>
    /// <param name="verticesCount">The vertices count.</param>
    API_FUNCTION() FORCE_INLINE void Draw(int32 startVertex, uint32 verticesCount)
    {
        DrawInstanced(verticesCount, 1, 0, startVertex);
    }

    /// <summary>
    /// Draws the instanced primitives.
    /// </summary>
    /// <param name="verticesCount">The vertices count.</param>
    /// <param name="instanceCount">Number of instances to draw.</param>
    /// <param name="startInstance">A value added to each index before reading per-instance data from a vertex buffer.</param>
    /// <param name="startVertex">A value added to each index before reading a vertex from the vertex buffer.</param>
    API_FUNCTION() virtual void DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance = 0, int32 startVertex = 0) = 0;

    /// <summary>
    /// Draws the indexed primitives.
    /// </summary>
    /// <param name="indicesCount">The indices count.</param>
    /// <param name="startVertex">A value added to each index before reading a vertex from the vertex buffer.</param>
    /// <param name="startIndex">The location of the first index read by the GPU from the index buffer.</param>
    API_FUNCTION() FORCE_INLINE void DrawIndexed(uint32 indicesCount, int32 startVertex = 0, int32 startIndex = 0)
    {
        DrawIndexedInstanced(indicesCount, 1, 0, startVertex, startIndex);
    }

    /// <summary>
    /// Draws the indexed, instanced primitives.
    /// </summary>
    /// <param name="indicesCount">The indices count.</param>
    /// <param name="instanceCount">Number of instances to draw.</param>
    /// <param name="startInstance">A value added to each index before reading per-instance data from a vertex buffer.</param>
    /// <param name="startVertex">A value added to each index before reading a vertex from the vertex buffer.</param>
    /// <param name="startIndex">The location of the first index read by the GPU from the index buffer.</param>
    API_FUNCTION() virtual void DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance = 0, int32 startVertex = 0, int32 startIndex = 0) = 0;

    /// <summary>
    /// Draws the instanced GPU-generated primitives. Buffer must contain GPUDrawIndirectArgs.
    /// </summary>
    /// <param name="bufferForArgs">The buffer with drawing arguments.</param>
    /// <param name="offsetForArgs">The aligned byte offset for arguments.</param>
    API_FUNCTION() virtual void DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) = 0;

    /// <summary>
    /// Draws the instanced GPU-generated indexed primitives. Buffer must contain GPUDrawIndexedIndirectArgs.
    /// </summary>
    /// <param name="bufferForArgs">The buffer with drawing arguments.</param>
    /// <param name="offsetForArgs">The aligned byte offset for arguments.</param>
    API_FUNCTION() virtual void DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) = 0;

public:
    /// <summary>
    /// Sets the rendering viewport and scissor rectangle.
    /// </summary>
    /// <param name="width">The width (in pixels).</param>
    /// <param name="height">The height (in pixels).</param>
    API_FUNCTION() FORCE_INLINE void SetViewportAndScissors(float width, float height)
    {
        const Viewport viewport(0.0f, 0.0f, width, height);
        SetViewport(viewport);
        const Rectangle rect(0.0f, 0.0f, width, height);
        SetScissor(rect);
    }

    /// <summary>
    /// Sets the rendering viewport and scissor rectangle.
    /// </summary>
    /// <param name="viewport">The viewport (in pixels).</param>
    API_FUNCTION() FORCE_INLINE void SetViewportAndScissors(const Viewport& viewport)
    {
        SetViewport(viewport);
        const Rectangle rect(viewport.Location.X, viewport.Location.Y, viewport.Width, viewport.Height);
        SetScissor(rect);
    }

    /// <summary>
    /// Sets the rendering viewport.
    /// </summary>
    /// <param name="width">The width (in pixels).</param>
    /// <param name="height">The height (in pixels).</param>
    API_FUNCTION() FORCE_INLINE void SetViewport(float width, float height)
    {
        const Viewport viewport(0.0f, 0.0f, width, height);
        SetViewport(viewport);
    }

    /// <summary>
    /// Sets the rendering viewport.
    /// </summary>
    /// <param name="viewport">The viewport (in pixels).</param>
    API_FUNCTION() virtual void SetViewport(API_PARAM(Ref) const Viewport& viewport) = 0;

    /// <summary>
    /// Sets the scissor rectangle.
    /// </summary>
    /// <param name="scissorRect">The scissor rectangle (in pixels).</param>
    API_FUNCTION() virtual void SetScissor(API_PARAM(Ref) const Rectangle& scissorRect) = 0;

public:
    /// <summary>
    /// Sets the graphics pipeline state.
    /// </summary>
    /// <param name="state">The state to bind.</param>
    API_FUNCTION() virtual void SetState(GPUPipelineState* state) = 0;

    /// <summary>
    /// Gets the current pipeline state binded to the graphics pipeline.
    /// </summary>
    /// <returns>The current state.</returns>
    API_FUNCTION() virtual GPUPipelineState* GetState() const = 0;

    /// <summary>
    /// Clears the context state.
    /// </summary>
    API_FUNCTION() virtual void ClearState() = 0;

    /// <summary>
    /// Flushes the internal cached context state with a command buffer.
    /// </summary>
    API_FUNCTION() virtual void FlushState() = 0;

    /// <summary>
    /// Flushes the command buffer (calls GPU execution).
    /// </summary>
    API_FUNCTION() virtual void Flush() = 0;

    /// <summary>
    /// Sets the state of the resource (or subresource).
    /// </summary>
    virtual void SetResourceState(GPUResource* resource, uint64 state, int32 subresource = -1);

    /// <summary>
    /// Forces graphics backend to rebind descriptors after command list was used by external graphics library.
    /// </summary>
    virtual void ForceRebindDescriptors();
};
