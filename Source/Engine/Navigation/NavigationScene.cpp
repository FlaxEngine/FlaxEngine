// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "NavigationScene.h"
#include "NavMesh.h"
#include "Navigation.h"
#include "NavMeshBoundsVolume.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Content/Assets/RawDataAsset.h"
#include "Engine/Core/Log.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif
#if COMPILE_WITH_ASSETS_IMPORTER
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#endif

NavigationScene::NavigationScene(::Scene* scene)
    : Scene(scene)
    , IsDataDirty(false)
{
    DataAsset.Loaded.Bind<NavigationScene, &NavigationScene::OnDataAssetLoaded>(this);
}

BoundingBox NavigationScene::GetNavigationBounds()
{
    if (Volumes.IsEmpty())
        return BoundingBox::Empty;

    PROFILE_CPU_NAMED("GetNavigationBounds");

    auto box = Volumes[0]->GetBox();
    for (int32 i = 1; i < Volumes.Count(); i++)
        BoundingBox::Merge(box, Volumes[i]->GetBox(), box);
    return box;
}

NavMeshBoundsVolume* NavigationScene::FindNavigationBoundsOverlap(const BoundingBox& bounds)
{
    NavMeshBoundsVolume* result = nullptr;

    for (int32 i = 0; i < Volumes.Count(); i++)
    {
        if (Volumes[i]->GetBox().Intersects(bounds))
        {
            result = Volumes[i];
            break;
        }
    }

    return result;
}

void NavigationScene::SaveNavMesh()
{
#if COMPILE_WITH_ASSETS_IMPORTER

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
        // Keep asset reference valid
        DataAsset.Unlink();
        return;
    }

    // Prepare
    Guid assetId = DataAsset.GetID();
    if (!assetId.IsValid())
        assetId = Guid::New();
    const String assetPath = Scene->GetDataFolderPath() / TEXT("NavMesh") + ASSET_FILES_EXTENSION_WITH_DOT;

    // Generate navmesh tiles data
    const int32 streamInitialCapacity = Math::RoundUpToPowerOf2((Data.Tiles.Count() + 1) * 1024);
    MemoryWriteStream stream(streamInitialCapacity);
    Data.Save(stream);
    BytesContainer bytesContainer;
    bytesContainer.Link(stream.GetHandle(), stream.GetPosition());

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

void NavigationScene::OnEnable()
{
    auto navMesh = Navigation::GetNavMesh();
    if (navMesh)
        navMesh->AddTiles(this);
}

void NavigationScene::OnDisable()
{
    auto navMesh = Navigation::GetNavMesh();
    if (navMesh)
        navMesh->RemoveTiles(this);
}

void NavigationScene::OnDataAssetLoaded()
{
    // Skip if already has data (prevent reloading navmesh on saving)
    if (Data.Tiles.HasItems())
        return;

    const bool isEnabled = Scene->IsDuringPlay() && Scene->IsActiveInHierarchy();

    // Remove added tiles
    if (isEnabled)
    {
        OnDisable();
    }

    // Load navmesh tiles
    BytesContainer data;
    data.Link(DataAsset->Data);
    Data.Load(data, false);
    IsDataDirty = false;

    // Add loaded tiles
    if (isEnabled)
    {
        OnEnable();
    }
}
