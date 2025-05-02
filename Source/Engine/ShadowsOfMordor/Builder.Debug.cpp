// Copyright (c) Wojciech Figat. All rights reserved.

#include "Builder.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/RenderTargetPool.h"
#if DEBUG_EXPORT_HEMISPHERES_PREVIEW
#include "Engine/Graphics/GPUContext.h"
#endif

#if DEBUG_EXPORT_LIGHTMAPS_PREVIEW || DEBUG_EXPORT_CACHE_PREVIEW || DEBUG_EXPORT_HEMISPHERES_PREVIEW

#include "Engine/Engine/Globals.h"

String GetDebugDataPath()
{
    auto result = Globals::ProjectCacheFolder / TEXT("ShadowsOfMordor_Debug");
    if (!FileSystem::DirectoryExists(result))
        FileSystem::CreateDirectory(result);
    return result;
}

#endif

#if DEBUG_EXPORT_LIGHTMAPS_PREVIEW

void ShadowsOfMordor::Builder::exportLightmapPreview(SceneBuildCache* scene, int32 lightmapIndex)
{
    auto settings = &scene->GetSettings();
    auto atlasSize = (int32)settings->AtlasSize;

    int32 bytesPerPixel = 3;
    int32 dataBmpSize = atlasSize * atlasSize * bytesPerPixel;
    byte* dataBmp = new byte[dataBmpSize];

    for (int sh = 0; sh < NUM_SH_TARGETS; sh++)
    {
        auto tmpPath = GetDebugDataPath() / String::Format(TEXT("Scene{2}_lighmap_{0}_{1}.bmp"), lightmapIndex, sh, scene->SceneIndex);

        for (int32 y = 0; y < atlasSize; y++)
        {
            for (int32 x = 0; x < atlasSize; x++)
            {
                const int32 pos = (y * atlasSize + x) * 3;
                const int32 texelAdress = ((atlasSize - y - 1) * atlasSize + x) * NUM_SH_TARGETS;

#if HEMISPHERES_IRRADIANCE_FORMAT == HEMISPHERES_FORMAT_R32G32B32A32
                auto textureData = scene->ImportLightmapTextureData.Get<Float4>();
                Color color = Color(Float4::Clamp(textureData[texelAdress + sh], Float4::Zero, Float4::One));
#elif HEMISPHERES_IRRADIANCE_FORMAT == HEMISPHERES_FORMAT_R16G16B16A16
                auto textureData = scene->ImportLightmapTextureData.Get<Half4>();
                Color color = Color(Float4::Clamp(textureData[texelAdress + sh].ToFloat4(), Float4::Zero, Float4::One));
#endif

                dataBmp[pos + 0] = static_cast<byte>(color.B * 255);
                dataBmp[pos + 1] = static_cast<byte>(color.G * 255);
                dataBmp[pos + 2] = static_cast<byte>(color.R * 255);
            }
        }
        FileSystem::SaveBitmapToFile(dataBmp, atlasSize, atlasSize, bytesPerPixel * 8, 0, tmpPath);
    }

    delete[] dataBmp;
}

#endif

#if DEBUG_EXPORT_CACHE_PREVIEW

void ShadowsOfMordor::Builder::exportCachePreview(SceneBuildCache* scene, GenerateHemispheresData& cacheData, LightmapBuildCache& lightmapEntry) const
{
    auto settings = &scene->GetSettings();
    auto atlasSize = (int32)settings->AtlasSize;

    int32 bytesPerPixel = 3;
    int32 dataSize = atlasSize * atlasSize * bytesPerPixel;
    byte* data = new byte[dataSize];

    {
        auto tmpPath = GetDebugDataPath() / String::Format(TEXT("Scene{1}_lightmapCache_{0}_Posiion.bmp"), _workerStagePosition0, scene->SceneIndex);
        auto mipData = cacheData.PositionsData.GetData(0, 0);

        for (int32 x = 0; x < cacheData.PositionsData.Width; x++)
        {
            for (int32 y = 0; y < cacheData.PositionsData.Height; y++)
            {
#if CACHE_POSITIONS_FORMAT == HEMISPHERES_FORMAT_R32G32B32A32
                Float3 color(mipData->Get<Float4>(x, y));
#elif CACHE_POSITIONS_FORMAT == HEMISPHERES_FORMAT_R16G16B16A16
                Float3 color = mipData->Get<Half4>(x, y).ToFloat3();
#endif
                color /= 100.0f;
                
                const int32 pos = ((cacheData.PositionsData.Height - y - 1) * cacheData.PositionsData.Width + x) * 3;
                data[pos + 0] = (byte)(color.Z * 255);
                data[pos + 1] = (byte)(color.Y * 255);
                data[pos + 2] = (byte)(color.X * 255);
            }
        }

        FileSystem::SaveBitmapToFile(data, atlasSize, atlasSize, bytesPerPixel * 8, 0, tmpPath);
    }

    {
        auto tmpPath = GetDebugDataPath() / String::Format(TEXT("Scene{1}_lightmapCache_{0}_Normal.bmp"), _workerStagePosition0, scene->SceneIndex);
        Platform::MemoryClear(data, dataSize);
        auto mipData = cacheData.NormalsData.GetData(0, 0);

        for (int32 x = 0; x < cacheData.NormalsData.Width; x++)
        {
            for (int32 y = 0; y < cacheData.NormalsData.Height; y++)
            {
#if CACHE_NORMALS_FORMAT == HEMISPHERES_FORMAT_R32G32B32A32
                Float3 color(mipData->Get<Float4>(x, y));
#elif CACHE_NORMALS_FORMAT == HEMISPHERES_FORMAT_R16G16B16A16
                Float3 color = mipData->Get<Half4>(x, y).ToFloat3();
#endif
                color.Normalize();

                const int32 pos = ((cacheData.NormalsData.Height - y - 1) * cacheData.NormalsData.Width + x) * 3;
                data[pos + 0] = (byte)(color.Z * 255);
                data[pos + 1] = (byte)(color.Y * 255);
                data[pos + 2] = (byte)(color.X * 255);
            }
        }

        FileSystem::SaveBitmapToFile(data, atlasSize, atlasSize, bytesPerPixel * 8, 0, tmpPath);
    }

    delete[] data;
}

