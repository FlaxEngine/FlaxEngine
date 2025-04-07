// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Graphics/PixelFormatSampler.h"
#include "Engine/Graphics/Shaders/VertexElement.h"

/// <summary>
/// General purpose utility for accessing mesh data (both read and write).
/// </summary>
class FLAXENGINE_API MeshAccessor
{
public:
    /// <summary>
    /// Mesh data stream.
    /// </summary>
    struct Stream
    {
        friend MeshAccessor;

    private:
        Span<byte> _data;
        PixelFormat _format;
        int32 _stride;
        PixelFormatSampler _sampler;

        Stream(Span<byte> data, PixelFormat format, int32 stride);

    public:
        Span<byte> GetData() const;
        PixelFormat GetFormat() const;
        int32 GetStride() const;
        int32 GetCount() const;
        bool IsValid() const;
        bool IsLinear(PixelFormat expectedFormat) const;

        FORCE_INLINE int32 GetInt(int32 index) const
        {
            return (int32)_sampler.Read(_data.Get() + index * _stride).X;
        }

        FORCE_INLINE float GetFloat(int32 index) const
        {
            return _sampler.Read(_data.Get() + index * _stride).X;
        }

        FORCE_INLINE Float2 GetFloat2(int32 index) const
        {
            return Float2(_sampler.Read(_data.Get() + index * _stride));
        }

        FORCE_INLINE Float3 GetFloat3(int32 index) const
        {
            return Float3(_sampler.Read(_data.Get() + index * _stride));
        }

        FORCE_INLINE Float4 GetFloat4(int32 index) const
        {
            return _sampler.Read(_data.Get() + index * _stride);
        }

        FORCE_INLINE void SetInt(int32 index, const int32 value)
        {
            _sampler.Write(_data.Get() + index * _stride, Float4((float)value));
        }

        FORCE_INLINE void SetFloat(int32 index, const float value)
        {
            _sampler.Write(_data.Get() + index * _stride, Float4(value));
        }

        FORCE_INLINE void SetFloat2(int32 index, const Float2& value)
        {
            _sampler.Write(_data.Get() + index * _stride, Float4(value));
        }

        FORCE_INLINE void SetFloat3(int32 index, const Float3& value)
        {
            _sampler.Write(_data.Get() + index * _stride, Float4(value));
        }

        FORCE_INLINE void SetFloat4(int32 index, const Float4& value)
        {
            _sampler.Write(_data.Get() + index * _stride, value);
        }

        // Check input data and stream type with IsLinear before calling.
        void SetLinear(const void* data);

        // Copies the contents of the input data span into the elements of this stream.
        void Set(Span<Float2> src);
        void Set(Span<Float3> src);
        void Set(Span<Color> src);

        // Copies the contents of this stream into a destination data span.
        void CopyTo(Span<Float2> dst) const;
        void CopyTo(Span<Float3> dst) const;
        void CopyTo(Span<Color> dst) const;
    };

private:
    BytesContainer _data[(int32)MeshBufferType::MAX];
    PixelFormat _formats[(int32)MeshBufferType::MAX] = {};
    GPUVertexLayout* _layouts[(int32)MeshBufferType::MAX] = {};

public:
    /// <summary>
    /// Loads the data from the mesh.
    /// </summary>
    /// <param name="mesh">The source mesh object to access.</param>
    /// <param name="forceGpu">If set to <c>true</c> the data will be downloaded from the GPU, otherwise it can be loaded from the drive (source asset file) or from memory (if cached). Downloading mesh from GPU requires this call to be made from the other thread than main thread. Virtual assets are always downloaded from GPU memory due to lack of dedicated storage container for the asset data.</param>
    /// <param name="buffers">Custom list of mesh buffers to load. Use empty value to access all of them.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadMesh(const MeshBase* mesh, bool forceGpu = false, Span<MeshBufferType> buffers = Span<MeshBufferType>());

    /// <summary>
    /// Loads the data from the provided mesh buffer.
    /// </summary>
    /// <param name="bufferType">Type of the mesh buffer to load.</param>
    /// <param name="bufferData">Data used by that buffer.</param>
    /// <param name="layout">Layout of the elements in the buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadBuffer(MeshBufferType bufferType, Span<byte> bufferData, GPUVertexLayout* layout);

    // Used internally via ModelBase::LoadMesh.
    bool LoadFromMeshData(const void* meshDataPtr);

    /// <summary>
    /// Allocates the data for the mesh vertex buffer.
    /// </summary>
    /// <param name="bufferType">Type of the mesh buffer to initialize.</param>
    /// <param name="count">Amount of items in the buffer.</param>
    /// <param name="layout">Layout of the elements in the buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool AllocateBuffer(MeshBufferType bufferType, int32 count, GPUVertexLayout* layout);

    /// <summary>
    /// Allocates the data for the mesh buffer.
    /// </summary>
    /// <param name="bufferType">Type of the mesh buffer to initialize.</param>
    /// <param name="count">Amount of items in the buffer.</param>
    /// <param name="format">Format of the elements in the buffer.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool AllocateBuffer(MeshBufferType bufferType, int32 count, PixelFormat format);

    /// <summary>
    /// Updates the mesh vertex and index buffers with data assigned to the accessor (eg. via AllocateBuffer).
    /// </summary>
    /// <param name="mesh">The target mesh to update.</param>
    /// <param name="calculateBounds">True if auto-calculate mesh local bounding box based on input positions stream. Otherwise, mesh bound swill stay unchanged.</param>
    /// <returns>True if failed, otherwise false.</returns>
    bool UpdateMesh(MeshBase* mesh, bool calculateBounds = true);

public:
    // Access stream with index buffer.
    Stream Index();

    // Access stream with a specific vertex attribute.
    Stream Attribute(VertexElement::Types attribute);

    // Access stream with vertex position attribute.
    FORCE_INLINE Stream Position()
    {
        return Attribute(VertexElement::Types::Position);
    }

    // Access stream with vertex color attribute.
    FORCE_INLINE Stream Color()
    {
        return Attribute(VertexElement::Types::Color);
    }

    // Access stream with vertex normal vector attribute.
    FORCE_INLINE Stream Normal()
    {
        return Attribute(VertexElement::Types::Normal);
    }

    // Access stream with vertex tangent vector attribute.
    FORCE_INLINE Stream Tangent()
    {
        return Attribute(VertexElement::Types::Tangent);
    }

    // Access stream with vertex skeleton bones blend indices attribute.
    FORCE_INLINE Stream BlendIndices()
    {
        return Attribute(VertexElement::Types::BlendIndices);
    }

    // Access stream with vertex skeleton bones blend weights attribute.
    FORCE_INLINE Stream BlendWeights()
    {
        return Attribute(VertexElement::Types::BlendWeights);
    }

    // Access stream with vertex texture coordinates attribute (specific UV channel).
    FORCE_INLINE Stream TexCoord(int32 channel = 0)
    {
        return Attribute((VertexElement::Types)((byte)VertexElement::Types::TexCoord0 + channel));
    }

public:
    // Unpacks normal/tangent vector from normalized range to full range.
    FORCE_INLINE static void UnpackNormal(Float3& normal)
    {
        normal = normal * 2.0f - 1.0f;
    }

    // Packs normal/tangent vector to normalized range from full range.
    FORCE_INLINE static void PackNormal(Float3& normal)
    {
        normal = normal * 0.5f + 0.5f;
    }
};
