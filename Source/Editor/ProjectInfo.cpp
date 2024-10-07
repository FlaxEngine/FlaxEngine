// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ProjectInfo.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Serialization/JsonTools.h"
#include <ThirdParty/pugixml/pugixml.hpp>
using namespace pugi;

Array<ProjectInfo*> ProjectInfo::ProjectsCache;

struct XmlCharAsChar
{
#if PLATFORM_TEXT_IS_CHAR16
    Char* Str = nullptr;
    
    XmlCharAsChar(const pugi::char_t* str)
    {
        if (!str)
            return;
        int32 length = 0;
        while (str[length])
            length++;
        Str = (Char*)Platform::Allocate(length * sizeof(Char), sizeof(Char));
        for (int32 i = 0; i <= length; i++)
            Str[i] = (Char)str[i];
    }

    ~XmlCharAsChar()
    {
        Platform::Free(Str);
    }
#else
    const Char* Str;

    XmlCharAsChar(const pugi::char_t* str)
        : Str(str)
    {
    }
#endif
};

void ShowProjectLoadError(const Char* errorMsg, const String& projectRootFolder)
{
    Platform::Error(String::Format(TEXT("Failed to load project. {0}\nPath: '{1}'"), errorMsg, projectRootFolder));
}

Vector3 GetVector3FromXml(const xml_node& parent, const PUGIXML_CHAR* name, const Vector3& defaultValue)
{
    const auto node = parent.child(name);
    if (!node.empty())
    {
        const auto x = node.child_value(PUGIXML_TEXT("X"));
        const auto y = node.child_value(PUGIXML_TEXT("Y"));
        const auto z = node.child_value(PUGIXML_TEXT("Z"));
        if (x && y && z)
        {
            XmlCharAsChar xs(x), ys(y), zs(z);
            Float3 v;
            if (!StringUtils::Parse(xs.Str, &v.X) && !StringUtils::Parse(ys.Str, &v.Y) && !StringUtils::Parse(zs.Str, &v.Z))
            {
                return (Vector3)v;
            }
        }
    }
    return defaultValue;
}

int32 GetIntFromXml(const xml_node& parent, const PUGIXML_CHAR* name, const int32 defaultValue)
{
    const auto node = parent.child_value(name);
    if (node)
    {
        XmlCharAsChar s(node);
        int32 v;
        if (!StringUtils::Parse(s.Str, &v))
        {
            return v;
        }
    }

    return defaultValue;
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

bool ProjectInfo::LoadOldProject(const String& projectPath)
{
    // Open Xml file
    xml_document doc;
    const xml_parse_result result = doc.load_file((const PUGIXML_CHAR*)*projectPath);
    if (result.status)
    {
        ShowProjectLoadError(TEXT("Xml file parsing error."), projectPath);
        return true;
    }

    // Get root node
    const xml_node root = doc.child(PUGIXML_TEXT("Project"));
    if (!root)
    {
        ShowProjectLoadError(TEXT("Missing Project root node in xml file."), projectPath);
        return true;
    }

    // Load data
    Name = (const Char*)root.child_value(PUGIXML_TEXT("Name"));
    ProjectPath = projectPath;
    ProjectFolderPath = StringUtils::GetDirectoryName(projectPath);
    DefaultScene = Guid::Empty;
    const auto defaultScene = root.child_value(PUGIXML_TEXT("DefaultSceneId"));
    if (defaultScene)
    {
        Guid::Parse((const Char*)defaultScene, DefaultScene);
    }
    DefaultSceneSpawn.Position = GetVector3FromXml(root, PUGIXML_TEXT("DefaultSceneSpawnPos"), Vector3::Zero);
    DefaultSceneSpawn.Direction = Quaternion::Euler(GetVector3FromXml(root, PUGIXML_TEXT("DefaultSceneSpawnDir"), Vector3::Zero)) * Vector3::Forward;
    MinEngineVersion = ::Version(0, 0, GetIntFromXml(root, PUGIXML_TEXT("MinVersionSupported"), 0));

    // Always reference engine project
    auto& flaxReference = References.AddOne();
    flaxReference.Name = TEXT("$(EnginePath)/Flax.flaxproj");
    flaxReference.Project = Load(Globals::StartupFolder / TEXT("Flax.flaxproj"));
    if (!flaxReference.Project)
    {
        ShowProjectLoadError(TEXT("Failed to load Flax Engine project."), projectPath);
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