#endif

#if DEBUG_EXPORT_HEMISPHERES_PREVIEW

int32 DebugExportHemispheresPerAtlasRow = 32;
int32 DebugExportHemispheresPerAtlas = DebugExportHemispheresPerAtlasRow * DebugExportHemispheresPerAtlasRow;
int32 DebugExportHemispheresAtlasSize = DebugExportHemispheresPerAtlasRow * HEMISPHERES_RESOLUTION;
int32 DebugExportHemispheresPosition = 0;
Array<GPUTexture*> DebugExportHemispheresAtlases;

void ShadowsOfMordor::Builder::addDebugHemisphere(GPUContext* context, GPUTextureView* hemisphere)
{
    // Get atlas
    if (DebugExportHemispheresAtlases.IsEmpty() || DebugExportHemispheresPosition >= DebugExportHemispheresPerAtlas)
    {
        DebugExportHemispheresPosition = 0;
        DebugExportHemispheresAtlases.Insert(0, RenderTargetPool::Get(GPUTextureDescription::New2D(DebugExportHemispheresAtlasSize, DebugExportHemispheresAtlasSize, PixelFormat::R32G32B32A32_Float)));
    }
    GPUTexture* atlas = DebugExportHemispheresAtlases[0];

    // Copy frame to atlas
    context->SetRenderTarget(atlas->View());
    const int32 x = (DebugExportHemispheresPosition % DebugExportHemispheresPerAtlasRow) * HEMISPHERES_RESOLUTION;
    const int32 y = (DebugExportHemispheresPosition / DebugExportHemispheresPerAtlasRow) * HEMISPHERES_RESOLUTION;
    context->SetViewportAndScissors(Viewport(static_cast<float>(x), static_cast<float>(y), HEMISPHERES_RESOLUTION, HEMISPHERES_RESOLUTION));
    context->Draw(hemisphere);

    // Move
    DebugExportHemispheresPosition++;
}

void ShadowsOfMordor::Builder::downloadDebugHemisphereAtlases(SceneBuildCache* scene)
{
    for (int32 atlasIndex = 0; atlasIndex < DebugExportHemispheresAtlases.Count(); atlasIndex++)
    {
        GPUTexture* atlas = DebugExportHemispheresAtlases[atlasIndex];

        TextureData textureData;
        if (atlas->DownloadData(textureData))
        {
            LOG(Error, "Cannot download hemispheres atlas data.");
            continue;
        }

        {
            auto tmpPath = GetDebugDataPath() / String::Format(TEXT("Scene{1}_hemispheresAtlas_{0}.bmp"), atlasIndex, scene->SceneIndex);

            int32 bytesPerPixel = 3;
            int32 dataSize = DebugExportHemispheresAtlasSize * DebugExportHemispheresAtlasSize * bytesPerPixel;
            byte* data = new byte[dataSize];
            Platform::MemoryClear(data, dataSize);

            auto mipData = textureData.GetData(0, 0);
            auto dddd = (Float4*)mipData->Data.Get();

            for (int x = 0; x < textureData.Width; x++)
            {
                for (int y = 0; y < textureData.Height; y++)
                {
                    int pos = ((textureData.Height - y - 1) * textureData.Width + x) * 3;
                    int srcPos = (y * textureData.Width + x);

                    Float4 color = Float4::Clamp(dddd[srcPos], Float4::Zero, Float4::One);

                    data[pos + 0] = (byte)(color.Z * 255);
                    data[pos + 1] = (byte)(color.Y * 255);
                    data[pos + 2] = (byte)(color.X * 255);
                }
            }

            FileSystem::SaveBitmapToFile(data, DebugExportHemispheresAtlasSize, DebugExportHemispheresAtlasSize, bytesPerPixel * 8, 0, tmpPath);
            delete[] data;
        }
    }
}

void ShadowsOfMordor::Builder::releaseDebugHemisphereAtlases()
{
    DebugExportHemispheresPosition = 0;
    for (int32 i = 0; i < DebugExportHemispheresAtlases.Count(); i++)
    {
        RenderTargetPool::Release(DebugExportHemispheresAtlases[i]);
    }
    DebugExportHemispheresAtlases.Clear();
}

#endif
