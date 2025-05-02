// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_VISUAL_STUDIO_DTE

#include "Engine/Core/Enums.h"
#include "Editor/Scripting/CodeEditor.h"

/// <summary>
/// Microsoft Visual Studio version types
/// </summary>
DECLARE_ENUM_8(VisualStudioVersion, VS2008, VS2010, VS2012, VS2013, VS2015, VS2017, VS2019, VS2022);

/// <summary>
/// Implementation of code editor utility that is using Microsoft Visual Studio.
/// </summary>
class VisualStudioEditor : public CodeEditor
{
private:

    VisualStudioVersion _version;
    CodeEditorTypes _type;
    String _execPath;
    String _CLSID;
    String _solutionPath;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="VisualStudioEditor"/> class.
    /// </summary>
    /// <param name="version">VS version</param>
    /// <param name="execPath">Executable file path</param>
    /// <param name="CLSID">CLSID of VS</param>
    VisualStudioEditor(VisualStudioVersion version, const String& execPath, const String& CLSID);

public:

    /// <summary>
    /// Gets version of Visual Studio.
    /// </summary>
    FORCE_INLINE VisualStudioVersion GetVersion() const
    {
        return _version;
    }

    /// <summary>
    /// Tries to find installed Visual Studio instances and dds them to the result list.
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
    bool UseAsyncForOpen() const override;
};

#endif
