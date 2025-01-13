// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
#include "Editor/Utilities/EditorUtilities.h"

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
    {
        // Remove old Mono files
        FileSystem::DeleteDirectory(dstMono);
        FileSystem::DeleteFile(data.DataOutputPath / TEXT("MonoPosixHelper.dll"));
    }
    String dstDotnet = data.DataOutputPath / TEXT("Dotnet");
    const DotNetAOTModes aotMode = data.Tools->UseAOT();
    const bool usAOT = aotMode != DotNetAOTModes::None;
    if (usAOT)
    {
        // Deploy Dotnet files into intermediate cooking directory for AOT
        FileSystem::DeleteDirectory(dstDotnet);
        dstDotnet = data.ManagedCodeOutputPath;
    }
    if (buildSettings.SkipDotnetPackaging && data.Tools->UseSystemDotnet())
    {
        // Use system-installed .NET Runtime
        FileSystem::DeleteDirectory(dstDotnet);
    }
    else
    {
        // Deploy .NET Runtime files
        FileSystem::CreateDirectory(dstDotnet);
        String srcDotnet = depsRoot / TEXT("Dotnet");
        if (FileSystem::DirectoryExists(srcDotnet))
        {
            // Use prebuilt .NET installation for that platform
            LOG(Info, "Using .NET Runtime {} at {}", data.Tools->GetName(), srcDotnet);
            if (EditorUtilities::CopyDirectoryIfNewer(dstDotnet, srcDotnet, true))
            {
                data.Error(TEXT("Failed to copy .NET runtime data files."));
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
            case BuildPlatform::WindowsARM64:
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
            if (canUseSystemDotnet && (aotMode == DotNetAOTModes::None || aotMode == DotNetAOTModes::ILC))
            {
                // Ask Flax.Build to provide .NET SDK location for the current platform
                String sdks;
                bool failed = ScriptsBuilder::RunBuildTool(String::Format(TEXT("-log -logMessagesOnly -logFileWithConsole -logfile=SDKs.txt -printSDKs {}"), GAME_BUILD_DOTNET_VER), data.CacheDirectory);
                failed |= File::ReadAllText(data.CacheDirectory / TEXT("SDKs.txt"), sdks);
                int32 idx = sdks.Find(TEXT("DotNetSdk, "), StringSearchCase::CaseSensitive);
                if (idx != -1)
                {
                    idx = sdks.Find(TEXT(", "), StringSearchCase::CaseSensitive, idx + 12);
                    idx += 2;
                    int32 end = sdks.Find(TEXT("\n"), StringSearchCase::CaseSensitive, idx);
                    if (sdks[end] == '\r')
                        end--;
                    srcDotnet = String(sdks.Get() + idx, end - idx).TrimTrailing();
                }
                if (failed || !FileSystem::DirectoryExists(srcDotnet))
                {
                    data.Error(TEXT("Failed to get .NET SDK location for the current host platform."));
                    return true;
                }

                // Select version to use
                Array<String> versions;
                FileSystem::GetChildDirectories(versions, srcDotnet / TEXT("host/fxr"));
                if (versions.Count() == 0)
                {
                    data.Error(TEXT("Failed to find any .NET hostfxr versions for the current host platform."));
                    return true;
                }
                for (String& version : versions)
                {
                    version = String(StringUtils::GetFileName(version));
                    const int32 dot = version.Find('.');
                    int majorVersion = 0;
                    if (dot != -1)
                        StringUtils::Parse(version.Substring(0, dot).Get(), &majorVersion);
                    if (majorVersion < GAME_BUILD_DOTNET_RUNTIME_MIN_VER || majorVersion > GAME_BUILD_DOTNET_RUNTIME_MAX_VER) // Check for major part
                        version.Clear();
                }
                Sorting::QuickSort(versions);
                const String version = versions.Last();
                if (version.IsEmpty())
                {
                    data.Error(TEXT("Failed to find supported .NET hostfxr version for the current host platform."));
                    return true;
                }

                FileSystem::NormalizePath(srcDotnet);
                LOG(Info, "Using .NET Runtime {} at {}", version, srcDotnet);

                // Check if previously deployed files are valid (eg. system-installed .NET was updated from version 7.0.3 to 7.0.5)
                {
                    const String dotnetCacheFilePath = data.CacheDirectory / TEXT("SystemDotnetInfo.txt");
                    String dotnetCachedValue = String::Format(TEXT("{};{}"), version, srcDotnet);
                    if (FileSystem::DirectoryExists(dstDotnet))
                    {
                        String cachedData;
                        if (FileSystem::FileExists(dotnetCacheFilePath))
                            File::ReadAllText(dotnetCacheFilePath, cachedData);
                        if (cachedData != dotnetCachedValue)
                        {
                            FileSystem::DeleteDirectory(dstDotnet);
                            FileSystem::CreateDirectory(dstDotnet);
                        }
                    }
                    File::WriteAllText(dotnetCacheFilePath, dotnetCachedValue, Encoding::ANSI);
                }

                // Deploy runtime files
                FileSystem::CopyFile(dstDotnet / TEXT("LICENSE.TXT"), srcDotnet / TEXT("LICENSE.txt"));
                FileSystem::CopyFile(dstDotnet / TEXT("LICENSE.TXT"), srcDotnet / TEXT("LICENSE.TXT"));
                FileSystem::CopyFile(dstDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"), srcDotnet / TEXT("ThirdPartyNotices.txt"));
                FileSystem::CopyFile(dstDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"), srcDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"));
                if (usAOT)
                {
                    failed |= EditorUtilities::CopyDirectoryIfNewer(dstDotnet, srcDotnet / TEXT("shared/Microsoft.NETCore.App") / version);
                }
                else
                {
#if 1
                    failed |= EditorUtilities::CopyDirectoryIfNewer(dstDotnet / TEXT("host/fxr") / version, srcDotnet / TEXT("host/fxr") / version);
#else
                    // TODO: hostfxr for target platform should be copied from nuget package location: microsoft.netcore.app.runtime.<RID>/<VERSION>/runtimes/<RID>/native/hostfxr.dll
                    String dstHostfxr = dstDotnet / TEXT("host/fxr") / version;
                    if (!FileSystem::DirectoryExists(dstHostfxr))
                        FileSystem::CreateDirectory(dstHostfxr);
                    const Char *platformName, *archName;
                    data.GetBuildPlatformName(platformName, archName);
                    if (data.Platform == BuildPlatform::Windows64 || data.Platform == BuildPlatform::WindowsARM64 || data.Platform == BuildPlatform::Windows32)
                        failed |= FileSystem::CopyFile(dstHostfxr / TEXT("hostfxr.dll"), depsRoot / TEXT("ThirdParty") / archName / TEXT("hostfxr.dll"));
                    else if (data.Platform == BuildPlatform::LinuxX64)
                        failed |= FileSystem::CopyFile(dstHostfxr / TEXT("hostfxr.so"), depsRoot / TEXT("ThirdParty") / archName / TEXT("hostfxr.so"));
                    else if (data.Platform == BuildPlatform::MacOSx64 || data.Platform == BuildPlatform::MacOSARM64)
                        failed |= FileSystem::CopyFile(dstHostfxr / TEXT("hostfxr.dylib"), depsRoot / TEXT("ThirdParty") / archName / TEXT("hostfxr.dylib"));
                    else
                        failed |= true;
#endif
                    failed |= EditorUtilities::CopyDirectoryIfNewer(dstDotnet / TEXT("shared/Microsoft.NETCore.App") / version, srcDotnet / TEXT("shared/Microsoft.NETCore.App") / version, true);
                }
                if (failed)
                {
                    data.Error(TEXT("Failed to copy .NET runtime data files."));
                    return true;
                }
            }
            else
            {
                // Ask Flax.Build to provide .NET Host Runtime location for the target platform
                String sdks;
                const Char *platformName, *archName;
                data.GetBuildPlatformName(platformName, archName);
                String args = String::Format(TEXT("-log -logMessagesOnly -logFileWithConsole -logfile=SDKs.txt -printDotNetRuntime -platform={} -arch={} {}"), platformName, archName, GAME_BUILD_DOTNET_VER);
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
                    data.Error(TEXT("Failed to get .NET SDK location for the current host platform."));
                    return true;
                }
                FileSystem::NormalizePath(srcDotnet);
                LOG(Info, "Using .NET Runtime {} at {}", TEXT("Host"), srcDotnet);

                // Get major Version
                Array<String> pathParts;
                srcDotnet.Split('/', pathParts);
                String version;
                for (int i = 0; i < pathParts.Count(); i++)
                {
                    if (pathParts[i] == TEXT("runtimes"))
                    {
                        Array<String> versionParts;
                        pathParts[i - 1].Split('.', versionParts);
                        if (!versionParts.IsEmpty())
                        {
                            const String majorVersion = versionParts[0].TrimTrailing();
                            int32 versionNum;
                            StringUtils::Parse(*majorVersion, majorVersion.Length(), &versionNum);
                            if (Math::IsInRange(versionNum, GAME_BUILD_DOTNET_RUNTIME_MIN_VER, GAME_BUILD_DOTNET_RUNTIME_MAX_VER)) // Check for major part
                                version = majorVersion;
                        }
                    }
                }

                if (version.IsEmpty())
                {
                    data.Error(TEXT("Failed to find supported .NET version for the current host platform."));
                    return true;
                }

                // Deploy runtime files
                const Char* corlibPrivateName = TEXT("System.Private.CoreLib.dll");
                const bool srcDotnetFromEngine = srcDotnet.Contains(TEXT("Source/Platforms"));
                String packFolder = srcDotnet / TEXT("../../../");
                String dstDotnetLibs = dstDotnet, srcDotnetLibs = srcDotnet;
                StringUtils::PathRemoveRelativeParts(packFolder);
                if (usAOT)
                {
                    if (srcDotnetFromEngine)
                    {
                        // AOT runtime files inside Engine Platform folder
                        packFolder /= TEXT("Dotnet");
                        dstDotnetLibs /= String::Format(TEXT("lib/net{}.0"), version);
                        srcDotnetLibs = packFolder / String::Format(TEXT("lib/net{}.0"), version);
                    }
                    else
                    {
                        // Runtime files inside Dotnet SDK folder but placed for AOT
                        dstDotnetLibs /= String::Format(TEXT("lib/net{}.0"), version);
                        srcDotnetLibs /= String::Format(TEXT("../lib/net{}.0"), version);
                    }
                }
                else
                {
                    if (srcDotnetFromEngine)
                    {
                        // Runtime files inside Engine Platform folder
                        dstDotnetLibs /= String::Format(TEXT("lib/net{}.0"), version);
                        srcDotnetLibs /= String::Format(TEXT("lib/net{}.0"), version);
                    }
                    else
                    {
                        // Runtime files inside Dotnet SDK folder
                        dstDotnetLibs /= TEXT("shared/Microsoft.NETCore.App");
                        srcDotnetLibs /= String::Format(TEXT("../lib/net{}.0"), version);
                    }
                }
                LOG(Info, "Copying .NET files from {} to {}", packFolder, dstDotnet);
                LOG(Info, "Copying .NET files from {} to {}", srcDotnetLibs, dstDotnetLibs);
                FileSystem::CopyFile(dstDotnet / TEXT("LICENSE.TXT"), packFolder / TEXT("LICENSE.txt"));
                FileSystem::CopyFile(dstDotnet / TEXT("LICENSE.TXT"), packFolder / TEXT("LICENSE.TXT"));
                FileSystem::CopyFile(dstDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"), packFolder / TEXT("ThirdPartyNotices.txt"));
                FileSystem::CopyFile(dstDotnet / TEXT("THIRD-PARTY-NOTICES.TXT"), packFolder / TEXT("THIRD-PARTY-NOTICES.TXT"));
                failed |= EditorUtilities::CopyDirectoryIfNewer(dstDotnetLibs, srcDotnetLibs, true);
                if (FileSystem::FileExists(srcDotnet / corlibPrivateName))
                    failed |= EditorUtilities::CopyFileIfNewer(dstDotnetLibs / corlibPrivateName, srcDotnet / corlibPrivateName);
                switch (data.Platform)
                {
#define DEPLOY_NATIVE_FILE(filename) failed |= FileSystem::CopyFile(data.NativeCodeOutputPath / TEXT(filename), srcDotnet / TEXT(filename));
                case BuildPlatform::AndroidARM64:
                    if (data.Configuration != BuildConfiguration::Release)
                    {
                        DEPLOY_NATIVE_FILE("libmono-component-debugger.so");
                        DEPLOY_NATIVE_FILE("libmono-component-diagnostics_tracing.so");
                        DEPLOY_NATIVE_FILE("libmono-component-hot_reload.so");
                    }
                    DEPLOY_NATIVE_FILE("libmonosgen-2.0.so");
                    DEPLOY_NATIVE_FILE("libSystem.IO.Compression.Native.so");
                    DEPLOY_NATIVE_FILE("libSystem.Native.so");
                    DEPLOY_NATIVE_FILE("libSystem.Globalization.Native.so");
                    DEPLOY_NATIVE_FILE("libSystem.Security.Cryptography.Native.Android.so");
                    break;
                case BuildPlatform::iOSARM64:
                    DEPLOY_NATIVE_FILE("libmonosgen-2.0.dylib");
                    DEPLOY_NATIVE_FILE("libSystem.IO.Compression.Native.dylib");
                    DEPLOY_NATIVE_FILE("libSystem.Native.dylib");
                    DEPLOY_NATIVE_FILE("libSystem.Net.Security.Native.dylib");
                    DEPLOY_NATIVE_FILE("libSystem.Security.Cryptography.Native.Apple.dylib");
                    break;
#undef DEPLOY_NATIVE_FILE
                default: ;
                }
                if (failed)
                {
                    data.Error(TEXT("Failed to copy .NET runtime data files."));
                    return true;
                }
            }
        }

        // Remove any leftover files copied from .NET SDK that are not needed by the engine runtime
        {
            Array<String> files;
            FileSystem::DirectoryGetFiles(files, dstDotnet, TEXT("*.exe"));
            for (const String& file : files)
            {
                LOG(Info, "Removing '{}'", FileSystem::ConvertAbsolutePathToRelative(dstDotnet, file));
                FileSystem::DeleteFile(file);
            }
        }

        // Optimize deployed C# class library (remove DLLs unused by scripts)
        if (aotMode == DotNetAOTModes::None && buildSettings.SkipUnusedDotnetLibsPackaging)
        {
            LOG(Info, "Optimizing .NET class library size to include only used assemblies");
            const String logFile = data.CacheDirectory / TEXT("StripDotnetLibs.txt");
            String args = String::Format(
                TEXT("-log -logfile=\"{}\" -runDotNetClassLibStripping -mutex -binaries=\"{}\" {}"),
                logFile, data.DataOutputPath, GAME_BUILD_DOTNET_VER);
            for (const String& define : data.CustomDefines)
            {
                args += TEXT(" -D");
                args += define;
            }
            if (ScriptsBuilder::RunBuildTool(args))
            {
                data.Error(TEXT("Failed to optimize .NET class library."));
                return true;
            }
        }
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
        if (FileSystem::CopyDirectory(dstMono, srcMono))
        {
            data.Error(TEXT("Failed to copy Mono runtime data files."));
            return true;
        }
    }
#endif

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
    data.AddRootEngineAsset(TEXT("Shaders/SDF"));
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
    if (!buildSettings.SkipDefaultFonts)
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
        if (FileSystem::DirectoryGetFiles(files, path))
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
