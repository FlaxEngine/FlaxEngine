// Copyright (c) Wojciech Figat. All rights reserved.

#include "Builder.h"
#include "Engine/Level/Scene/Lightmap.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Content/Content.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/ContentImporters/ImportTexture.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUBufferDescription.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

ShadowsOfMordor::Builder::LightmapBuildCache::~LightmapBuildCache()
{
    SAFE_DELETE_GPU_RESOURCE(LightmapData);
}

bool ShadowsOfMordor::Builder::LightmapBuildCache::Init(const LightmapSettings* settings)
{
    if (LightmapData)
        return false;
    LightmapData = GPUDevice::Instance->CreateBuffer(TEXT("LightmapBuildCache"));
    const auto elementsCount = (int32)settings->AtlasSize * (int32)settings->AtlasSize * NUM_SH_TARGETS;
    if (LightmapData->Init(GPUBufferDescription::Typed(elementsCount, HemispheresFormatToPixelFormat[HEMISPHERES_IRRADIANCE_FORMAT], true)))
        return true;
    return false;
}

ShadowsOfMordor::Builder::SceneBuildCache::SceneBuildCache()
    : Scene(nullptr)
    , TempLightmapData(nullptr)
    , LightmapsCount(0)
    , HemispheresCount(0)
    , MergedHemispheresCount(0)
    , ImportLightmapIndex(0)
    , ImportLightmapTextureIndex(0)
{
}

const LightmapSettings& ShadowsOfMordor::Builder::SceneBuildCache::GetSettings() const
{
    ASSERT(Scene);
    return Scene->Info.LightmapSettings;
}

bool ShadowsOfMordor::Builder::SceneBuildCache::WaitForLightmaps()
{
    Texture* lightmaps[3];
    for (int32 lightmapIndex = 0; lightmapIndex < Lightmaps.Count(); lightmapIndex++)
    {
        const auto lightmap = Scene->LightmapsData.GetLightmap(lightmapIndex);
        lightmap->GetTextures(lightmaps);

        for (int32 textureIndex = 0; textureIndex < NUM_SH_TARGETS; textureIndex++)
        {
            AssetReference<Texture> lightmapTexture = lightmaps[textureIndex];
            if (lightmapTexture == nullptr)
            {
                LOG(Error, "Missing lightmap {0} texture{1}", lightmapIndex, textureIndex);
                return true;
            }

            // Wait for loading end and check result
            if (lightmapTexture->WaitForLoaded())
            {
                LOG(Error, "Failed to load lightmap {0} texture {1}", lightmapIndex, textureIndex);
                return true;
            }

            // TODO: disable streaming for lightmap texture here (enable it later after baking)

            // Wait texture to be streamed to the target quality
            const auto t = lightmapTexture->GetTexture();
            const int32 stepSize = 30;
            const int32 maxWaitTime = 60000;
            int32 stepsCount = static_cast<int32>(maxWaitTime / stepSize);
            while ((t->ResidentMipLevels() < t->MipLevels() || t->ResidentMipLevels() == 0) && stepsCount-- > 0)
                Platform::Sleep(stepSize);
            if (t->ResidentMipLevels() < t->MipLevels() || t->ResidentMipLevels() == 0)
            {
                LOG(Error, "Waiting for lightmap no. {0} texture {1} to be fully resided timed out (loaded mips: {2}, mips count: {3})", lightmapIndex, textureIndex, t->ResidentMipLevels(), t->MipLevels());
                return true;
            }
        }
    }

    return false;
}

