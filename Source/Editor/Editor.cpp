// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "Editor.h"
#include "ProjectInfo.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Scripting/ScriptsBuilder.h"
#include "Windows/SplashScreen.h"
#include "Managed/ManagedEditor.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/ShadowsOfMordor/Builder.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Cache/AssetsCache.h"
#include "Engine/Serialization/JsonWriters.h"
#include "FlaxEngine.Gen.h"
#if PLATFORM_LINUX || PLATFORM_MAC
#include "Engine/Tools/TextureTool/TextureTool.h"
#endif
#if SPLASH_SCREEN_IMMEDIATE
#include "Engine/Graphics/GPUDevice.h"
#endif

namespace EditorImpl
{
    bool HasFocus = false;
    bool UpgradeOldProject = false;
    Version OldProjectMinVersion;
    SplashScreen* Splash = nullptr;

    void OnUpdate();
}

// Version with major.minor components
struct ProjectVersion
{
    int32 Major = 0, Minor = 0;

    ProjectVersion() = default;
    ProjectVersion(int32 major, int32 minor)
        : Major(major)
        , Minor(minor)
    {
    }

    bool operator==(const ProjectVersion& other) const
    {
        return Major == other.Major && Minor == other.Minor;
    }
    bool operator<(const ProjectVersion& other) const
    {
        return Major < other.Major || (Major == other.Major && Minor < other.Minor);
    }
    bool operator<=(const ProjectVersion& other) const
    {
        return Major <= other.Major || (Major == other.Major && Minor <= other.Minor);
    }
    bool operator>(const ProjectVersion& other) const
    {
        return other < *this;
    }
    bool operator>=(const ProjectVersion& other) const
    {
        return other <= *this;
    }
    bool operator!=(const ProjectVersion& other) const
    {
        return !(*this == other);
    }
};

class SplashScreenService : public EngineService
{
public:
    SplashScreenService()
        : EngineService(TEXT("Splash Screen"), -29) // Right after: Graphics, Render2D and Windows Manager
    {
    }

    bool Init() override
    {
        if (CommandLine::Options.Headless.IsTrue())
            return false;

        // Show splash screen
        PROFILE_CPU_NAMED("Splash");
        EditorImpl::Splash = New<SplashScreen>();
        EditorImpl::Splash->SetTitle(Editor::Project->Name);
        EditorImpl::Splash->Show();

#if SPLASH_SCREEN_IMMEDIATE
        // Run a dummy frame to show splash screen without waiting on engine to start
        GPUDevice::Instance->Draw();
#endif

        return false;
    }
};

SplashScreenService SplashScreenServiceInstance;
ManagedEditor* Editor::Managed = nullptr;
ProjectInfo* Editor::Project = nullptr;
bool Editor::IsPlayMode = false;
bool Editor::IsOldProjectOpened = true;
int32 Editor::LastProjectOpenedEngineBuild = 0;
Version Editor::LastOpenedVersion;

void Editor::CloseSplashScreen()
{
    SAFE_DELETE(EditorImpl::Splash);
}

