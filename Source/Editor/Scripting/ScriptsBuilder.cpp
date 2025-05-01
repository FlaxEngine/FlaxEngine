// Copyright (c) Wojciech Figat. All rights reserved.

#include "ScriptsBuilder.h"
#include "CodeEditor.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Managed/ManagedEditor.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Debug/Exceptions/FileNotFoundException.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/FileSystemWatcher.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Scripting/Internal/MainThreadManagedInvokeAction.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Level/Level.h"
#include "FlaxEngine.Gen.h"

enum class EventType
{
    Unknown = -1,
    CompileBegin = 0,
    CompileStarted = 1,
    CompileEndGood = 2,
    CompileEndFailed = 3,
    ReloadCalled = 4,
    ReloadBegin = 5,
    Reload = 6,
    ReloadEnd = 7,
    ScriptsLoaded = 8,
};

struct EventData
{
    EventType Type;
};

namespace ScriptsBuilderImpl
{
    CriticalSection _locker;
    bool _isInited = false;
    bool _isCompileRequested = false;
    bool _isCompileRunning = false;
    bool _wasProjectStructureChanged = false;
    bool _lastCompilationFailed = false;
    int32 _compilationsCount = 0;
    DateTime _lastSourceCodeEdited = 0;
    DateTime _lastCompileAction = 0;

    Array<FileSystemWatcher*> SourceFoldersWatchers;

    CriticalSection _compileEventsLocker;
    Array<EventData> _compileEvents;

    MMethod* Internal_OnEvent = nullptr;
    MMethod* Internal_OnCompileEvent = nullptr;
    MMethod* Internal_OnCodeEditorEvent = nullptr;

    void CallEvent(EventType type);
    void CallCompileEvent(EventData& data);
    void CallCodeEditorEvent(bool isEnd);

    void sourceDirEvent(const String& path, FileSystemAction action);
    void onEditorAssemblyUnloading(MAssembly* assembly);
    void onScriptsReloadStart();
    void onScriptsReload();
    void onScriptsReloadEnd();
    void onScriptsLoaded();

    void GetClassName(const StringAnsi& fullname, StringAnsi& className);

    void onCodeEditorAsyncOpenBegin()
    {
        CallCodeEditorEvent(false);
    }

    void onCodeEditorAsyncOpenEnd()
    {
        CallCodeEditorEvent(true);
    }

    bool compileGameScriptsAsyncInner();
    bool compileGameScriptsAsync();
}

using namespace ScriptsBuilderImpl;

class ScriptsBuilderService : public EngineService
{
public:

    ScriptsBuilderService()
        : EngineService(TEXT("Scripts Builder"))
    {
    }

    bool Init() override;
    void Update() override;
    void Dispose() override;
};

ScriptsBuilderService ScriptsBuilderServiceInstance;

Delegate<bool> ScriptsBuilder::OnCompilationEnd;
Action ScriptsBuilder::OnCompilationSuccess;
Action ScriptsBuilder::OnCompilationFailed;

void ScriptsBuilderImpl::sourceDirEvent(const String& path, FileSystemAction action)
{
    // Discard non-source files or generated files
    if ((!path.EndsWith(TEXT(".cs")) &&
        !path.EndsWith(TEXT(".cpp")) &&
        !path.EndsWith(TEXT(".h"))) ||
        path.EndsWith(TEXT(".Gen.cs")))
        return;

    ScopeLock scopeLock(_locker);
    _lastSourceCodeEdited = DateTime::Now();
}

int32 ScriptsBuilder::GetCompilationsCount()
{
    Platform::MemoryBarrier();
    return _compilationsCount;
}

String ScriptsBuilder::GetBuildToolPath()
{
#if USE_NETCORE && (PLATFORM_LINUX || PLATFORM_MAC)
    return Globals::StartupFolder / TEXT("Binaries/Tools/Flax.Build");
#else
    return Globals::StartupFolder / TEXT("Binaries/Tools/Flax.Build.exe");
#endif
}

