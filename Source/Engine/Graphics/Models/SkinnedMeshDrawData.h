// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Common.h"
#include "Engine/Graphics/GPUBuffer.h"

/// <summary>
/// Data storage for the skinned meshes rendering
/// </summary>
class FLAXENGINE_API SkinnedMeshDrawData
{
private:

    bool _hasValidData = false;
    bool _isDirty = false;

public:

    /// <summary>
    /// The bones count.
    /// </summary>
    int32 BonesCount = 0;

    /// <summary>
    /// The bone matrices buffer. Contains prepared skeletal bones transformations (stored as 4x3, 3 Vector4 behind each other).
    /// </summary>
    GPUBuffer* BoneMatrices = nullptr;

    /// <summary>
    /// The bone matrices buffer used during the previous update. Used by per-bone motion blur.
    /// </summary>
    GPUBuffer* PrevBoneMatrices = nullptr;

    /// <summary>
    /// The CPU data buffer with the bones transformations (ready to be flushed with the GPU).
    /// </summary>
    Array<byte> Data;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="SkinnedMeshDrawData"/> class.
    /// </summary>
    SkinnedMeshDrawData()
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="SkinnedMeshDrawData"/> class.
    /// </summary>
    ~SkinnedMeshDrawData();

public:

    /// <summary>
    /// Determines whether this instance is ready for rendering.
    /// </summary>
    /// <returns>True if has valid data and can be rendered, otherwise false.</returns>
    FORCE_INLINE bool IsReady() const
    {
        return BoneMatrices != nullptr && BoneMatrices->IsAllocated();
    }

    /// <summary>
    /// Setups the data container for the specified bones amount.
    /// </summary>
    /// <param name="bonesCount">The bones count.</param>
    void Setup(int32 bonesCount);

    /// <summary>
    /// Sets the bone matrices data for the GPU buffer. Ensure to call Flush before rendering.
    /// </summary>
    /// <param name="bones">The bones data.</param>
    /// <param name="dropHistory">True if drop previous update bones used for motion blur, otherwise will keep them and do the update.</param>
    void SetData(const Matrix* bones, bool dropHistory);

    /// <summary>
    /// Flushes the bones data buffer with the GPU by sending the data fro the CPU.
    /// </summary>
    /// <param name="context">The GPU context.</param>
    void Flush(class GPUContext* context);
};
