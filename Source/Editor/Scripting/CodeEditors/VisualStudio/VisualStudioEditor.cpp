// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_VISUAL_STUDIO_DTE

#include "VisualStudioEditor.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Log.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include "VisualStudioConnection.h"

VisualStudioEditor::VisualStudioEditor(VisualStudioVersion version, const String& execPath, const String& CLSID)
    : _version(version)
    , _execPath(execPath)
    , _CLSID(CLSID)
{
    switch (version)
    {
    case VisualStudioVersion::VS2008:
        _type = CodeEditorTypes::VS2008;
        break;
    case VisualStudioVersion::VS2010:
        _type = CodeEditorTypes::VS2010;
        break;
    case VisualStudioVersion::VS2012:
        _type = CodeEditorTypes::VS2012;
        break;
    case VisualStudioVersion::VS2013:
        _type = CodeEditorTypes::VS2013;
        break;
    case VisualStudioVersion::VS2015:
        _type = CodeEditorTypes::VS2015;
        break;
    case VisualStudioVersion::VS2017:
        _type = CodeEditorTypes::VS2017;
        break;
    case VisualStudioVersion::VS2019:
        _type = CodeEditorTypes::VS2019;
        break;
    case VisualStudioVersion::VS2022:
        _type = CodeEditorTypes::VS2022;
        break;
    case VisualStudioVersion::VS2026:
        _type = CodeEditorTypes::VS2026;
        break;
    default: CRASH;
        break;
    }
    // Initialize with the preferred extension; GetSolutionPath() will check both at runtime
    const Char* ext = (_version >= VisualStudioVersion::VS2022) ? TEXT(".slnx") : TEXT(".sln");
    _solutionPath = Globals::ProjectFolder / Editor::Project->Name + ext;
    _solutionPath.Replace('/', '\\'); // Use Windows-style path separators
}

String VisualStudioEditor::GetSolutionPath() const
{
    // VS2022+ prefers .slnx; older versions prefer .sln. Fall back to the other if preferred is missing.
    const String base = Globals::ProjectFolder / Editor::Project->Name;
    const bool preferSlnx = (_version >= VisualStudioVersion::VS2022);
    String preferred = base + (preferSlnx ? TEXT(".slnx") : TEXT(".sln"));
    preferred.Replace('/', '\\');
    if (FileSystem::FileExists(preferred))
        return preferred;
    String fallback = base + (preferSlnx ? TEXT(".sln") : TEXT(".slnx"));
    fallback.Replace('/', '\\');
    if (FileSystem::FileExists(fallback))
        return fallback;
    return preferred; // Neither exists yet; return preferred so generation targets the right format
}

void VisualStudioEditor::FindEditors(Array<CodeEditor*>* output)
{
    String installDir;
    String clsID;
    String regVsRootNode;

    // Select registry node for
    if (Platform::Is64BitPlatform())
        regVsRootNode = TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\");
    else
        regVsRootNode = TEXT("SOFTWARE\\Microsoft\\VisualStudio\\");

    VisualStudio::InstanceInfo infos[8];
    const int32 count = VisualStudio::GetVisualStudioVersions(infos, ARRAY_COUNT(infos));
    for (int32 i = 0; i < count; i++)
    {
        auto& info = infos[i];
        VisualStudioVersion version;
        if (info.VersionMajor >= 18)
        {
            version = VisualStudioVersion::VS2026;
        }
        else
        {
            switch (info.VersionMajor)
            {
            case 17:
                version = VisualStudioVersion::VS2022;
                break;
            case 16:
                version = VisualStudioVersion::VS2019;
                break;
            case 15:
                version = VisualStudioVersion::VS2017;
                break;
            default:
                continue;
            }
        }

        String executablePath(info.ExecutablePath);
        if (!FileSystem::FileExists(executablePath))
            continue;

        // Create editor
        clsID = info.CLSID;
        auto editor = New<VisualStudioEditor>(version, executablePath, clsID);
        output->Add(editor);
    }

    struct VersionData
    {
        VisualStudioVersion Version;
        const Char* RegistryKey;
    };

    const VersionData versions[] =
    {
        // Order matters
        { VisualStudioVersion::VS2015, TEXT("14.0"), },
        { VisualStudioVersion::VS2013, TEXT("12.0"), },
        { VisualStudioVersion::VS2012, TEXT("11.0"), },
        { VisualStudioVersion::VS2012, TEXT("11.0"), },
        { VisualStudioVersion::VS2008, TEXT("9.0") },
    };

    for (int32 i = 0; i < ARRAY_COUNT(versions); i++)
    {
        auto registryKey = regVsRootNode + versions[i].RegistryKey;

        // Read install directory
        if (Platform::ReadRegValue(HKEY_LOCAL_MACHINE, registryKey, TEXT("InstallDir"), &installDir) || installDir.IsEmpty())
            continue;

        // Ensure that file exists
        String execPath = installDir + TEXT("devenv.exe");
        if (!FileSystem::FileExists(execPath))
            continue;

        // Read version info id
        clsID.Clear();
        Platform::ReadRegValue(HKEY_LOCAL_MACHINE, registryKey, TEXT("ThisVersionDTECLSID"), &clsID);

        // Create editor
        auto editor = New<VisualStudioEditor>(versions[i].Version, execPath, clsID);
        output->Add(editor);
    }
}

