// Copyright (c) Wojciech Figat. All rights reserved.

#include "ZedEditor.h"
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

bool TryGetPathForCommand(const String& cmd, String& path)
{
    const StringAnsi runCommand(StringAnsi::Format("/bin/bash -c \"type -p {0}\"", cmd.ToStringAnsi()));
    char buffer[128];
    FILE* pipe = popen(runCommand.Get(), "r");
    if (pipe)
    {
        StringAnsi pathAnsi;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            pathAnsi += buffer;
        pclose(pipe);
        path = String(pathAnsi.Get(), pathAnsi.Length() != 0 ? pathAnsi.Length() - 1 : 0);
        if (FileSystem::FileExists(path))
        {
            return true;
        }
    }
    path = String::Empty;
    return false;
}

ZedEditor::ZedEditor(const String& execPath)
    : _execPath(execPath)
    , _workspacePath(Globals::ProjectFolder)
{
}

void ZedEditor::FindEditors(Array<CodeEditor*>* output)
{
#if PLATFORM_WINDOWS
    String cmd;
    if (Platform::ReadRegValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Classes\\Applications\\Zed.exe\\shell\\open\\command"), TEXT(""), &cmd) || cmd.IsEmpty())
    {
        if (Platform::ReadRegValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Applications\\Zed.exe\\shell\\open\\command"), TEXT(""), &cmd) || cmd.IsEmpty())
        {
            // No hits in registry, try default path instead
        }
    }
    String path;
    if (cmd.IsEmpty())
    {
        path = cmd.Substring(1, cmd.Length() - String(TEXT("\" \"%1\"")).Length() - 1);
    }
    else
    {
        String localAppDataPath;
        FileSystem::GetSpecialFolderPath(SpecialFolder::LocalAppData, localAppDataPath);
        path = localAppDataPath / TEXT("Programs/Zed/Zed.exe");
    }
    if (FileSystem::FileExists(path))
    {
        output->Add(New<ZedEditor>(path));
    }
#elif PLATFORM_LINUX
    // Detect official release
    String zedPath;
    if (TryGetPathForCommand(TEXT("zed"), zedPath))
    {
        output->Add(New<ZedEditor>(zedPath));
        return;
    }
    {
        const String home = LinuxPlatform::GetHomeDirectory();
        const String path(home / TEXT(".local/bin/zed"));
        if (FileSystem::FileExists(path))
        {
            output->Add(New<ZedEditor>(path));
            return;
        }
    }

    // Detect unofficial releases
    String zeditorPath;
    if (TryGetPathForCommand(TEXT("zeditor"), zeditorPath))
    {
        output->Add(New<ZedEditor>(zeditorPath));
        return;
    }
    String zeditPath;
    if (TryGetPathForCommand(TEXT("zedit"), zeditPath))
    {
        output->Add(New<ZedEditor>(zeditPath));
        return;
    }

    // Detect Flatpak installations
    {
        CreateProcessSettings procSettings;
        procSettings.FileName = TEXT("/bin/bash -c \"flatpak list --app --columns=application | grep dev.zed.Zed -c\"");
        procSettings.HiddenWindow = true;
        if (Platform::CreateProcess(procSettings) == 0)
        {
            const String runPath(TEXT("flatpak run dev.zed.Zed"));
            output->Add(New<ZedEditor>(runPath));
            return;
        }
    }
#elif PLATFORM_MAC
    // Prefer the Zed CLI application over bundled app, as this handles opening files in existing instance better.
    // The bundle also contains the CLI application under Zed.app/Contents/MacOS/zed, but using this doesn't make any difference.
    const String cliPath(TEXT("/usr/local/bin/zed"));
    if (FileSystem::FileExists(cliPath))
    {
        output->Add(New<ZedEditor>(cliPath));
        return;
    }

    // System installed app
    NSURL* AppURL = [[NSWorkspace sharedWorkspace]URLForApplicationWithBundleIdentifier:@"dev.zed.Zed"];
    if (AppURL != nullptr)
    {
        const String path = AppleUtils::ToString((CFStringRef)[AppURL path]);
        output->Add(New<ZedEditor>(path));
        return;
    }

    // Predefined locations
    String userFolder;
    FileSystem::GetSpecialFolderPath(SpecialFolder::Documents, userFolder);
    String paths[3] =
    {
        TEXT("/Applications/Zed.app"),
        userFolder + TEXT("/../Zed.app"),
        userFolder + TEXT("/../Downloads/Zed.app"),
    };
    for (const String& path : paths)
    {
        if (FileSystem::DirectoryExists(path))
        {
            output->Add(New<ZedEditor>(path));
            break;
        }
    }
#endif
}

CodeEditorTypes ZedEditor::GetType() const
{
    return CodeEditorTypes::Zed;
}

String ZedEditor::GetName() const
{
    return TEXT("Zed");
}

void ZedEditor::OpenFile(const String& path, int32 line)
{
    // Generate VS solution files for intellisense
    if (!FileSystem::FileExists(Globals::ProjectFolder / Editor::Project->Name + TEXT(".sln")))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vs2022"));
    }

    // Open file
    line = line > 0 ? line : 1;
    CreateProcessSettings procSettings;
    procSettings.FileName = _execPath;
    procSettings.Arguments = String::Format(TEXT("\"{0}\" \"{1}:{2}\""), _workspacePath, path, line);
    procSettings.HiddenWindow = false;
    procSettings.WaitForEnd = false;
    procSettings.LogOutput = false;
    procSettings.ShellExecute = true;
    Platform::CreateProcess(procSettings);
}

void ZedEditor::OpenSolution()
{
    // Generate VS solution files for intellisense
    if (!FileSystem::FileExists(Globals::ProjectFolder / Editor::Project->Name + TEXT(".sln")))
    {
        ScriptsBuilder::GenerateProject(TEXT("-vs2022"));
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

bool ZedEditor::UseAsyncForOpen() const
{
    return false;
}
