// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "DeployDataStep.h"
#include "Engine/Platform/FileSystem.h"
#include "Editor/Cooker/PlatformTools.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Renderer/ReflectionsPass.h"
#include "Engine/Renderer/AntiAliasing/SMAA.h"

bool DeployDataStep::Perform(CookingData& data)
{
    data.StepProgress(TEXT("Deploying engine data"), 0);
    const String depsRoot = data.GetPlatformBinariesRoot();
    const auto gameSettings = GameSettings::Get();

    // Setup output folders and copy required data
    const auto contentDir = data.OutputPath / TEXT("Content");
    if (FileSystem::DirectoryExists(contentDir))
    {
        // Remove old content files
        FileSystem::DeleteDirectory(contentDir, true);

        // Give some time for Explorer (if location was viewed)
        Platform::Sleep(10);
    }
    FileSystem::CreateDirectory(contentDir);
    const auto srcMono = depsRoot / TEXT("Mono");
    const auto dstMono = data.OutputPath / TEXT("Mono");
    if (!FileSystem::DirectoryExists(dstMono))
    {
        if (!FileSystem::DirectoryExists(srcMono))
        {
            data.Error(TEXT("Missing Mono runtime data files."));
            return true;
        }

        if (FileSystem::CopyDirectory(dstMono, srcMono, true))
        {
            data.Error(TEXT("Failed to copy Mono runtime data files."));
            return true;
        }
    }

    // Deploy engine data for the target platform
    if (data.Tools->OnDeployBinaries(data))
        return true;

    // Register engine in-build assets
    data.AddRootEngineAsset(TEXT("Shaders/AtmospherePreCompute"));
    data.AddRootEngineAsset(TEXT("Shaders/ColorGrading"));
    data.AddRootEngineAsset(TEXT("Shaders/DebugDraw"));
    data.AddRootEngineAsset(TEXT("Shaders/DepthOfField"));
    data.AddRootEngineAsset(TEXT("Shaders/EyeAdaptation"));
    data.AddRootEngineAsset(TEXT("Shaders/Fog"));
    data.AddRootEngineAsset(TEXT("Shaders/Forward"));
    data.AddRootEngineAsset(TEXT("Shaders/FXAA"));
    data.AddRootEngineAsset(TEXT("Shaders/TAA"));
    data.AddRootEngineAsset(TEXT("Shaders/SMAA"));
    data.AddRootEngineAsset(TEXT("Shaders/GBuffer"));
    data.AddRootEngineAsset(TEXT("Shaders/GUI"));
    data.AddRootEngineAsset(TEXT("Shaders/Histogram"));
    data.AddRootEngineAsset(TEXT("Shaders/Lights"));
    data.AddRootEngineAsset(TEXT("Shaders/MultiScaler"));
    data.AddRootEngineAsset(TEXT("Shaders/PostProcessing"));
    data.AddRootEngineAsset(TEXT("Shaders/MotionBlur"));
    data.AddRootEngineAsset(TEXT("Shaders/BitonicSort"));
    data.AddRootEngineAsset(TEXT("Shaders/GPUParticlesSorting"));
    data.AddRootEngineAsset(TEXT("Shaders/Quad"));
    data.AddRootEngineAsset(TEXT("Shaders/Reflections"));
    data.AddRootEngineAsset(TEXT("Shaders/Shadows"));
    data.AddRootEngineAsset(TEXT("Shaders/Sky"));
    data.AddRootEngineAsset(TEXT("Shaders/SSAO"));
    data.AddRootEngineAsset(TEXT("Shaders/SSR"));
    data.AddRootEngineAsset(TEXT("Shaders/VolumetricFog"));
    data.AddRootEngineAsset(TEXT("Engine/DefaultMaterial"));
    data.AddRootEngineAsset(TEXT("Engine/DefaultDeformableMaterial"));
    data.AddRootEngineAsset(TEXT("Engine/DefaultTerrainMaterial"));
    if (!gameSettings->NoSplashScreen && !gameSettings->SplashScreen.IsValid())
        data.AddRootEngineAsset(TEXT("Engine/Textures/Logo"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/NormalTexture"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/BlackTexture"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/WhiteTexture"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/DefaultLensStarburst"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/DefaultLensColor"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/DefaultLensDirt"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/Bokeh/Circle"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/Bokeh/Hexagon"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/Bokeh/Octagon"));
    data.AddRootEngineAsset(TEXT("Engine/Textures/Bokeh/Cross"));
    data.AddRootEngineAsset(TEXT("Engine/Models/Sphere"));
    data.AddRootEngineAsset(TEXT("Engine/Models/SphereLowPoly"));
    data.AddRootEngineAsset(TEXT("Engine/Models/Box"));
    data.AddRootEngineAsset(TEXT("Engine/Models/SimpleBox"));
    data.AddRootEngineAsset(TEXT("Engine/Models/Quad"));
    data.AddRootEngineAsset(TEXT("Engine/SkyboxMaterial"));
    data.AddRootEngineAsset(PRE_INTEGRATED_GF_ASSET_NAME);
    data.AddRootEngineAsset(SMAA_AREA_TEX);
    data.AddRootEngineAsset(SMAA_SEARCH_TEX);
    if (data.Configuration != BuildConfiguration::Release)
        data.AddRootEngineAsset(TEXT("Editor/Fonts/Roboto-Regular"));

    // Register game assets
    data.StepProgress(TEXT("Deploying game data"), 50);
    auto& buildSettings = *BuildSettings::Get();
    for (auto& e : buildSettings.AdditionalAssets)
        data.AddRootAsset(e.GetID());
    Array<String> files;
    for (auto& e : buildSettings.AdditionalAssetFolders)
    {
        String path = FileSystem::ConvertRelativePathToAbsolute(Globals::ProjectFolder, e);
        if (FileSystem::DirectoryGetFiles(files, path, TEXT("*"), DirectorySearchOption::AllDirectories))
        {
            data.Error(TEXT("Failed to find additional assets to deploy."));
            return true;
        }

        for (auto& q : files)
            data.AddRootAsset(q);
        files.Clear();
    }

    return false;
}
