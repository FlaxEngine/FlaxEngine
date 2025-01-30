// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_ANDROID

#include "AndroidPlatformTools.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Cooker/GameCooker.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/Android/AndroidPlatformSettings.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"

IMPLEMENT_ENGINE_SETTINGS_GETTER(AndroidPlatformSettings, AndroidPlatform);

namespace
{
    struct AndroidPlatformCache
    {
        AndroidPlatformSettings::TextureQuality TexturesQuality;
    };

    void DeployIcon(const CookingData& data, const TextureData& iconData, const Char* subDir, int32 iconSize, int32 adaptiveIconSize)
    {
        const String mipmapPath = data.OriginalOutputPath / TEXT("app/src/main/res") / subDir;
        const String iconPath = mipmapPath / TEXT("icon.png");
        if (!FileSystem::DirectoryExists(mipmapPath))
            FileSystem::CreateDirectory(mipmapPath);
        EditorUtilities::ExportApplicationImage(iconData, iconSize, iconSize, PixelFormat::B8G8R8A8_UNorm, iconPath);
    }

    PixelFormat GetQualityTextureFormat(bool sRGB, PixelFormat format)
    {
        const auto platformSettings = AndroidPlatformSettings::Get();
        switch (platformSettings->TexturesQuality)
        {
        case AndroidPlatformSettings::TextureQuality::Uncompressed:
            return PixelFormatExtensions::FindUncompressedFormat(format);
        case AndroidPlatformSettings::TextureQuality::ASTC_High:
            return sRGB ? PixelFormat::ASTC_4x4_UNorm_sRGB : PixelFormat::ASTC_4x4_UNorm;
        case AndroidPlatformSettings::TextureQuality::ASTC_Medium:
            return sRGB ? PixelFormat::ASTC_6x6_UNorm_sRGB : PixelFormat::ASTC_6x6_UNorm;
        case AndroidPlatformSettings::TextureQuality::ASTC_Low:
            return sRGB ? PixelFormat::ASTC_8x8_UNorm_sRGB : PixelFormat::ASTC_8x8_UNorm;
        default:
            return format;
        }
    }
}

const Char* AndroidPlatformTools::GetDisplayName() const
{
    return TEXT("Android");
}

const Char* AndroidPlatformTools::GetName() const
{
    return TEXT("Android");
}

PlatformType AndroidPlatformTools::GetPlatform() const
{
    return PlatformType::Android;
}

ArchitectureType AndroidPlatformTools::GetArchitecture() const
{
    return _arch;
}

PixelFormat AndroidPlatformTools::GetTextureFormat(CookingData& data, TextureBase* texture, PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R11G11B10_Float:
        // Not all Android devices support R11G11B10 textures (eg. M6 Note)
        return PixelFormat::R16G16B16A16_UNorm;
    case PixelFormat::BC1_Typeless:
    case PixelFormat::BC2_Typeless:
    case PixelFormat::BC3_Typeless:
    case PixelFormat::BC4_Typeless:
    case PixelFormat::BC5_Typeless:
    case PixelFormat::BC1_UNorm:
    case PixelFormat::BC2_UNorm:
    case PixelFormat::BC3_UNorm:
    case PixelFormat::BC4_UNorm:
    case PixelFormat::BC5_UNorm:
        return GetQualityTextureFormat(false, format);
    case PixelFormat::BC1_UNorm_sRGB:
    case PixelFormat::BC2_UNorm_sRGB:
    case PixelFormat::BC3_UNorm_sRGB:
    case PixelFormat::BC7_UNorm_sRGB:
        return GetQualityTextureFormat(true, format);
    case PixelFormat::BC4_SNorm:
        return PixelFormat::R8_SNorm;
    case PixelFormat::BC5_SNorm:
        return PixelFormat::R16G16_SNorm;
    case PixelFormat::BC6H_Typeless:
    case PixelFormat::BC6H_Uf16:
    case PixelFormat::BC6H_Sf16:
    case PixelFormat::BC7_Typeless:
    case PixelFormat::BC7_UNorm:
        return PixelFormat::R16G16B16A16_Float; // TODO: ASTC HDR
    default:
        return format;
    }
}

