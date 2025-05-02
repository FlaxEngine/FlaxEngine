// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_IOS

#include "iOSPlatformTools.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/iOS/iOSPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Engine/Globals.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Cooker/GameCooker.h"
#include "Editor/Utilities/EditorUtilities.h"

IMPLEMENT_SETTINGS_GETTER(iOSPlatformSettings, iOSPlatform);

namespace
{
    struct iOSPlatformCache
    {
        iOSPlatformSettings::TextureQuality TexturesQuality;
    };

    String GetAppName()
    {
        const auto gameSettings = GameSettings::Get();
        String productName = gameSettings->ProductName;
        productName.Replace(TEXT(" "), TEXT(""));
        productName.Replace(TEXT("."), TEXT(""));
        productName.Replace(TEXT("-"), TEXT(""));
        return productName;
    }

    const Char* GetExportMethod(iOSPlatformSettings::ExportMethods method)
    {
        switch (method)
        {
        case iOSPlatformSettings::ExportMethods::AppStore: return TEXT("app-store");
        case iOSPlatformSettings::ExportMethods::Development: return TEXT("development");
        case iOSPlatformSettings::ExportMethods::AdHoc: return TEXT("ad-hoc");
        case iOSPlatformSettings::ExportMethods::Enterprise: return TEXT("enterprise");
        default: return TEXT("");
        }
    }

    String GetUIInterfaceOrientation(iOSPlatformSettings::UIInterfaceOrientations orientations)
    {
        String result;
        if (EnumHasAnyFlags(orientations, iOSPlatformSettings::UIInterfaceOrientations::Portrait))
            result += TEXT("UIInterfaceOrientationPortrait ");
        if (EnumHasAnyFlags(orientations, iOSPlatformSettings::UIInterfaceOrientations::PortraitUpsideDown))
            result += TEXT("UIInterfaceOrientationPortraitUpsideDown ");
        if (EnumHasAnyFlags(orientations, iOSPlatformSettings::UIInterfaceOrientations::LandscapeLeft))
            result += TEXT("UIInterfaceOrientationLandscapeLeft ");
        if (EnumHasAnyFlags(orientations, iOSPlatformSettings::UIInterfaceOrientations::LandscapeRight))
            result += TEXT("UIInterfaceOrientationLandscapeRight ");
        result = result.TrimTrailing();
        return result;
    }

    PixelFormat GetQualityTextureFormat(bool sRGB, PixelFormat format)
    {
        const auto platformSettings = iOSPlatformSettings::Get();
        switch (platformSettings->TexturesQuality)
        {
        case iOSPlatformSettings::TextureQuality::Uncompressed:
            return PixelFormatExtensions::FindUncompressedFormat(format);
        case iOSPlatformSettings::TextureQuality::ASTC_High:
            return sRGB ? PixelFormat::ASTC_4x4_UNorm_sRGB : PixelFormat::ASTC_4x4_UNorm;
        case iOSPlatformSettings::TextureQuality::ASTC_Medium:
            return sRGB ? PixelFormat::ASTC_6x6_UNorm_sRGB : PixelFormat::ASTC_6x6_UNorm;
        case iOSPlatformSettings::TextureQuality::ASTC_Low:
            return sRGB ? PixelFormat::ASTC_8x8_UNorm_sRGB : PixelFormat::ASTC_8x8_UNorm;
        default:
            return format;
        }
    }
}

const Char* iOSPlatformTools::GetDisplayName() const
{
    return TEXT("iOS");
}

const Char* iOSPlatformTools::GetName() const
{
    return TEXT("iOS");
}

PlatformType iOSPlatformTools::GetPlatform() const
{
    return PlatformType::iOS;
}

ArchitectureType iOSPlatformTools::GetArchitecture() const
{
    return ArchitectureType::ARM64;
}

DotNetAOTModes iOSPlatformTools::UseAOT() const
{
    return DotNetAOTModes::MonoAOTDynamic;    
}

