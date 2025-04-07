// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Graphics/DynamicBuffer.h"

/// <summary>
/// The mesh deformation data container.
/// </summary>
struct MeshDeformationData
{
    uint64 Key;
    MeshBufferType Type;
    uint32 DirtyMinIndex = 0;
    uint32 DirtyMaxIndex = MAX_uint32 - 1;
    bool Dirty = true;
    BoundingBox Bounds;
    DynamicVertexBuffer VertexBuffer;

    MeshDeformationData(uint64 key, MeshBufferType type, uint32 stride, GPUVertexLayout* layout)
        : Key(key)
        , Type(type)
        , VertexBuffer(0, stride, TEXT("MeshDeformation"), layout)
    {
    }

    NON_COPYABLE(MeshDeformationData);

    ~MeshDeformationData()
    {
    }

    bool LoadMeshAccessor(class MeshAccessor& accessor) const;
};

/// <summary>
/// The mesh deformation utility for editing or morphing models dynamically at runtime (eg. via Blend Shapes or Cloth).
/// </summary>
class FLAXENGINE_API MeshDeformation
{
private:
    Dictionary<uint32, Delegate<const MeshBase*, MeshDeformationData&>> _deformers;
    Array<MeshDeformationData*> _deformations;

public:
    ~MeshDeformation()
    {
        Clear();
    }
    
    void GetBounds(int32 lodIndex, int32 meshIndex, BoundingBox& bounds) const;
    void Clear();
    void Dirty();
    void Dirty(int32 lodIndex, int32 meshIndex, MeshBufferType type);
    void Dirty(int32 lodIndex, int32 meshIndex, MeshBufferType type, const BoundingBox& bounds);
    void AddDeformer(int32 lodIndex, int32 meshIndex, MeshBufferType type, const Function<void(const MeshBase* mesh, MeshDeformationData& deformation)>& deformer);
    void RemoveDeformer(int32 lodIndex, int32 meshIndex, MeshBufferType type, const Function<void(const MeshBase* mesh, MeshDeformationData& deformation)>& deformer);
    void RunDeformers(const MeshBase* mesh, MeshBufferType type, GPUBuffer*& vertexBuffer);
};
