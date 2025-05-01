// Copyright (c) Wojciech Figat. All rights reserved.

#include "MeshDeformation.h"
#include "MeshAccessor.h"
#include "Engine/Graphics/Models/MeshBase.h"
#include "Engine/Profiler/ProfilerCPU.h"

struct Key
{
    union
    {
        struct
        {
            uint8 Lod;
            uint8 BufferType;
            uint16 MeshIndex;
        };

        uint32 Value;
    };
};

FORCE_INLINE static uint32 GetKey(int32 lodIndex, int32 meshIndex, MeshBufferType type)
{
    Key key;
    key.Value = 0;
    key.Lod = (uint8)lodIndex;
    key.BufferType = (uint8)type;
    key.MeshIndex = (uint16)meshIndex;
    return key.Value;
}

bool MeshDeformationData::LoadMeshAccessor(MeshAccessor& accessor) const
{
    return accessor.LoadBuffer(Type, ToSpan(VertexBuffer.Data), VertexBuffer.GetLayout());
}

void MeshDeformation::GetBounds(int32 lodIndex, int32 meshIndex, BoundingBox& bounds) const
{
    const auto key = GetKey(lodIndex, meshIndex, MeshBufferType::Vertex0);
    for (MeshDeformationData* deformation : _deformations)
    {
        if (deformation->Key == key)
        {
            bounds = deformation->Bounds;
            break;
        }
    }
}

void MeshDeformation::Clear()
{
    for (MeshDeformationData* e : _deformations)
        Delete(e);
    _deformations.Clear();
}

void MeshDeformation::Dirty()
{
    for (MeshDeformationData* deformation : _deformations)
        deformation->Dirty = true;
}

void MeshDeformation::Dirty(int32 lodIndex, int32 meshIndex, MeshBufferType type)
{
    const auto key = GetKey(lodIndex, meshIndex, type);
    for (MeshDeformationData* deformation : _deformations)
    {
        if (deformation->Key == key)
        {
            deformation->Dirty = true;
            break;
        }
    }
}

void MeshDeformation::Dirty(int32 lodIndex, int32 meshIndex, MeshBufferType type, const BoundingBox& bounds)
{
    const auto key = GetKey(lodIndex, meshIndex, type);
    for (MeshDeformationData* deformation : _deformations)
    {
        if (deformation->Key == key)
        {
            deformation->Dirty = true;
            deformation->Bounds = bounds;
            break;
        }
    }
}

void MeshDeformation::AddDeformer(int32 lodIndex, int32 meshIndex, MeshBufferType type, const Function<void(const MeshBase* mesh, MeshDeformationData& deformation)>& deformer)
{
    const auto key = GetKey(lodIndex, meshIndex, type);
    _deformers[key].Bind(deformer);
    Dirty(lodIndex, meshIndex, type);
}

void MeshDeformation::RemoveDeformer(int32 lodIndex, int32 meshIndex, MeshBufferType type, const Function<void(const MeshBase* mesh, MeshDeformationData& deformation)>& deformer)
{
    const auto key = GetKey(lodIndex, meshIndex, type);
    _deformers[key].Unbind(deformer);
    Dirty(lodIndex, meshIndex, type);
}

void MeshDeformation::RunDeformers(const MeshBase* mesh, MeshBufferType type, GPUBuffer*& vertexBuffer)
{
    const auto key = GetKey(mesh->GetLODIndex(), mesh->GetIndex(), type);
    if (const auto* e = _deformers.TryGet(key))
    {
        PROFILE_CPU();
        const int32 vertexStride = vertexBuffer->GetStride();

        // Get mesh deformation container
        MeshDeformationData* deformation = nullptr;
        for (int32 i = 0; i < _deformations.Count(); i++)
        {
            if (_deformations[i]->Key == key)
            {
                deformation = _deformations[i];
                break;
            }
        }
        if (!e->IsBinded())
        {
            // Auto-recycle unused deformations
            if (deformation)
            {
                _deformations.Remove(deformation);
                Delete(deformation);
            }
            _deformers.Remove(key);
            return;
        }
        if (!deformation)
        {
            deformation = New<MeshDeformationData>(key, type, vertexStride, vertexBuffer->GetVertexLayout());
            deformation->VertexBuffer.Data.Resize(vertexBuffer->GetSize());
            deformation->Bounds = mesh->GetBox();
            _deformations.Add(deformation);
        }

        if (deformation->Dirty)
        {
            // Get original mesh vertex buffer data (cached on CPU)
            BytesContainer vertexData;
            int32 vertexCount;
            if (mesh->DownloadDataCPU(type, vertexData, vertexCount))
                return;
            ASSERT(vertexData.Length() / vertexCount == vertexStride);

            // Init dirty range with valid data (use the dirty range from the previous update to be cleared with initial data)
            deformation->VertexBuffer.Data.Resize(vertexData.Length());
            const uint32 dirtyDataStart = Math::Min<uint32>(deformation->DirtyMinIndex, vertexCount - 1) * vertexStride;
            const uint32 dirtyDataLength = Math::Min<uint32>(deformation->DirtyMaxIndex - deformation->DirtyMinIndex + 1, vertexCount) * vertexStride;
            Platform::MemoryCopy(deformation->VertexBuffer.Data.Get() + dirtyDataStart, vertexData.Get() + dirtyDataStart, dirtyDataLength);

            // Reset dirty state
            deformation->DirtyMinIndex = MAX_uint32 - 1;
            deformation->DirtyMaxIndex = 0;
            deformation->Dirty = false;

            // Run deformers
            (*e)(mesh, *deformation);

            // Upload modified vertex data to the GPU
            deformation->VertexBuffer.Flush();
        }

        // Override vertex buffer for draw call
        vertexBuffer = deformation->VertexBuffer.GetBuffer();
    }
}
