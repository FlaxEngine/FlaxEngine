// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "CSGBuilder.h"
#include "CSGMesh.h"
#include "CSGData.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Level/Actor.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Physics/CollisionData.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/ContentImporters/CreateCollisionData.h"
#include "Engine/Content/Assets/RawDataAsset.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif

#if COMPILE_WITH_CSG_BUILDER

using namespace CSG;

// Enable/disable locking scene during building CSG brushes nodes
#define CSG_USE_SCENE_LOCKS 0

struct BuildData;

namespace CSGBuilderImpl
{
    Array<Scene*> ScenesToRebuild;

    void onSceneUnloading(Scene* scene, const Guid& sceneId);
    bool buildInner(Scene* scene, BuildData& data);
    void build(Scene* scene);
    bool generateRawDataAsset(Scene* scene, RawData& meshData, Guid& assetId, const String& assetPath);
}

using namespace CSGBuilderImpl;

class CSGBuilderService : public EngineService
{
public:

    CSGBuilderService()
        : EngineService(TEXT("CSG Builder"), 90)
    {
    }

    bool Init() override;
    void Update() override;
};

CSGBuilderService CSGBuilderServiceInstance;

Delegate<Brush*> Builder::OnBrushModified;

void CSGBuilderImpl::onSceneUnloading(Scene* scene, const Guid& sceneId)
{
    // Ensure to remove scene (prevent crashes)
    ScenesToRebuild.Remove(scene);
}

bool CSGBuilderService::Init()
{
    Level::SceneUnloading.Bind(onSceneUnloading);

    return false;
}

void CSGBuilderService::Update()
{
    // Check if build is pending
    if (ScenesToRebuild.HasItems() && Engine::IsReady())
    {
        auto now = DateTime::NowUTC();

        for (int32 i = 0; ScenesToRebuild.HasItems() && i < ScenesToRebuild.Count(); i++)
        {
            auto scene = ScenesToRebuild[i];
            if (now - scene->CSGData.BuildTime >= 0)
            {
                scene->CSGData.BuildTime.Ticks = 0;
                ScenesToRebuild.RemoveAt(i--);
                build(scene);
            }
        }
    }
}

bool Builder::IsActive()
{
    return ScenesToRebuild.HasItems();
}

void Builder::Build(Scene* scene, float timeoutMs)
{
    if (scene == nullptr)
        return;

#if USE_EDITOR
    // Disable building during play mode
    if (Editor::IsPlayMode)
        return;
#endif

    // Register building
    if (!ScenesToRebuild.Contains(scene))
    {
        ScenesToRebuild.Add(scene);
    }
    scene->CSGData.BuildTime = DateTime::NowUTC() + TimeSpan::FromMilliseconds(timeoutMs);
}

namespace CSG
{
    typedef Dictionary<Actor*, Mesh*> MeshesLookup;

    bool walkTree(Actor* actor, MeshesArray& meshes, MeshesLookup& cache)
    {
        // Check if actor is a brush
        auto brush = dynamic_cast<Brush*>(actor);
        if (brush)
        {
            // Check if can build it
            if (brush->CanUseCSG())
            {
                // Skip subtract/common meshes from the beginning (they have no effect)
                if (meshes.Count() > 0 || brush->GetBrushMode() == Mode::Additive)
                {
                    // Create new mesh and build for given brush
                    auto mesh = New<CSG::Mesh>();
                    mesh->Build(brush);

                    // Save results
                    meshes.Add(mesh);
                    cache.Add(actor, mesh);
                }
                else
                {
                    // Info
                    LOG(Info, "Skipping CSG brush '{0}'", actor->ToString());
                }
            }
        }

        return true;
    }

    Mesh* Combine(Actor* actor, MeshesLookup& cache, Mesh* combineParent)
    {
        ASSERT(actor);
        Mesh* result = nullptr;
        Mesh* myBrush = nullptr;
        cache.TryGet(actor, myBrush);

        // Get first child mesh with valid data (has additive brush)
        int32 childIndex = 0;
        while (childIndex < actor->Children.Count())
        {
            auto child = Combine(actor->Children[childIndex], cache, combineParent);

            childIndex++;
            if (child)
            {
                // If brush was based on additive brush or current actor is a brush we can stop searching
                if (child->HasMode(Mode::Additive) || myBrush)
                {
                    // End searching
                    result = child;
                    break;
                }

                if (combineParent)
                {
                    // Combine
                    combineParent->PerformOperation(child);
                }
            }
        }

        // Check if has any child with CSG brush
        if (result)
        {
            // Check if has own brush
            if (myBrush)
            {
                // Combine with first child
                myBrush->PerformOperation(result);

                // Set this actor brush as a result
                result = myBrush;
            }

            // Merge with the other children
            while (childIndex < actor->Children.Count())
            {
                auto child = Combine(actor->Children[childIndex], cache, result);
                if (child)
                {
                    // Combine
                    result->PerformOperation(child);
                }

                childIndex++;
            }
        }
        else
        {
            // Use this actor brush (may be empty)
            result = myBrush;
        }

        return result;
    }

