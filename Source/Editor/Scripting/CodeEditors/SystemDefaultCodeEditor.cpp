// Copyright (c) Wojciech Figat. All rights reserved.

#include "SystemDefaultCodeEditor.h"
#include "Engine/Platform/CreateProcessSettings.h"

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
}
