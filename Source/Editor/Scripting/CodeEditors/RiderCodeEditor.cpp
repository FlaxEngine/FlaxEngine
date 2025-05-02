// Copyright (c) Wojciech Figat. All rights reserved.

#include "RiderCodeEditor.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Log.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Serialization/Json.h"

#if PLATFORM_WINDOWS
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#endif

namespace
{
    struct RiderInstallation
    {
        String path;
        String version;

        RiderInstallation(const String& path_, const String& version_)
            : path(path_)
            , version(version_)
        {
        }
    };

    void SearchDirectory(Array<RiderInstallation*>* installations, const String& directory, String launchOverridePath = String::Empty)
    {
        if (!FileSystem::DirectoryExists(directory))
            return;

        // Load product info
        Array<byte> productInfoData;
        const String productInfoPath = directory / TEXT("product-info.json");
        if (!FileSystem::FileExists(productInfoPath) || File::ReadAllBytes(productInfoPath, productInfoData))
            return;
        rapidjson_flax::Document document;
        document.Parse((char*)productInfoData.Get(), productInfoData.Count());
        if (document.HasParseError())
            return;

        // Check if this is actually rider and not another jetbrains product
        if (document.FindMember("name")->value != "JetBrains Rider")
            return;

        // Find version
        auto versionMember = document.FindMember("version");
        if (versionMember == document.MemberEnd())
            return;

        // Find executable file path
        auto launchMember = document.FindMember("launch");
        if (launchMember == document.MemberEnd() || !launchMember->value.IsArray() || launchMember->value.Size() == 0)
            return;

        auto launcherPathMember = launchMember->value[0].FindMember("launcherPath");
        if (launcherPathMember == launchMember->value[0].MemberEnd())
            return;

        auto launcherPath = launcherPathMember->value.GetText();
        auto exePath = directory / launcherPath;
        if (!launcherPath.HasChars() || !FileSystem::FileExists(exePath))
            return;

        if (launchOverridePath != String::Empty)
            installations->Add(New<RiderInstallation>(launchOverridePath, versionMember->value.GetText()));
        else
            installations->Add(New<RiderInstallation>(exePath, versionMember->value.GetText()));
    }

#if PLATFORM_WINDOWS
    bool FindRegistryKeyItems(HKEY hKey, Array<String>& results)
    {
        Char nameBuffer[256];
        for (int32 i = 0;; i++)
        {
            const LONG result = RegEnumKeyW(hKey, i, nameBuffer, ARRAY_COUNT(nameBuffer));
            if (result == ERROR_NO_MORE_ITEMS)
                break;
            if (result != ERROR_SUCCESS)
                return false;
            results.Add(nameBuffer);
        }
        return true;
    }

    void SearchRegistry(Array<RiderInstallation*>* installations, HKEY root, const Char* key, const Char* valueName = TEXT(""))
    {
        // Open key
        HKEY keyH;
        if (RegOpenKeyExW(root, key, 0, KEY_READ, &keyH) != ERROR_SUCCESS)
            return;

        // Iterate over subkeys
        Array<String> subKeys;
        if (FindRegistryKeyItems(keyH, subKeys))
        {
            for (auto& subKey : subKeys)
            {
                HKEY subKeyH;
                if (RegOpenKeyExW(keyH, *subKey, 0, KEY_READ, &subKeyH) != ERROR_SUCCESS)
                    continue;

                // Read subkey value
                DWORD type;
                DWORD cbData;
                if (RegQueryValueExW(subKeyH, valueName, nullptr, &type, nullptr, &cbData) != ERROR_SUCCESS || type != REG_SZ)
                {
                    RegCloseKey(subKeyH);
                    continue;
                }
                Array<Char> data;
                data.Resize((int32)cbData / sizeof(Char));
                if (RegQueryValueExW(subKeyH, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(data.Get()), &cbData) != ERROR_SUCCESS)
                {
                    RegCloseKey(subKeyH);
                    continue;
                }

                // Check if it's a valid installation path
                String path(data.Get(), data.Count() - 1);
                SearchDirectory(installations, path);

                RegCloseKey(subKeyH);
            }
        }

        RegCloseKey(keyH);
    }
#endif
}