    Mesh* Combine(Scene* scene, MeshesLookup& cache)
    {
#if CSG_USE_SCENE_LOCKS
		auto Level = Level::Instance();
		Level->Lock();
#endif

        Mesh* result = Combine(scene, cache, nullptr);

#if CSG_USE_SCENE_LOCKS
		Level->Unlock();
#endif

        return result;
    }
}

struct BuildData
{
    MeshesArray meshes;
    MeshesLookup cache;
    Guid outputModelAssetId = Guid::Empty;
    Guid outputRawDataAssetId = Guid::Empty;
    Guid outputCollisionDataAssetId = Guid::Empty;

    BuildData(int32 meshesCapacity = 32)
        : meshes(meshesCapacity * 32)
        , cache(meshesCapacity * 4)
    {
    }
};

bool CSGBuilderImpl::buildInner(Scene* scene, BuildData& data)
{
    // Setup CSG meshes list and build them
    {
        Function<bool(Actor*, MeshesArray&, MeshesLookup&)> treeWalkFunction(walkTree);
        SceneQuery::TreeExecute<Array<CSG::Mesh*>&, MeshesLookup&>(treeWalkFunction, data.meshes, data.cache);
    }
    if (data.meshes.IsEmpty())
        return false;

    // Process all meshes (performs actual CSG opterations on geometry in tree structure)
    CSG::Mesh* combinedMesh = Combine(scene, data.cache);
    if (combinedMesh == nullptr)
        return false;

    // TODO: split too big meshes (too many verts, to far parts, etc.)

    // Triangulate meshes
    {
        // TODO: setup valid loop for splited meshes

        // Convert CSG meshes into raw triangles data
        RawData meshData;
        Array<RawModelVertex> vertexBuffer;
        combinedMesh->Triangulate(meshData, vertexBuffer);
        meshData.RemoveEmptySlots();
        if (meshData.Slots.HasItems())
        {
            const auto sceneDataFolderPath = scene->GetDataFolderPath();

            // Convert CSG mesh data to common storage type
            ModelData modelData;
            meshData.ToModelData(modelData);

            // Convert CSG mesh to the local transformation of the scene
            if (!scene->GetTransform().IsIdentity())
            {
                Matrix t;
                scene->GetWorldToLocalMatrix(t);
                modelData.TransformBuffer(t);
            }

            // Import model data to the asset
            {
                Guid modelDataAssetId = scene->CSGData.Model.GetID();
                if (!modelDataAssetId.IsValid())
                    modelDataAssetId = Guid::New();
                const String modelDataAssetPath = sceneDataFolderPath / TEXT("CSG_Mesh") + ASSET_FILES_EXTENSION_WITH_DOT;
                if (AssetsImportingManager::Create(AssetsImportingManager::CreateModelTag, modelDataAssetPath, modelDataAssetId, &modelData))
                {
                    LOG(Warning, "Failed to import CSG mesh data");
                    return true;
                }
                data.outputModelAssetId = modelDataAssetId;
            }

            // Generate asset with CSG mesh metadata (for collisions and brush queries)
            {
                Guid rawDataAssetId = scene->CSGData.Data.GetID();
                if (!rawDataAssetId.IsValid())
                    rawDataAssetId = Guid::New();
                const String rawDataAssetPath = sceneDataFolderPath / TEXT("CSG_Data") + ASSET_FILES_EXTENSION_WITH_DOT;
                if (generateRawDataAsset(scene, meshData, rawDataAssetId, rawDataAssetPath))
                {
                    LOG(Warning, "Failed to create raw CSG data");
                    return true;
                }
                data.outputRawDataAssetId = rawDataAssetId;
            }

            // Generate CSG mesh collision asset
            {
                // Convert CSG mesh to scene local space (fix issues when scene has transformation applied)
                if (!scene->GetTransform().IsIdentity())
                {
                    Matrix m1, m2;
                    scene->GetTransform().GetWorld(m1);
                    Matrix::Invert(m1, m2);

                    for (int32 lodIndex = 0; lodIndex < modelData.LODs.Count(); lodIndex++)
                    {
                        auto lod = &modelData.LODs[lodIndex];
                        for (int32 meshIndex = 0; meshIndex < lod->Meshes.Count(); meshIndex++)
                        {
                            const auto v = &lod->Meshes[meshIndex]->Positions;
                            Vector3::Transform(v->Get(), m2, v->Get(), v->Count());
                        }
                    }
                }

#if COMPILE_WITH_PHYSICS_COOKING
                CollisionCooking::Argument arg;
                arg.Type = CollisionDataType::TriangleMesh;
                arg.OverrideModelData = &modelData;
                Guid collisionDataAssetId = scene->CSGData.CollisionData.GetID();
                if (!collisionDataAssetId.IsValid())
                    collisionDataAssetId = Guid::New();
                const String collisionDataAssetPath = sceneDataFolderPath / TEXT("CSG_Collision") + ASSET_FILES_EXTENSION_WITH_DOT;
                if (AssetsImportingManager::Create(AssetsImportingManager::CreateCollisionDataTag, collisionDataAssetPath, collisionDataAssetId, &arg))
                {
                    LOG(Warning, "Failed to cook CSG mesh collision data");
                    return true;
                }
                data.outputCollisionDataAssetId = collisionDataAssetId;
#else
                data.outputCollisionDataAssetId = Guid::Empty;
#endif
            }
        }
    }

    return false;
}