bool Editor::CheckProjectUpgrade()
{
    PROFILE_CPU();
    PROFILE_MEM(Editor);
    const auto versionFilePath = Globals::ProjectCacheFolder / TEXT("version");

    // Load version cache file
    struct VersionCache
    {
        // When changing this ensure that Flax Launcher properly reads the version
        ProjectVersion Version = { FLAXENGINE_VERSION_MAJOR, FLAXENGINE_VERSION_MINOR };
        int32 Build = FLAXENGINE_VERSION_BUILD;
        int32 RealSize = sizeof(Real); // Rebuild when changing between Large Worlds
    };
    VersionCache versionCache;
    if (FileSystem::FileExists(versionFilePath))
    {
        auto file = File::Open(versionFilePath, FileMode::OpenExisting, FileAccess::Read, FileShare::Read);
        if (file)
        {
            bool failed = file->Read(&versionCache, sizeof(versionCache));

            // Invalidate results if data has issues
            if (failed || versionCache.Version.Major < 0 || versionCache.Version.Minor < 0 || versionCache.Version.Major > 100 || versionCache.Version.Minor > 1000)
            {
                versionCache = VersionCache();
                LOG(Warning, "Invalid version cache data");
            }
            else
            {
                LOG(Info, "Last project open version: {0}.{1}.{2}", versionCache.Version.Major, versionCache.Version.Minor, versionCache.Build);
                LastProjectOpenedEngineBuild = versionCache.Build;
                LastOpenedVersion = Version(versionCache.Version.Major, versionCache.Version.Minor, versionCache.Build);
            }

            Delete(file);
        }
    }

    // Check if need to backup and upgrade project
    ProjectVersion engineVersion(FLAXENGINE_VERSION_MAJOR, FLAXENGINE_VERSION_MINOR);
    ProjectVersion minEngineVersion = ProjectVersion(Project->MinEngineVersion.Major(), Project->MinEngineVersion.Minor());
    EditorImpl::OldProjectMinVersion = Project->MinEngineVersion;
    if ((versionCache.Version == engineVersion && LastProjectOpenedEngineBuild != 0) || minEngineVersion == engineVersion)
    {
        // Project was opened last time or saved with the current engine version
        IsOldProjectOpened = false;
    }
    else if (minEngineVersion < engineVersion)
    {
        // Project was created/saved with an older engine version so perform upgrade (silent)
        EditorImpl::UpgradeOldProject = true;
        LOG(Info, "The project was used with an older editor version ({}.{})", minEngineVersion.Major, minEngineVersion.Minor);

        // Re-save project with a current version to skip upgrading next time it's opened
        HashSet<ProjectInfo*> projects;
        Project->GetAllProjects(projects);
        for (auto& e : projects)
        {
            if (e.Item->Name == TEXT("Flax"))
                continue;
            const String& projectPath = e.Item->ProjectPath;
            StringAnsi fileData;
            if (!File::ReadAllText(projectPath, fileData))
            {
                rapidjson_flax::Document document;
                document.Parse(fileData.Get(), fileData.Length());
                if (!document.HasParseError())
                {
                    const auto minEngineVersionMember = document.FindMember("MinEngineVersion");
                    if (minEngineVersionMember != document.MemberEnd())
                        minEngineVersionMember->value.SetString(FLAXENGINE_VERSION_TEXT);
                    else
                        document.AddMember("MinEngineVersion", FLAXENGINE_VERSION_TEXT, document.GetAllocator());
                }
                rapidjson_flax::StringBuffer buffer;
                PrettyJsonWriter writer(buffer);
                document.Accept(writer.GetWriter());
                File::WriteAllBytes(projectPath, (byte*)buffer.GetString(), (int32)buffer.GetSize());
            }
        }
    }
    else if (versionCache.Version < engineVersion)
    {
        // Project was opened with an older older engine version so ask user if backup or cancel operation
        LOG(Info, "The project was last opened with an older editor version");
        EditorImpl::UpgradeOldProject = true;
        const auto result = MessageBox::Show(TEXT("The project was last opened with an older editor version.\nLoading it may modify existing data, which can result in older editor versions being unable to open it.\n\nDo you want to perform a backup before or cancel the operation?"), TEXT("Project upgrade"), MessageBoxButtons::YesNoCancel, MessageBoxIcon::Question);
        if (result == DialogResult::Yes)
        {
            if (BackupProject())
            {
                LOG(Warning, "Backup failed");
                return true;
            }
        }
        else if (result == DialogResult::No)
        {
            // Don't backup, just load
        }
        else
        {
            // Cancel loading
            return true;
        }
    }
    // Check if last version was newer
    else if (versionCache.Version > engineVersion)
    {
        // Project was opened with a newer version previously so ask user if backup or cancel operation
        LOG(Warning, "The project was last opened with a newer editor version");
        const auto result = MessageBox::Show(TEXT("The project was last opened with a newer editor version.\nLoading it may fail and corrupt existing data.\n\nDo you want to perform a backup before loading or cancel the operation?"), TEXT("Project upgrade"), MessageBoxButtons::YesNoCancel, MessageBoxIcon::Warning);
        if (result == DialogResult::Yes)
        {
            if (BackupProject())
            {
                LOG(Warning, "Backup failed");
                return true;
            }
        }
        else if (result == DialogResult::No)
        {
            // Don't backup, just load
        }
        else
        {
            // Cancel
            return true;
        }
    }

    // When changing between major/minor version clear some caches to prevent possible issues
    if (versionCache.Version != engineVersion || versionCache.RealSize != sizeof(Real))
    {
        PROFILE_CPU_NAMED("Clean");
        LOG(Info, "Cleaning cache files from different engine version");
        FileSystem::DeleteDirectory(Globals::ProjectFolder / TEXT("Cache/Cooker"));
        FileSystem::DeleteDirectory(Globals::ProjectFolder / TEXT("Cache/Thumbnails"));
        FileSystem::DeleteDirectory(Globals::ProjectFolder / TEXT("Cache/Shaders"));
        HashSet<ProjectInfo*> projects;
        Project->GetAllProjects(projects);
        for (auto& e : projects)
        {
            if (e.Item->Name == TEXT("Flax"))
                continue;
            FileSystem::DeleteDirectory(e.Item->ProjectFolderPath / TEXT("Cache/Intermediate"));
        }
    }

    // Update version the cache file
    versionCache = VersionCache();
    if (File::WriteAllBytes(versionFilePath, &versionCache, sizeof(versionCache)))
    {
        LOG(Error, "Failed to create version cache file");
    }

    return false;
}

