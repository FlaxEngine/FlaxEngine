// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_ANDROID

#include "AndroidPlatformTools.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Android/AndroidPlatformSettings.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"

IMPLEMENT_SETTINGS_GETTER(AndroidPlatformSettings, AndroidPlatform);

namespace
{
    void DeployIcon(const CookingData& data, const TextureData& iconData, const Char* subDir, int32 iconSize, int32 adaptiveIconSize)
    {
        const String mipmapPath = data.OriginalOutputPath / TEXT("app/src/main/res") / subDir;
        const String iconPath = mipmapPath / TEXT("icon.png");
        if (!FileSystem::DirectoryExists(mipmapPath))
            FileSystem::CreateDirectory(mipmapPath);
        EditorUtilities::ExportApplicationImage(iconData, iconSize, iconSize, PixelFormat::B8G8R8A8_UNorm, iconPath);
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
    // TODO: add ETC compression support for Android
    // TODO: add ASTC compression support for Android

    // BC formats are not widely supported on Android
    if (PixelFormatExtensions::IsCompressedBC(format))
    {
        switch (format)
        {
        case PixelFormat::BC1_Typeless:
        case PixelFormat::BC2_Typeless:
        case PixelFormat::BC3_Typeless:
            return PixelFormat::R8G8B8A8_Typeless;
        case PixelFormat::BC1_UNorm:
        case PixelFormat::BC2_UNorm:
        case PixelFormat::BC3_UNorm:
            return PixelFormat::R8G8B8A8_UNorm;
        case PixelFormat::BC1_UNorm_sRGB:
        case PixelFormat::BC2_UNorm_sRGB:
        case PixelFormat::BC3_UNorm_sRGB:
            return PixelFormat::R8G8B8A8_UNorm_sRGB;
        case PixelFormat::BC4_Typeless:
            return PixelFormat::R8_Typeless;
        case PixelFormat::BC4_UNorm:
            return PixelFormat::R8_UNorm;
        case PixelFormat::BC4_SNorm:
            return PixelFormat::R8_SNorm;
        case PixelFormat::BC5_Typeless:
            return PixelFormat::R16G16_Typeless;
        case PixelFormat::BC5_UNorm:
            return PixelFormat::R16G16_UNorm;
        case PixelFormat::BC5_SNorm:
            return PixelFormat::R16G16_SNorm;
        case PixelFormat::BC7_Typeless:
        case PixelFormat::BC6H_Typeless:
            return PixelFormat::R16G16B16A16_Typeless;
        case PixelFormat::BC7_UNorm:
        case PixelFormat::BC6H_Uf16:
        case PixelFormat::BC6H_Sf16:
            return PixelFormat::R16G16B16A16_Float;
        case PixelFormat::BC7_UNorm_sRGB:
            return PixelFormat::R16G16B16A16_UNorm;
        default:
            return format;
        }
    }

    return format;
}

void AndroidPlatformTools::OnBuildStarted(CookingData& data)
{
    // Adjust the cooking output folder to be located inside the Gradle assets directory
    data.OutputPath /= TEXT("app/assets");

    PlatformTools::OnBuildStarted(data);
}

bool AndroidPlatformTools::OnPostProcess(CookingData& data)
{
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = AndroidPlatformSettings::Get();
    const auto platformDataPath = data.GetPlatformBinariesRoot();
    const auto assetsPath = data.OutputPath;
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
    {
        String productName = gameSettings->ProductName;
        productName.Replace(TEXT(" "), TEXT(""));
        productName.Replace(TEXT("."), TEXT(""));
        productName.Replace(TEXT("-"), TEXT(""));
        String companyName = gameSettings->CompanyName;
        companyName.Replace(TEXT(" "), TEXT(""));
        companyName.Replace(TEXT("."), TEXT(""));
        companyName.Replace(TEXT("-"), TEXT(""));
        packageName.Replace(TEXT("${PROJECT_NAME}"), *productName, StringSearchCase::IgnoreCase);
        packageName.Replace(TEXT("${COMPANY_NAME}"), *companyName, StringSearchCase::IgnoreCase);
        packageName = packageName.ToLower();
        for (int32 i = 0; i < packageName.Length(); i++)
        {
            const auto c = packageName[i];
            if (c != '_' && c != '.' && !StringUtils::IsAlnum(c))
            {
                LOG(Error, "Android Package Name \'{0}\' contains invalid character. Only letters, numbers, dots and underscore characters are allowed.", packageName);
                return true;
            }
        }
        if (packageName.IsEmpty())
        {
            LOG(Error, "Android Package Name is empty.", packageName);
            return true;
        }
    }

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

    // Setup Android application attributes
    String attributes;
    if (data.Configuration != BuildConfiguration::Release)
    {
        attributes += TEXT("\n        android:debuggable=\"true\"");
    }

    // Copy fresh Gradle project template
    if (FileSystem::CopyDirectory(data.OriginalOutputPath, platformDataPath / TEXT("Project"), true))
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

    // Format project template files
    const String buildGradlePath = data.OriginalOutputPath / TEXT("app/build.gradle");
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${PackageName}"), packageName);
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${ProjectVersion}"), projectVersion);
    EditorUtilities::ReplaceInFile(buildGradlePath, TEXT("${PackageAbi}"), abi);
    const String manifestPath = data.OriginalOutputPath / TEXT("app/src/main/AndroidManifest.xml");
    EditorUtilities::ReplaceInFile(manifestPath, TEXT("${PackageName}"), packageName);
    EditorUtilities::ReplaceInFile(manifestPath, TEXT("${ProjectVersion}"), projectVersion);
    EditorUtilities::ReplaceInFile(manifestPath, TEXT("${AndroidPermissions}"), permissions);
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

    // Generate Mono files hash id used to skip deploying Mono files if already extracted on device (Mono cannot access files packed into .apk via unix file access)
    File::WriteAllText(assetsPath / TEXT("hash.txt"), Guid::New().ToString(), Encoding::ANSI);

    // TODO: expose event to inject custom gradle and manifest options or custom binaries into app

    const auto buildSettings = BuildSettings::Get();
    if (buildSettings->SkipPackaging)
    {
        return false;
    }

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
    if (!envVars.TryGet(TEXT("ANDROID_SDK"), androidSdk) || !FileSystem::DirectoryExists(androidSdk))
    {
        LOG(Error, "Missing or invalid ANDROID_SDK env variable. {0}", androidSdk);
        return true;
    }
    if (!envVars.ContainsKey(TEXT("ANDROID_SDK_ROOT")))
        envVars[TEXT("ANDROID_SDK_ROOT")] = androidSdk;

    // Build Gradle project into package
    LOG(Info, "Building Gradle project into package...");
#if PLATFORM_WINDOWS
    const Char* gradlew = TEXT("gradlew.bat");
#else
    const Char* gradlew = TEXT("gradlew");
#endif
    const bool distributionPackage = buildSettings->ForDistribution;
    const String gradleCommand = String::Format(TEXT("\"{0}\" {1}"), data.OriginalOutputPath / gradlew, distributionPackage ? TEXT("assemble") : TEXT("assembleDebug"));
    const int32 result = Platform::RunProcess(gradleCommand, data.OriginalOutputPath, envVars, true);
    if (result != 0)
    {
        data.Error(TEXT("Failed to build Gradle project into package (result code: {0}). See log for more info."), result);
        return true;
    }

    // Copy result package
    const String apk = data.OriginalOutputPath / (distributionPackage ? TEXT("app/build/outputs/apk/release/app-release-unsigned.apk") : TEXT("app/build/outputs/apk/debug/app-debug.apk"));
    const String outputApk = data.OriginalOutputPath / gameSettings->ProductName + TEXT(".apk");
    if (FileSystem::CopyFile(outputApk, apk))
    {
        LOG(Error, "Failed to copy package from {0} to {1}", apk, outputApk);
        return true;
    }
    LOG(Info, "Output Android application package: {0} (size: {1} MB)", outputApk, FileSystem::GetFileSize(outputApk) / 1024 / 1024);

    return false;
}

#endif
