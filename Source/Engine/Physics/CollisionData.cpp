// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "CollisionData.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/Utilities.h"
#include "Engine/Physics/CollisionCooking.h"
#include "Engine/Threading/Threading.h"
#include <ThirdParty/PhysX/extensions/PxDefaultStreams.h>
#include <ThirdParty/PhysX/geometry/PxTriangleMesh.h>
#include <ThirdParty/PhysX/geometry/PxConvexMesh.h>
#include <ThirdParty/PhysX/PxPhysics.h>

REGISTER_BINARY_ASSET(CollisionData, "FlaxEngine.CollisionData", true);

CollisionData::CollisionData(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
    , _convexMesh(nullptr)
    , _triangleMesh(nullptr)
{
}

#if COMPILE_WITH_PHYSICS_COOKING

bool CollisionData::CookCollision(CollisionDataType type, ModelBase* modelObj, int32 modelLodIndex, uint32 materialSlotsMask, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit)
{
    // Validate state
    if (!IsVirtual())
    {
        LOG(Warning, "Only virtual assets can be modified at runtime.");
        return true;
    }

    // Prepare
    CollisionCooking::Argument arg;
    arg.Type = type;
    arg.Model = modelObj;
    arg.ModelLodIndex = modelLodIndex;
    arg.MaterialSlotsMask = materialSlotsMask;
    arg.ConvexFlags = convexFlags;
    arg.ConvexVertexLimit = convexVertexLimit;

    // Cook collision
    SerializedOptions options;
    BytesContainer outputData;
    if (CollisionCooking::CookCollision(arg, options, outputData))
    {
        return true;
    }

    // Clear state
    unload(true);

    // Load data
    if (load(&options, outputData.Get(), outputData.Length()) != LoadResult::Ok)
    {
        return true;
    }

    // Mark as loaded (eg. Mesh Colliders using this asset will update shape for physics simulation)
    onLoaded();
    return false;
}

bool CollisionData::CookCollision(CollisionDataType type, const Span<Vector3>& vertices, const Span<uint32>& triangles, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit)
{
    CHECK_RETURN(vertices.Length() != 0, true);
    CHECK_RETURN(triangles.Length() != 0 && triangles.Length() % 3 == 0, true);
    ModelData modelData;
    modelData.LODs.Resize(1);
    auto meshData = New<MeshData>();
    modelData.LODs[0].Meshes.Add(meshData);
    meshData->Positions.Set(vertices.Get(), vertices.Length());
    meshData->Indices.Set(triangles.Get(), triangles.Length());
    return CookCollision(type, &modelData, convexFlags, convexVertexLimit);
}

bool CollisionData::CookCollision(CollisionDataType type, ModelData* modelData, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit)
{
    // Validate state
    if (!IsVirtual())
    {
        LOG(Warning, "Only virtual assets can be modified at runtime.");
        return true;
    }

    // Prepare
    CollisionCooking::Argument arg;
    arg.Type = type;
    arg.OverrideModelData = modelData;
    arg.ConvexFlags = convexFlags;
    arg.ConvexVertexLimit = convexVertexLimit;

    // Cook collision
    SerializedOptions options;
    BytesContainer outputData;
    if (CollisionCooking::CookCollision(arg, options, outputData))
    {
        return true;
    }

    // Clear state
    unload(true);

    // Load data
    if (load(&options, outputData.Get(), outputData.Length()) != LoadResult::Ok)
    {
        return true;
    }

    // Mark as loaded (eg. Mesh Colliders using this asset will update shape for physics simulation)
    onLoaded();
    return false;
}

#endif

void CollisionData::ExtractGeometry(Array<Vector3>& vertexBuffer, Array<int32>& indexBuffer) const
{
    vertexBuffer.Clear();
    indexBuffer.Clear();

    ScopeLock lock(Locker);

    uint32 numVertices = 0;
    uint32 numIndices = 0;

    // Convex Mesh
    if (_convexMesh)
    {
        numVertices = _convexMesh->getNbVertices();

        const uint32 numPolygons = _convexMesh->getNbPolygons();
        for (uint32 i = 0; i < numPolygons; i++)
        {
            PxHullPolygon face;
            const bool status = _convexMesh->getPolygonData(i, face);
            ASSERT(status);

            numIndices += (face.mNbVerts - 2) * 3;
        }
    }
        // Triangle Mesh
    else if (_triangleMesh)
    {
        numVertices = _triangleMesh->getNbVertices();
        numIndices = _triangleMesh->getNbTriangles() * 3;
    }
        // No collision data
    else
    {
        return;
    }

    // Prepare vertex and index buffers
    vertexBuffer.Resize(numVertices);
    indexBuffer.Resize(numIndices);
    auto outVertices = vertexBuffer.Get();
    auto outIndices = indexBuffer.Get();

    if (_convexMesh)
    {
        const PxVec3* convexVertices = _convexMesh->getVertices();
        const byte* convexIndices = _convexMesh->getIndexBuffer();

        for (uint32 i = 0; i < numVertices; i++)
            *outVertices++ = P2C(convexVertices[i]);

        uint32 numPolygons = _convexMesh->getNbPolygons();
        for (uint32 i = 0; i < numPolygons; i++)
        {
            PxHullPolygon face;
            bool status = _convexMesh->getPolygonData(i, face);
            ASSERT(status);

            const PxU8* faceIndices = convexIndices + face.mIndexBase;
            for (uint32 j = 2; j < face.mNbVerts; j++)
            {
                *outIndices++ = faceIndices[0];
                *outIndices++ = faceIndices[j];
                *outIndices++ = faceIndices[j - 1];
            }
        }
    }
    else
    {
        const PxVec3* vertices = _triangleMesh->getVertices();
        for (uint32 i = 0; i < numVertices; i++)
            *outVertices++ = P2C(vertices[i]);

        if (_triangleMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES)
        {
            const uint16* indices = (const uint16*)_triangleMesh->getTriangles();

            uint32 numTriangles = numIndices / 3;
            for (uint32 i = 0; i < numTriangles; i++)
            {
                outIndices[i * 3 + 0] = (uint32)indices[i * 3 + 0];
                outIndices[i * 3 + 1] = (uint32)indices[i * 3 + 1];
                outIndices[i * 3 + 2] = (uint32)indices[i * 3 + 2];
            }
        }
        else
        {
            const uint32* indices = (const uint32*)_triangleMesh->getTriangles();

            uint32 numTriangles = numIndices / 3;
            for (uint32 i = 0; i < numTriangles; i++)
            {
                outIndices[i * 3 + 0] = indices[i * 3 + 0];
                outIndices[i * 3 + 1] = indices[i * 3 + 1];
                outIndices[i * 3 + 2] = indices[i * 3 + 2];
            }
        }
    }
}

#if USE_EDITOR

const Array<Vector3>& CollisionData::GetDebugLines()
{
    if (_hasMissingDebugLines && IsLoaded())
    {
        ScopeLock lock(Locker);
        _hasMissingDebugLines = false;

        Array<Vector3> vertexBuffer;
        Array<int32> indexBuffer;
        ExtractGeometry(vertexBuffer, indexBuffer);

        // Get lines
        _debugLines.Resize(indexBuffer.Count() * 2);
        int32 lineIndex = 0;
        for (int32 i = 0; i < indexBuffer.Count(); i += 3)
        {
            const auto a = vertexBuffer[indexBuffer[i + 0]];
            const auto b = vertexBuffer[indexBuffer[i + 1]];
            const auto c = vertexBuffer[indexBuffer[i + 2]];

            _debugLines[lineIndex++] = a;
            _debugLines[lineIndex++] = b;

            _debugLines[lineIndex++] = b;
            _debugLines[lineIndex++] = c;

            _debugLines[lineIndex++] = c;
            _debugLines[lineIndex++] = a;
        }
    }

    return _debugLines;
}

#endif

Asset::LoadResult CollisionData::load()
{
    const auto chunk0 = GetChunk(0);
    if (chunk0 == nullptr || chunk0->IsMissing() || chunk0->Size() < sizeof(SerializedOptions))
        return LoadResult::MissingDataChunk;

    const auto options = (SerializedOptions*)chunk0->Get();
    const auto dataPtr = chunk0->Get() + sizeof(SerializedOptions);
    const int32 dataSize = chunk0->Size() - sizeof(SerializedOptions);

    return load(options, dataPtr, dataSize);
}

CollisionData::LoadResult CollisionData::load(const SerializedOptions* options, byte* dataPtr, int32 dataSize)
{
    // Load options
    _options.Type = options->Type;
    _options.Model = options->Model;
    _options.ModelLodIndex = options->ModelLodIndex;
    _options.ConvexFlags = options->ConvexFlags;
    _options.ConvexVertexLimit = options->ConvexVertexLimit < 4 ? 255 : options->ConvexVertexLimit;

    // Load data (rest of the chunk is a cooked collision data)
    if (_options.Type == CollisionDataType::None && dataSize > 0)
    {
        LOG(Warning, "Missing collision data.");
        return LoadResult::InvalidData;
    }
    if (_options.Type != CollisionDataType::None)
    {
        if (dataSize <= 0)
            return LoadResult::InvalidData;

        // Create PhysX object
        PxDefaultMemoryInputData input(dataPtr, dataSize);
        if (_options.Type == CollisionDataType::ConvexMesh)
        {
            _convexMesh = Physics::GetPhysics()->createConvexMesh(input);
            _options.Box = P2C(_convexMesh->getLocalBounds());
        }
        else if (_options.Type == CollisionDataType::TriangleMesh)
        {
            _triangleMesh = Physics::GetPhysics()->createTriangleMesh(input);
            _options.Box = P2C(_triangleMesh->getLocalBounds());
        }
        else
        {
            LOG(Warning, "Invalid collision data type.");
            return LoadResult::InvalidData;
        }
    }

    return LoadResult::Ok;
}

void CollisionData::unload(bool isReloading)
{
    if (_convexMesh)
    {
        Physics::RemoveObject(_convexMesh);
        _convexMesh = nullptr;
    }
    if (_triangleMesh)
    {
        Physics::RemoveObject(_triangleMesh);
        _triangleMesh = nullptr;
    }
    _options = CollisionDataOptions();
#if USE_EDITOR
    _hasMissingDebugLines = true;
    _debugLines.Resize(0);
#endif
}

AssetChunksFlag CollisionData::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
