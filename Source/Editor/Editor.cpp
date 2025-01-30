// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_EDITOR

#include "Editor.h"
#include "ProjectInfo.h"
#include "Engine/Core/Log.h"
#include "Scripting/ScriptsBuilder.h"
#include "Windows/SplashScreen.h"
#include "Managed/ManagedEditor.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Engine/Engine.h"
#include "Engine/ShadowsOfMordor/Builder.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "FlaxEngine.Gen.h"
#if PLATFORM_LINUX
#include "Engine/Tools/TextureTool/TextureTool.h"
#endif

namespace EditorImpl
{
    bool IsOldProjectXmlFormat = false;
    bool HasFocus = false;
    SplashScreen* Splash = nullptr;

    void OnUpdate();
}

ManagedEditor* Editor::Managed = nullptr;
ProjectInfo* Editor::Project = nullptr;
bool Editor::IsPlayMode = false;
bool Editor::IsOldProjectOpened = true;
int32 Editor::LastProjectOpenedEngineBuild = 0;

void Editor::CloseSplashScreen()
{
    SAFE_DELETE(EditorImpl::Splash);
}

bool Editor::CheckProjectUpgrade()
{
    const auto versionFilePath = Globals::ProjectCacheFolder / TEXT("version");

    // Load version cache file
    int32 lastMajor = FLAXENGINE_VERSION_MAJOR;
    int32 lastMinor = FLAXENGINE_VERSION_MINOR;
    int32 lastBuild = FLAXENGINE_VERSION_BUILD;
    if (FileSystem::FileExists(versionFilePath))
    {
        auto file = FileReadStream::Open(versionFilePath);
        if (file)
        {
            file->ReadInt32(&lastMajor);
            file->ReadInt32(&lastMinor);
            file->ReadInt32(&lastBuild);

            // Invalidate results if data has issues
            if (file->HasError() || lastMajor < 0 || lastMinor < 0 || lastMajor > 100 || lastMinor > 1000)
            {
                lastMajor = FLAXENGINE_VERSION_MAJOR;
                lastMinor = FLAXENGINE_VERSION_MINOR;
                lastBuild = FLAXENGINE_VERSION_BUILD;
                LOG(Warning, "Invalid version cache data");
            }
            else
            {
                LOG(Info, "Last project open version: {0}.{1}.{2}", lastMajor, lastMinor, lastBuild);
                LastProjectOpenedEngineBuild = lastBuild;
            }

            Delete(file);
        }
    }
    else
    {
        LOG(Warning, "Missing version cache file");
    }

    // Check if project is in the old, deprecated layout
    if (EditorImpl::IsOldProjectXmlFormat)
    {
        // [Deprecated: 16.04.2020, expires 16.04.2021]
        LOG(Warning, "The project is in an old format and will be upgraded.");
        const auto result = MessageBox::Show(TEXT("The Flax project is in an old format and will be upgraded. Loading it may modify existing data so older editor version won't open it. Do you want to perform a backup before or cancel operation?"), TEXT("Project upgrade"), MessageBoxButtons::YesNoCancel, MessageBoxIcon::Question);
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

        const String& root = Globals::ProjectFolder;
        const String& name = Project->Name;
        String codeName;
        for (int32 i = 0; i < name.Length(); i++)
        {
            Char c = name[i];
            if (StringUtils::IsAlnum(c) && c != ' ' && c != '.')
                codeName += c;
        }
        const String& sourceFolder = Globals::ProjectSourceFolder;
        const String gameModuleFolder = sourceFolder / codeName;
        const String gameEditorModuleFolder = sourceFolder / codeName + TEXT("Editor");
        const String tempSourceSetup = Globals::ProjectCacheFolder / TEXT("UpgradeSource");
        const String tempSourceSetupGame = tempSourceSetup / codeName;
        const String tempSourceSetupGameEditor = tempSourceSetup / codeName + TEXT("Editor");

        // Remove old project files
        FileSystem::DeleteFile(root / name + TEXT(".sln"));
        FileSystem::DeleteFile(root / name + TEXT(".csproj"));
        FileSystem::DeleteFile(root / name + TEXT(".csproj.user"));
        FileSystem::DeleteFile(root / name + TEXT(".Editor.csproj"));
        FileSystem::DeleteFile(root / name + TEXT(".Editor.csproj.user"));

        // Remove old cache files
        FileSystem::DeleteDirectory(root / TEXT("Cache/Assemblies"));
        FileSystem::DeleteDirectory(root / TEXT("Cache/bin"));
        FileSystem::DeleteDirectory(root / TEXT("Cache/obj"));
        FileSystem::DeleteDirectory(root / TEXT("Cache/Shaders"));

        // Move C# files to new locations
        FileSystem::DeleteDirectory(tempSourceSetup);
        FileSystem::CreateDirectory(tempSourceSetup);
        Array<String> files;
        FileSystem::DirectoryGetFiles(files, sourceFolder);
        bool useEditorModule = false;
        for (auto& file : files)
        {
            String tempSourceFile;
            if (file.Contains(TEXT("/Editor/")))
            {
                useEditorModule = true;
                tempSourceFile = tempSourceSetupGameEditor / StringUtils::GetFileName(file);
            }
            else
            {
                tempSourceFile = tempSourceSetupGame / FileSystem::ConvertAbsolutePathToRelative(sourceFolder, file);
            }
            FileSystem::CreateDirectory(StringUtils::GetDirectoryName(tempSourceFile));
            FileSystem::CopyFile(tempSourceFile, file);
        }
        FileSystem::DeleteDirectory(sourceFolder);
        FileSystem::CopyDirectory(sourceFolder, tempSourceSetup);
        FileSystem::DeleteDirectory(tempSourceSetup);

        // Generate module files
        File::WriteAllText(gameModuleFolder / String::Format(TEXT("{0}.Build.cs"), codeName), String::Format(TEXT(
                               "using Flax.Build;\n"
                               "using Flax.Build.NativeCpp;\n"
                               "\n"
                               "public class {0} : GameModule\n"
                               "{{\n"
                               "    /// <inheritdoc />\n"
                               "    public override void Setup(BuildOptions options)\n"
                               "    {{\n"
                               "        base.Setup(options);\n"
                               "\n"
                               "        // Ignore compilation warnings due to missing code documentation comments\n"
                               "        options.ScriptingAPI.IgnoreMissingDocumentationWarnings = true;\n"
                               "\n"
                               "        // Here you can modify the build options for your game module\n"
                               "        // To reference another module use: options.PublicDependencies.Add(\"Audio\");\n"
                               "        // To add C++ define use: options.PublicDefinitions.Add(\"COMPILE_WITH_FLAX\");\n"
                               "        // To learn more see scripting documentation.\n"
                               "        BuildNativeCode = false;\n"
                               "    }}\n"
                               "}}\n"
                           ), codeName), Encoding::UTF8);
        if (useEditorModule)
        {
            File::WriteAllText(gameEditorModuleFolder / String::Format(TEXT("{0}Editor.Build.cs"), codeName), String::Format(TEXT(
                                   "using Flax.Build;\n"
                                   "using Flax.Build.NativeCpp;\n"
                                   "\n"
                                   "public class {0}Editor : GameEditorModule\n"
                                   "{{\n"
                                   "    /// <inheritdoc />\n"
                                   "    public override void Setup(BuildOptions options)\n"
                                   "    {{\n"
                                   "        base.Setup(options);\n"
                                   "\n"
                                   "        // Reference game source module to access game code types\n"
                                   "        options.PublicDependencies.Add(\"{0}\");\n"
                                   "\n"
                                   "        // Ignore compilation warnings due to missing code documentation comments\n"
                                   "        options.ScriptingAPI.IgnoreMissingDocumentationWarnings = true;\n"
                                   "\n"
                                   "        // Here you can modify the build options for your game editor module\n"
                                   "        // To reference another module use: options.PublicDependencies.Add(\"Audio\");\n"
                                   "        // To add C++ define use: options.PublicDefinitions.Add(\"COMPILE_WITH_FLAX\");\n"
                                   "        // To learn more see scripting documentation.\n"
                                   "        BuildNativeCode = false;\n"
                                   "    }}\n"
                                   "}}\n"
                               ), codeName), Encoding::UTF8);
        }

        // Generate target files
        File::WriteAllText(sourceFolder / String::Format(TEXT("{0}Target.Build.cs"), codeName), String::Format(TEXT(
                               "using Flax.Build;\n"
                               "\n"
                               "public class {0}Target : GameProjectTarget\n"
                               "{{\n"
                               "    /// <inheritdoc />\n"
                               "    public override void Init()\n"
                               "    {{\n"
                               "        base.Init();\n"
                               "\n"
                               "        // Reference the modules for game\n"
                               "        Modules.Add(nameof({0}));\n"
                               "    }}\n"
                               "}}\n"
                           ), codeName), Encoding::UTF8);
        const String editorTargetGameEditorModule = useEditorModule ? String::Format(TEXT("        Modules.Add(\"{0}Editor\");\n"), codeName) : String::Empty;
        File::WriteAllText(sourceFolder / String::Format(TEXT("{0}EditorTarget.Build.cs"), codeName), String::Format(TEXT(
                               "using Flax.Build;\n"
                               "\n"
                               "public class {0}EditorTarget : GameProjectEditorTarget\n"
                               "{{\n"
                               "    /// <inheritdoc />\n"
                               "    public override void Init()\n"
                               "    {{\n"
                               "        base.Init();\n"
                               "\n"
                               "        // Reference the modules for editor\n"
                               "        Modules.Add(nameof({0}));\n"
                               "{1}"
                               "    }}\n"
                               "}}\n"
                           ), codeName, editorTargetGameEditorModule), Encoding::UTF8);

        // Generate new project file
        Project->ProjectPath = root / String::Format(TEXT("{0}.flaxproj"), codeName);
        Project->GameTarget = codeName + TEXT("Target");
        Project->EditorTarget = codeName + TEXT("EditorTarget");
        Project->SaveProject();

        // Remove old project file
        FileSystem::DeleteFile(root / TEXT("Project.xml"));

        LOG(Warning, "Project layout upgraded!");
    }
    // Check if last version was the same
    else if (lastMajor == FLAXENGINE_VERSION_MAJOR && lastMinor == FLAXENGINE_VERSION_MINOR)
    {
        // Do nothing
        IsOldProjectOpened = false;
    }
    // Check if last version was older
    else if (lastMajor < FLAXENGINE_VERSION_MAJOR || (lastMajor == FLAXENGINE_VERSION_MAJOR && lastMinor < FLAXENGINE_VERSION_MINOR))
    {
        LOG(Warning, "The project was opened with the older editor version last time");
        const auto result = MessageBox::Show(TEXT("The project was opened with the older editor version last time. Loading it may modify existing data so older editor version won't open it. Do you want to perform a backup before or cancel operation?"), TEXT("Project upgrade"), MessageBoxButtons::YesNoCancel, MessageBoxIcon::Question);
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
    // Check if last version was newer
    else if (lastMajor > FLAXENGINE_VERSION_MAJOR || (lastMajor == FLAXENGINE_VERSION_MAJOR && lastMinor > FLAXENGINE_VERSION_MINOR))
    {
        LOG(Warning, "The project was opened with the newer editor version last time");
        const auto result = MessageBox::Show(TEXT("The project was opened with the newer editor version last time. Loading it may fail and corrupt existing data. Do you want to perform a backup before or cancel operation?"), TEXT("Project upgrade"), MessageBoxButtons::YesNoCancel, MessageBoxIcon::Warning);
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
    if (lastMajor != FLAXENGINE_VERSION_MAJOR || lastMinor != FLAXENGINE_VERSION_MINOR)
    {
        LOG(Info, "Cleaning cache files from different engine version");
        FileSystem::DeleteDirectory(Globals::ProjectFolder / TEXT("Cache/Cooker"));
        FileSystem::DeleteDirectory(Globals::ProjectFolder / TEXT("Cache/Intermediate"));
    }

    // Upgrade old 0.7 projects
    // [Deprecated: 01.11.2020, expires 01.11.2021]
    if (lastMajor == 0 && lastMinor == 7 && lastBuild <= 6197)
    {
        Array<String> files;
        FileSystem::DirectoryGetFiles(files, Globals::ProjectSourceFolder, TEXT("*.Gen.cs"));
        for (auto& file : files)
            FileSystem::DeleteFile(file);
    }

    // Update version the cache file
    {
        auto file = FileWriteStream::Open(versionFilePath);
        if (file)
        {
            file->WriteInt32(FLAXENGINE_VERSION_MAJOR);
            file->WriteInt32(FLAXENGINE_VERSION_MINOR);
            file->WriteInt32(FLAXENGINE_VERSION_BUILD);
            Delete(file);
        }
        else
        {
            LOG(Error, "Failed to create version cache file");
        }
    }

    return false;
}

bool Editor::BackupProject()
{
    // Create backup directory
    auto dstPath = Globals::ProjectFolder + TEXT(" - Backup");
    {
        int32 count = 0;
        while (count < 1000 && FileSystem::DirectoryExists(dstPath))
        {
            dstPath = Globals::ProjectFolder + TEXT(" - Backup") + StringUtils::ToString(count++);
        }
    }

    LOG(Info, "Backup project to \"{0}\"", dstPath);

    // Copy everything
    return FileSystem::CopyDirectory(dstPath, Globals::ProjectFolder);
}

int32 Editor::LoadProduct()
{
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
    if (CommandLine::Options.NewProject)
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
    if (CommandLine::Options.NewProject)
    {
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

    // Missing project case
    if (projectPath.IsEmpty())
    {
#if PLATFORM_HAS_HEADLESS_MODE
        if (CommandLine::Options.Headless)
        {
            Platform::Fatal(TEXT("Missing project path."));
            return -1;
        }
#endif

        // Ask user to pick a project to open
        Array<String> files;
        if (FileSystem::ShowOpenFileDialog(
            nullptr,
            StringView::Empty,
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

    // Check if opening old project (the one with Project.xml file)
    // [Deprecated: 16.04.2020, expires 16.04.2021]
    const String projectXmlPath = projectPath / TEXT("Project.xml");
    Project = New<ProjectInfo>();
    ProjectInfo::ProjectsCache.Add(Project);
    if (FileSystem::FileExists(projectXmlPath))
    {
        // Load project
        const bool loadResult = Project->LoadOldProject(projectXmlPath);
        if (loadResult)
        {
            Platform::Fatal(TEXT("Cannot load project."));
            return -2;
        }
        EditorImpl::IsOldProjectXmlFormat = true;
    }
    else
    {
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
        const bool loadResult = Project->LoadProject(projectFiles[0]);
        if (loadResult)
        {
            Platform::Fatal(TEXT("Cannot load project."));
            return -2;
        }
    }

    HashSet<ProjectInfo*> projects;
    Project->GetAllProjects(projects);

    // Validate project min supported version (older engine may try to load newer project)
    // Special check if project specifies only build number, then major/minor fields are set to 0
    const auto engineVersion = FLAXENGINE_VERSION;
    for (auto e : projects)
    {
        const auto project = e.Item;
        if (project->MinEngineVersion > engineVersion ||
            (project->MinEngineVersion.Major() == 0 && project->MinEngineVersion.Minor() == 0 && project->MinEngineVersion.Build() > engineVersion.Build())
        )
        {
            Platform::Fatal(String::Format(TEXT("Cannot open project \"{0}\".\nIt requires version {1} but editor has version {2}.\nPlease update the editor."), project->Name, project->MinEngineVersion.ToString(), engineVersion.ToString()));
            return -2;
        }
    }

    return 0;
}

Window* Editor::CreateMainWindow()
{
    Window* window = Managed->GetMainWindow();

#if PLATFORM_LINUX
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
    if (CommandLine::Options.GenProjectFiles)
    {
        const String customArgs = TEXT("-verbose -log -logfile=\"Cache/Intermediate/ProjectFileLog.txt\"");
        const bool failed = ScriptsBuilder::GenerateProject(customArgs);
        exit(failed ? 1 : 0);
        return true;
    }
    PROFILE_CPU();

    // If during last lightmaps baking engine crashed we could try to restore the progress
    ShadowsOfMordor::Builder::Instance()->CheckIfRestoreState();

    Engine::Update.Bind(&EditorImpl::OnUpdate);
    Managed = New<ManagedEditor>();

    // Show splash screen
    if (!CommandLine::Options.Headless.IsTrue())
    {
        PROFILE_CPU_NAMED("Splash");
        if (EditorImpl::Splash == nullptr)
            EditorImpl::Splash = New<SplashScreen>();
        EditorImpl::Splash->SetTitle(Project->Name);
        EditorImpl::Splash->Show();
    }

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
    Managed->BeforeRun();
}

void Editor::BeforeExit()
{
    CloseSplashScreen();

    Managed->Exit();
    SAFE_DELETE(Managed);
    Project = nullptr;
    ProjectInfo::ProjectsCache.ClearDelete();
}

void EditorImpl::OnUpdate()
{
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