bool sortInstallations(RiderInstallation* const& i1, RiderInstallation* const& i2)
{
    Array<String> values1, values2;
    i1->version.Split('.', values1);
    i2->version.Split('.', values2);

    int32 version1[3] = { 0 };
    int32 version2[3] = { 0 };
    StringUtils::Parse(values1[0].Get(), &version1[0]);
    StringUtils::Parse(values1[1].Get(), &version1[1]);

    if (values1.Count() > 2)
        StringUtils::Parse(values1[2].Get(), &version1[2]);

    StringUtils::Parse(values2[0].Get(), &version2[0]);
    StringUtils::Parse(values2[1].Get(), &version2[1]);

    if (values2.Count() > 2)
        StringUtils::Parse(values2[2].Get(), &version2[2]);

    // Compare by MAJOR.MINOR.BUILD
    if (version1[0] == version2[0])
    {
        if (version1[1] == version2[1])
            return version1[2] > version2[2];
        return version1[1] > version2[1];
    }
    return version1[0] > version2[0];
}

RiderCodeEditor::RiderCodeEditor(const String& execPath)
    : _execPath(execPath)
    , _solutionPath(Globals::ProjectFolder / Editor::Project->Name + TEXT(".sln"))
{
}

void RiderCodeEditor::FindEditors(Array<CodeEditor*>* output)
{
    Array<RiderInstallation*> installations;
    Array<String> subDirectories;

    String localAppDataPath;
    FileSystem::GetSpecialFolderPath(SpecialFolder::LocalAppData, localAppDataPath);

#if PLATFORM_WINDOWS
    // Lookup from all known registry locations
    SearchRegistry(&installations, HKEY_CURRENT_USER, TEXT("SOFTWARE\\WOW6432Node\\JetBrains\\Rider for Unreal Engine"));
    SearchRegistry(&installations, HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\WOW6432Node\\JetBrains\\Rider for Unreal Engine"));
    SearchRegistry(&installations, HKEY_CURRENT_USER, TEXT("SOFTWARE\\JetBrains\\JetBrains Rider"));
    SearchRegistry(&installations, HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\JetBrains\\JetBrains Rider"));
    SearchRegistry(&installations, HKEY_CURRENT_USER, TEXT("SOFTWARE\\JetBrains\\Rider"), TEXT("InstallDir"));
    SearchRegistry(&installations, HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\JetBrains\\Rider"), TEXT("InstallDir"));
    SearchRegistry(&installations, HKEY_CURRENT_USER, TEXT("SOFTWARE\\WOW6432Node\\JetBrains\\JetBrains Rider"));
    SearchRegistry(&installations, HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\WOW6432Node\\JetBrains\\JetBrains Rider"));

    // Versions installed via JetBrains Toolbox
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("Programs"));
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("JetBrains\\Toolbox\\apps\\Rider\\ch-0\\"));
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("JetBrains\\Toolbox\\apps\\Rider\\ch-1\\")); // Beta versions
#endif
#if PLATFORM_LINUX
    // TODO: detect Snap installations
    // TODO: detect by reading the jetbrains-rider.desktop file from ~/.local/share/applications and /usr/share/applications?

    SearchDirectory(&installations, TEXT("/usr/share/rider/"));
    FileSystem::GetChildDirectories(subDirectories, TEXT("/usr/share/rider"));

    // Default suggested location for standalone installations
    FileSystem::GetChildDirectories(subDirectories, TEXT("/opt/"));
    
    // Versions installed via JetBrains Toolbox
    SearchDirectory(&installations, localAppDataPath / TEXT("JetBrains/Toolbox/apps/rider/"));
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("JetBrains/Toolbox/apps/Rider/ch-0"));
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("JetBrains/Toolbox/apps/Rider/ch-1")); // Beta versions

    // Detect Flatpak installations
    SearchDirectory(&installations,
        TEXT("/var/lib/flatpak/app/com.jetbrains.Rider/current/active/files/extra/rider/"),
        TEXT("flatpak run com.jetbrains.Rider"));