bool ScriptsBuilder::LastCompilationFailed()
{
    return _lastCompilationFailed;
}

void ScriptsBuilder::FilterNamespaceText(String& value)
{
    value.Replace(TEXT(" "), TEXT(""), StringSearchCase::CaseSensitive);
    value.Replace(TEXT("."), TEXT(""), StringSearchCase::CaseSensitive);
    value.Replace(TEXT("-"), TEXT(""), StringSearchCase::CaseSensitive);
}

bool ScriptsBuilder::IsSourceDirty()
{
    ScopeLock scopeLock(_locker);
    return _lastSourceCodeEdited > _lastCompileAction;
}

bool ScriptsBuilder::IsSourceWorkspaceDirty()
{
    ScopeLock scopeLock(_locker);
    return _wasProjectStructureChanged;
}

bool ScriptsBuilder::IsSourceDirtyFor(const TimeSpan& timeout)
{
    ScopeLock scopeLock(_locker);
    return _lastSourceCodeEdited > _lastCompileAction && DateTime::Now() > _lastSourceCodeEdited + timeout;
}

bool ScriptsBuilder::IsCompiling()
{
    ScopeLock scopeLock(_locker);
    return _isCompileRunning;
}

bool ScriptsBuilder::IsReady()
{
    ScopeLock scopeLock(_locker);
    return !IsSourceDirty() && !_isCompileRequested && !_isCompileRunning;
}

void ScriptsBuilder::MarkWorkspaceDirty()
{
    ScopeLock scopeLock(_locker);
    _lastSourceCodeEdited = DateTime::Now();
    _wasProjectStructureChanged = true;
}

void ScriptsBuilder::CheckForCompile()
{
    ScopeLock scopeLock(_locker);
    if (IsSourceDirty())
        Compile();
}

void ScriptsBuilderImpl::onScriptsReloadStart()
{
    CallEvent(EventType::ReloadBegin);
}

void ScriptsBuilderImpl::onScriptsReload()
{
    CallEvent(EventType::Reload);
}

void ScriptsBuilderImpl::onScriptsReloadEnd()
{
    CallEvent(EventType::ReloadEnd);
}

void ScriptsBuilderImpl::onScriptsLoaded()
{
    CallEvent(EventType::ScriptsLoaded);
}

void ScriptsBuilder::Compile()
{
    ScopeLock scopeLock(_locker);

    // Request compile job
    _isCompileRequested = true;
}

bool ScriptsBuilder::RunBuildTool(const StringView& args, const StringView& workingDir)
{
    PROFILE_CPU();
    const String buildToolPath = GetBuildToolPath();
    if (!FileSystem::FileExists(buildToolPath))
    {
        Log::FileNotFoundException(buildToolPath).SetLevel(LogType::Fatal);
        return true;
    }

    // Prepare build options
    StringBuilder cmdLine(args.Length() + buildToolPath.Length() + 200);
#if !USE_NETCORE && (PLATFORM_LINUX || PLATFORM_MAC)
    const String monoPath = Globals::MonoPath / TEXT("bin/mono");
    if (!FileSystem::FileExists(monoPath))
    {
        Log::FileNotFoundException(monoPath).SetLevel(LogType::Fatal);
        return true;
    }
    const String monoPath = TEXT("mono");
    cmdLine.Append(monoPath);
    cmdLine.Append(TEXT(" "));
    // TODO: Set env var for the mono MONO_GC_PARAMS=nursery-size64m to boost build performance -> profile it
#endif
    cmdLine.Append(buildToolPath);

    // Call build tool
    CreateProcessSettings procSettings;
    procSettings.FileName = StringView(*cmdLine, cmdLine.Length());
    procSettings.Arguments = args.Get();
    procSettings.WorkingDirectory = workingDir;
    const int32 result = Platform::CreateProcess(procSettings);
    if (result != 0)
        LOG(Error, "Failed to run build tool, result: {0:x}", (uint32)result);
    return result != 0;
}

