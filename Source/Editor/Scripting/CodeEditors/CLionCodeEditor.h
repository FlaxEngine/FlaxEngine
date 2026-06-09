// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Editor/Scripting/CodeEditor.h"

/// <summary>
/// Implementation of code editor utility that is using CLion from JetBrains.
/// </summary>
class CLionCodeEditor : public CodeEditor
{
private:

    String _execPath;
    String _execArgsPrefix;
    String _projectPath;

    String GetProcessArguments(const String& arguments) const;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="CLionCodeEditor"/> class.
    /// </summary>
    /// <param name="execPath">Executable file path</param>
    /// <param name="execArgsPrefix">Additional arguments to pass before CLion arguments.</param>
    CLionCodeEditor(const String& execPath, const String& execArgsPrefix = String::Empty);

public:

    /// <summary>
    /// Tries to find installed CLion instances. Adds them to the result list.
    /// </summary>
    /// <param name="output">The output editors.</param>
    static void FindEditors(Array<CodeEditor*>* output);

public:

    // [CodeEditor]
    CodeEditorTypes GetType() const override;
    String GetName() const override;
    String GetGenerateProjectCustomArgs() const override;
    void OpenFile(const String& path, int32 line) override;
    void OpenSolution() override;
    void OnFileAdded(const String& path) override;
};