void ShadowsOfMordor::Builder::SceneBuildCache::UpdateLightmaps()
{
    Texture* lightmaps[3];
    for (int32 lightmapIndex = 0; lightmapIndex < Lightmaps.Count(); lightmapIndex++)
    {
        // Cache data
        auto& lightmapEntry = Lightmaps[lightmapIndex];
        auto lightmap = Scene->LightmapsData.GetLightmap(lightmapIndex);
        ASSERT(lightmap);
        lightmap->GetTextures(lightmaps);

        // Download buffer data
        if (lightmapEntry.LightmapData->DownloadData(ImportLightmapTextureData))
        {
            LOG(Error, "Cannot download LightmapData.");
            return;
        }

        // Import all textures but don't use file proxy to improve performance
        for (int32 textureIndex = 0; textureIndex < NUM_SH_TARGETS; textureIndex++)
        {
            // Get asset name
            String assetPath;
            if (lightmaps[textureIndex])
                assetPath = lightmaps[textureIndex]->GetPath();
            else
                Scene->LightmapsData.GetCachedLightmapPath(&assetPath, lightmapIndex, textureIndex);

            // Import texture with custom options
#if COMPILE_WITH_ASSETS_IMPORTER
            Guid id = Guid::Empty;
            ImportTexture::Options options;
            options.Type = TextureFormatType::HdrRGBA;
            options.IndependentChannels = true;
#if PLATFORM_WINDOWS
            options.Compress = Scene->GetLightmapSettings().CompressLightmaps;
#else
            options.Compress = false; // TODO: use better BC7 compressor that would handle alpha more precisely (otherwise lightmaps have artifacts, see TextureTool.stb.cpp)
#endif
            options.GenerateMipMaps = true;
            options.IsAtlas = false;
            options.sRGB = false;
            options.NeverStream = false;
            ImportLightmapIndex = lightmapIndex;
            ImportLightmapTextureIndex = textureIndex;
            options.InternalLoad.Bind<SceneBuildCache, &SceneBuildCache::onImportLightmap>(this);
            if (AssetsImportingManager::Create(AssetsImportingManager::CreateTextureTag, assetPath, id, &options))
            {
                LOG(Error, "Cannot create new lightmap {0}:{1}", lightmapIndex, textureIndex);
                return;
            }
            const auto result = Content::LoadAsync<Texture>(id);
            if (result == nullptr)
#else
#error "Cannot import lightmaps. Assets importer module iss missing."
            auto result = nullptr;
#endif
            {
                LOG(Error, "Cannot load new lightmap {0}:{1}", lightmapIndex, textureIndex);
                return;
            }

            // Update lightmap
            lightmap->UpdateTexture(result, textureIndex);
        }

#if DEBUG_EXPORT_LIGHTMAPS_PREVIEW
        // Temporary save lightmaps (after last bounce)
        if (Builder->_giBounceRunningIndex == Builder->_bounceCount - 1)
        {
            exportLightmapPreview(this, lightmapIndex);
        }
#endif

        ImportLightmapTextureData.Release();
    }
}

bool ShadowsOfMordor::Builder::SceneBuildCache::Init(ShadowsOfMordor::Builder* builder, int32 index, ::Scene* scene)
{
    Builder = builder;
    SceneIndex = index;
    Scene = scene;
    const int32 atlasSize = (int32)GetSettings().AtlasSize;
    TempLightmapData = GPUDevice::Instance->CreateBuffer(TEXT("LightmapBuildCache"));
    const auto elementsCount = atlasSize * atlasSize * NUM_SH_TARGETS;
    if (TempLightmapData->Init(GPUBufferDescription::Typed(elementsCount, HemispheresFormatToPixelFormat[HEMISPHERES_IRRADIANCE_FORMAT], true)))
        return true;

    LOG(Info, "Scene \'{0}\' quality: {1}", scene->GetName(), scene->Info.LightmapSettings.Quality);
    return false;
}

void ShadowsOfMordor::Builder::SceneBuildCache::Release()
{
    EntriesLocker.Lock();

    // Cleanup
    Entries.Resize(0);
    Lightmaps.Resize(0);
    Charts.Resize(0);
    Scene = nullptr;

    EntriesLocker.Unlock();

    SAFE_DELETE_GPU_RESOURCE(TempLightmapData);
}

#if COMPILE_WITH_ASSETS_IMPORTER

bool ShadowsOfMordor::Builder::SceneBuildCache::onImportLightmap(TextureData& image)
{
    // Cache data
    const int32 lightmapIndex = ImportLightmapIndex;
    const int32 textureIndex = ImportLightmapTextureIndex;

    // Setup image
    image.Width = image.Height = (int32)GetSettings().AtlasSize;
    image.Depth = 1;
    image.Format = HemispheresFormatToPixelFormat[HEMISPHERES_IRRADIANCE_FORMAT];
    image.Items.Resize(1);
    image.Items[0].Mips.Resize(1);
    auto& mip = image.Items[0].Mips[0];
    mip.RowPitch = PixelFormatExtensions::SizeInBytes(image.Format) * image.Width;
    mip.DepthPitch = mip.RowPitch * image.Height;
    mip.Lines = image.Height;
    mip.Data.Allocate(mip.DepthPitch);

#if HEMISPHERES_IRRADIANCE_FORMAT == HEMISPHERES_FORMAT_R32G32B32A32
    auto pos = (Float4*)mip.Data.Get();
    const auto textureData = ImportLightmapTextureData.Get<Float4>();
    for (int32 y = 0; y < image.Height; y++)
    {
        for (int32 x = 0; x < image.Width; x++)
        {
            const int32 texelAddress = (y * image.Width + x) * NUM_SH_TARGETS;
            *pos = textureData[texelAddress + textureIndex];
            pos++;
        }
    }
#elif HEMISPHERES_IRRADIANCE_FORMAT == HEMISPHERES_FORMAT_R16G16B16A16
    auto pos = (Half4*)mip.Data.Get();
    const auto textureData = ImportLightmapTextureData.Get<Half4>();
    for (int32 y = 0; y < image.Height; y++)
    {
        for (int32 x = 0; x < image.Width; x++)
        {
            const int32 texelAddress = (y * image.Width + x) * NUM_SH_TARGETS;
            *pos = textureData[texelAddress + textureIndex];
            pos++;
        }
    }
#endif

    return false;
}

#endif
