// Copyright (c) Wojciech Figat. All rights reserved.

#include "Builder.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Level/Actors/BoxBrush.h"
#include "Engine/Level/Actors/StaticModel.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Level.h"
#include "Engine/Terrain/Terrain.h"
#include "Engine/Terrain/TerrainPatch.h"
#include "Engine/Foliage/Foliage.h"
#include "Engine/Threading/Threading.h"

bool canUseMaterialWithLightmap(MaterialBase* material, ShadowsOfMordor::Builder::SceneBuildCache* scene)
{
    // Check objects with missing materials can be used
    if (material == nullptr)
        return scene->GetSettings().UseGeometryWithNoMaterials;

    return material->CanUseLightmap();
}

bool cacheStaticGeometryTree(Actor* actor, ShadowsOfMordor::Builder::SceneBuildCache* scene)
{
    ShadowsOfMordor::Builder::GeometryEntry entry;
    const bool useLightmap = actor->GetIsActive() && actor->HasStaticFlag(StaticFlags::Lightmap);
    auto& results = scene->Entries;

    // Switch actor type
    if (auto staticModel = dynamic_cast<StaticModel*>(actor))
    {
        // Check if model has been linked and is loaded
        auto model = staticModel->Model.Get();
        if (model && !model->WaitForLoaded())
        {
            entry.Type = ShadowsOfMordor::Builder::GeometryType::StaticModel;
            entry.UVsBox = Rectangle(Float2::Zero, Float2::One);
            entry.AsStaticModel.Actor = staticModel;
            entry.Scale = Math::Clamp(staticModel->GetScaleInLightmap(), 0.0f, LIGHTMAP_SCALE_MAX);

            // Use the first LOD
            const int32 lodIndex = 0;
            auto& lod = model->LODs[lodIndex];

            bool anyValid = false;
            for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
            {
                const auto& mesh = lod.Meshes[meshIndex];
                auto& bufferEntry = staticModel->Entries[mesh.GetMaterialSlotIndex()];
                if (bufferEntry.Visible && canUseMaterialWithLightmap(staticModel->GetMaterial(meshIndex), scene))
                {
                    if (mesh.HasLightmapUVs())
                    {
                        anyValid = true;
                    }
                    else
                    {
                        LOG(Warning, "Model \'{0}\' mesh index {1} (lod: {2}) has missing lightmap UVs (at actor: {3})", model->GetPath(), meshIndex, lodIndex, staticModel->GetNamePath());
                    }
                }
            }

            if (useLightmap && anyValid && entry.Scale > ZeroTolerance)
            {
                Matrix worldMatrix;
                staticModel->GetLocalToWorldMatrix(worldMatrix);
                entry.Box = model->GetBox(worldMatrix);
                results.Add(entry);
            }
            else
            {
                staticModel->RemoveLightmap();
            }
        }
    }
    else if (auto terrain = dynamic_cast<Terrain*>(actor))
    {
        entry.AsTerrain.Actor = terrain;
        entry.Type = ShadowsOfMordor::Builder::GeometryType::Terrain;
        entry.UVsBox = Rectangle(Float2::Zero, Float2::One);
        entry.Scale = Math::Clamp(terrain->GetScaleInLightmap(), 0.0f, LIGHTMAP_SCALE_MAX);
        for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
        {
            auto patch = terrain->GetPatch(patchIndex);
            entry.AsTerrain.PatchIndex = patchIndex;
            for (int32 chunkIndex = 0; chunkIndex < Terrain::ChunksCount; chunkIndex++)
            {
                auto chunk = patch->Chunks[chunkIndex];
                entry.AsTerrain.ChunkIndex = chunkIndex;
                auto material = chunk.OverrideMaterial ? chunk.OverrideMaterial.Get() : terrain->Material.Get();
                const bool canUseLightmap = useLightmap
                        && canUseMaterialWithLightmap(material, scene)
                        && entry.Scale > ZeroTolerance;
                if (canUseLightmap)
                {
                    entry.Box = chunk.GetBounds();
                    results.Add(entry);
                }
                else
                {
                    chunk.RemoveLightmap();
                }
            }
        }
    }
    else if (auto foliage = dynamic_cast<Foliage*>(actor))
    {
        entry.AsFoliage.Actor = foliage;
        entry.Type = ShadowsOfMordor::Builder::GeometryType::Foliage;
        entry.UVsBox = Rectangle(Float2::Zero, Float2::One);
        for (auto i = foliage->Instances.Begin(); i.IsNotEnd(); ++i)
        {
            auto& instance = *i;
            auto& type = foliage->FoliageTypes[instance.Type];
            entry.AsFoliage.InstanceIndex = i.Index();
            entry.AsFoliage.TypeIndex = instance.Type;
            entry.Scale = Math::Clamp(type.ScaleInLightmap, 0.0f, LIGHTMAP_SCALE_MAX);
            const bool canUseLightmap = useLightmap
                    && canUseMaterialWithLightmap(type.Entries[0].Material, scene)
                    && entry.Scale > ZeroTolerance;
            auto model = type.Model.Get();
            if (canUseLightmap && model && !model->WaitForLoaded())
            {
                BoundingBox::FromSphere(instance.Bounds, entry.Box);
                const int32 lodIndex = 0;
                auto& lod = model->LODs[lodIndex];
                for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
                {
                    entry.AsFoliage.MeshIndex = meshIndex;
                    results.Add(entry);
                }
            }
            else
            {
                instance.RemoveLightmap();
            }
        }
    }

    return actor->GetIsActive();
}