bool ScriptsBuilder::GenerateProject(const StringView& customArgs)
{
    String args(TEXT("-log -mutex -genproject "));
    args += customArgs;
    _wasProjectStructureChanged = false;
    return RunBuildTool(args);
}

void ScriptsBuilderImpl::GetClassName(const StringAnsi& fullname, StringAnsi& className)
{
    const auto lastDotIndex = fullname.FindLast('.');
    if (lastDotIndex != -1)
    {
        //namespaceName = fullname.Substring(0, lastDotIndex);
        className = fullname.Substring(lastDotIndex + 1);
    }
    else
    {
        className = fullname;
    }
}

MClass* ScriptsBuilder::FindScript(const StringView& scriptName)
{
    PROFILE_CPU();
    const StringAnsi scriptNameStd = scriptName.ToStringAnsi();

    const ScriptingTypeHandle scriptingType = Scripting::FindScriptingType(scriptNameStd);
    if (scriptingType)
    {
        MClass* mclass = scriptingType.GetType().ManagedClass;
        if (mclass)
        {
            return mclass;
        }
    }

    // Check all assemblies (ignoring the typename namespace)
    auto& modules = BinaryModule::GetModules();
    StringAnsi className;
    GetClassName(scriptNameStd, className);
    MClass* scriptClass = Script::GetStaticClass();
    for (int32 j = 0; j < modules.Count(); j++)
    {
        auto managedModule = dynamic_cast<ManagedBinaryModule*>(modules[j]);
        if (!managedModule)
            continue;
        auto assembly = managedModule->Assembly;
        auto& classes = assembly->GetClasses();
        for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
        {
            MClass* mclass = i->Value;

            // Managed scripts
            if (mclass->IsSubClassOf(scriptClass) && !mclass->IsStatic() && !mclass->IsAbstract() && !mclass->IsInterface())
            {
                StringAnsi mclassName;
                GetClassName(mclass->GetFullName(), mclassName);
                if (className == mclassName)
                {
                    LOG(Info, "Found {0} type for type {1} (assembly {2})", String(mclass->GetFullName()), String(scriptName.Get(), scriptName.Length()), assembly->ToString());
                    return mclass;
                }
            }
        }
    }

    LOG(Warning, "Failed to find script class of name {0}", String(scriptNameStd));
    return nullptr;
}

void ScriptsBuilder::GetExistingEditors(int32* result, int32 count)
{
    auto& editors = CodeEditingManager::GetEditors();
    for (int32 i = 0; i < editors.Count() && i < count; i++)
    {
        result[static_cast<int32>(editors[i]->GetType())] = 1;
    }
}

void ScriptsBuilder::GetBinariesConfiguration(StringView& target, StringView& platform, StringView& architecture, StringView& configuration)
{
    const Char *targetPtr, *platformPtr, *architecturePtr, *configurationPtr;
    GetBinariesConfiguration(targetPtr, platformPtr, architecturePtr, configurationPtr);
    target = targetPtr;
    platform = platformPtr;
    architecture = architecturePtr;
    configuration = configurationPtr;
}

