// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Scripting/CodeEditor.h"

/// <summary>
/// Implementation of code editor utility that is using Microsoft Visual Studio Code.
/// </summary>
class VisualStudioCodeEditor : public CodeEditor
{
private:

    String _execPath;
    String _workspacePath;
    bool _isInsiders;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="VisualStudioEditor"/> class.
    /// </summary>
    /// <param name="execPath">Executable file path</param>
    /// <param name="isInsiders">Is insiders edition</param>
    VisualStudioCodeEditor(const String& execPath, const bool isInsiders);

public:

    /// <summary>
    /// Tries to find installed Visual Studio Code instance. Adds them to the result list.
    /// </summary>
    /// <param name="output">The output editors.</param>
    static void FindEditors(Array<CodeEditor*>* output);

public:

    // [CodeEditor]
    CodeEditorTypes GetType() const override;
    String GetName() const override;
    void OpenFile(const String& path, int32 line) override;
    void OpenSolution() override;
    bool UseAsyncForOpen() const override;
};
