// Copyright (c) Wojciech Figat. All rights reserved.

#include "ProjectInfo.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Serialization/JsonTools.h"

Array<ProjectInfo*> ProjectInfo::ProjectsCache;

void ShowProjectLoadError(const Char* errorMsg, const String& projectRootFolder)
{
    Platform::Error(String::Format(TEXT("Failed to load project. {0}\nPath: '{1}'"), errorMsg, projectRootFolder));
}

bool ProjectInfo::SaveProject()
{
    // Serialize object to Json
    rapidjson_flax::StringBuffer buffer;
    PrettyJsonWriter writerObj(buffer);
    auto& stream = *(JsonWriter*)&writerObj;
    stream.StartObject();
    {
        stream.JKEY("Name");
        stream.String(Name);

        stream.JKEY("Version");
        stream.String(Version.ToString());

        stream.JKEY("Company");
        stream.String(Company);

        stream.JKEY("Copyright");
        stream.String(Copyright);

        stream.JKEY("GameTarget");
        stream.String(GameTarget);

        stream.JKEY("EditorTarget");
        stream.String(EditorTarget);

        stream.JKEY("References");
        stream.StartArray();
        for (auto& reference : References)
        {
            stream.StartObject();
            stream.JKEY("Name");
            stream.String(reference.Name);
            stream.EndObject();
        }
        stream.EndArray();

        if (DefaultScene.IsValid())
        {
            stream.JKEY("DefaultScene");
            stream.Guid(DefaultScene);
        }

        if (DefaultSceneSpawn != Ray(Vector3::Zero, Vector3::Forward))
        {
            stream.JKEY("DefaultSceneSpawn");
            stream.Ray(DefaultSceneSpawn);
        }

        stream.JKEY("MinEngineVersion");
        stream.String(MinEngineVersion.ToString());

        if (EngineNickname.HasChars())
        {
            stream.JKEY("EngineNickname");
            stream.String(EngineNickname);
        }
    }
    stream.EndObject();

    // Write to file
    return File::WriteAllBytes(ProjectPath, (const byte*)buffer.GetString(), (int32)buffer.GetSize());
}

bool ProjectInfo::LoadProject(const String& projectPath)
{
    // Load Json file
    StringAnsi fileData;
    if (File::ReadAllText(projectPath, fileData))
    {
        ShowProjectLoadError(TEXT("Failed to read file contents."), projectPath);
        return true;
    }

    // Parse Json data
    rapidjson_flax::Document document;
    document.Parse(fileData.Get(), fileData.Length());
    if (document.HasParseError())
    {
        ShowProjectLoadError(TEXT("Failed to parse project contents. Ensure to have valid Json format."), projectPath);
        return true;
    }

    // Parse properties
    Name = JsonTools::GetString(document, "Name", String::Empty);
    ProjectPath = projectPath;
    ProjectFolderPath = StringUtils::GetDirectoryName(projectPath);
    const auto versionMember = document.FindMember("Version");
    if (versionMember != document.MemberEnd())
    {
        auto& version = versionMember->value;
        if (version.IsString())
        {
            Version::Parse(version.GetText(), &Version);
        }
        else if (version.IsObject())
        {
            Version = ::Version(
                JsonTools::GetInt(version, "Major", 0),
                JsonTools::GetInt(version, "Minor", 0),
                JsonTools::GetInt(version, "Build", -1),
                JsonTools::GetInt(version, "Revision", -1));
        }
    }
    if (Version.Revision() == 0)
        Version = ::Version(Version.Major(), Version.Minor(), Version.Build());
    if (Version.Build() == 0 && Version.Revision() == -1)
        Version = ::Version(Version.Major(), Version.Minor());
    Company = JsonTools::GetString(document, "Company", String::Empty);
    Copyright = JsonTools::GetString(document, "Copyright", String::Empty);
    GameTarget = JsonTools::GetString(document, "GameTarget", String::Empty);
    EditorTarget = JsonTools::GetString(document, "EditorTarget", String::Empty);
    EngineNickname = JsonTools::GetString(document, "EngineNickname", String::Empty);
    const auto referencesMember = document.FindMember("References");
    if (referencesMember != document.MemberEnd())
    {
        const auto& references = referencesMember->value.GetArray();
        References.Resize(references.Size());
        for (int32 i = 0; i < References.Count(); i++)
        {
            auto& reference = References[i];
            auto& value = references[i];
            reference.Name = JsonTools::GetString(value, "Name", String::Empty);

            String referencePath;
            if (reference.Name.StartsWith(TEXT("$(EnginePath)")))
            {
                // Relative to engine root
                referencePath = Globals::StartupFolder / reference.Name.Substring(14);
            }
            else if (reference.Name.StartsWith(TEXT("$(ProjectPath)")))
            {
                // Relative to project root
                referencePath = ProjectFolderPath / reference.Name.Substring(15);
            }
            else if (FileSystem::IsRelative(reference.Name))
            {
                // Relative to workspace
                referencePath = Globals::StartupFolder / reference.Name;
            }
            else
            {
                // Absolute
                referencePath = reference.Name;
            }
            StringUtils::PathRemoveRelativeParts(referencePath);

            // Load referenced project
            reference.Project = Load(referencePath);
            if (reference.Project == nullptr)
            {
                LOG(Error, "Failed to load referenced project ({0}, from {1})", reference.Name, referencePath);
                return true;
            }
        }
    }
    DefaultScene = JsonTools::GetGuid(document, "DefaultScene");
    DefaultSceneSpawn = JsonTools::GetRay(document, "DefaultSceneSpawn", Ray(Vector3::Zero, Vector3::Forward));
    const auto minEngineVersionMember = document.FindMember("MinEngineVersion");
    if (minEngineVersionMember != document.MemberEnd())
    {
        auto& minEngineVersion = minEngineVersionMember->value;
        if (minEngineVersion.IsString())
        {
            Version::Parse(minEngineVersion.GetText(), &MinEngineVersion);
        }
        else if (minEngineVersionMember->value.IsObject())
        {
            MinEngineVersion = ::Version(
                JsonTools::GetInt(minEngineVersion, "Major", 0),
                JsonTools::GetInt(minEngineVersion, "Minor", 0),
                JsonTools::GetInt(minEngineVersion, "Build", 0));
        }
    }

    // Validate properties
    if (Name.Length() == 0)
    {
        ShowProjectLoadError(TEXT("Missing project name."), projectPath);
        return true;
    }

    return false;
}

ProjectInfo* ProjectInfo::Load(const String& path)
{
    // Try to reuse loaded file
    for (int32 i = 0; i < ProjectsCache.Count(); i++)
    {
        if (ProjectsCache[i]->ProjectPath == path)
            return ProjectsCache[i];
    }

    // Load
    PROFILE_MEM(Editor);
    auto project = New<ProjectInfo>();
    if (project->LoadProject(path))
    {
        Delete(project);
        return nullptr;
    }

    // Cache project
    ProjectsCache.Add(project);
    return project;
}