void ScriptsBuilder::GetBinariesConfiguration(const Char*& target, const Char*& platform, const Char*& architecture, const Char*& configuration)
{
    // Special case when opening engine project
    if (Editor::Project->ProjectFolderPath == Globals::StartupFolder)
    {
        target = platform = architecture = configuration = nullptr;
        return;
    }

    // Pick game target
    if (Editor::Project->EditorTarget.HasChars())
    {
        target = Editor::Project->EditorTarget.Get();
    }
    else if (Editor::Project->GameTarget.HasChars())
    {
        target = Editor::Project->GameTarget.Get();
    }
    else
    {
        target = TEXT("");
        LOG(Warning, "Missing editor/game targets in project. Please specify EditorTarget and GameTarget properties in .flaxproj file.");
    }

#if PLATFORM_WINDOWS
    platform = TEXT("Windows");
#elif PLATFORM_LINUX
    platform = TEXT("Linux");
#elif PLATFORM_MAC
    platform = TEXT("Mac");
#else
#error "Unknown platform"
#endif

#if PLATFORM_ARCH_X64
    architecture = TEXT("x64");
#elif PLATFORM_ARCH_X86
    architecture = TEXT("x86");
#elif PLATFORM_ARCH_ARM
    architecture = TEXT("arm");
#elif PLATFORM_ARCH_ARM64
    architecture = TEXT("arm64");
#else
#error "Unknown architecture"
#endif

#if BUILD_DEBUG
    configuration = TEXT("Debug");
#elif BUILD_DEVELOPMENT
    configuration = TEXT("Development");
#elif BUILD_RELEASE
    configuration = TEXT("Release");
#else
#error "Unknown configuration"
#endif
}

bool ScriptsBuilderImpl::compileGameScriptsAsyncInner()
{
    LOG(Info, "Starting scripts compilation...");
    CallEvent(EventType::CompileStarted);

    // Call compilation
    const Char *target, *platform, *architecture, *configuration;
    ScriptsBuilder::GetBinariesConfiguration(target, platform, architecture, configuration);
    if (StringUtils::Length(target) == 0)
    {
        LOG(Info, "Missing EditorTarget in project. Skipping compilation.");
        CallEvent(EventType::ReloadCalled);
        Scripting::Reload();
        return false;
    }
    auto args = String::Format(
        TEXT("-log -logfile= -build -mutex -buildtargets={0} -skiptargets=FlaxEditor -platform={1} -arch={2} -configuration={3}"),
        target, platform, architecture, configuration);
    if (Scripting::HasGameModulesLoaded())
    {
        // Add postfix to output binaries to prevent file locking collisions when doing hot-reload in Editor
        args += String::Format(TEXT(" -hotreload=\".HotReload.{0}\""), _compilationsCount - 1);
    }
    if (ScriptsBuilder::RunBuildTool(args))
        return true;

    // Reload scripts
    CallEvent(EventType::ReloadCalled);
    Scripting::Reload();

    return false;
}

void ScriptsBuilderImpl::CallEvent(EventType type)
{
    ScopeLock lock(_compileEventsLocker);

    const int32 index = _compileEvents.Count();
    _compileEvents.AddDefault(1);
    auto& data = _compileEvents[index];
    data.Type = type;

    // Flush events on a main tread
    if (IsInMainThread())
    {
        for (int32 i = 0; i < _compileEvents.Count(); i++)
            CallCompileEvent(_compileEvents[i]);
        _compileEvents.Clear();
    }
}

void ScriptsBuilderImpl::CallCompileEvent(EventData& data)
{
    ASSERT(IsInMainThread());

    // Special case for a single events with no data
    if (data.Type != EventType::Unknown)
    {
        // Call C# event
        if (Internal_OnEvent == nullptr)
        {
            auto scriptsBuilderClass = ScriptsBuilder::GetStaticClass();
            if (scriptsBuilderClass)
                Internal_OnEvent = scriptsBuilderClass->GetMethod("Internal_OnEvent", 1);
            if (Internal_OnEvent == nullptr)
            {
                LOG(Fatal, "Invalid Editor assembly!");
            }
        }
        /*MObject* exception = nullptr;
        void* args[1];
        args[0] = &data.Type;
        Internal_OnEvent->Invoke(nullptr, args, &exception);
        if (exception)
        {
            DebugLog::LogException(exception);
        }*/

        MainThreadManagedInvokeAction::ParamsBuilder params;
        params.AddParam(data.Type);
        MainThreadManagedInvokeAction::Invoke(Internal_OnEvent, params);
    }
}

