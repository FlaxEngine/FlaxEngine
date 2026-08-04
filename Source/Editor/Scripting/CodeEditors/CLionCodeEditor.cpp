// Copyright (c) Wojciech Figat. All rights reserved.

#include "CLionCodeEditor.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/File.h"
#include "Engine/Serialization/Json.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Scripting/ScriptsBuilder.h"

#if PLATFORM_WINDOWS
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#elif PLATFORM_MAC
#include "Engine/Platform/Apple/AppleUtils.h"
#include <AppKit/AppKit.h>
#endif

namespace
{
    struct CLionInstallation
    {
        String path;
        String argumentsPrefix;
        String version;

        CLionInstallation(const String& path_, const String& argumentsPrefix_, const String& version_)
            : path(path_)
            , argumentsPrefix(argumentsPrefix_)
            , version(version_)
        {
        }
    };

    void AddInstallation(Array<CLionInstallation*>* installations, String path, const String& version, const String& argumentsPrefix = String::Empty)
    {
        if (path.IsEmpty())
            return;

        StringUtils::PathRemoveRelativeParts(path);
        for (CLionInstallation* installation : *installations)
        {
            if (installation->path == path && installation->argumentsPrefix == argumentsPrefix)
                return;
        }
        installations->Add(New<CLionInstallation>(path, argumentsPrefix, version));
    }

    bool IsCLionProduct(rapidjson_flax::Document& document)
    {
        auto productCodeMember = document.FindMember("productCode");
        if (productCodeMember != document.MemberEnd() && productCodeMember->value == "CL")
            return true;

        auto nameMember = document.FindMember("name");
        if (nameMember == document.MemberEnd())
            return false;

        const String name = nameMember->value.GetText();
        return name == TEXT("CLion") || name == TEXT("JetBrains CLion");
    }

    void SearchDirectory(Array<CLionInstallation*>* installations, const String& directory, String launchOverridePath = String::Empty, String launchArgumentsPrefix = String::Empty)
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
        if (document.HasParseError() || !IsCLionProduct(document))
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

        AddInstallation(installations, launchOverridePath != String::Empty ? launchOverridePath : exePath, versionMember->value.GetText(), launchArgumentsPrefix);
    }

    void SearchExecutable(Array<CLionInstallation*>* installations, const String& path)
    {
        if (FileSystem::FileExists(path))
            AddInstallation(installations, path, TEXT("0.0.0"));
    }

    void ParseVersion(const String& text, int32 version[3])
    {
        version[0] = 0;
        version[1] = 0;
        version[2] = 0;

        Array<String> values;
        text.Split('.', values);
        for (int32 i = 0; i < values.Count() && i < 3; i++)
            StringUtils::Parse(values[i].Get(), &version[i]);
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

    void SearchRegistry(Array<CLionInstallation*>* installations, HKEY root, const Char* key, const Char* valueName = TEXT(""))
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

bool SortInstallations(CLionInstallation* const& i1, CLionInstallation* const& i2)
{
    int32 version1[3], version2[3];
    ParseVersion(i1->version, version1);
    ParseVersion(i2->version, version2);

    // Compare by MAJOR.MINOR.BUILD
    if (version1[0] == version2[0])
    {
        if (version1[1] == version2[1])
            return version1[2] > version2[2];
        return version1[1] > version2[1];
    }
    return version1[0] > version2[0];
}

CLionCodeEditor::CLionCodeEditor(const String& execPath, const String& execArgsPrefix)
    : _execPath(execPath)
    , _execArgsPrefix(execArgsPrefix)
    , _projectPath(Globals::ProjectFolder / TEXT("Cache/Projects/CMake") / Editor::Project->Name)
{
}

String CLionCodeEditor::GetProcessArguments(const String& arguments) const
{
    return _execArgsPrefix.HasChars() ? _execArgsPrefix + TEXT(" ") + arguments : arguments;
}

void CLionCodeEditor::FindEditors(Array<CodeEditor*>* output)
{
    Array<CLionInstallation*> installations;
    Array<String> subDirectories;

    String localAppDataPath;
    FileSystem::GetSpecialFolderPath(SpecialFolder::LocalAppData, localAppDataPath);

#if PLATFORM_WINDOWS
    // Lookup from all known registry locations
    SearchRegistry(&installations, HKEY_CURRENT_USER, TEXT("SOFTWARE\\JetBrains\\CLion"));
    SearchRegistry(&installations, HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\JetBrains\\CLion"));
    SearchRegistry(&installations, HKEY_CURRENT_USER, TEXT("SOFTWARE\\JetBrains\\CLion"), TEXT("InstallDir"));
    SearchRegistry(&installations, HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\JetBrains\\CLion"), TEXT("InstallDir"));
    SearchRegistry(&installations, HKEY_CURRENT_USER, TEXT("SOFTWARE\\WOW6432Node\\JetBrains\\CLion"));
    SearchRegistry(&installations, HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\WOW6432Node\\JetBrains\\CLion"));

    // Versions installed via JetBrains Toolbox
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("Programs"));
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("JetBrains\\Toolbox\\apps\\CLion\\ch-0\\"));
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("JetBrains\\Toolbox\\apps\\CLion\\ch-1\\")); // Beta versions
#endif
#if PLATFORM_LINUX
    // TODO: detect Snap installations by reading the desktop file from ~/.local/share/applications and /usr/share/applications.

    SearchDirectory(&installations, TEXT("/usr/share/clion/"));
    FileSystem::GetChildDirectories(subDirectories, TEXT("/usr/share/clion"));

    // Default suggested location for standalone installations
    FileSystem::GetChildDirectories(subDirectories, TEXT("/opt/"));

    // Versions installed via JetBrains Toolbox
    SearchDirectory(&installations, localAppDataPath / TEXT("JetBrains/Toolbox/apps/clion/"));
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("JetBrains/Toolbox/apps/CLion/ch-0"));
    FileSystem::GetChildDirectories(subDirectories, localAppDataPath / TEXT("JetBrains/Toolbox/apps/CLion/ch-1")); // Beta versions

    // Detect Flatpak installations
    SearchDirectory(&installations,
        TEXT("/var/lib/flatpak/app/com.jetbrains.CLion/current/active/files/extra/clion/"),
        TEXT("flatpak"),
        TEXT("run com.jetbrains.CLion"));

    SearchExecutable(&installations, TEXT("/usr/bin/clion"));
    SearchExecutable(&installations, TEXT("/snap/bin/clion"));
#endif

#if PLATFORM_MAC
    String applicationSupportFolder;
    FileSystem::GetSpecialFolderPath(SpecialFolder::ProgramData, applicationSupportFolder);

    NSURL* appURL = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:@"com.jetbrains.CLion"];
    if (appURL != nullptr)
    {
        const String appPath = AppleUtils::ToString((CFStringRef)[appURL path]);
        SearchDirectory(&installations, appPath / TEXT("Contents/Resources"), appPath);
    }

    Array<String> subMacDirectories;
    FileSystem::GetChildDirectories(subMacDirectories, applicationSupportFolder / TEXT("JetBrains/Toolbox/apps/CLion/ch-0/"));
    FileSystem::GetChildDirectories(subMacDirectories, applicationSupportFolder / TEXT("JetBrains/Toolbox/apps/CLion/ch-1/"));
    for (const String& directory : subMacDirectories)
    {
        String clionAppPath = directory / TEXT("CLion.app");
        SearchDirectory(&installations, clionAppPath / TEXT("Contents/Resources"), clionAppPath);
    }

    // Check the local installer version
    SearchDirectory(&installations, TEXT("/Applications/CLion.app/Contents/Resources"), TEXT("/Applications/CLion.app"));

    String userFolder;
    FileSystem::GetSpecialFolderPath(SpecialFolder::Documents, userFolder);
    String clionAppPath = userFolder / TEXT("../Applications/CLion.app");
    SearchDirectory(&installations, clionAppPath / TEXT("Contents/Resources"), clionAppPath);