bool Editor::BackupProject()
{
    PROFILE_CPU();

    // Create backup directory
    auto dstPath = Globals::ProjectFolder + TEXT(" - Backup");
    LOG(Info, "Backup project to \"{0}\"", dstPath);
    {
        int32 count = 0;
        while (count < 1000 && FileSystem::DirectoryExists(dstPath))
        {
            dstPath = Globals::ProjectFolder + TEXT(" - Backup") + StringUtils::ToString(count++);
        }
    }

    // Copy everything
    return FileSystem::CopyDirectory(dstPath, Globals::ProjectFolder);
}

int32 Editor::LoadProduct()
{
    PROFILE_MEM(Editor);

    // Flax Editor product
    Globals::ProductName = TEXT("Flax Editor");
    Globals::CompanyName = TEXT("Flax");

#if FLAX_TESTS
    // Flax Tests use auto-generated temporary project
    CommandLine::Options.Project = Globals::TemporaryFolder / TEXT("Project");
    CommandLine::Options.NewProject = true;
#endif

    // Gather project directory from the command line
    String projectPath = CommandLine::Options.Project.TrimTrailing();
    const int32 startIndex = projectPath.StartsWith('\"') || projectPath.StartsWith('\'') ? 1 : 0;
    const int32 length = projectPath.Length() - (projectPath.EndsWith('\"') || projectPath.EndsWith('\'') ? 1 : 0) - startIndex;
    if (length > 0)
    {
        projectPath = projectPath.Substring(startIndex, length - startIndex);
        StringUtils::PathRemoveRelativeParts(projectPath);
        if (FileSystem::IsRelative(projectPath))
        {
            projectPath = Platform::GetWorkingDirectory() / projectPath;
            StringUtils::PathRemoveRelativeParts(projectPath);
        }
        if (projectPath.EndsWith(TEXT(".flaxproj")))
        {
            projectPath = StringUtils::GetDirectoryName(projectPath);
        }
    }
    else
    {
        projectPath.Clear();
    }

    // Create new project option
    if (CommandLine::Options.NewProject.IsTrue())
    {
        Array<String> projectFiles;
        FileSystem::DirectoryGetFiles(projectFiles, projectPath, TEXT("*.flaxproj"), DirectorySearchOption::TopDirectoryOnly);
        if (projectFiles.Count() > 1)
        {
            Platform::Fatal(TEXT("Too many project files."));
            return -2;
        }
        else if (projectFiles.Count() == 1)
        {
            LOG(Info, "Skip creating new project because it already exists");
            CommandLine::Options.NewProject.Reset();
        }
        else
        {
            Array<String> files;
            FileSystem::DirectoryGetFiles(files, projectPath, TEXT("*"), DirectorySearchOption::TopDirectoryOnly);
            if (files.Count() > 0)
            {
                Platform::Fatal(String::Format(TEXT("Target project folder '{0}' is not empty."), projectPath));
                return -1;
            }
        }
    }
    if (CommandLine::Options.NewProject.IsTrue())
    {
        PROFILE_CPU_NAMED("New");
        if (projectPath.IsEmpty())
            projectPath = Platform::GetWorkingDirectory();
        else if (!FileSystem::DirectoryExists(projectPath))
            FileSystem::CreateDirectory(projectPath);
        FileSystem::NormalizePath(projectPath);
        String folderName = StringUtils::GetFileName(projectPath);
        String tmpName;
        for (int32 i = 0; i < folderName.Length(); i++)
        {
            Char c = folderName[i];
            if (StringUtils::IsAlnum(c) && c != ' ' && c != '.')
                tmpName += c;
        }

        // Create project file
        ProjectInfo newProject;
        newProject.Name = MoveTemp(tmpName);
        newProject.ProjectPath = projectPath / newProject.Name + TEXT(".flaxproj");
        newProject.ProjectFolderPath = projectPath;
        newProject.Version = Version(1, 0);
        newProject.Company = TEXT("My Company");
        newProject.MinEngineVersion = FLAXENGINE_VERSION;
        newProject.GameTarget = TEXT("GameTarget");
        newProject.EditorTarget = TEXT("GameEditorTarget");
        auto& flaxRef = newProject.References.AddOne();
        flaxRef.Name = TEXT("$(EnginePath)/Flax.flaxproj");
        flaxRef.Project = nullptr;
        if (newProject.SaveProject())
            return 10;

        // Generate source files
        if (FileSystem::CreateDirectory(projectPath / TEXT("Content")))
            return 11;
        if (FileSystem::CreateDirectory(projectPath / TEXT("Source/Game")))
            return 11;
        bool failed = File::WriteAllText(projectPath / TEXT("Source/GameTarget.Build.cs"),TEXT(
                                             "using Flax.Build;\n"
                                             "\n"
                                             "public class GameTarget : GameProjectTarget\n"
                                             "{\n"
                                             "    /// <inheritdoc />\n"
                                             "    public override void Init()\n"
                                             "    {\n"
                                             "        base.Init();\n"
                                             "\n"
                                             "        // Reference the modules for game\n"
                                             "        Modules.Add(nameof(Game));\n"
                                             "    }\n"
                                             "}\n"), Encoding::UTF8);
        failed |= File::WriteAllText(projectPath / TEXT("Source/GameEditorTarget.Build.cs"),TEXT(
                                         "using Flax.Build;\n"
                                         "\n"
                                         "public class GameEditorTarget : GameProjectEditorTarget\n"
                                         "{\n"
                                         "    /// <inheritdoc />\n"
                                         "    public override void Init()\n"
                                         "    {\n"
                                         "        base.Init();\n"
                                         "\n"
                                         "        // Reference the modules for editor\n"
                                         "        Modules.Add(nameof(Game));\n"
                                         "    }\n"
                                         "}\n"), Encoding::UTF8);
        failed |= File::WriteAllText(projectPath / TEXT("Source/Game/Game.Build.cs"),TEXT(
                                         "using Flax.Build;\n"
                                         "using Flax.Build.NativeCpp;\n"
                                         "\n"
                                         "public class Game : GameModule\n"
                                         "{\n"
                                         "    /// <inheritdoc />\n"
                                         "    public override void Init()\n"
                                         "    {\n"
                                         "        base.Init();\n"
                                         "\n"
                                         "        // C#-only scripting\n"
                                         "        BuildNativeCode = false;\n"
                                         "    }\n"
                                         "\n"
                                         "    /// <inheritdoc />\n"
                                         "    public override void Setup(BuildOptions options)\n"
                                         "    {\n"
                                         "        base.Setup(options);\n"
                                         "\n"
                                         "        options.ScriptingAPI.IgnoreMissingDocumentationWarnings = true;\n"
                                         "\n"
                                         "        // Here you can modify the build options for your game module\n"
                                         "        // To reference another module use: options.PublicDependencies.Add(\"Audio\");\n"
                                         "        // To add C++ define use: options.PublicDefinitions.Add(\"COMPILE_WITH_FLAX\");\n"
                                         "        // To learn more see scripting documentation.\n"
                                         "    }\n"
                                         "}\n"), Encoding::UTF8);
        if (failed)
            return 12;
    }

    // Get the last opened project path
    String localCachePath;
    FileSystem::GetSpecialFolderPath(SpecialFolder::AppData, localCachePath);
    String editorConfigPath = localCachePath / TEXT("Flax");
    String lastProjectSettingPath = editorConfigPath / TEXT("LastProject.txt");
    if (!FileSystem::DirectoryExists(editorConfigPath))
        FileSystem::CreateDirectory(editorConfigPath);
    String lastProjectPath;
    if (FileSystem::FileExists(lastProjectSettingPath))
        File::ReadAllText(lastProjectSettingPath, lastProjectPath);
    if (!FileSystem::DirectoryExists(lastProjectPath))
        lastProjectPath = String::Empty;

    // Try to open the last project when requested
    if (projectPath.IsEmpty() && CommandLine::Options.LastProject.IsTrue() && !lastProjectPath.IsEmpty())
        projectPath = lastProjectPath;

    // Missing project case
    if (projectPath.IsEmpty())
    {
#if PLATFORM_HAS_HEADLESS_MODE
        if (CommandLine::Options.Headless.IsTrue())
        {
            Platform::Fatal(TEXT("Missing project path."));
            return -1;
        }
#endif

        // Ask user to pick a project to open
        Array<String> files;
        if (FileSystem::ShowOpenFileDialog(
            nullptr,
            lastProjectPath,
            TEXT("Project files (*.flaxproj)\0*.flaxproj\0All files (*.*)\0*.*\0"),
            false,
            TEXT("Select project to open in Editor"),
            files) || files.Count() != 1)
        {
            return -1;
        }
        if (!FileSystem::FileExists(files[0]))
        {
            Platform::Fatal(TEXT("Cannot open selected project file because it doesn't exist."));
            return -1;
        }
        projectPath = StringUtils::GetDirectoryName(files[0]);
        StringUtils::PathRemoveRelativeParts(projectPath);
    }

    // Check folder with project exists
    if (!FileSystem::DirectoryExists(projectPath))
    {
        Platform::Fatal(String::Format(TEXT("Project folder '{0}' is missing"), projectPath));
        return -1;
    }
    Globals::ProjectFolder = projectPath;
    ASSERT(!FileSystem::IsRelative(Globals::ProjectFolder));

    // Load project
    Array<String> projectFiles;
    FileSystem::DirectoryGetFiles(projectFiles, projectPath, TEXT("*.flaxproj"), DirectorySearchOption::TopDirectoryOnly);
    if (projectFiles.Count() == 0)
    {
        Platform::Fatal(TEXT("Missing project file (*.flaxproj)."));
        return -2;
    }
    if (projectFiles.Count() > 1)
    {
        Platform::Fatal(TEXT("Too many project files."));
        return -2;
    }
    Project = New<ProjectInfo>();
    ProjectInfo::ProjectsCache.Add(Project);
    const bool loadResult = Project->LoadProject(projectFiles[0]);
    if (loadResult)
    {
        Platform::Fatal(TEXT("Cannot load project."));
        return -2;
    }

    HashSet<ProjectInfo*> projects;
    Project->GetAllProjects(projects);

    // Validate project min supported version (older engine may try to load newer project)
    // Special check if project specifies only build number, then major/minor fields are set to 0
    const auto engineVersion = FLAXENGINE_VERSION;
    for (const auto& e : projects)
    {
        const auto project = e.Item;
        if (project->MinEngineVersion > engineVersion ||
            (project->MinEngineVersion.Major() == 0 && project->MinEngineVersion.Minor() == 0 && project->MinEngineVersion.Build() > engineVersion.Build())
        )
        {
            String msg = String::Format(TEXT("Cannot open project \"{0}\".\nIt requires version {1} but editor has version {2}.\nPlease update the editor or press 'Cancel' to try loading it."), project->Name, project->MinEngineVersion.ToString(), engineVersion.ToString());
            const auto result = MessageBox::Show(msg, TEXT("Engine version"), MessageBoxButtons::OKCancel, MessageBoxIcon::Error);
            if (result == DialogResult::Cancel)
                break; // Ignore error and try loading project
            return -2;
        }
    }

#if !FLAX_TESTS
    // Update the last opened project path
    if (lastProjectPath.Compare(Project->ProjectFolderPath) != 0)
        File::WriteAllText(lastProjectSettingPath, Project->ProjectFolderPath, Encoding::UTF8);
#endif

    return 0;
}