void ScriptsBuilderImpl::CallCodeEditorEvent(bool isEnd)
{
    if (Internal_OnCodeEditorEvent == nullptr)
    {
        auto scriptsBuilderClass = ScriptsBuilder::GetStaticClass();
        if (scriptsBuilderClass)
            Internal_OnCodeEditorEvent = scriptsBuilderClass->GetMethod("Internal_OnCodeEditorEvent", 1);
        ASSERT(Internal_OnCodeEditorEvent);
    }

    MainThreadManagedInvokeAction::ParamsBuilder params;
    params.AddParam(isEnd);
    MainThreadManagedInvokeAction::Invoke(Internal_OnCodeEditorEvent, params);
}

void ScriptsBuilderImpl::onEditorAssemblyUnloading(MAssembly* assembly)
{
    Internal_OnEvent = nullptr;
    Internal_OnCompileEvent = nullptr;
    Internal_OnCodeEditorEvent = nullptr;
}

bool ScriptsBuilderImpl::compileGameScriptsAsync()
{
    // Start
    {
        ScopeLock scopeLock(_locker);

        _isCompileRequested = false;
        _lastCompileAction = DateTime::Now();
        _compilationsCount++;
        _isCompileRunning = true;

        ScriptsBuilderServiceInstance.Init();

        CallEvent(EventType::CompileBegin);
    }

    // Do work
    const bool success = !compileGameScriptsAsyncInner();

    // End
    {
        ScopeLock scopeLock(_locker);

        _lastCompilationFailed = !success;

        ScriptsBuilder::OnCompilationEnd(success);
        if (success)
            ScriptsBuilder::OnCompilationSuccess();
        else
            ScriptsBuilder::OnCompilationFailed();
        if (success)
            CallEvent(EventType::CompileEndGood);
        else
            CallEvent(EventType::CompileEndFailed);
        _isCompileRunning = false;
    }

    return false;
}

bool ScriptsBuilderService::Init()
{
    // Check flag
    if (_isInited)
        return false;
    _isInited = true;

    // Link for Editor assembly unload event to clear cached Internal_OnCompilationEnd to prevent errors
    auto editorAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    editorAssembly->Unloading.Bind(onEditorAssemblyUnloading);

    // Listen to scripts reloading events and forward them to c#
    Level::ScriptsReloadStart.Bind(onScriptsReloadStart);
    Level::ScriptsReload.Bind(onScriptsReload);
    Level::ScriptsReloadEnd.Bind(onScriptsReloadEnd);
    Scripting::ScriptsLoaded.Bind(onScriptsLoaded);

    // Listen to code editors manager events
    CodeEditingManager::AsyncOpenBegin.Bind(onCodeEditorAsyncOpenBegin);
    CodeEditingManager::AsyncOpenEnd.Bind(onCodeEditorAsyncOpenEnd);

    // Create source folder watcher to handle scripts modification events (create/delete scripts events are handled by the editor itself)
    auto project = Editor::Project;
    HashSet<ProjectInfo*> projects;
    project->GetAllProjects(projects);
    for (const auto& e : projects)
    {
        ProjectInfo* project = e.Item;
        if (project->Name == TEXT("Flax"))
            continue;
        auto watcher = New<FileSystemWatcher>(project->ProjectFolderPath / TEXT("Source"), true);
        watcher->OnEvent.Bind(sourceDirEvent);
        SourceFoldersWatchers.Add(watcher);
    }

    // Verify project
    if (project->EditorTarget.IsEmpty())
    {
        LOG(Warning, "Missing {0} property in opened project", TEXT("EditorTarget"));
    }
    if (project->GameTarget.IsEmpty())
    {
        LOG(Warning, "Missing {0} property in opened project", TEXT("GameTarget"));
    }

    // Remove any remaining files from previous Editor run hot-reloads
    const Char *target, *platform, *architecture, *configuration;
    ScriptsBuilder::GetBinariesConfiguration(target, platform, architecture, configuration);
    if (StringUtils::Length(target) != 0)
    {
        const String targetOutput = Globals::ProjectFolder / TEXT("Binaries") / target / platform / architecture / configuration;
        Array<String> files;
        FileSystem::DirectoryGetFiles(files, targetOutput, TEXT("*.HotReload.*"), DirectorySearchOption::TopDirectoryOnly);

        for (const auto& reference : Editor::Project->References)
        {
            if (reference.Project->Name == TEXT("Flax"))
                continue;

            String referenceTarget;
            if (reference.Project->EditorTarget.HasChars())
            {
                referenceTarget = reference.Project->EditorTarget.Get();
            }
            else if (reference.Project->GameTarget.HasChars())
            {
                referenceTarget = reference.Project->GameTarget.Get();
            }
            if (referenceTarget.IsEmpty())
                continue;

            const String referenceTargetOutput = reference.Project->ProjectFolderPath / TEXT("Binaries") / referenceTarget / platform / architecture / configuration;
            FileSystem::DirectoryGetFiles(files, referenceTargetOutput, TEXT("*.HotReload.*"), DirectorySearchOption::TopDirectoryOnly);
        }

        if (files.HasItems())
            LOG(Info, "Removing {0} files from previous Editor run hot-reloads", files.Count());
        for (auto& file : files)
        {
            FileSystem::DeleteFile(file);
        }
    }

    bool forceRecompile = false;

    // Check last Editor version that was using a project is different from current
    if (Editor::IsOldProjectOpened)
        forceRecompile = true;

    // Check if need to force recompile game scripts
    if (forceRecompile)
    {
        LOG(Warning, "Forcing scripts recompilation");
        FileSystem::DeleteDirectory(Globals::ProjectCacheFolder / TEXT("Intermediate"));
        ScriptsBuilder::Compile();
    }

    return false;
}