PixelFormat iOSPlatformTools::GetTextureFormat(CookingData& data, TextureBase* texture, PixelFormat format)
{
    switch (format)
    {
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
    return format;
}

bool iOSPlatformTools::IsNativeCodeFile(CookingData& data, const String& file)
{
    String extension = FileSystem::GetExtension(file);
    return extension.IsEmpty() || extension == TEXT("dylib");
}

void iOSPlatformTools::LoadCache(CookingData& data, IBuildCache* cache, const Span<byte>& bytes)
{
    const auto platformSettings = iOSPlatformSettings::Get();
    bool invalidTextures = true;
    if (bytes.Length() == sizeof(iOSPlatformCache))
    {
        auto* platformCache = (iOSPlatformCache*)bytes.Get();
        invalidTextures = platformCache->TexturesQuality != platformSettings->TexturesQuality;
    }
    if (invalidTextures)
    {
        LOG(Info, "{0} option has been modified.", TEXT("TexturesQuality"));
        cache->InvalidateCacheTextures();
    }
}

Array<byte> iOSPlatformTools::SaveCache(CookingData& data, IBuildCache* cache)
{
    const auto platformSettings = iOSPlatformSettings::Get();
    iOSPlatformCache platformCache;
    platformCache.TexturesQuality = platformSettings->TexturesQuality;
    Array<byte> result;
    result.Add((const byte*)&platformCache, sizeof(platformCache));
    return result;
}

void iOSPlatformTools::OnBuildStarted(CookingData& data)
{
    // Adjust the cooking output folders for packaging app
    const Char* subDir = TEXT("FlaxGame/Data");
    data.DataOutputPath /= subDir;
    data.NativeCodeOutputPath /= subDir;
    data.ManagedCodeOutputPath /= subDir;

    PlatformTools::OnBuildStarted(data);
}

bool iOSPlatformTools::OnPostProcess(CookingData& data)
{
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = iOSPlatformSettings::Get();
    const auto platformDataPath = data.GetPlatformBinariesRoot();
    const auto projectVersion = Editor::Project->Version.ToString();
    const auto appName = GetAppName();

    // Setup package name (eg. com.company.project)
    String appIdentifier = platformSettings->AppIdentifier;
    if (EditorUtilities::FormatAppPackageName(appIdentifier))
        return true;

    // Copy fresh XCode project template
    if (FileSystem::CopyDirectory(data.OriginalOutputPath, platformDataPath / TEXT("Project")))
    {
        LOG(Error, "Failed to deploy XCode project to {0} from {1}", data.OriginalOutputPath, platformDataPath);
        return true;
    }

    // Fix MoltenVK lib (copied from VulkanSDK xcframework)
    FileSystem::MoveFile(data.DataOutputPath / TEXT("libMoltenVK.dylib"), data.DataOutputPath / TEXT("MoltenVK"), true);
    {
        // Fix rpath to point into dynamic library (rather than framework location)
        CreateProcessSettings procSettings;
        procSettings.HiddenWindow = true;
        procSettings.WorkingDirectory = data.DataOutputPath;
        procSettings.FileName = TEXT("/usr/bin/install_name_tool");
        procSettings.Arguments = TEXT("-id \"@rpath/libMoltenVK.dylib\" libMoltenVK.dylib");
        Platform::CreateProcess(procSettings);
    }

    // Format project template files
    Dictionary<String, String> configReplaceMap;
    configReplaceMap[TEXT("${AppName}")] = appName;
    configReplaceMap[TEXT("${AppIdentifier}")] = appIdentifier;
    configReplaceMap[TEXT("${AppTeamId}")] = platformSettings->AppTeamId;
    configReplaceMap[TEXT("${AppVersion}")] = platformSettings->AppVersion;
    configReplaceMap[TEXT("${ProjectName}")] = gameSettings->ProductName;
    configReplaceMap[TEXT("${ProjectVersion}")] = projectVersion;
    configReplaceMap[TEXT("${HeaderSearchPaths}")] = Globals::StartupFolder;
    configReplaceMap[TEXT("${ExportMethod}")] = GetExportMethod(platformSettings->ExportMethod);
    configReplaceMap[TEXT("${UISupportedInterfaceOrientations_iPhone}")] = GetUIInterfaceOrientation(platformSettings->SupportedInterfaceOrientationsiPhone);
    configReplaceMap[TEXT("${UISupportedInterfaceOrientations_iPad}")] = GetUIInterfaceOrientation(platformSettings->SupportedInterfaceOrientationsiPad);
    {
        // Initialize auto-generated areas as empty
        configReplaceMap[TEXT("${PBXBuildFile}")] = String::Empty;
        configReplaceMap[TEXT("${PBXCopyFilesBuildPhaseFiles}")] = String::Empty;
        configReplaceMap[TEXT("${PBXFileReference}")] = String::Empty;
        configReplaceMap[TEXT("${PBXFrameworksBuildPhase}")] = String::Empty;
        configReplaceMap[TEXT("${PBXFrameworksGroup}")] = String::Empty;
        configReplaceMap[TEXT("${PBXFilesGroup}")] = String::Empty;
        configReplaceMap[TEXT("${PBXResourcesGroup}")] = String::Empty;
    }
    {
        // Rename dotnet license files to not mislead the actual game licensing
        FileSystem::MoveFile(data.DataOutputPath / TEXT("Dotnet/DOTNET-LICENSE.TXT"), data.DataOutputPath / TEXT("Dotnet/LICENSE.TXT"), true);
        FileSystem::MoveFile(data.DataOutputPath / TEXT("Dotnet/DOTNET-THIRD-PARTY-NOTICES.TXT"), data.DataOutputPath / TEXT("Dotnet/THIRD-PARTY-NOTICES.TXT"), true);
    }
    Array<String> files;
    FileSystem::DirectoryGetFiles(files, data.DataOutputPath, TEXT("*"), DirectorySearchOption::AllDirectories);
    for (const String& file : files)
    {
        String name = StringUtils::GetFileName(file);
        if (name == TEXT(".DS_Store") || name == TEXT("FlaxGame"))
            continue;
        String fileId = Guid::New().ToString(Guid::FormatType::N).Left(24);
        String projectPath = FileSystem::ConvertAbsolutePathToRelative(data.DataOutputPath, file);
        if (name.EndsWith(TEXT(".dylib")))
        {
            String frameworkId = Guid::New().ToString(Guid::FormatType::N).Left(24);
            String frameworkEmbedId = Guid::New().ToString(Guid::FormatType::N).Left(24);
            configReplaceMap[TEXT("${PBXBuildFile}")] += String::Format(TEXT("\t\t{0} /* {1} in Frameworks */ = {{isa = PBXBuildFile; fileRef = {2} /* {1} */; }};\n"), frameworkId, name, fileId);
            configReplaceMap[TEXT("${PBXBuildFile}")] += String::Format(TEXT("\t\t{0} /* {1} in Embed Frameworks */ = {{isa = PBXBuildFile; fileRef = {2} /* {1} */; settings = {{ATTRIBUTES = (CodeSignOnCopy, ); }}; }};\n"), frameworkEmbedId, name, fileId);
            configReplaceMap[TEXT("${PBXCopyFilesBuildPhaseFiles}")] += String::Format(TEXT("\t\t\t\t{0} /* {1} in Embed Frameworks */,\n"), frameworkEmbedId, name);
            configReplaceMap[TEXT("${PBXFileReference}")] += String::Format(TEXT("\t\t{0} /* {1} */ = {{isa = PBXFileReference; lastKnownFileType = \"compiled.mach-o.dylib\"; name = \"{1}\"; path = \"FlaxGame/Data/{2}\"; sourceTree = \"<group>\"; }};\n"), fileId, name, projectPath);
            configReplaceMap[TEXT("${PBXFrameworksBuildPhase}")] += String::Format(TEXT("\t\t\t\t{0} /* {1} in Frameworks */,\n"), frameworkId, name);
            configReplaceMap[TEXT("${PBXFrameworksGroup}")] += String::Format(TEXT("\t\t\t\t{0} /* {1} */,\n"), fileId, name);
        }
        else
        {
            String fileRefId = Guid::New().ToString(Guid::FormatType::N).Left(24);
            configReplaceMap[TEXT("${PBXBuildFile}")] += String::Format(TEXT("\t\t{0} /* {1} in Resources */ = {{isa = PBXBuildFile; fileRef = {2} /* {1} */; }};\n"), fileRefId, name, fileId);
            configReplaceMap[TEXT("${PBXFileReference}")] += String::Format(TEXT("\t\t{0} /* {1} */ = {{isa = PBXFileReference; lastKnownFileType = file; name = \"{1}\"; path = \"Data/{2}\"; sourceTree = \"<group>\"; }};\n"), fileId, name, projectPath);
            configReplaceMap[TEXT("${PBXFilesGroup}")] += String::Format(TEXT("\t\t\t\t{0} /* {1} */,\n"), fileId, name);
            configReplaceMap[TEXT("${PBXResourcesGroup}")] += String::Format(TEXT("\t\t\t\t{0} /* {1} in Resources */,\n"), fileRefId, name);
        }
    }
    bool failed = false;
    failed |= EditorUtilities::ReplaceInFile(data.OriginalOutputPath / TEXT("FlaxGame.xcodeproj/project.pbxproj"), configReplaceMap);
    failed |= EditorUtilities::ReplaceInFile(data.OriginalOutputPath / TEXT("ExportOptions.plist"), configReplaceMap);
    if (failed)
    {
        LOG(Error, "Failed to format XCode project");
        return true;
    }

    // Export images
    // TODO: provide per-device icons (eg. iPad 1x, iPad 2x) instead of a single icon size
    TextureData iconData;
    if (!EditorUtilities::GetApplicationImage(platformSettings->OverrideIcon, iconData))
    {
        String outputPath = data.OriginalOutputPath / TEXT("FlaxGame/Assets.xcassets/AppIcon.appiconset/ios_store_icon.png");
        if (EditorUtilities::ExportApplicationImage(iconData, 1024, 1024, PixelFormat::R8G8B8A8_UNorm, outputPath))
        {
            LOG(Error, "Failed to export application icon.");
            return true;
        }
    }

    // Package application
    const auto buildSettings = BuildSettings::Get();
    if (buildSettings->SkipPackaging)
        return false;
    GameCooker::PackageFiles();
    {
        LOG(Info, "Building app package...");
        const Char* configuration = data.Configuration == BuildConfiguration::Release ? TEXT("Release") : TEXT("Debug");
        CreateProcessSettings procSettings;
        procSettings.HiddenWindow = true;
        procSettings.WorkingDirectory = data.OriginalOutputPath;
        procSettings.FileName = TEXT("/usr/bin/xcodebuild");
        procSettings.Arguments = String::Format(TEXT("-project FlaxGame.xcodeproj -configuration {} -scheme FlaxGame -archivePath FlaxGame.xcarchive archive"), configuration);
        int32 result = Platform::CreateProcess(procSettings);
        if (result != 0)
        {
            data.Error(String::Format(TEXT("Failed to package app (result code: {0}). See log for more info."), result));
            return true;
        }
        procSettings.FileName = TEXT("/usr/bin/xcodebuild");
        procSettings.Arguments = TEXT("-exportArchive -archivePath FlaxGame.xcarchive -allowProvisioningUpdates -exportPath . -exportOptionsPlist ExportOptions.plist");
        result = Platform::CreateProcess(procSettings);
        if (result != 0)
        {
            data.Error(String::Format(TEXT("Failed to package app (result code: {0}). See log for more info."), result));
            return true;
        }
        const String ipaPath = data.OriginalOutputPath / TEXT("FlaxGame.ipa");
        LOG(Info, "Output application package: {0} (size: {1} MB)", ipaPath, FileSystem::GetFileSize(ipaPath) / 1024 / 1024);
    }

    return false;
}

#endif
