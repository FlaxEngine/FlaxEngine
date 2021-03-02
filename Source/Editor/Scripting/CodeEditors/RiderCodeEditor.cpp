// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "RiderCodeEditor.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Log.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Engine/Globals.h"

#if PLATFORM_WINDOWS

#include "Engine/Platform/File.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include "Engine/Serialization/Json.h"

namespace
{
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

    void SearchDirectory(Array<CodeEditor*>* output, const String& directory)
    {
        if (!FileSystem::DirectoryExists(directory))
            return;

        // Load product info
        Array<byte> productInfoData;
        const String productInfoPath = directory / TEXT("product-info.json");
        if (File::ReadAllBytes(productInfoPath, productInfoData))
            return;
        rapidjson_flax::Document document;
        document.Parse((char*)productInfoData.Get(), productInfoData.Count());
        if (document.HasParseError())
            return;

        // Find executable file path
        auto launchMember = document.FindMember("launch");
        if (launchMember != document.MemberEnd() && launchMember->value.IsArray() && launchMember->value.Size() > 0)
        {
            auto launcherPathMember = launchMember->value[0].FindMember("launcherPath");
            if (launcherPathMember != launchMember->value[0].MemberEnd())
            {
                auto launcherPath = launcherPathMember->value.GetText();
                auto exePath = directory / launcherPath;
                if (launcherPath.HasChars() && FileSystem::FileExists(exePath))
                {
                    output->Add(New<RiderCodeEditor>(exePath));
                }
            }
        }
    }

    void SearchRegistry(Array<CodeEditor*>* output, HKEY root, const Char* key)
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
                if (RegQueryValueExW(subKeyH, TEXT(""), nullptr, &type, nullptr, &cbData) != ERROR_SUCCESS || type != REG_SZ)
                {
                    RegCloseKey(subKeyH);
                    continue;
                }
                Array<Char> data;
                data.Resize((int32)cbData / sizeof(Char));
                if (RegQueryValueExW(subKeyH, TEXT(""), nullptr, nullptr, reinterpret_cast<LPBYTE>(data.Get()), &cbData) != ERROR_SUCCESS)
                {
                    RegCloseKey(subKeyH);
                    continue;
                }

                // Check if it's a valid installation path
                String path(data.Get(), data.Count() - 1);
                SearchDirectory(output, path);

                RegCloseKey(subKeyH);
            }
        }

        RegCloseKey(keyH);
    }
}

#endif

RiderCodeEditor::RiderCodeEditor(const String& execPath)
    : _execPath(execPath)
    , _solutionPath(Globals::ProjectFolder / Editor::Project->Name + TEXT(".sln"))
{
}

void RiderCodeEditor::FindEditors(Array<CodeEditor*>* output)
{
#if PLATFORM_WINDOWS
    SearchRegistry(output, HKEY_CURRENT_USER, TEXT("SOFTWARE\\WOW6432Node\\JetBrains\\JetBrains Rider"));
    SearchRegistry(output, HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\WOW6432Node\\JetBrains\\JetBrains Rider"));
#endif
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
        ScriptsBuilder::GenerateProject(TEXT("-vs2019"));
    }

    // Open file
    line = line > 0 ? line : 1;
    const String args = String::Format(TEXT("\"{0}\" --line {2} \"{1}\""), _solutionPath, path, line);
    Platform::StartProcess(_execPath, args, StringView::Empty);
}

void RiderCodeEditor::OpenSolution()
{
    // Generate project files if solution is missing
    if (!FileSystem::FileExists(_solutionPath))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vs2019"));
    }

    // Open solution
    const String args = String::Format(TEXT("\"{0}\""), _solutionPath);
    Platform::StartProcess(_execPath, args, StringView::Empty);
}

void RiderCodeEditor::OnFileAdded(const String& path)
{
    ScriptsBuilder::GenerateProject();
}
