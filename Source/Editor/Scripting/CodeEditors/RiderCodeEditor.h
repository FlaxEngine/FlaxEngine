// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Scripting/CodeEditor.h"

/// <summary>
/// Implementation of code editor utility that is using Rider from JetBrains.
/// </summary>
class RiderCodeEditor : public CodeEditor
{
private:

    String _execPath;
    String _solutionPath;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="VisualStudioEditor"/> class.
    /// </summary>
    /// <param name="execPath">Executable file path</param>
    RiderCodeEditor(const String& execPath);

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
    void OnFileAdded(const String& path) override;
};