#endif

    for (const String& directory : subDirectories)
        SearchDirectory(&installations, directory);

    // Sort found installations by version number
    Sorting::QuickSort(installations.Get(), installations.Count(), &SortInstallations);

    for (CLionInstallation* installation : installations)
    {
        output->Add(New<CLionCodeEditor>(installation->path, installation->argumentsPrefix));
        Delete(installation);
    }
}

CodeEditorTypes CLionCodeEditor::GetType() const
{
    return CodeEditorTypes::CLion;
}

String CLionCodeEditor::GetName() const
{
    return TEXT("CLion");
}

String CLionCodeEditor::GetGenerateProjectCustomArgs() const
{
    return TEXT("-clion");
}

void CLionCodeEditor::OpenFile(const String& path, int32 line)
{
    // Generate project files if missing
    if (!FileSystem::FileExists(_projectPath / TEXT("CMakeLists.txt")) ||
        !FileSystem::FileExists(_projectPath / TEXT("CMakePresets.json")))
    {
        ScriptsBuilder::GenerateProject(GetGenerateProjectCustomArgs());
    }

    // Open file
    line = line > 0 ? line : 1;
    CreateProcessSettings procSettings;

#if !PLATFORM_MAC
    procSettings.FileName = _execPath;
    procSettings.Arguments = GetProcessArguments(String::Format(TEXT("\"{0}\" --line {2} \"{1}\""), _projectPath, path, line));
#else
    procSettings.FileName = "/usr/bin/open";
    procSettings.Arguments = String::Format(TEXT("-n -a \"{0}\" --args \"{1}\" --line {3} \"{2}\""), _execPath, _projectPath, path, line);
#endif

    procSettings.HiddenWindow = false;
    procSettings.WaitForEnd = false;
    procSettings.LogOutput = false;
    procSettings.ShellExecute = true;
    Platform::CreateProcess(procSettings);
}

void CLionCodeEditor::OpenSolution()
{
    // Generate project files if missing
    if (!FileSystem::FileExists(_projectPath / TEXT("CMakeLists.txt")) ||
        !FileSystem::FileExists(_projectPath / TEXT("CMakePresets.json")))
    {
        ScriptsBuilder::GenerateProject(GetGenerateProjectCustomArgs());
    }

    // Open solution
    CreateProcessSettings procSettings;
#if !PLATFORM_MAC
    procSettings.FileName = _execPath;
    procSettings.Arguments = GetProcessArguments(String::Format(TEXT("\"{0}\""), _projectPath));
#else
    procSettings.FileName = "/usr/bin/open";
    procSettings.Arguments = String::Format(TEXT("-n -a \"{0}\" \"{1}\""), _execPath, _projectPath);
#endif
    procSettings.HiddenWindow = false;
    procSettings.WaitForEnd = false;
    procSettings.LogOutput = false;
    procSettings.ShellExecute = true;
    Platform::CreateProcess(procSettings);
}

void CLionCodeEditor::OnFileAdded(const String& path)
{
    ScriptsBuilder::GenerateProject(GetGenerateProjectCustomArgs());
}