Window* Editor::CreateMainWindow()
{
    PROFILE_MEM(Editor);
    Window* window = Managed->GetMainWindow();

#if PLATFORM_LINUX || PLATFORM_MAC
    // Set window icon
    const String iconPath = Globals::BinariesFolder / TEXT("Logo.png");
    if (FileSystem::FileExists(iconPath))
    {
        TextureData icon;
        if (TextureTool::ImportTexture(iconPath, icon))
        {
            LOG(Warning, "Failed to load icon file.");
        }
        else
        {
            window->SetIcon(icon);
        }
    }
    else
    {
        LOG(Warning, "Missing icon file.");
    }
#endif
    return window;
}

bool Editor::Init()
{
    // Scripts project files generation from command line
    if (CommandLine::Options.GenProjectFiles.IsTrue())
    {
        const String customArgs = TEXT("-verbose -log -logfile=\"Cache/Intermediate/ProjectFileLog.txt\"");
        const bool failed = ScriptsBuilder::GenerateProject(customArgs);
        exit(failed ? 1 : 0);
        return true;
    }
    PROFILE_CPU();
    PROFILE_MEM(Editor);

    // When loading project that was opened with older engine version, load all assets to auto-save ones that were deprecated (see Asset::onLoad that does resave)
    if (EditorImpl::UpgradeOldProject)
    {
        PROFILE_CPU_NAMED("Upgrade");

        // Upgrade assets
        AssetsCache* registry = Content::GetRegistry();
        auto assets = Content::GetAllAssets();
        LOG(Info, "Upgrading {} assets...", assets.Count());
        int32 loadedCount = 0;
        for (const Guid& id : assets)
        {
            AssetInfo info;
            if (registry->FindAsset(id, info))
            {
                // Skip assets that don't need this reload
                if (info.TypeName == TEXT("FlaxEngine.Texture") ||
                    info.TypeName == TEXT("FlaxEngine.CubeTexture") ||
                    info.TypeName == TEXT("FlaxEngine.AudioClip") ||
                    info.TypeName == TEXT("FlaxEngine.SkinnedModel") ||
                    info.TypeName == TEXT("FlaxEngine.Model"))
                    continue;

                if (Content::LoadAsync(info.ID))
                {
                    loadedCount++;
                }
            }
        }
        LOG(Info, "Upgrade ended with {} assets loaded", loadedCount);
        auto stats = Content::GetStats();
        LOG(Info, "Loaded assets: {}, loading assets: {}", stats.LoadedAssetsCount, stats.LoadingAssetsCount);

        // TODO: should we upgrade prefabs and scenes? they can contain deprecated data too
    }

    // If during last lightmaps baking engine crashed we could try to restore the progress
    ShadowsOfMordor::Builder::Instance()->CheckIfRestoreState();

    Engine::Update.Bind(&EditorImpl::OnUpdate);
    Managed = New<ManagedEditor>();

    // Initialize managed editor
    Managed->Init();

    // Start play if requested by cmd line
    if (CommandLine::Options.Play.HasValue())
    {
        Managed->RequestStartPlayOnEditMode();
    }

    return false;
}

void Editor::BeforeRun()
{
    PROFILE_MEM(Editor);
    Managed->BeforeRun();
}

void Editor::BeforeExit()
{
    PROFILE_MEM(Editor);
    CloseSplashScreen();

    Managed->Exit();
    SAFE_DELETE(Managed);
    Project = nullptr;
    ProjectInfo::ProjectsCache.ClearDelete();
}

void EditorImpl::OnUpdate()
{
    PROFILE_MEM(Editor);

    // Update c# editor
    Editor::Managed->Update();

    // If editor thread doesn't have the focus, don't suck up too much CPU time
    const auto hasFocus = Engine::HasFocus;
    if (HasFocus && !hasFocus)
    {
        // Drop our priority to speed up whatever is in the foreground
        Platform::SetThreadPriority(ThreadPriority::BelowNormal);
    }
    else if (hasFocus && !HasFocus)
    {
        // Boost our priority back to normal
        Platform::SetThreadPriority(ThreadPriority::Normal);
    }
    HasFocus = hasFocus;
}

#endif
