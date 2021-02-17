// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PHYSICS_COOKING

#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Physics/CollisionData.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Content/Assets/Model.h"

namespace physx
{
    class PxHeightFieldDesc;
    class PxOutputStream;
}

/// <summary>
/// Physical collision data cooking tools. Allows to bake heightfield, convex and triangle mesh colliders data.
/// </summary>
class CollisionCooking
{
public:

    struct CookingInput
    {
        int32 VertexCount = 0;
        Vector3* VertexData = nullptr;
        int32 IndexCount = 0;
        void* IndexData = nullptr;
        bool Is16bitIndexData = false;
        ConvexMeshGenerationFlags ConvexFlags = ConvexMeshGenerationFlags::None;
        int32 ConvexVertexLimit = 255;
    };

    /// <summary>
    /// Collision data cooking input argument format.
    /// </summary>
    struct Argument
    {
        CollisionDataType Type = CollisionDataType::None;
        ModelData* OverrideModelData = nullptr;
        AssetReference<Model> Model;
        int32 ModelLodIndex = 0;
        ConvexMeshGenerationFlags ConvexFlags = ConvexMeshGenerationFlags::None;
        int32 ConvexVertexLimit = 255;
    };

    /// <summary>
    /// Attempts to cook a convex mesh from the provided mesh data. Assumes the input data is valid and contains vertex
    /// positions. If the method returns false the resulting convex mesh will be in the output parameter.
    /// </summary>
    /// <param name="input">The input.</param>
    /// <param name="output">The output.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CookConvexMesh(CookingInput& input, BytesContainer& output);

    /// <summary>
    /// Attempts to cook a triangle mesh from the provided mesh data. Assumes the input data is valid and contains vertex
    /// positions as well as face indices. If the method returns false the resulting convex mesh will be in the output parameter.
    /// </summary>
    /// <param name="input">The input.</param>
    /// <param name="output">The output.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CookTriangleMesh(CookingInput& input, BytesContainer& output);

    /// <summary>
    /// Cooks the collision from the model and prepares the data for the <see cref="CollisionData"/> format.
    /// </summary>
    /// <param name="arg">The input argument descriptor.</param>
    /// <param name="outputOptions">The output options container.</param>
    /// <param name="outputData">The output data container.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CookCollision(const Argument& arg, CollisionData::SerializedOptions& outputOptions, BytesContainer& outputData);

    /// <summary>
    /// Cooks a heightfield. The results are written to the stream. To create a heightfield object there is an option to precompute some of calculations done while loading the heightfield data.
    /// </summary>
    /// <param name="desc">The heightfield descriptor to read the HF from.</param>
    /// <param name="stream">The user stream to output the cooked data.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CookHeightField(const physx::PxHeightFieldDesc& desc, physx::PxOutputStream& stream);
};

#endif