void AndroidPlatformTools::LoadCache(CookingData& data, IBuildCache* cache, const Span<byte>& bytes)
{
    const auto platformSettings = AndroidPlatformSettings::Get();
    bool invalidTextures = true;
    if (bytes.Length() == sizeof(AndroidPlatformCache))
    {
        auto* platformCache = (AndroidPlatformCache*)bytes.Get();
        invalidTextures = platformCache->TexturesQuality != platformSettings->TexturesQuality;
    }
    if (invalidTextures)
    {
        LOG(Info, "{0} option has been modified.", TEXT("TexturesQuality"));
        cache->InvalidateCacheTextures();
    }
}

Array<byte> AndroidPlatformTools::SaveCache(CookingData& data, IBuildCache* cache)
{
    const auto platformSettings = AndroidPlatformSettings::Get();
    AndroidPlatformCache platformCache;
    platformCache.TexturesQuality = platformSettings->TexturesQuality;
    Array<byte> result;
    result.Add((const byte*)&platformCache, sizeof(platformCache));
    return result;
}
void AndroidPlatformTools::OnBuildStarted(CookingData& data)
{
    // Adjust the cooking output folder to be located inside the Gradle assets directory
    data.DataOutputPath /= TEXT("app/assets");
    data.NativeCodeOutputPath /= TEXT("app/assets");
    data.ManagedCodeOutputPath /= TEXT("app/assets");
}

