// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_ASSETS_EXPORTER

#include "AssetsExportingManager.h"
#include "AssetExporters.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Content/Content.h"
#include "Engine/Render2D/SpriteAtlas.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Engine/EngineService.h"

Dictionary<String, ExportAssetFunction> AssetsExportingManager::Exporters;

class AssetsExportingManagerService : public EngineService
{
public:
    AssetsExportingManagerService()
        : EngineService(TEXT("AssetsExportingManager"), -300)
    {
    }

    bool Init() override;
    void Dispose() override;
};

AssetsExportingManagerService AssetsExportingManagerServiceInstance;

ExportAssetContext::ExportAssetContext(const String& inputPath, const String& outputFolder, void* arg)
{
    InputPath = inputPath;
    OutputFilename = StringUtils::GetFileNameWithoutExtension(inputPath);
    OutputFolder = outputFolder;
    CustomArg = arg;
}

ExportAssetResult ExportAssetContext::Run(const ExportAssetFunction& callback)
{
    ASSERT(callback.IsBinded());

    // Check if input file exists
    if (!FileSystem::FileExists(InputPath))
        return ExportAssetResult::MissingInputFile;

    // Load asset (it will perform any required auto-conversions to have valid data)
    auto asset = Content::LoadAsync<::Asset>(InputPath);
    if (asset == nullptr || asset->WaitForLoaded())
        return ExportAssetResult::CannotLoadAsset;
    Asset = asset;

    // Call action
    return callback(*this);
}

const ExportAssetFunction* AssetsExportingManager::GetExporter(const String& typeName)
{
    return Exporters.TryGet(typeName);
}

bool AssetsExportingManager::CanExport(const String& inputPath)
{
    AssetInfo info;
    if (!Content::GetAssetInfo(inputPath, info))
        return false;

    return Exporters.ContainsKey(info.TypeName);
}

bool AssetsExportingManager::Export(const String& inputPath, const String& outputFolder, void* arg)
{
    AssetInfo info;
    if (!Content::GetAssetInfo(inputPath, info))
    {
        LOG(Warning, "Cannot find asset at location {0}", inputPath);
        return true;
    }

    ExportAssetFunction callback;
    if (!Exporters.TryGet(info.TypeName, callback))
    {
        LOG(Warning, "Cannot find exporter for the asset at location {0} (typename: {1})", inputPath, info.TypeName);
        return true;
    }

    return Export(callback, inputPath, outputFolder, arg);
}

bool AssetsExportingManager::Export(const ExportAssetFunction& callback, const String& inputPath, const String& outputFolder, void* arg)
{
    LOG(Info, "Exporting asset '{0}' to '{1}'...", inputPath, outputFolder);

    const auto startTime = DateTime::Now();

    ExportAssetContext context(inputPath, outputFolder, arg);
    const auto result = context.Run(callback);

    if (result != ExportAssetResult::Ok)
    {
        LOG(Error, "Asset exporting failed! Result: {0}", ::ToString(result));
        return true;
    }

    const auto endTime = DateTime::Now();
    const auto exportTime = endTime - startTime;
    LOG(Info, "Asset exported in {0}ms", Math::RoundToInt((float)exportTime.GetTotalMilliseconds()));

    return false;
}

bool AssetsExportingManagerService::Init()
{
    // Initialize with in-build exporters
    AssetsExportingManager::Exporters.Add(Texture::TypeName, AssetExporters::ExportTexture);
    AssetsExportingManager::Exporters.Add(SpriteAtlas::TypeName, AssetExporters::ExportTexture);
    AssetsExportingManager::Exporters.Add(CubeTexture::TypeName, AssetExporters::ExportCubeTexture);
    AssetsExportingManager::Exporters.Add(AudioClip::TypeName, AssetExporters::ExportAudioClip);
    AssetsExportingManager::Exporters.Add(Model::TypeName, AssetExporters::ExportModel);
    AssetsExportingManager::Exporters.Add(SkinnedModel::TypeName, AssetExporters::ExportSkinnedModel);

    return false;
}

void AssetsExportingManagerService::Dispose()
{
    // Cleanup
    AssetsExportingManager::Exporters.Clear();
    AssetsExportingManager::Exporters.SetCapacity(0);
}

#endif