void ScriptsBuilderService::Update()
{
    // Send compilation events
    {
        ScopeLock scopeLock(_compileEventsLocker);

        for (int32 i = 0; i < _compileEvents.Count(); i++)
            CallCompileEvent(_compileEvents[i]);
        _compileEvents.Clear();
    }

    // Check if compile code (if has been edited)
    const TimeSpan timeToCallCompileIfDirty = TimeSpan::FromMilliseconds(150);
    auto mainWindow = Engine::MainWindow;
    if (ScriptsBuilder::IsSourceDirtyFor(timeToCallCompileIfDirty) && mainWindow && mainWindow->IsFocused())
    {
        // Check if auto reload is enabled
        if (Editor::Managed->CanAutoReloadScripts())
            ScriptsBuilder::Compile();
    }

    ScopeLock scopeLock(_locker);

    // Check if compilation has been requested (some time ago since we want to batch calls to reduce amount of compilations)
    if (_isCompileRequested)
    {
        // Check if compilation isn't running
        if (!_isCompileRunning)
        {
            // Check if editor state can perform scripts reloading
            if (Editor::Managed->CanReloadScripts())
            {
                // Call compilation (and switch flags)
                _isCompileRequested = false;
                _isCompileRunning = true;
                Function<bool()> action(compileGameScriptsAsync);
                Task::StartNew(action);
            }
        }
    }
}

void ScriptsBuilderService::Dispose()
{
    // Don't exit while scripts compilation is still running
    if (ScriptsBuilder::IsCompiling())
    {
        LOG(Warning, "Scripts compilation is running, waiting for the end...");
        int32 timeOutMilliseconds = 5000;
        while (ScriptsBuilder::IsCompiling() && timeOutMilliseconds > 0)
        {
            const int sleepTimeMilliseconds = 50;
            timeOutMilliseconds -= sleepTimeMilliseconds;
            Platform::Sleep(sleepTimeMilliseconds);
        }
        LOG(Warning, "Scripts compilation wait ended");
    }

    SourceFoldersWatchers.ClearDelete();
}
