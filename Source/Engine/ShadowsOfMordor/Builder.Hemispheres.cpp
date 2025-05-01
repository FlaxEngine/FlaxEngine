// Copyright (c) Wojciech Figat. All rights reserved.

#include "Builder.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Level/SceneQuery.h"
#include "Engine/Level/Actors/BoxBrush.h"
#include "Engine/ContentImporters/ImportTexture.h"

namespace
{
    void SampleCache(ShadowsOfMordor::GenerateHemispheresData& data, int32 texelX, int32 texelY, Float3& outPosition, Float3& outNormal)
    {
        const auto mipDataPositions = data.PositionsData.GetData(0, 0);
#if CACHE_POSITIONS_FORMAT == HEMISPHERES_FORMAT_R32G32B32A32
        outPosition = Float3(mipDataPositions->Get<Float4>(texelX, texelY));
#elif CACHE_POSITIONS_FORMAT == HEMISPHERES_FORMAT_R16G16B16A16
        outPosition = mipDataPositions->Get<Half4>(texelX, texelY).ToFloat3();
#else
#error "Unknown format."
#endif

        const auto mipDataNormals = data.NormalsData.GetData(0, 0);
#if CACHE_NORMALS_FORMAT == HEMISPHERES_FORMAT_R32G32B32A32
        outNormal = Float3(mipDataNormals->Get<Float4>(texelX, texelY));
#elif CACHE_NORMALS_FORMAT == HEMISPHERES_FORMAT_R16G16B16A16
        outNormal = mipDataNormals->Get<Half4>(texelX, texelY).ToFloat3();
#else
#error "Unknown format."
#endif
    }

    void RejectTexel(ShadowsOfMordor::GenerateHemispheresData& data, int32 texelX, int32 texelY)
    {
        const auto mipDataNormals = data.NormalsData.GetData(0, 0);
#if CACHE_NORMALS_FORMAT == HEMISPHERES_FORMAT_R32G32B32A32
        mipDataNormals->Get<Float4>(texelX, texelY) = Float4::Zero;
#elif CACHE_NORMALS_FORMAT == HEMISPHERES_FORMAT_R16G16B16A16
        mipDataNormals->Get<Half4>(texelX, texelY) = Half4::Zero;
#else
#error "Unknown format."
#endif
    }
}

void ShadowsOfMordor::Builder::generateHemispheres()
{
    reportProgress(BuildProgressStep::GenerateHemispheresCache, 0.0f);

    // Clear all lightmaps
    _workerStagePosition0 = 0;
    if (runStage(CleanLightmaps))
        return;

    auto scene = _scenes[_workerActiveSceneIndex];
    auto lightmapsCount = scene->Lightmaps.Count();
    auto& settings = scene->GetSettings();

    // Collect Hemispheres render tasks
    int32 hemispheresCount = 0, mergedHemispheresCount = 0;
    GenerateHemispheresData cacheData;

    // Config
    float normalizedQuality = Math::Saturate((float)settings.Quality / 100.0f);
    float maxMergeRadius = Math::Lerp(5.0f, 1.0f, normalizedQuality) / LightmapTexelsPerWorldUnit;
    float normalSimilarityMin = Math::Lerp(0.8f, 0.95f, normalizedQuality);
    int32 maxTexelsDistance = static_cast<int32>(Math::Lerp(2.0f, 1.0f, normalizedQuality));
    int32 atlasSize = static_cast<int32>(settings.AtlasSize);

    // Process every lightmap
    for (_workerStagePosition0 = 0; _workerStagePosition0 < lightmapsCount; _workerStagePosition0++)
    {
        // Prepare
        auto& lightmapEntry = scene->Lightmaps[_workerStagePosition0];
        lightmapEntry.Hemispheres.Clear();
        lightmapEntry.Hemispheres.EnsureCapacity(Math::Square(atlasSize / 2));
        Float3 position, normal;

        // Fill cache
        if (runStage(RenderCache))
            return;
        if (waitForJobDataSync())
            return;

        // Post-process cache
        if (runStage(PostprocessCache))
            return;

        // Wait for GPU commands to sync
        if (waitForJobDataSync())
            return;
        if (checkBuildCancelled())
            return;

        // Download cache to CPU memory from GPU memory
        if (_cachePositions->DownloadData(cacheData.PositionsData)
            || _cacheNormals->DownloadData(cacheData.NormalsData))
        {
            LOG(Fatal, "Cannot download data from the GPU. Target: ShadowsOfMordor::Builder::RenderPositionsAndNormals");
            return;
        }
        if (checkBuildCancelled())
            return;

#if DEBUG_EXPORT_CACHE_PREVIEW
        // Here we can export cache to drive
        exportCachePreview(scene, cacheData, lightmapEntry);
#endif

        // For each texel
        for (int32 texelX = 0; texelX < atlasSize; texelX++)
        {
            for (int32 texelY = 0; texelY < atlasSize; texelY++)
            {
                // Sample cache for current texel
                SampleCache(cacheData, texelX, texelY, position, normal);

                // Reject 'empty' texels
                if (normal.IsZero())
                    continue;
                normal.Normalize();

                // Try to merge similar hemispheres (threshold values are controlled by the quality slider)
                int32 mergedCount = 1;
                Float3 mergedSumPos = position, mergedSumNorm = normal;
                for (int32 x = -maxTexelsDistance; x <= maxTexelsDistance; x++)
                {
                    for (int32 y = -maxTexelsDistance; y <= maxTexelsDistance; y++)
                    {
                        int32 xx = Math::Clamp(x + texelX, 0, atlasSize - 1);
                        int32 yy = Math::Clamp(y + texelY, 0, atlasSize - 1);

                        // Skip current texel
                        if (xx == texelX && yy == texelY)
                            continue;

                        // Sample cache for possible to use texel
                        Float3 pp, nn;
                        SampleCache(cacheData, xx, yy, pp, nn);
                        nn.Normalize();

                        if (Float3::Distance(position, pp) <= maxMergeRadius
                            && Float3::Dot(normal, nn) >= normalSimilarityMin)
                        {
                            // Merge them!
                            mergedCount++;
                            mergedSumPos += pp;
                            mergedSumNorm += nn;

                            // Remove this hemisphere
                            RejectTexel(cacheData, xx, yy);
                        }
                    }
                }
                if (mergedCount > 1)
                    mergedHemispheresCount += mergedCount;

                // TODO: check if we need to use avg pos and normal?
                //position = mergedSumPos / mergedCount;
                //normal = mergedSumNorm / mergedCount;
                //normal.Normalize();

                // Enqueue hemisphere data to perform batched rendering
                HemisphereData data;
                data.Position = position;
                data.Normal = normal;
                data.TexelX = texelX;
                data.TexelY = texelY;
                lightmapEntry.Hemispheres.Add(data);
                hemispheresCount++;
            }
        }

        // Progress Point
        reportProgress(BuildProgressStep::GenerateHemispheresCache, (float)_workerStagePosition0 / lightmapsCount);
        if (checkBuildCancelled())
            return;
    }

    // Update stats
    scene->HemispheresCount = hemispheresCount;
    scene->MergedHemispheresCount = mergedHemispheresCount;

    reportProgress(BuildProgressStep::GenerateHemispheresCache, 1.0f);
}