void CSGBuilderImpl::build(Scene* scene)
{
    // Start
    auto startTime = DateTime::Now();
    LOG(Info, "Start building CSG...");

    // Build
    BuildData data;
    bool failed = buildInner(scene, data);

    // Link new (or empty) CSG mesh
    scene->CSGData.Data = Content::LoadAsync<RawDataAsset>(data.outputRawDataAssetId);
    scene->CSGData.Model = Content::LoadAsync<Model>(data.outputModelAssetId);
    scene->CSGData.CollisionData = Content::LoadAsync<CollisionData>(data.outputCollisionDataAssetId);
    // TODO: also set CSGData.InstanceBuffer - lightmap scales for the entries so csg mesh gets better quality in lightmaps
    scene->CSGData.PostCSGBuild();

    // End
    data.meshes.ClearDelete();
    auto endTime = DateTime::Now();
    LOG(Info, "CSG build in {0} ms! {1} brush(es)", (endTime - startTime).GetTotalMilliseconds(), data.meshes.Count());
}

bool CSGBuilderImpl::generateRawDataAsset(Scene* scene, RawData& meshData, Guid& assetId, const String& assetPath)
{
    // Prepare data
    MemoryWriteStream stream(4096);
    {
        // Header (with version number)
        stream.WriteInt32(1);
        const int32 brushesCount = meshData.Brushes.Count();
        stream.WriteInt32(brushesCount);

        // Surfaces data locations
        int32 surfacesDataOffset = sizeof(int32) * 2 + (sizeof(Guid) + sizeof(int32)) * brushesCount;
        for (auto brush = meshData.Brushes.Begin(); brush.IsNotEnd(); ++brush)
        {
            auto& surfaces = brush->Value.Surfaces;

            stream.Write(&brush->Key);
            stream.WriteInt32(surfacesDataOffset);

            // Calculate offset in data storage to the next brush data
            int32 brushDataSize = 0;
            for (int32 i = 0; i < surfaces.Count(); i++)
            {
                brushDataSize += sizeof(int32) + sizeof(Triangle) * surfaces[i].Triangles.Count();
            }
            surfacesDataOffset += brushDataSize;
        }

        // Surfaces data
        for (auto brush = meshData.Brushes.Begin(); brush.IsNotEnd(); ++brush)
        {
            auto& surfaces = brush->Value.Surfaces;

            for (int32 i = 0; i < surfaces.Count(); i++)
            {
                auto& triangles = surfaces[i].Triangles;

                stream.WriteInt32(triangles.Count());
                stream.Write(triangles.Get(), triangles.Count());
            }
        }
    }

    // Serialize
    BytesContainer bytesContainer;
    bytesContainer.Link(stream.GetHandle(), stream.GetPosition());
    return AssetsImportingManager::Create(AssetsImportingManager::CreateRawDataTag, assetPath, assetId, (void*)&bytesContainer);
}

#endif
