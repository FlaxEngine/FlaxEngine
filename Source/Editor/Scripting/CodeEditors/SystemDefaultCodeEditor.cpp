// Copyright (c) Wojciech Figat. All rights reserved.

#include "SystemDefaultCodeEditor.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Engine/Globals.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Scripting/ScriptsBuilder.h"

CodeEditorTypes SystemDefaultCodeEditor::GetType() const
{
    return CodeEditorTypes::SystemDefault;
}

String SystemDefaultCodeEditor::GetName() const
{
    return TEXT("System Default");
}

void SystemDefaultCodeEditor::OpenFile(const String& path, int32 line)
{
    CreateProcessSettings procSettings;
    procSettings.FileName = path;
    procSettings.HiddenWindow = false;
    procSettings.WaitForEnd = false;
    procSettings.LogOutput = false;
    procSettings.ShellExecute = true;
    Platform::CreateProcess(procSettings);
}

void SystemDefaultCodeEditor::OpenSolution()
{
    String slnxPath = Globals::ProjectFolder / Editor::Project->Name + TEXT(".slnx");
    String slnPath = Globals::ProjectFolder / Editor::Project->Name + TEXT(".sln");
    String solutionPath;
    if (FileSystem::FileExists(slnxPath))
        solutionPath = slnxPath;
    else if (FileSystem::FileExists(slnPath))
        solutionPath = slnPath;
    else
    {
        ScriptsBuilder::GenerateProject();
        if (FileSystem::FileExists(slnxPath))
            solutionPath = slnxPath;
        else if (FileSystem::FileExists(slnPath))
            solutionPath = slnPath;
    }

    if (FileSystem::FileExists(solutionPath))
    {
        OpenFile(solutionPath, 0);
    }
}
