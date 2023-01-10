// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "SystemDefaultCodeEditor.h"
#include "Engine/Core/Types/StringView.h"

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
    Platform::StartProcess(path, StringView::Empty, StringView::Empty);
}

void SystemDefaultCodeEditor::OpenSolution()
{
}
