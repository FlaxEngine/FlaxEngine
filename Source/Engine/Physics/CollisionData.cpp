// Copyright (c) Wojciech Figat. All rights reserved.

#include "CollisionData.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Physics/PhysicsScene.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Physics/CollisionCooking.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"

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
    if (!IsVirtual())
    {
        LOG(Warning, "Only virtual assets can be modified at runtime.");
        return true;
    }
    if (IsInMainThread() && modelObj && modelObj->IsVirtual())
    {
        LOG(Error, "Cannot cook collision data for virtual models on a main thread (virtual models data is stored on GPU only). Use thread pool or async task.");
        return true;
    }
    PROFILE_CPU();

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
        return true;

    // Load data
    unload(true);
    if (load(&options, outputData.Get(), outputData.Length()) != LoadResult::Ok)
        return true;

    // Mark as loaded (eg. Mesh Colliders using this asset will update shape for physics simulation)
    onLoaded();
    return false;
}

bool CollisionData::CookCollision(CollisionDataType type, const Span<Float3>& vertices, const Span<uint32>& triangles, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit)
{
    PROFILE_CPU();
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

bool CollisionData::CookCollision(CollisionDataType type, const Span<Float3>& vertices, const Span<int32>& triangles, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit)
{
    PROFILE_CPU();
    CHECK_RETURN(vertices.Length() != 0, true);
    CHECK_RETURN(triangles.Length() != 0 && triangles.Length() % 3 == 0, true);
    ModelData modelData;
    modelData.LODs.Resize(1);
    auto meshData = New<MeshData>();
    modelData.LODs[0].Meshes.Add(meshData);
    meshData->Positions.Set(vertices.Get(), vertices.Length());
    meshData->Indices.Resize(triangles.Length());
    for (int32 i = 0; i < triangles.Length(); i++)
        meshData->Indices.Get()[i] = triangles.Get()[i];
    return CookCollision(type, &modelData, convexFlags, convexVertexLimit);
}

bool CollisionData::CookCollision(CollisionDataType type, ModelData* modelData, ConvexMeshGenerationFlags convexFlags, int32 convexVertexLimit)
{
    if (!IsVirtual())
    {
        LOG(Warning, "Only virtual assets can be modified at runtime.");
        return true;
    }
    PROFILE_CPU();

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
        return true;

    // Clear state
    unload(true);

    // Load data
    if (load(&options, outputData.Get(), outputData.Length()) != LoadResult::Ok)
        return true;

    // Mark as loaded (eg. Mesh Colliders using this asset will update shape for physics simulation)
    onLoaded();
    return false;
}

#endif

bool CollisionData::GetModelTriangle(uint32 faceIndex, MeshBase*& mesh, uint32& meshTriangleIndex) const
{
    mesh = nullptr;
    meshTriangleIndex = MAX_uint32;
    if (!IsLoaded())
        return false;
    PROFILE_CPU();
    ScopeLock lock(Locker);
    if (_triangleMesh)
    {
        uint32 trianglesCount;
        const uint32* remap = PhysicsBackend::GetTriangleMeshRemap(_triangleMesh, trianglesCount);
        if (remap && faceIndex < trianglesCount)
        {
            // Get source triangle index from the triangle mesh
            meshTriangleIndex = remap[faceIndex];

            // Check if model was used when cooking
            AssetReference<ModelBase> model;
            model = _options.Model;
            if (!model)
                return true;

            // Follow code-path similar to CollisionCooking.cpp to pick a mesh that contains this triangle (collision is cooked from merged all source meshes from the model)
            if (model->WaitForLoaded())
                return false;
            const int32 lodIndex = Math::Clamp(_options.ModelLodIndex, 0, model->GetLODsCount());
            Array<MeshBase*> meshes;
            model->GetMeshes(meshes, lodIndex);
            uint32 triangleCounter = 0;
            for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
            {
                MeshBase* m = meshes[meshIndex];
                if ((_options.MaterialSlotsMask & (1 << m->GetMaterialSlotIndex())) == 0)
                    continue;
                const uint32 count = m->GetTriangleCount();
                if (meshTriangleIndex - triangleCounter <= count)
                {
                    mesh = m;
                    meshTriangleIndex -= triangleCounter;
                    return true;
                }
                triangleCounter += count;
            }
        }
    }

    return false;
}

void CollisionData::ExtractGeometry(Array<Float3>& vertexBuffer, Array<int32>& indexBuffer) const
{
    PROFILE_CPU();
    vertexBuffer.Clear();
    indexBuffer.Clear();

    ScopeLock lock(Locker);
    if (_convexMesh)
        PhysicsBackend::GetConvexMeshTriangles(_convexMesh, vertexBuffer, indexBuffer);
    else if (_triangleMesh)
        PhysicsBackend::GetTriangleMeshTriangles(_triangleMesh, vertexBuffer, indexBuffer);
}

#if USE_EDITOR

const Array<Float3>& CollisionData::GetDebugLines()
{
    if (_hasMissingDebugLines && IsLoaded())
    {
        PROFILE_CPU();
        ScopeLock lock(Locker);
        _hasMissingDebugLines = false;

        // Get triangles
        ExtractGeometry(_debugVertexBuffer, _debugIndexBuffer);

        // Get lines
        _debugLines.Resize(_debugIndexBuffer.Count() * 2);
        int32 lineIndex = 0;
        for (int32 i = 0; i < _debugIndexBuffer.Count(); i += 3)
        {
            const auto a = _debugVertexBuffer[_debugIndexBuffer[i + 0]];
            const auto b = _debugVertexBuffer[_debugIndexBuffer[i + 1]];
            const auto c = _debugVertexBuffer[_debugIndexBuffer[i + 2]];

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

void CollisionData::GetDebugTriangles(Array<Float3>*& vertexBuffer, Array<int32>*& indexBuffer)
{
    GetDebugLines();
    vertexBuffer = &_debugVertexBuffer;
    indexBuffer = &_debugIndexBuffer;
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
    _options.MaterialSlotsMask = options->MaterialSlotsMask == 0 ? MAX_uint32 : options->MaterialSlotsMask;

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

        // Create physics object
        if (_options.Type == CollisionDataType::ConvexMesh)
        {
            _convexMesh = PhysicsBackend::CreateConvexMesh(dataPtr, dataSize, _options.Box);
            if (!_convexMesh)
            {
                LOG(Error, "Failed to create convex mesh");
                return LoadResult::Failed;
            }
        }
        else if (_options.Type == CollisionDataType::TriangleMesh)
        {
            _triangleMesh = PhysicsBackend::CreateTriangleMesh(dataPtr, dataSize, _options.Box);
            if (!_triangleMesh)
            {
                LOG(Error, "Failed to create triangle mesh");
                return LoadResult::Failed;
            }
        }
        else
        {
            LOG(Error, "Invalid collision data type.");
            return LoadResult::InvalidData;
        }
    }

    return LoadResult::Ok;
}

void CollisionData::unload(bool isReloading)
{
    if (_convexMesh)
    {
        PhysicsBackend::DestroyObject(_convexMesh);
        _convexMesh = nullptr;
    }
    if (_triangleMesh)
    {
        PhysicsBackend::DestroyObject(_triangleMesh);
        _triangleMesh = nullptr;
    }
    _options = CollisionDataOptions();
#if USE_EDITOR
    _hasMissingDebugLines = true;
    _debugLines.Resize(0);
    _debugVertexBuffer.Resize(0);
    _debugIndexBuffer.Resize(0);
#endif
}

AssetChunksFlag CollisionData::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
