// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_TOOLS_MAC

#include "MacPlatformTools.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/Mac/MacPlatformSettings.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Config/BuildSettings.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Engine/Globals.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Cooker/GameCooker.h"
#include "Editor/Utilities/EditorUtilities.h"
#include <ThirdParty/pugixml/pugixml.hpp>
using namespace pugi;

IMPLEMENT_SETTINGS_GETTER(MacPlatformSettings, MacPlatform);

namespace
{
    String GetAppName()
    {
        const auto gameSettings = GameSettings::Get();
        String productName = gameSettings->ProductName;
        productName.Replace(TEXT(" "), TEXT(""));
        productName.Replace(TEXT("."), TEXT(""));
        productName.Replace(TEXT("-"), TEXT(""));
        return productName;
    }
}

MacPlatformTools::MacPlatformTools(ArchitectureType arch)
    : _arch(arch)
{
}

const Char* MacPlatformTools::GetDisplayName() const
{
    return TEXT("Mac");
}

const Char* MacPlatformTools::GetName() const
{
    return TEXT("Mac");
}

PlatformType MacPlatformTools::GetPlatform() const
{
    return PlatformType::Mac;
}

ArchitectureType MacPlatformTools::GetArchitecture() const
{
    return _arch;
}

bool MacPlatformTools::UseSystemDotnet() const
{
    return true;
}

bool MacPlatformTools::IsNativeCodeFile(CookingData& data, const String& file)
{
    String extension = FileSystem::GetExtension(file);
    return extension.IsEmpty() || extension == TEXT("dylib");
}

void MacPlatformTools::OnBuildStarted(CookingData& data)
{
    // Adjust the cooking output folders for packaging app
    const auto appName = GetAppName();
    String contents = appName + TEXT(".app/Contents/");
    data.DataOutputPath /= contents;
    data.NativeCodeOutputPath /= contents / TEXT("MacOS");
    data.ManagedCodeOutputPath /= contents;

    PlatformTools::OnBuildStarted(data);
}

