// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "DeployDataStep.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Renderer/ReflectionsPass.h"
#include "Engine/Renderer/AntiAliasing/SMAA.h"
#include "Engine/Engine/Globals.h"
#include "Editor/Cooker/PlatformTools.h"

bool DeployDataStep::Perform(CookingData& data)
{
    data.StepProgress(TEXT("Deploying engine data"), 0);
    const String depsRoot = data.GetPlatformBinariesRoot();
    const auto& gameSettings = *GameSettings::Get();
    const auto& buildSettings = *BuildSettings::Get();

    // Setup output folders and copy required data
    const auto contentDir = data.DataOutputPath / TEXT("Content");
    if (FileSystem::DirectoryExists(contentDir))
    {
        // Remove old content files
        FileSystem::DeleteDirectory(contentDir, true);

        // Give some time for Explorer (if location was viewed)
        Platform::Sleep(10);
    }
    FileSystem::CreateDirectory(contentDir);
    const String dstMono = data.DataOutputPath / TEXT("Mono");
#if USE_NETCORE
    // TODO: Optionally copy all files needed for self-contained deployment
    {
        // Remove old Mono files
        FileSystem::DeleteDirectory(dstMono);
        FileSystem::DeleteFile(data.DataOutputPath / TEXT("MonoPosixHelper.dll"));
    }
#else
    if (!FileSystem::DirectoryExists(dstMono))
    {
        // Deploy Mono files (from platform data folder)
        const String srcMono = depsRoot / TEXT("Mono");
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
#endif
    const String dstDotnet = data.DataOutputPath / TEXT("Dotnet");
    if (buildSettings.SkipDotnetPackaging && data.Tools->UseSystemDotnet())
    {
        // Use system-installed .Net Runtime
        FileSystem::DeleteDirectory(dstDotnet);
    }
    else
    {
        // Deploy .Net Runtime files
        FileSystem::CreateDirectory(dstDotnet);
        String srcDotnet = depsRoot / TEXT("Dotnet");
        if (FileSystem::DirectoryExists(srcDotnet))
        {
            // Use prebuilt .Net installation for that platform
            LOG(Info, "Using .Net Runtime {} at {}", data.Tools->GetName(), srcDotnet);
            if (FileSystem::CopyDirectory(dstDotnet, srcDotnet, true))
            {
                data.Error(TEXT("Failed to copy .Net runtime data files."));
                return true;
            }
        }
        else
        {
            bool canUseSystemDotnet = false;
            switch (data.Platform)
            {
            case BuildPlatform::Windows32:
            case BuildPlatform::Windows64:
                canUseSystemDotnet = PLATFORM_TYPE == PlatformType::Windows;
                break;
            case BuildPlatform::LinuxX64:
                canUseSystemDotnet = PLATFORM_TYPE == PlatformType::Linux;
                break;
            case BuildPlatform::MacOSx64:
            case BuildPlatform::MacOSARM64:
                canUseSystemDotnet = PLATFORM_TYPE == PlatformType::Mac;
                break;
            }
            if (canUseSystemDotnet)
            {
                // Ask Flax.Build to provide .Net SDK location for the current platform
                String sdks;
                bool failed = ScriptsBuilder::RunBuildTool(TEXT("-log -logMessagesOnly -logFileWithConsole -logfile=SDKs.txt -printSDKs"), data.CacheDirectory);
                failed |= File::ReadAllText(data.CacheDirectory / TEXT("SDKs.txt"), sdks);
                int32 idx = sdks.Find(TEXT("] DotNetSdk, "), StringSearchCase::CaseSensitive);
                if (idx != -1)
                {
                    idx = sdks.Find(TEXT(", "), StringSearchCase::CaseSensitive, idx + 14);
                    idx += 2;
                    int32 end = sdks.Find(TEXT("\n"), StringSearchCase::CaseSensitive, idx);
                    if (sdks[end] == '\r')
                        end--;
                    srcDotnet = String(sdks.Get() + idx, end - idx).TrimTrailing();
                }
                if (failed || !FileSystem::DirectoryExists(srcDotnet))
                {
                    data.Error(TEXT("Failed to get .Net SDK location for a current platform."));
                    return true;
                }

                // Select version to use
                Array<String> versions;
                FileSystem::GetChildDirectories(versions, srcDotnet / TEXT("host/fxr"));
                if (versions.Count() == 0)
                {
                    data.Error(TEXT("Failed to get .Net SDK location for a current platform."));
                    return true;
                }
                for (String& version : versions)
                {
                    version = StringUtils::GetFileName(version);
                    if (!version.StartsWith(TEXT("7.")))
                        version.Clear();
                }
                Sorting::QuickSort(versions.Get(), versions.Count());
                const String version = versions.Last();
                LOG(Info, "Using .Net Runtime {} at {}", version, srcDotnet);

                // Deploy runtime files
                FileSystem::CopyFile(dstDotnet / TEXT("LICENSE.TXT"), srcDotnet / TEXT("LICENSE.txt"));
                FileSystem::CopyFile(dstDotnet / TEXT("LICENSE.TXT"), srcDotnet / TEXT("LICENSE.TXT"));
                FileSystem::CopyFile(dstDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"), srcDotnet / TEXT("ThirdPartyNotices.txt"));
                FileSystem::CopyFile(dstDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"), srcDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"));
                failed |= FileSystem::CopyDirectory(dstDotnet / TEXT("host/fxr") / version, srcDotnet / TEXT("host/fxr") / version, true);
                failed |= FileSystem::CopyDirectory(dstDotnet / TEXT("shared/Microsoft.NETCore.App") / version, srcDotnet / TEXT("shared/Microsoft.NETCore.App") / version, true);
                if (failed)
                {
                    data.Error(TEXT("Failed to copy .Net runtime data files."));
                    return true;
                }
            }
            else
            {
                // Ask Flax.Build to provide .Net Host Runtime location for the target platform
                String sdks;
                const Char* platformName, *archName;
                data.GetBuildPlatformName(platformName, archName);
                String args = String::Format(TEXT("-log -logMessagesOnly -logFileWithConsole -logfile=SDKs.txt -printDotNetRuntime -platform={} -arch={}"), platformName, archName);
                bool failed = ScriptsBuilder::RunBuildTool(args, data.CacheDirectory);
                failed |= File::ReadAllText(data.CacheDirectory / TEXT("SDKs.txt"), sdks);
                Array<String> parts;
                sdks.Split(',', parts);
                failed |= parts.Count() != 3;
                if (!failed)
                {
                    srcDotnet = parts[2].TrimTrailing();
                }
                if (failed || !FileSystem::DirectoryExists(srcDotnet))
                {
                    data.Error(TEXT("Failed to get .Net SDK location for a current platform."));
                    return true;
                }
                LOG(Info, "Using .Net Runtime {} at {}", TEXT("Host"), srcDotnet);

                // Deploy runtime files
                const String packFolder = srcDotnet / TEXT("../../../");
                FileSystem::CopyFile(dstDotnet / TEXT("LICENSE.TXT"), packFolder / TEXT("LICENSE.txt"));
                FileSystem::CopyFile(dstDotnet / TEXT("LICENSE.TXT"), packFolder / TEXT("LICENSE.TXT"));
                FileSystem::CopyFile(dstDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"), packFolder / TEXT("ThirdPartyNotices.txt"));
                FileSystem::CopyFile(dstDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"), packFolder / TEXT("THIRD-PARTY-NOTICES.TXT"));
                failed |= FileSystem::CopyDirectory(dstDotnet / TEXT("shared/Microsoft.NETCore.App"), srcDotnet / TEXT("../lib/net7.0"), true);
                failed |= FileSystem::CopyFile(dstDotnet / TEXT("shared/Microsoft.NETCore.App") / TEXT("System.Private.CoreLib.dll"), srcDotnet / TEXT("System.Private.CoreLib.dll"));
                if (failed)
                {
                    data.Error(TEXT("Failed to copy .Net runtime data files."));
                    return true;
                }
            }
        }
    }

    // Deploy engine data for the target platform
    if (data.Tools->OnDeployBinaries(data))
        return true;
    GameCooker::DeployFiles();

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
    data.AddRootEngineAsset(TEXT("Shaders/ProbesFilter"));
    data.AddRootEngineAsset(TEXT("Shaders/PostProcessing"));
    data.AddRootEngineAsset(TEXT("Shaders/MotionBlur"));
    data.AddRootEngineAsset(TEXT("Shaders/BitonicSort"));
    data.AddRootEngineAsset(TEXT("Shaders/GPUParticlesSorting"));
    data.AddRootEngineAsset(TEXT("Shaders/GlobalSignDistanceField"));
    data.AddRootEngineAsset(TEXT("Shaders/GI/GlobalSurfaceAtlas"));
    data.AddRootEngineAsset(TEXT("Shaders/GI/DDGI"));
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
    if (!gameSettings.NoSplashScreen && !gameSettings.SplashScreen.IsValid())
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

    // Register custom assets (eg. plugins)
    data.StepProgress(TEXT("Deploying custom data"), 30);
    GameCooker::OnCollectAssets(data.RootAssets);

    // Register game assets
    data.StepProgress(TEXT("Deploying game data"), 50);
    for (auto& e : buildSettings.AdditionalAssets)
        data.AddRootAsset(e.GetID());
    for (auto& e : buildSettings.AdditionalScenes)
        data.AddRootAsset(e.ID);
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
