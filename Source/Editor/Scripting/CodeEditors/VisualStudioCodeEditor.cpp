// Copyright (c) Wojciech Figat. All rights reserved.

#include "VisualStudioCodeEditor.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Core/Log.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Engine/Globals.h"
#if PLATFORM_LINUX
#include <stdio.h>
#elif PLATFORM_WINDOWS
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#elif PLATFORM_MAC
#include "Engine/Platform/Apple/AppleUtils.h"
#include <AppKit/AppKit.h>
#endif

VisualStudioCodeEditor::VisualStudioCodeEditor(const String& execPath, const bool isInsiders)
    : _execPath(execPath)
    , _workspacePath(Globals::ProjectFolder / Editor::Project->Name + TEXT(".code-workspace"))
    , _isInsiders(isInsiders)
{
}

void VisualStudioCodeEditor::FindEditors(Array<CodeEditor*>* output)
{
#if PLATFORM_WINDOWS
    String cmd;
    bool isInsiders = false;
    if (Platform::ReadRegValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command"), TEXT(""), &cmd) || cmd.IsEmpty())
    {
        if (Platform::ReadRegValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command"), TEXT(""), &cmd) || cmd.IsEmpty())
        {
            if (Platform::ReadRegValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Classes\\Applications\\Code - Insiders.exe\\shell\\open\\command"), TEXT(""), &cmd) || cmd.IsEmpty())
            {
                if (Platform::ReadRegValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Applications\\Code - Insiders.exe\\shell\\open\\command"), TEXT(""), &cmd) || cmd.IsEmpty())
                {
                    return;
                }
                else
                    isInsiders = true;
            }
            else
                isInsiders = true;
        }
    }
    const String path = cmd.Substring(1, cmd.Length() - String(TEXT("\" \"%1\"")).Length() - 1);
    if (FileSystem::FileExists(path))
    {
        output->Add(New<VisualStudioCodeEditor>(path, isInsiders));
    }
#elif PLATFORM_LINUX
    char buffer[128];
    FILE* pipe = popen("/bin/bash -c \"type -p code\"", "r");
    if (pipe)
    {
        StringAnsi pathAnsi;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
            pathAnsi += buffer;
        pclose(pipe);
        const String path(pathAnsi.Get(), pathAnsi.Length() != 0 ? pathAnsi.Length() - 1 : 0);
        if (FileSystem::FileExists(path))
        {
            output->Add(New<VisualStudioCodeEditor>(path, false));
            return;
        }
    }
    {
        const String path(TEXT("/usr/bin/code"));
        if (FileSystem::FileExists(path))
        {
            output->Add(New<VisualStudioCodeEditor>(path, false));
            return;
        }
    }

    // Detect Flatpak installations
    {
        CreateProcessSettings procSettings;
        procSettings.FileName = TEXT("/bin/bash -c \"flatpak list --app --columns=application | grep com.visualstudio.code -c\"");
        procSettings.HiddenWindow = true;
        if (Platform::CreateProcess(procSettings) == 0)
        {
            const String runPath(TEXT("flatpak run com.visualstudio.code"));
            output->Add(New<VisualStudioCodeEditor>(runPath, false));
            return;
        }
    }
#elif PLATFORM_MAC
    // System installed app
	NSURL* AppURL = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:@"com.microsoft.VSCode"];
	if (AppURL != nullptr)
	{
        const String path = AppleUtils::ToString((CFStringRef)[AppURL path]);
        output->Add(New<VisualStudioCodeEditor>(path, false));
        return;
	}

    // Predefined locations
    String userFolder;
    FileSystem::GetSpecialFolderPath(SpecialFolder::Documents, userFolder);
    String paths[3] =
    {
        TEXT("/Applications/Visual Studio Code.app"),
        userFolder + TEXT("/../Visual Studio Code.app"),
        userFolder + TEXT("/../Downloads/Visual Studio Code.app"),
    };
    for (const String& path : paths)
    {
        if (FileSystem::DirectoryExists(path))
        {
            output->Add(New<VisualStudioCodeEditor>(path, false));
            break;
        }
    }
#endif
}

CodeEditorTypes VisualStudioCodeEditor::GetType() const
{
    return _isInsiders ? CodeEditorTypes::VSCodeInsiders : CodeEditorTypes::VSCode;
}

String VisualStudioCodeEditor::GetName() const
{
    return _isInsiders ? TEXT("Visual Studio Code - Insiders") : TEXT("Visual Studio Code");
}

void VisualStudioCodeEditor::OpenFile(const String& path, int32 line)
{
    // Generate VS solution files for intellisense
    if (!FileSystem::FileExists(Globals::ProjectFolder / Editor::Project->Name + TEXT(".sln")))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vs2022"));
    }

    // Generate project files if missing
    if (!FileSystem::FileExists(Globals::ProjectFolder / TEXT(".vscode/tasks.json")) ||
        !FileSystem::FileExists(_workspacePath))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vscode"));
    }

    // Open file
    line = line > 0 ? line : 1;
    CreateProcessSettings procSettings;
    procSettings.FileName = _execPath;
    procSettings.Arguments = String::Format(TEXT("\"{0}\" -g \"{1}:{2}\""), _workspacePath, path, line);
    procSettings.HiddenWindow = false;
    procSettings.WaitForEnd = false;
    procSettings.LogOutput = false;
    procSettings.ShellExecute = true;
    Platform::CreateProcess(procSettings);
}

void VisualStudioCodeEditor::OpenSolution()
{
    // Generate VS solution files for intellisense
    if (!FileSystem::FileExists(Globals::ProjectFolder / Editor::Project->Name + TEXT(".sln")))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vs2022"));
    }

    // Generate project files if solution is missing
    if (!FileSystem::FileExists(Globals::ProjectFolder / TEXT(".vscode/tasks.json")) ||
        !FileSystem::FileExists(_workspacePath))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vscode"));
    }

    // Open solution
    CreateProcessSettings procSettings;
    procSettings.FileName = _execPath;
    procSettings.Arguments = String::Format(TEXT("\"{0}\""), _workspacePath);
    procSettings.HiddenWindow = false;
    procSettings.WaitForEnd = false;
    procSettings.LogOutput = false;
    procSettings.ShellExecute = true;
    Platform::CreateProcess(procSettings);
}

bool VisualStudioCodeEditor::UseAsyncForOpen() const
{
    return false;
}
