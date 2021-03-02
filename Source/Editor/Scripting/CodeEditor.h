// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Types of in-build code editors.
/// </summary>
API_ENUM(Namespace="FlaxEditor", Attributes="HideInEditor") enum class CodeEditorTypes
{
    /// <summary>
    /// Custom/external editor
    /// </summary>
    Custom,

    /// <summary>
    /// Default program associated by the file extension on the system
    /// </summary>
    SystemDefault,

    /// <summary>
    /// Visual Studio 2008
    /// </summary>
    VS2008,

    /// <summary>
    /// Visual Studio 2010
    /// </summary>
    VS2010,

    /// <summary>
    /// Visual Studio 2012
    /// </summary>
    VS2012,

    /// <summary>
    /// Visual Studio 2013
    /// </summary>
    VS2013,

    /// <summary>
    /// Visual Studio 2015
    /// </summary>
    VS2015,

    /// <summary>
    /// Visual Studio 2017
    /// </summary>
    VS2017,

    /// <summary>
    /// Visual Studio 2019
    /// </summary>
    VS2019,

    /// <summary>
    /// Visual Studio Code
    /// </summary>
    VSCode,

    /// <summary>
    /// Visual Studio Code Insiders
    /// </summary>
    VSCodeInsiders,

    /// <summary>
    /// Rider
    /// </summary>
    Rider,

    MAX
};

/// <summary>
/// Base class for all code editors.
/// </summary>
class CodeEditor
{
public:

    /// <summary>
    /// Finalizes an instance of the <see cref="CodeEditor"/> class.
    /// </summary>
    virtual ~CodeEditor() = default;

    /// <summary>
    /// Gets the type of the editor (used by the in-build editors).
    /// </summary>
    /// <returns>The name</returns>
    virtual CodeEditorTypes GetType() const
    {
        return CodeEditorTypes::Custom;
    }

    /// <summary>
    /// Gets the name of the editor.
    /// </summary>
    /// <returns>The name</returns>
    virtual String GetName() const = 0;

    /// <summary>
    /// Opens the file.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="line">The target line (use 0 to not use it).</param>
    virtual void OpenFile(const String& path, int32 line) = 0;

    /// <summary>
    /// Opens the solution project.
    /// </summary>
    virtual void OpenSolution() = 0;

    /// <summary>
    /// Called when source file gets added to the workspace. Can be used to automatically include new files into the project files.
    /// </summary>
    /// <param name="path">The path.</param>
    virtual void OnFileAdded(const String& path)
    {
    }

    /// <summary>
    /// Determines whether use the asynchronous task for open solution/file.
    /// </summary>
    /// <returns>True if use async thread for open task.</returns>
    virtual bool UseAsyncForOpen() const
    {
        return false;
    }
};

/// <summary>
/// Editor utility to managed and use different code editors. Allows to open solution and source code files.
/// </summary>
API_CLASS(Static, Namespace="FlaxEditor") class CodeEditingManager
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(CodeEditingManager);
public:

    /// <summary>
    /// Gets all found editors.
    /// </summary>
    /// <returns>The editors. Read-only.</returns>
    static const Array<CodeEditor*>& GetEditors();

    /// <summary>
    /// Determines whether asynchronous open action is running in a background.
    /// </summary>
    /// <returns><c>true</c> if asynchronous open action is running in a background; otherwise, <c>false</c>.</returns>
    API_PROPERTY() static bool IsAsyncOpenRunning();

    /// <summary>
    /// Gets the in-build code editor or null if not found.
    /// </summary>
    /// <param name="editorType">Type of the editor.</param>
    /// <returns>The editor object or null if not found.</returns>
    static CodeEditor* GetCodeEditor(CodeEditorTypes editorType);

    /// <summary>
    /// Opens the file. Handles async opening.
    /// </summary>
    /// <param name="editorType">The code editor type.</param>
    /// <param name="path">The file path.</param>
    /// <param name="line">The target line (use 0 to not use it).</param>
    API_FUNCTION() static void OpenFile(CodeEditorTypes editorType, const String& path, int32 line);

    /// <summary>
    /// Opens the file. Handles async opening.
    /// </summary>
    /// <param name="editor">The code editor.</param>
    /// <param name="path">The file path.</param>
    /// <param name="line">The target line (use 0 to not use it).</param>
    static void OpenFile(CodeEditor* editor, const String& path, int32 line);

    /// <summary>
    /// Opens the solution project. Handles async opening.
    /// </summary>
    /// <param name="editorType">The code editor type.</param>
    API_FUNCTION() static void OpenSolution(CodeEditorTypes editorType);

    /// <summary>
    /// Called when source file gets added to the workspace. Can be used to automatically include new files into the project files.
    /// </summary>
    /// <param name="editorType">The code editor type.</param>
    /// <param name="path">The path.</param>
    API_FUNCTION() static void OnFileAdded(CodeEditorTypes editorType, const String& path);

    /// <summary>
    /// Opens the solution project. Handles async opening.
    /// </summary>
    /// <param name="editor">The code editor.</param>
    static void OpenSolution(CodeEditor* editor);

    /// <summary>
    /// The asynchronous open begins.
    /// </summary>
    static Action AsyncOpenBegin;

    /// <summary>
    /// The asynchronous open ends.
    /// </summary>
    static Action AsyncOpenEnd;
};
