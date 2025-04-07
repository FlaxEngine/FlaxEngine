// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Scripting/CodeEditor.h"

/// <summary>
/// Basic implementation for editing source code on a target platform.
/// </summary>
class SystemDefaultCodeEditor : public CodeEditor
{
public:

    // [CodeEditor]
    CodeEditorTypes GetType() const override;
    String GetName() const override;
    void OpenFile(const String& path, int32 line) override;
    void OpenSolution() override;
};
