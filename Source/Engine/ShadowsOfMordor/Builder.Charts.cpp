// Copyright (c) Wojciech Figat. All rights reserved.

#include "Builder.h"
#include "AtlasChartsPacker.h"
#include "Engine/Level/Scene/SceneLightmapsData.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/ContentImporters/ImportTexture.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Level/Scene/Lightmap.h"

bool ShadowsOfMordor::Builder::sortCharts(const LightmapUVsChart& a, const LightmapUVsChart& b)
{
    // Sort by area
    return (b.Width * b.Height) < (a.Width * a.Height);
}

void ShadowsOfMordor::Builder::generateCharts()
{
    reportProgress(BuildProgressStep::GenerateLightmapCharts, 0.0f);

    auto scene = _scenes[_workerActiveSceneIndex];
    auto& settings = scene->GetSettings();
    ScopeLock lock(scene->EntriesLocker);

    // Generate lightmap UVs charts
    const int32 entriesCount = scene->Entries.Count();
    scene->Charts.EnsureCapacity(entriesCount);
    const int32 MaximumChartSize = (int32)settings.AtlasSize - settings.ChartsPadding * 2;
    for (int32 i = 0; i < entriesCount; i++)
    {
        LightmapUVsChart chart;
        chart.Result.TextureIndex = INVALID_INDEX;

        GeometryEntry& entry = scene->Entries[i];
        entry.ChartIndex = INVALID_INDEX;

        // Calculate desired area for the entry's chart (based on object dimensions and settings)
        // Reject missing models or too small objects
        Float3 size = entry.Box.GetSize();
        float dimensionsCoeff = size.AverageArithmetic();
        if (size.X <= 1.0f)
            dimensionsCoeff = Float2(size.Y, size.Z).AverageArithmetic();
        else if (size.Y <= 1.0f)
            dimensionsCoeff = Float2(size.X, size.Z).AverageArithmetic();
        else if (size.Z <= 1.0f)
            dimensionsCoeff = Float2(size.Y, size.X).AverageArithmetic();
        const float scale = settings.GlobalObjectsScale * entry.Scale * LightmapTexelsPerWorldUnit * dimensionsCoeff;
        if (scale <= ZeroTolerance)
            continue;

        // Apply lightmap uvs bounding box (in uv space) to reduce waste of lightmap atlas space
        chart.Width = Math::Clamp(Math::CeilToInt(scale * entry.UVsBox.GetWidth()), LightmapMinChartSize, MaximumChartSize);
        chart.Height = Math::Clamp(Math::CeilToInt(scale * entry.UVsBox.GetHeight()), LightmapMinChartSize, MaximumChartSize);

        // Register lightmap atlas chart entry
        chart.EntryIndex = i;
        scene->Charts.Add(chart);

        // Progress Point
        reportProgress(BuildProgressStep::GenerateLightmapCharts, static_cast<float>(i) / entriesCount);
    }

    reportProgress(BuildProgressStep::GenerateLightmapCharts, 1.0f);
}

void ShadowsOfMordor::Builder::packCharts()
{
    reportProgress(BuildProgressStep::PackLightmapCharts, 0.0f);
    auto scene = _scenes[_workerActiveSceneIndex];

    // Pack UV charts into atlases
    Array<AtlasChartsPacker*> packers;
    if (scene->Charts.HasItems())
    {
        // Sort charts from the biggest to the smallest
        Sorting::QuickSort(scene->Charts.Get(), scene->Charts.Count(), &sortCharts);

        reportProgress(BuildProgressStep::PackLightmapCharts, 0.1f);

        // Cache charts indices after sorting operation
        scene->EntriesLocker.Lock();
        for (int32 chartIndex = 0; chartIndex < scene->Charts.Count(); chartIndex++)
        {
            auto& chart = scene->Charts[chartIndex];
            scene->Entries[chart.EntryIndex].ChartIndex = chartIndex;
        }
        scene->EntriesLocker.Unlock();

        reportProgress(BuildProgressStep::PackLightmapCharts, 0.5f);

        // Pack all the charts
        for (int32 i = 0; i < scene->Charts.Count(); i++)
        {
            auto chart = &scene->Charts[i];

            bool cannotPack = true;
            for (int32 j = 0; j < packers.Count(); j++)
            {
                if (packers[j]->Insert(chart))
                {
                    chart->Result.TextureIndex = j;
                    cannotPack = false;
                    break;
                }
            }

            if (cannotPack)
            {
                auto packer = New<AtlasChartsPacker>(&scene->GetSettings());
                auto result = packer->Insert(chart);
                ASSERT(result);
                chart->Result.TextureIndex = packers.Count();
                packers.Add(packer);
            }
        }
    }
    const int32 lightmapsCount = scene->LightmapsCount = packers.Count();
    LOG(Info, "Scene \'{0}\': building {1} lightmap(s) ({2} chart(s) to bake)...", scene->Scene->GetName(), lightmapsCount, scene->Charts.Count());
    packers.ClearDelete();

    // Progress Point
    reportProgress(BuildProgressStep::PackLightmapCharts, 1.0f);
}

void ShadowsOfMordor::Builder::updateLightmaps()
{
    reportProgress(BuildProgressStep::UpdateLightmapsCollection, 0.0f);

    auto scene = _scenes[_workerActiveSceneIndex];
    auto& settings = scene->GetSettings();
    const int32 lightmapsCount = scene->LightmapsCount;

    // Update lightmaps collection
    scene->Scene->LightmapsData.UpdateLightmapsCollection(lightmapsCount, (int32)settings.AtlasSize);
    scene->Lightmaps.Resize(lightmapsCount, false);
    for (int32 lightmapIndex = 0; lightmapIndex < lightmapsCount; lightmapIndex++)
    {
        if (scene->Lightmaps[lightmapIndex].Init(&settings))
            return;
    }

    // Wait for all lightmaps to be ready (after creating new lightmaps assets we need to wait for resources to be prepared)
    GPUDevice::Instance->Locker.Lock();
    for (int32 lightmapIndex = 0; lightmapIndex < lightmapsCount; lightmapIndex++)
    {
        Texture* textures[NUM_SH_TARGETS];
        scene->Scene->LightmapsData.GetLightmap(lightmapIndex)->GetTextures(textures);
        for (int32 textureIndex = 0; textureIndex < NUM_SH_TARGETS; textureIndex++)
        {
            auto texture = textures[textureIndex];
            GPUDevice::Instance->Locker.Unlock();
            if (texture == nullptr || texture->WaitForLoaded())
            {
                LOG(Error, "Lightmap load failed.");
                return;
            }
            GPUDevice::Instance->Locker.Lock();
        }

        reportProgress(BuildProgressStep::UpdateLightmapsCollection, (float)lightmapIndex / lightmapsCount);
    }
    GPUDevice::Instance->Locker.Unlock();

    reportProgress(BuildProgressStep::UpdateLightmapsCollection, 1.0f);
}