bool MacPlatformTools::OnPostProcess(CookingData& data)
{
    const auto gameSettings = GameSettings::Get();
    const auto platformSettings = MacPlatformSettings::Get();
    const auto platformDataPath = data.GetPlatformBinariesRoot();
    const auto projectVersion = Editor::Project->Version.ToString();
    const auto appName = GetAppName();

    // Setup package name (eg. com.company.project)
    String appIdentifier = platformSettings->AppIdentifier;
    {
        String productName = gameSettings->ProductName;
        productName.Replace(TEXT(" "), TEXT(""));
        productName.Replace(TEXT("."), TEXT(""));
        productName.Replace(TEXT("-"), TEXT(""));
        String companyName = gameSettings->CompanyName;
        companyName.Replace(TEXT(" "), TEXT(""));
        companyName.Replace(TEXT("."), TEXT(""));
        companyName.Replace(TEXT("-"), TEXT(""));
        appIdentifier.Replace(TEXT("${PROJECT_NAME}"), *productName, StringSearchCase::IgnoreCase);
        appIdentifier.Replace(TEXT("${COMPANY_NAME}"), *companyName, StringSearchCase::IgnoreCase);
        appIdentifier = appIdentifier.ToLower();
        for (int32 i = 0; i < appIdentifier.Length(); i++)
        {
            const auto c = appIdentifier[i];
            if (c != '_' && c != '.' && !StringUtils::IsAlnum(c))
            {
                LOG(Error, "Apple app identifier \'{0}\' contains invalid character. Only letters, numbers, dots and underscore characters are allowed.", appIdentifier);
                return true;
            }
        }
        if (appIdentifier.IsEmpty())
        {
            LOG(Error, "Apple app identifier is empty.", appIdentifier);
            return true;
        }
    }

    // Find executable
    String executableName;
    {
        Array<String> files;
        FileSystem::DirectoryGetFiles(files, data.NativeCodeOutputPath, TEXT("*"), DirectorySearchOption::TopDirectoryOnly);
        for (auto& file : files)
        {
            if (FileSystem::GetExtension(file).IsEmpty())
            {
                executableName = StringUtils::GetFileName(file);
                break;
            }
        }
    }

    // Deploy app icon
    TextureData iconData;
    if (!EditorUtilities::GetApplicationImage(platformSettings->OverrideIcon, iconData))
    {
        String iconFolderPath = data.DataOutputPath / TEXT("Resources");
        String tmpFolderPath = iconFolderPath / TEXT("icon.iconset");
        if (!FileSystem::DirectoryExists(tmpFolderPath))
            FileSystem::CreateDirectory(tmpFolderPath);
        String srcIconPath = tmpFolderPath / TEXT("icon_1024x1024.png");
        if (EditorUtilities::ExportApplicationImage(iconData, 1024, 1024, PixelFormat::R8G8B8A8_UNorm, srcIconPath))
        {
            LOG(Error, "Failed to export application icon.");
            return true;
        }
        bool failed = Platform::RunProcess(TEXT("sips -z 16 16 icon_1024x1024.png --out icon_16x16.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 32 32 icon_1024x1024.png --out icon_16x16@2x.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 32 32 icon_1024x1024.png --out icon_32x32.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 64 64 icon_1024x1024.png --out icon_32x32@2x.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 128 128 icon_1024x1024.png --out icon_128x128.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 256 256 icon_1024x1024.png --out icon_128x128@2x.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 256 256 icon_1024x1024.png --out icon_256x256.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 512 512 icon_1024x1024.png --out icon_256x256@2x.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 512 512 icon_1024x1024.png --out icon_512x512.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("sips -z 1024 1024 icon_1024x1024.png --out icon_512x512@2x.png"), tmpFolderPath);
        failed |= Platform::RunProcess(TEXT("iconutil -c icns icon.iconset"), iconFolderPath);
        if (failed)
        {
            LOG(Error, "Failed to export application icon.");
            return true;
        }
        FileSystem::DeleteDirectory(tmpFolderPath);
    }

    // Create PkgInfo file
    const String pkgInfoPath = data.DataOutputPath / TEXT("PkgInfo");
    File::WriteAllText(pkgInfoPath, TEXT("APPL???"), Encoding::ANSI);

    // Create Info.plist file with package description
    const String plistPath = data.DataOutputPath / TEXT("Info.plist");
    {
        xml_document doc;
        xml_node plist = doc.child_or_append(PUGIXML_TEXT("plist"));
        plist.append_attribute(PUGIXML_TEXT("version")).set_value(PUGIXML_TEXT("1.0"));
        xml_node dict = plist.child_or_append(PUGIXML_TEXT("dict"));

#define ADD_ENTRY(key, value) \
    dict.append_child(PUGIXML_TEXT("key")).set_child_value(PUGIXML_TEXT(key)); \
    dict.append_child(PUGIXML_TEXT("string")).set_child_value(PUGIXML_TEXT(value))
#define ADD_ENTRY_STR(key, value) \
    dict.append_child(PUGIXML_TEXT("key")).set_child_value(PUGIXML_TEXT(key)); \
    { std::u16string valueStr(value.GetText()); \
    dict.append_child(PUGIXML_TEXT("string")).set_child_value(pugi::string_t(valueStr.begin(), valueStr.end()).c_str()); }

        ADD_ENTRY("CFBundleDevelopmentRegion", "English");
        ADD_ENTRY("CFBundlePackageType", "APPL");
        ADD_ENTRY("NSPrincipalClass", "NSApplication");
        ADD_ENTRY("LSApplicationCategoryType", "public.app-category.games");
        ADD_ENTRY("LSMinimumSystemVersion", "10.15");
        ADD_ENTRY("CFBundleIconFile", "icon.icns");
        ADD_ENTRY_STR("CFBundleExecutable", executableName);
        ADD_ENTRY_STR("CFBundleIdentifier", appIdentifier);
        ADD_ENTRY_STR("CFBundleGetInfoString", gameSettings->ProductName);
        ADD_ENTRY_STR("CFBundleVersion", projectVersion);
        ADD_ENTRY_STR("NSHumanReadableCopyright", gameSettings->CopyrightNotice);

        dict.append_child(PUGIXML_TEXT("key")).set_child_value(PUGIXML_TEXT("CFBundleSupportedPlatforms"));
        xml_node CFBundleSupportedPlatforms = dict.append_child(PUGIXML_TEXT("array"));
        CFBundleSupportedPlatforms.append_child(PUGIXML_TEXT("string")).set_child_value(PUGIXML_TEXT("MacOSX"));

        dict.append_child(PUGIXML_TEXT("key")).set_child_value(PUGIXML_TEXT("LSMinimumSystemVersionByArchitecture"));
        xml_node LSMinimumSystemVersionByArchitecture = dict.append_child(PUGIXML_TEXT("dict"));
        switch (_arch)
        {
        case ArchitectureType::x64:
            LSMinimumSystemVersionByArchitecture.append_child(PUGIXML_TEXT("key")).set_child_value(PUGIXML_TEXT("x86_64"));
            break;
        case ArchitectureType::ARM64:
            LSMinimumSystemVersionByArchitecture.append_child(PUGIXML_TEXT("key")).set_child_value(PUGIXML_TEXT("arm64"));
            break;
        }
        LSMinimumSystemVersionByArchitecture.append_child(PUGIXML_TEXT("string")).set_child_value(PUGIXML_TEXT("10.15"));
        
#undef ADD_ENTRY
#undef ADD_ENTRY_STR

        if (!doc.save_file(*StringAnsi(plistPath)))
        {
            LOG(Error, "Failed to save {0}", plistPath);
            return true;
        }
    }

    // TODO: sign binaries

    // TODO: expose event to inject custom post-processing before app packaging (eg. third-party plugins)
    
    // Package application
    const auto buildSettings = BuildSettings::Get();
    if (buildSettings->SkipPackaging)
        return false;
    GameCooker::PackageFiles();
    LOG(Info, "Building app package...");
    const String dmgPath = data.OriginalOutputPath / appName + TEXT(".dmg");
    const String dmgCommand = String::Format(TEXT("hdiutil create {0}.dmg -volname {0} -fs HFS+ -srcfolder {0}.app"), appName);
    const int32 result = Platform::RunProcess(dmgCommand, data.OriginalOutputPath);
    if (result != 0)
    {
        data.Error(TEXT("Failed to package app (result code: {0}). See log for more info."), result);
        return true;
    }
    // TODO: sign dmg
    LOG(Info, "Output application package: {0} (size: {1} MB)", dmgPath, FileSystem::GetFileSize(dmgPath) / 1024 / 1024);

    return false;
}

#endif
