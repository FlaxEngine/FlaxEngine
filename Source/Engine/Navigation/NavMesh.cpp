// Copyright (c) Wojciech Figat. All rights reserved.

#include "NavMesh.h"
#include "NavMeshRuntime.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"
#if COMPILE_WITH_ASSETS_IMPORTER
#include "Engine/Core/Log.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif
#endif

NavMesh::NavMesh(const SpawnParams& params)
    : Actor(params)
    , IsDataDirty(false)
{
    DataAsset.Loaded.Bind<NavMesh, &NavMesh::OnDataAssetLoaded>(this);
}

void NavMesh::SaveNavMesh()
{
#if COMPILE_WITH_ASSETS_IMPORTER

    // Skip if scene is missing
    const auto scene = GetScene();
    if (!scene)
        return;

#if USE_EDITOR
    // Skip if game is running in editor (eg. game scripts update dynamic navmesh)
    if (Editor::IsPlayMode)
        return;
#endif

    // Clear flag
    IsDataDirty = false;

    // Check if has no navmesh data generated (someone could just remove navmesh volumes or generate for empty scene)
    if (Data.Tiles.IsEmpty())
    {
        DataAsset = nullptr;
        return;
    }

    // Prepare
    Guid assetId = DataAsset.GetID();
    if (!assetId.IsValid())
        assetId = Guid::New();
    const String assetPath = scene->GetDataFolderPath() / TEXT("NavMesh") + Properties.Name + ASSET_FILES_EXTENSION_WITH_DOT;

    // Generate navmesh tiles data
    const int32 streamInitialCapacity = Math::RoundUpToPowerOf2((Data.Tiles.Count() + 1) * 1024);
    MemoryWriteStream stream(streamInitialCapacity);
    Data.Save(stream);
    BytesContainer bytesContainer;
    bytesContainer.Link(ToSpan(stream));

    // Save asset to file
    if (AssetsImportingManager::Create(AssetsImportingManager::CreateRawDataTag, assetPath, assetId, (void*)&bytesContainer))
    {
        LOG(Warning, "Failed to save navmesh tiles data to file.");
        return;
    }

    // Link the created asset
    DataAsset = assetId;

#endif
}

void NavMesh::ClearData()
{
    if (Data.Tiles.HasItems())
    {
        IsDataDirty = true;
        Data.TileSize = 0.0f;
        Data.Tiles.Resize(0);
    }
}

NavMeshRuntime* NavMesh::GetRuntime(bool createIfMissing) const
{
    return NavMeshRuntime::Get(Properties, createIfMissing);
}

void NavMesh::AddTiles()
{
    auto navMesh = NavMeshRuntime::Get(Properties, true);
    navMesh->AddTiles(this);
}

void NavMesh::RemoveTiles()
{
    auto navMesh = NavMeshRuntime::Get(Properties, false);
    if (navMesh)
        navMesh->RemoveTiles(this);
}

void NavMesh::OnDataAssetLoaded()
{
    // Skip if already has data (prevent reloading navmesh on saving)
    if (Data.Tiles.HasItems())
        return;
    ScopeLock lock(DataAsset->Locker);

    // Remove added tiles
    if (_navMeshActive)
    {
        RemoveTiles();
    }

    // Load navmesh tiles
    BytesContainer data;
    data.Link(DataAsset->Data);
    Data.Load(data, false);
    IsDataDirty = false;

    // Add loaded tiles
    if (_navMeshActive)
    {
        AddTiles();
    }
}

void NavMesh::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

#if USE_EDITOR
    // Save navmesh tiles to asset (if modified)
    if (IsDataDirty)
        SaveNavMesh();
#endif

    SERIALIZE_GET_OTHER_OBJ(NavMesh);
    SERIALIZE(DataAsset);
    SERIALIZE(Properties);
}

void NavMesh::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(DataAsset);
    DESERIALIZE(Properties);
}

void NavMesh::OnEnable()
{
    // Base
    Actor::OnEnable();

    if (!_navMeshActive)
    {
        GetScene()->Navigation.Meshes.Add(this);
        AddTiles();
        _navMeshActive = true;
    }
}

void NavMesh::OnDisable()
{
    if (_navMeshActive)
    {
        RemoveTiles();
        GetScene()->Navigation.Meshes.Remove(this);
        _navMeshActive = false;
    }

    // Base
    Actor::OnDisable();
}

void NavMesh::Initialize()
{
    // Base
    Actor::Initialize();

    if (!_navMeshActive && IsActiveInHierarchy())
    {
        GetScene()->Navigation.Meshes.Add(this);
        AddTiles();
        _navMeshActive = true;
    }
}