void ShadowsOfMordor::Builder::cacheEntries()
{
    auto scene = _scenes[_workerActiveSceneIndex];

    reportProgress(BuildProgressStep::CacheEntries, 0);
    scene->EntriesLocker.Lock();

    // Gather static scene geometry entries
    Function<bool(Actor*, SceneBuildCache*)> cacheGeometry = &cacheStaticGeometryTree;
    scene->Scene->TreeExecute(cacheGeometry, scene);

    scene->EntriesLocker.Unlock();
    reportProgress(BuildProgressStep::CacheEntries, 1.0f);
}

void ShadowsOfMordor::Builder::updateEntries()
{
    auto scene = _scenes[_workerActiveSceneIndex];

    reportProgress(BuildProgressStep::UpdateEntries, 0.0f);
    scene->EntriesLocker.Lock();

    // Update entries (and cache entries list per lightmap as linear array instead of tree structure)
    ScopeLock lock(Level::ScenesLock); // TODO: maybe don;t lock scene?
    const int32 entriesCount = scene->Entries.Count();
    for (int32 i = 0; i < entriesCount; i++)
    {
        auto& e = scene->Entries[i];
        if (e.ChartIndex == INVALID_INDEX)
        {
            // Removed previously baked lightmap info
            LightmapEntry emptyEntry;
            switch (e.Type)
            {
            case GeometryType::StaticModel:
            {
                auto staticModel = e.AsStaticModel.Actor;
                if (staticModel)
                    staticModel->Lightmap = emptyEntry;
            }
            break;
            case GeometryType::Terrain:
            {
                auto terrain = e.AsTerrain.Actor;
                if (terrain)
                    terrain->GetPatch(e.AsTerrain.PatchIndex)->Chunks[e.AsTerrain.ChunkIndex].Lightmap = emptyEntry;
            }
            break;
            case GeometryType::Foliage:
            {
                auto foliage = e.AsFoliage.Actor;
                if (foliage)
                    foliage->Instances[e.AsFoliage.InstanceIndex].Lightmap = emptyEntry;
            }
            break;
            }
            continue;
        }
        auto& chart = scene->Charts[e.ChartIndex];

        // Update result uvs by taking into account lightmap uvs box
        chart.Result.UVsArea.Size /= e.UVsBox.Size;
        chart.Result.UVsArea.Location += e.UVsBox.Location * chart.Result.UVsArea.Size;

        switch (e.Type)
        {
        case GeometryType::StaticModel:
        {
            auto staticModel = e.AsStaticModel.Actor;
            if (staticModel)
            {
                // Update data
                staticModel->Lightmap = chart.Result;
            }
            else
            {
                // Discard chart due to data leaks
                chart.Result.TextureIndex = INVALID_INDEX;
            }
        }
        break;
        case GeometryType::Terrain:
        {
            auto terrain = e.AsTerrain.Actor;
            if (terrain)
            {
                // Update data
                terrain->GetPatch(e.AsTerrain.PatchIndex)->Chunks[e.AsTerrain.ChunkIndex].Lightmap = chart.Result;
            }
            else
            {
                // Discard chart due to data leaks
                chart.Result.TextureIndex = INVALID_INDEX;
            }
        }
        break;
        case GeometryType::Foliage:
        {
            auto foliage = e.AsFoliage.Actor;
            if (foliage)
            {
                // Update data
                foliage->Instances[e.AsFoliage.InstanceIndex].Lightmap = chart.Result;
            }
            else
            {
                // Discard chart due to data leaks
                chart.Result.TextureIndex = INVALID_INDEX;
            }
        }
        break;
        }

        // Cache entry link
        if (chart.Result.TextureIndex != INVALID_INDEX)
            scene->Lightmaps[chart.Result.TextureIndex].Entries.Add(i);

        reportProgress(BuildProgressStep::UpdateEntries, static_cast<float>(i) / entriesCount);
    }

    scene->EntriesLocker.Unlock();
    reportProgress(BuildProgressStep::UpdateEntries, 1.0f);
}