#endif

#if PLATFORM_MAC
    String applicationSupportFolder;
    FileSystem::GetSpecialFolderPath(SpecialFolder::ProgramData, applicationSupportFolder);

    Array<String> subMacDirectories;
    FileSystem::GetChildDirectories(subMacDirectories, applicationSupportFolder / TEXT("JetBrains/Toolbox/apps/Rider/ch-0/"));
    FileSystem::GetChildDirectories(subMacDirectories, applicationSupportFolder / TEXT("JetBrains/Toolbox/apps/Rider/ch-1/"));
    for (const String& directory : subMacDirectories)
    {
        String riderAppDirectory = directory / TEXT("Rider.app/Contents/Resources");
        SearchDirectory(&installations, riderAppDirectory);
    }

    // Check the local installer version
    SearchDirectory(&installations, TEXT("/Applications/Rider.app/Contents/Resources"));
#endif

    for (const String& directory : subDirectories)
        SearchDirectory(&installations, directory);

    // Sort found installations by version number
    Sorting::QuickSort(installations.Get(), installations.Count(), &sortInstallations);

    for (RiderInstallation* installation : installations)
    {
        output->Add(New<RiderCodeEditor>(installation->path));
        Delete(installation);
    }
}

CodeEditorTypes RiderCodeEditor::GetType() const
{
    return CodeEditorTypes::Rider;
}

String RiderCodeEditor::GetName() const
{
    return TEXT("Rider");
}

void RiderCodeEditor::OpenFile(const String& path, int32 line)
{
    // Generate project files if solution is missing
    if (!FileSystem::FileExists(_solutionPath))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vs2022"));
    }

    // Open file
    line = line > 0 ? line : 1;
    CreateProcessSettings procSettings;

#if !PLATFORM_MAC
    procSettings.FileName = _execPath;
    procSettings.Arguments = String::Format(TEXT("\"{0}\" --line {2} \"{1}\""), _solutionPath, path, line);
#else
    // This follows pretty much how all the other engines open rider which deals with cross architecture issues
    procSettings.FileName = "/usr/bin/open";
    procSettings.Arguments = String::Format(TEXT("-n -a \"{0}\" --args \"{1}\" --line {3} \"{2}\""), _execPath, _solutionPath, path, line);
#endif

    procSettings.HiddenWindow = false;
    procSettings.WaitForEnd = false;
    procSettings.LogOutput = false;
    procSettings.ShellExecute = true;
    Platform::CreateProcess(procSettings);
}

void RiderCodeEditor::OpenSolution()
{
    // Generate project files if solution is missing
    if (!FileSystem::FileExists(_solutionPath))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vs2022"));
    }

    // Open solution
    CreateProcessSettings procSettings;
#if !PLATFORM_MAC
    procSettings.FileName = _execPath;
    procSettings.Arguments = String::Format(TEXT("\"{0}\""), _solutionPath);
#else
    // This follows pretty much how all the other engines open rider which deals with cross architecture issues
    procSettings.FileName = "/usr/bin/open";
    procSettings.Arguments = String::Format(TEXT("-n -a \"{0}\" \"{1}\""), _execPath, _solutionPath);
#endif
    procSettings.HiddenWindow = false;
    procSettings.WaitForEnd = false;
    procSettings.LogOutput = false;
    procSettings.ShellExecute = true;
    Platform::CreateProcess(procSettings);
}

void RiderCodeEditor::OnFileAdded(const String& path)
{
    ScriptsBuilder::GenerateProject();
}