CodeEditorTypes VisualStudioEditor::GetType() const
{
    return _type;
}

String VisualStudioEditor::GetName() const
{
    const Char* name;
    switch (_version)
    {
    case VisualStudioVersion::VS2008:
        name = TEXT("Visual Studio 2008");
        break;
    case VisualStudioVersion::VS2010:
        name = TEXT("Visual Studio 2010");
        break;
    case VisualStudioVersion::VS2012:
        name = TEXT("Visual Studio 2012");
        break;
    case VisualStudioVersion::VS2013:
        name = TEXT("Visual Studio 2013");
        break;
    case VisualStudioVersion::VS2015:
        name = TEXT("Visual Studio 2015");
        break;
    case VisualStudioVersion::VS2017:
        name = TEXT("Visual Studio 2017");
        break;
    case VisualStudioVersion::VS2019:
        name = TEXT("Visual Studio 2019");
        break;
    case VisualStudioVersion::VS2022:
        name = TEXT("Visual Studio 2022");
        break;
    case VisualStudioVersion::VS2026:
        name = TEXT("Visual Studio 2026");
        break;
    default:
        name = ToString(_version);
        break;
    }
    return String(name);
}

String VisualStudioEditor::GetGenerateProjectCustomArgs() const
{
    if (_version >= VisualStudioVersion::VS2022)
        return TEXT("-vs2026");
    return String::Format(TEXT("-{0}"), String(ToString(_version)).ToLower());
}

void VisualStudioEditor::OpenFile(const String& path, int32 line)
{
    // Generate project files if solution is missing
    String solutionPath = GetSolutionPath();
    if (!FileSystem::FileExists(solutionPath))
    {
        ScriptsBuilder::GenerateProject(GetGenerateProjectCustomArgs());
        solutionPath = GetSolutionPath();
    }

    // Open file
    const VisualStudio::Connection connection(*_CLSID, *solutionPath);
    String tmp = path;
    tmp.Replace('/', '\\'); // Use Windows-style path separators
    const auto result = connection.OpenFile(*tmp, line);
    if (result.Failed())
    {
        LOG(Warning, "Cannot open file \'{0}\':{1}. {2}.", path, line, String(result.Message.c_str()));
    }
}

void VisualStudioEditor::OpenSolution()
{
    // Generate project files if solution is missing
    String solutionPath = GetSolutionPath();
    if (!FileSystem::FileExists(solutionPath))
    {
        ScriptsBuilder::GenerateProject(GetGenerateProjectCustomArgs());
        solutionPath = GetSolutionPath();
    }

    // Open solution
    const VisualStudio::Connection connection(*_CLSID, *solutionPath);
    const auto result = connection.OpenSolution();
    if (result.Failed())
    {
        LOG(Warning, "Cannot open solution. {0}", String(result.Message.c_str()));
    }
}

void VisualStudioEditor::OnFileAdded(const String& path)
{
    // TODO: finish dynamic files adding to the project - for now just regenerate it
    ScriptsBuilder::GenerateProject(GetGenerateProjectCustomArgs());
}

bool VisualStudioEditor::UseAsyncForOpen() const
{
    // Need to generate project files if missing first
    String solutionPath = GetSolutionPath();
    if (!FileSystem::FileExists(solutionPath))
        return true;

    // Open in async only when no solution opened
    const VisualStudio::Connection connection(*_CLSID, *solutionPath);
    return !connection.IsActive();
}

#endif