bool AndroidPlatformTools::OnPostProcess(CookingData& data)
{
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = AndroidPlatformSettings::Get();
    const auto platformDataPath = data.GetPlatformBinariesRoot();
    const auto assetsPath = data.DataOutputPath;
    const auto jniLibsPath = data.OriginalOutputPath / TEXT("app/jniLibs");
    const auto projectVersion = Editor::Project->Version.ToString();
    const Char* abi;
    switch (data.Platform)
    {
    case BuildPlatform::AndroidARM64:
        abi = TEXT("arm64-v8a");
        break;
    default:
        LOG(Error, "Invalid platform.");
        return true;
    }

    // Setup package name (eg. com.company.project)
    String packageName = platformSettings->PackageName;
    if (EditorUtilities::FormatAppPackageName(packageName))
        return true;

    // Setup Android application permissions
    HashSet<String> permissionsList;
    for (auto& e : platformSettings->Permissions)
    {
        permissionsList.Add(e);
    }
    {
        // Access game files
        permissionsList.Add(TEXT("android.permission.READ_EXTERNAL_STORAGE"));
        permissionsList.Add(TEXT("android.permission.WRITE_EXTERNAL_STORAGE"));

        // TODO: expose event to collect android permissions

        // Access sockets for C# debugging
        if (data.Configuration != BuildConfiguration::Release)
        {
            permissionsList.Add(TEXT("android.permission.INTERNET"));
        }
    }
    String permissions;
    LOG(Info, "Android permissions:");
    for (auto& e : permissionsList)
    {
        LOG(Info, "   {0}", e.Item);
        permissions += String::Format(TEXT("\n    <uses-permission android:name=\"{0}\" />"), e.Item);
    }

    // Setup default Android screen orientation
    auto defaultOrienation = platformSettings->DefaultOrientation;
    String orientation = String("fullSensor");
    switch (defaultOrienation)
    {
    case AndroidPlatformSettings::ScreenOrientation::Portrait:
        orientation = String("userPortrait");
        break;
    case AndroidPlatformSettings::ScreenOrientation::Landscape:
        orientation = String("userLandscape");
        break;
    case AndroidPlatformSettings::ScreenOrientation::SensorPortrait:
        orientation = String("sensorPortrait");
        break;
    case AndroidPlatformSettings::ScreenOrientation::SensorLandscape:
        orientation = String("sensorLandscape");
        break;
    case AndroidPlatformSettings::ScreenOrientation::AutoRotation:
        orientation = String("fullSensor");
        break;
    default:
        break;
    }

    // Setup Android application attributes
    String attributes;
    if (data.Configuration != BuildConfiguration::Release)
    {
        attributes += TEXT("\n        android:debuggable=\"true\"");
    }

    // Copy fresh Gradle project template
    if (FileSystem::CopyDirectory(data.OriginalOutputPath, platformDataPath / TEXT("Project")))
    {
        LOG(Error, "Failed to deploy Gradle project to {0} from {1}", data.OriginalOutputPath, platformDataPath);
        return true;
    }

    // Deploy app icons
    TextureData iconData;
    if (!EditorUtilities::GetApplicationImage(platformSettings->OverrideIcon, iconData))
    {
        // TODO: add support for adaptive icons (separate background and foreground with additional margin)
        const bool useAdaptiveIcons = false;
        DeployIcon(data, iconData, TEXT("mipmap"), 192, 108);
        DeployIcon(data, iconData, TEXT("mipmap-hdpi"), 72, 162);
        DeployIcon(data, iconData, TEXT("mipmap-mdpi"), 48, 108);
        DeployIcon(data, iconData, TEXT("mipmap-xhdpi"), 96, 216);
        DeployIcon(data, iconData, TEXT("mipmap-xxhdpi"), 144, 324);
        DeployIcon(data, iconData, TEXT("mipmap-xxxhdpi"), 192, 432);
        const String mipmapPath = data.OriginalOutputPath / TEXT("app/src/main/res/mipmap-anydpi-v26");
        if (useAdaptiveIcons)
        {
            if (!FileSystem::DirectoryExists(mipmapPath))
                FileSystem::CreateDirectory(mipmapPath);
            const Char* iconConfig = TEXT("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                "<adaptive-icon xmlns:android=\"http://schemas.android.com/apk/res/android\">\n"
                "    <background android:drawable=\"@mipmap/icon_background\"/>\n"
                "    <foreground android:drawable=\"@mipmap/icon_foreground\"/>\n"
                "</adaptive-icon>\n"
                "\n");
            File::WriteAllText(mipmapPath / TEXT("icon.xml"), iconConfig, Encoding::ANSI);
        }
        else
        {
            FileSystem::DeleteFile(mipmapPath / TEXT("icon.xml"));
        }
    }

    String versionCode = platformSettings->VersionCode;
    if (versionCode.IsEmpty())
    {
        LOG(Error, "AndroidSettings: Invalid version code");
        return true;
    }

    String minimumSdk = platformSettings->MinimumAPILevel;
    if (minimumSdk.IsEmpty())
    {
        LOG(Error, "AndroidSettings: Invalid minimum API level");
        return true;
    }

    String targetSdk = platformSettings->TargetAPILevel;
    if (targetSdk.IsEmpty())
    {
        LOG(Error, "AndroidSettings: Invalid target API level");
        return true;
    }

    // Format project template files
    const String buildGradlePath = data.OriginalOutputPath / TEXT("app/build.gradle");
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${PackageName}"), packageName);
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${VersionCode}"), versionCode);
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${MinimumSdk}"), minimumSdk);
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${TargetSdk}"), targetSdk);
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${ProjectVersion}"), projectVersion);
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${PackageAbi}"), abi);
    const String manifestPath = data.OriginalOutputPath / TEXT("app/src/main/AndroidManifest.xml");
    EditorUtilities::ReplaceInFile(manifestPath, TEXT("${PackageName}"), packageName);
    EditorUtilities::ReplaceInFile(manifestPath, TEXT("${ProjectVersion}"), projectVersion);
    EditorUtilities::ReplaceInFile(manifestPath, TEXT("${AndroidPermissions}"), permissions);
    EditorUtilities::ReplaceInFile(manifestPath, TEXT("${DefaultOrientation}"), orientation);
    EditorUtilities::ReplaceInFile(manifestPath, TEXT("${AndroidAttributes}"), attributes);
    const String stringsPath = data.OriginalOutputPath / TEXT("app/src/main/res/values/strings.xml");
    EditorUtilities::ReplaceInFile(stringsPath, TEXT("${ProjectName}"), gameSettings->ProductName);

    // Deploy native binaries to the output location (per-ABI)
    const String abiBinariesPath = jniLibsPath / abi;
    FileSystem::CreateDirectory(abiBinariesPath);
    Array<String> abiBinaries;
    FileSystem::DirectoryGetFiles(abiBinaries, assetsPath, TEXT("*.so"));
    for (auto& e : abiBinaries)
    {
        if (FileSystem::MoveFile(abiBinariesPath / StringUtils::GetFileName(e), e, true))
        {
            LOG(Error, "Failed to deploy binary file {0} to {1}", e, abiBinariesPath);
            return true;
        }
    }

    // Generate Dotnet files hash id used to skip deploying Dotnet files if already extracted on device (Dotnet cannot access files packed into .apk via unix file access)
    File::WriteAllText(assetsPath / TEXT("hash.txt"), Guid::New().ToString(), Encoding::ANSI);

    // TODO: expose event to inject custom gradle and manifest options or custom binaries into app

    const auto buildSettings = BuildSettings::Get();
    if (buildSettings->SkipPackaging)
        return false;
    GameCooker::PackageFiles();

    // Validate environment variables
    Dictionary<String, String> envVars;
    Platform::GetEnvironmentVariables(envVars);
    String javaHome;
    if (!envVars.TryGet(TEXT("JAVA_HOME"), javaHome) || !FileSystem::DirectoryExists(javaHome))
    {
        LOG(Error, "Missing or invalid JAVA_HOME env variable. {0}", javaHome);
        return true;
    }
    String androidSdk;
    if (!envVars.TryGet(TEXT("ANDROID_HOME"), androidSdk) || !FileSystem::DirectoryExists(androidSdk))
    {
        if (!envVars.TryGet(TEXT("ANDROID_SDK"), androidSdk) || !FileSystem::DirectoryExists(androidSdk))
        {
            LOG(Error, "Missing or invalid ANDROID_HOME env variable. {0}", androidSdk);
            return true;
        }
    }

    // Build Gradle project into package
    LOG(Info, "Building Gradle project into package...");
#if PLATFORM_WINDOWS
    const Char* gradlew = TEXT("gradlew.bat");
#else
    const Char* gradlew = TEXT("gradlew");
#endif
#if PLATFORM_LINUX
    {
        CreateProcessSettings procSettings;
        procSettings.FileName = String::Format(TEXT("chmod +x \"{0}/gradlew\""), data.OriginalOutputPath);
        procSettings.WorkingDirectory = data.OriginalOutputPath;
        procSettings.HiddenWindow = true;
        Platform::CreateProcess(procSettings);
    }
#endif
    const bool distributionPackage = buildSettings->ForDistribution || data.Configuration == BuildConfiguration::Release;
    
    if (platformSettings->BuildAAB)
    {
        // .aab
        {
            CreateProcessSettings procSettings;
            procSettings.FileName = String::Format(TEXT("\"{0}\" {1}"), data.OriginalOutputPath / gradlew, distributionPackage ? TEXT(":app:bundle") : TEXT(":app:bundleDebug"));
            procSettings.WorkingDirectory = data.OriginalOutputPath;
            const int32 result = Platform::CreateProcess(procSettings);
            if (result != 0)
            {
                data.Error(String::Format(TEXT("Failed to build Gradle project into .aab package (result code: {0}). See log for more info."), result));
                return true;
            }
        }
        // Copy result package
        const String aab = data.OriginalOutputPath / (distributionPackage ? TEXT("app/build/outputs/bundle/release/app-release.aab") : TEXT("app/build/outputs/bundle/debug/app-debug.aab"));
        const String outputAab = data.OriginalOutputPath / EditorUtilities::GetOutputName() + TEXT(".aab");
        if (FileSystem::CopyFile(outputAab, aab))
        {
            LOG(Error, "Failed to copy .aab package from {0} to {1}", aab, outputAab);
            return true;
        }
        LOG(Info, "Output Android AAB application package: {0} (size: {1} MB)", outputAab, FileSystem::GetFileSize(outputAab) / 1024 / 1024);
    }

    // .apk
    {
        CreateProcessSettings procSettings;
        procSettings.FileName = String::Format(TEXT("\"{0}\" {1}"), data.OriginalOutputPath / gradlew, distributionPackage ? TEXT("assemble") : TEXT("assembleDebug"));
        procSettings.WorkingDirectory = data.OriginalOutputPath;
        const int32 result = Platform::CreateProcess(procSettings);
        if (result != 0)
        {
            data.Error(String::Format(TEXT("Failed to build Gradle project into .apk package (result code: {0}). See log for more info."), result));
            return true;
        }
    }
    // Copy result package
    const String apk = data.OriginalOutputPath / (distributionPackage ? TEXT("app/build/outputs/apk/release/app-release-unsigned.apk") : TEXT("app/build/outputs/apk/debug/app-debug.apk"));
    const String outputApk = data.OriginalOutputPath / EditorUtilities::GetOutputName() + TEXT(".apk");
    if (FileSystem::CopyFile(outputApk, apk))
    {
        LOG(Error, "Failed to copy .apk package from {0} to {1}", apk, outputApk);
        return true;
    }
    LOG(Info, "Output Android APK application package: {0} (size: {1} MB)", outputApk, FileSystem::GetFileSize(outputApk) / 1024 / 1024);
    

    return false;
}

#endif
