// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine.h"
#include "Game.h"
#include "Time.h"
#include "CommandLine.h"
#include "Globals.h"
#include "EngineService.h"
#include "Application.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Core/Core.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/MainThreadTask.h"
#include "Engine/Threading/ThreadRegistry.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Graphics/RenderTargetPool.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Threading/TaskGraph.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Managed/ManagedEditor.h"
#else
#include "Engine/Utilities/Encryption.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Core/Config/PlatformSettings.h"
#endif

namespace EngineImpl
{
    bool IsReady = false;
#if !USE_EDITOR
    bool RunInBackground = false;
#endif
    String CommandLine = nullptr;
    int32 Fps = 0, FpsAccumulatedFrames = 0;
    double FpsAccumulated = 0.0;

    void InitLog();
    void InitPaths();
    void InitMainWindow();
}

DateTime Engine::StartupTime;
bool Engine::HasFocus = false;
uint64 Engine::UpdateCount = 0;
uint64 Engine::FrameCount = 0;
Action Engine::FixedUpdate;
Action Engine::Update;
TaskGraph* Engine::UpdateGraph = nullptr;
Action Engine::LateUpdate;
Action Engine::LateFixedUpdate;
Action Engine::Draw;
Action Engine::Pause;
Action Engine::Unpause;
Action Engine::RequestingExit;
Delegate<StringView, void*> Engine::ReportCrash;
FatalErrorType Engine::FatalError = FatalErrorType::None;
bool Engine::IsRequestingExit = false;
int32 Engine::ExitCode = 0;
Window* Engine::MainWindow = nullptr;

int32 Engine::Main(const Char* cmdLine)
{
    EngineImpl::CommandLine = cmdLine;
    Globals::MainThreadID = Platform::GetCurrentThreadID();
    StartupTime = DateTime::Now();

    EngineService::Sort();

    if (CommandLine::Parse(cmdLine))
    {
        Platform::Fatal(TEXT("Invalid command line."));
        return -1;
    }

#if FLAX_TESTS
    // Configure engine for test running environment
    CommandLine::Options.Headless = true;
    CommandLine::Options.Null = true;
    CommandLine::Options.Mute = true;
    CommandLine::Options.Std = true;
#endif

    if (Platform::Init())
    {
        Platform::Fatal(TEXT("Cannot init platform."));
        return -1;
    }

    Platform::SetHighDpiAwarenessEnabled(!CommandLine::Options.LowDPI.IsTrue());
    Time::StartupTime = DateTime::Now();
    Globals::StartupFolder = Globals::BinariesFolder = Platform::GetMainDirectory();
#if USE_EDITOR
    Globals::StartupFolder /= TEXT("../../../..");
#if PLATFORM_MAC
    if (Globals::BinariesFolder.EndsWith(TEXT(".app/Contents")))
    {
        // If running editor from application package on macOS
        Globals::StartupFolder = Globals::BinariesFolder;
        Globals::BinariesFolder /= TEXT("MacOS");
    }
#endif
#endif
    StringUtils::PathRemoveRelativeParts(Globals::StartupFolder);
    FileSystem::NormalizePath(Globals::BinariesFolder);

    FileSystem::GetSpecialFolderPath(SpecialFolder::Temporary, Globals::TemporaryFolder);
    if (Globals::TemporaryFolder.IsEmpty())
        Platform::Fatal(TEXT("Failed to gather temporary folder directory."));
    Globals::TemporaryFolder /= Guid::New().ToString(Guid::FormatType::D);

    // Load game info or project info
    {
        const int32 result = Application::LoadProduct();
        if (result != 0)
            return result;
    }

    EngineImpl::InitPaths();
    EngineImpl::InitLog();

#if USE_EDITOR
    if (Editor::CheckProjectUpgrade())
    {
        // End
        LOG(Warning, "Loading project cancelled. Closing...");
        Log::Logger::Dispose();
        return 0;
    }
#endif

    // Initialize engine
    UpdateGraph = New<TaskGraph>();
    EngineService::OnInit();
    if (Application::Init())
        return -10;

    // Become ready before run
    Platform::BeforeRun();
    EngineImpl::InitMainWindow();
    Application::BeforeRun();
#if !USE_EDITOR && (PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_MAC)
    EngineImpl::RunInBackground = PlatformSettings::Get()->RunInBackground;
#endif
    Log::Logger::WriteFloor();
    LOG_FLUSH();
    Time::Synchronize();
    EngineImpl::IsReady = true;

    // Main engine loop
    const bool useSleep = true; // TODO: this should probably be a platform setting
    while (!ShouldExit())
    {
        // Reduce CPU usage by introducing idle time if the engine is running very fast and has enough time to spend
        if ((useSleep && Time::UpdateFPS > ZeroTolerance) || !Platform::GetHasFocus())
        {
            double nextTick = Time::GetNextTick();
            double timeToTick = nextTick - Platform::GetTimeSeconds();

            // Sleep less than needed, some platforms may sleep slightly more than requested
            if (timeToTick > 0.002)
            {
                PROFILE_CPU_NAMED("Idle");
                Platform::Sleep(1);
            }
        }

        // App paused logic
        if (Platform::GetIsPaused())
        {
            OnPause();
            do
            {
                Platform::Sleep(10);
                Platform::Tick();
            } while (Platform::GetIsPaused() && !ShouldExit());
            if (ShouldExit())
                break;
            OnUnpause();
        }

        // Use the same time for all ticks to improve synchronization
        const double time = Platform::GetTimeSeconds();

        // Update application (will gather data and other platform related events)
        {
            PROFILE_CPU_NAMED("Platform.Tick");
            Platform::Tick();
        }

        // Update game logic
        if (Time::OnBeginUpdate(time))
        {
            OnUpdate();
            OnLateUpdate();
            Time::OnEndUpdate();
        }

        // Start physics simulation
        if (Time::OnBeginPhysics(time))
        {
            OnFixedUpdate();
            OnLateFixedUpdate();
            Time::OnEndPhysics();
        }

        // Draw frame
        if (Time::OnBeginDraw(time))
        {
            OnDraw();
            Time::OnEndDraw();
            FrameMark;
        }
    }

    // Call on exit event
    OnExit();

    // Delete temporary directory only if Engine is closing normally (after crash user/developer can restore some data)
    if (FileSystem::DirectoryExists(Globals::TemporaryFolder))
    {
        FileSystem::DeleteDirectory(Globals::TemporaryFolder);
    }

    return ExitCode;
}

void Engine::Exit(int32 exitCode, FatalErrorType error)
{
    ASSERT(IsInMainThread());
    FatalError = error;

    // Call on exit event
    OnExit();

    // Exit application
    exit(exitCode);
}

void Engine::RequestExit(int32 exitCode, FatalErrorType error)
{
    if (IsRequestingExit)
        return;
#if USE_EDITOR
    // Send to editor (will leave play mode if need to)
    if (!Editor::Managed->OnAppExit())
        return;
#endif
    IsRequestingExit = true;
    ExitCode = exitCode;
    PRAGMA_DISABLE_DEPRECATION_WARNINGS;
    Globals::IsRequestingExit = true;
    Globals::ExitCode = exitCode;
    PRAGMA_ENABLE_DEPRECATION_WARNINGS;
    FatalError = error;
    RequestingExit();
}

#if !BUILD_SHIPPING

void Engine::Crash(FatalErrorType error)
{
    switch (error)
    {
    case FatalErrorType::None:
    case FatalErrorType::Exception:
        *((int32*)3) = 11;
        break;
    default:
        Platform::Fatal(TEXT("Crash Test"), nullptr, error);
        break;
    }
}

#endif

void Engine::OnFixedUpdate()
{
    PROFILE_CPU_NAMED("Fixed Update");

    Physics::FlushRequests();

    // Call event
    FixedUpdate();

    // Update services
    EngineService::OnFixedUpdate();

    if (!Time::GetGamePaused())
    {
        const float dt = Time::Physics.DeltaTime.GetTotalSeconds();
        Physics::Simulate(dt);

        // After this point we should not modify physic objects state (rendering operations is mostly readonly)
        // That's because auto-simulation mode is performing rendering during physics simulation
    }
}

void Engine::OnLateFixedUpdate()
{
    PROFILE_CPU_NAMED("Late Fixed Update");

    // Collect physics simulation results (does nothing if Simulate hasn't been called in the previous loop step)
    Physics::CollectResults();

    // Call event
    LateFixedUpdate();

    // Update services
    EngineService::OnLateFixedUpdate();
}

void Engine::OnUpdate()
{
    PROFILE_CPU_NAMED("Update");

    UpdateCount++;

    const auto mainWindow = MainWindow;

#if !USE_EDITOR
    // Pause game if window lost focus and cannot run in a background
    bool isGameRunning = true;
    if (mainWindow && !mainWindow->IsFocused())
    {
        isGameRunning = EngineImpl::RunInBackground;
    }
    Time::SetGamePaused(!isGameRunning);
#endif

    // Determine if application has focus (flag used by the other parts of the engine)
    HasFocus = (mainWindow && mainWindow->IsFocused()) || Platform::GetHasFocus();

    // Simulate lags
    //Platform::Sleep(100);

    MainThreadTask::RunAll(Time::Update.UnscaledDeltaTime.GetTotalSeconds());

    // Call event
    Update();

    // Update services
    EngineService::OnUpdate();

    // Run async
    UpdateGraph->Execute();
}

void Engine::OnLateUpdate()
{
    PROFILE_CPU_NAMED("Late Update");

    // Call event
    LateUpdate();

    // Update services
    EngineService::OnLateUpdate();
}

void Engine::OnDraw()
{
    PROFILE_CPU_NAMED("Draw");

    // Begin frame rendering
    FrameCount++;
    const double time = Platform::GetTimeSeconds();
    auto device = GPUDevice::Instance;
    device->Locker.Lock();
#if COMPILE_WITH_PROFILER
    ProfilerGPU::BeginFrame();
#endif

    device->Draw();

    // End frame rendering
#if COMPILE_WITH_PROFILER
    ProfilerGPU::EndFrame();
#endif
    device->Locker.Unlock();

    // Calculate FPS
    EngineImpl::FpsAccumulatedFrames++;
    if (time - EngineImpl::FpsAccumulated >= 1.0)
    {
        EngineImpl::Fps = EngineImpl::FpsAccumulatedFrames;
        EngineImpl::FpsAccumulatedFrames = 0;
        EngineImpl::FpsAccumulated = time;
    }

#if !LOG_ENABLE_AUTO_FLUSH
    // Flush log file every fourth frame
    if (FrameCount % 4 == 0)
    {
        LOG_FLUSH();
    }
#endif
}

bool Engine::IsHeadless()
{
#if PLATFORM_HAS_HEADLESS_MODE
    return CommandLine::Options.Headless.IsTrue();
#else
    return false;
#endif
}

bool Engine::IsReady()
{
    return EngineImpl::IsReady;
}

bool Engine::ShouldExit()
{
    return IsRequestingExit;
}

bool Engine::IsEditor()
{
#if USE_EDITOR
    return true;
#else
    return false;
#endif
}

int32 Engine::GetFramesPerSecond()
{
    return EngineImpl::Fps;
}

const String& Engine::GetCommandLine()
{
    return EngineImpl::CommandLine;
}

JsonAsset* Engine::GetCustomSettings(const StringView& key)
{
    const auto settings = GameSettings::Get();
    if (!settings)
        return nullptr;
    Guid assetId = Guid::Empty;
    settings->CustomSettings.TryGet(key, assetId);
    return Content::LoadAsync<JsonAsset>(assetId);
}

void Engine::FocusGameViewport()
{
#if USE_EDITOR
    Editor::Managed->FocusGameViewport();
#else
    if (MainWindow)
    {
        MainWindow->BringToFront();
        MainWindow->Focus();
    }
#endif
}

bool Engine::HasGameViewportFocus()
{
#if USE_EDITOR
    return Editor::Managed->HasGameViewportFocus();
#else
    return HasFocus;
#endif
}

void Engine::OnPause()
{
    LOG(Info, "App paused");
    Pause();

    RenderTargetPool::Flush(true);
}

void Engine::OnUnpause()
{
    LOG(Info, "App unpaused");
    Unpause();

    Time::Synchronize();
}

void Engine::OnExit()
{
    LOG_FLUSH();

    // Start disposing process
    EngineImpl::IsReady = false;

    // Collect physics simulation results because we cannot exit with physics running
    Physics::CollectResults();

    // Before
    Application::BeforeExit();
    EngineService::OnBeforeExit();
    Platform::BeforeExit();

    LOG_FLUSH();

    // Unload Engine services
    EngineService::OnDispose();
    SAFE_DELETE(UpdateGraph);

    LOG_FLUSH();

    // Kill all remaining threads
    ThreadRegistry::KillEmAll();

    // Cleanup
    ObjectsRemovalService::ForceFlush();
#if COMPILE_WITH_PROFILER
    ProfilerCPU::Dispose();
    ProfilerGPU::Dispose();
#endif

    // Close logging service
    Log::Logger::Dispose();

    Platform::Exit();
}

void EngineImpl::InitLog()
{
    // Initialize logger
    Log::Logger::Init();

    // Log build info
    LOG(Info, "Flax Engine");
#if BUILD_DEBUG
    const Char* mode = TEXT("Debug");
#elif BUILD_DEVELOPMENT
    const Char* mode = TEXT("Development");
#elif BUILD_RELEASE
    const Char* mode = TEXT("Release");
#endif
    LOG(Info, "Platform: {0} {1} ({2})", ToString(PLATFORM_TYPE), ToString(PLATFORM_ARCH), mode);
#if COMPILE_WITH_DEV_ENV
    LOG(Info, "Compiled for Dev Environment");
#endif
    LOG(Info, "Version " FLAXENGINE_VERSION_TEXT);
    const Char* cpp = TEXT("?");
    if (__cplusplus == 202101L) cpp = TEXT("C++23");
    else if (__cplusplus == 202002L) cpp = TEXT("C++20");
    else if (__cplusplus == 201703L) cpp = TEXT("C++17");
    else if (__cplusplus == 201402L) cpp = TEXT("C++14");
    else if (__cplusplus == 201103L) cpp = TEXT("C++11");
    LOG(Info, "Compiled: {0} {1} {2}", TEXT(__DATE__), TEXT(__TIME__), cpp);
#ifdef _MSC_VER
    const String mcsVer = StringUtils::ToString(_MSC_FULL_VER);
    LOG(Info, "Compiled with Visual C++ {0}.{1}.{2}.{3:0^2d}", mcsVer.Substring(0, 2), mcsVer.Substring(2, 2), mcsVer.Substring(4, 5), _MSC_BUILD);
#elif defined(__clang__)
    LOG(Info, "Compiled with Clang {0}.{1}.{2}", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
    LOG(Info, "Compiled with GCC {0}.{1}.{2}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(__INTEL_COMPILER)
    LOG(Info, "Compiled with Intel Compiler {0} {1}", __INTEL_COMPILER, __INTEL_COMPILER_BUILD_DATE);
#else
    LOG(Info, "Compiled with unrecognized C++ compiler");
#endif

    // Log product info
    LOG(Info, "Product: {0}, Company: {1}", Globals::ProductName, Globals::CompanyName);
    LOG(Info, "Current culture: {0}", Platform::GetUserLocaleName());
    LOG(Info, "Command line: {0}", CommandLine);
    LOG(Info, "Base folder: {0}", Globals::StartupFolder);
    LOG(Info, "Binaries folder: {0}", Globals::BinariesFolder);
    LOG(Info, "Temporary folder: {0}", Globals::TemporaryFolder);
    LOG(Info, "Project folder: {0}", Globals::ProjectFolder);
#if USE_EDITOR
    LOG(Info, "Project name: {0}", Editor::Project->Name);
#endif

    // Log platform into
    Platform::LogInfo();

    LOG_FLUSH();
}

void EngineImpl::InitPaths()
{
    // Cache other global paths
    FileSystem::GetSpecialFolderPath(SpecialFolder::LocalAppData, Globals::ProductLocalFolder);
    if (Globals::ProductLocalFolder.IsEmpty())
        Platform::Fatal(TEXT("Failed to gather local app data folder directory."));
#if !PLATFORM_UWP
    Globals::ProductLocalFolder /= Globals::CompanyName / Globals::ProductName;
#endif
#if USE_EDITOR
    Globals::EngineContentFolder = Globals::StartupFolder / TEXT("Content");
#if USE_MONO
#if PLATFORM_WINDOWS
    Globals::MonoPath = Globals::StartupFolder / TEXT("Source/Platforms/Editor/Windows/Mono");
#elif PLATFORM_LINUX
    Globals::MonoPath = Globals::StartupFolder / TEXT("Source/Platforms/Editor/Linux/Mono");
#elif PLATFORM_MAC
    Globals::MonoPath = Globals::StartupFolder / TEXT("Source/Platforms/Editor/Mac/Mono");
#else
    #error "Please specify the Mono data location for Editor on this platform."
#endif
#endif
#else
#if USE_MONO
    Globals::MonoPath = Globals::StartupFolder / TEXT("Mono");
#endif
#endif
    Globals::ProjectContentFolder = Globals::ProjectFolder / TEXT("Content");
#if USE_EDITOR
    Globals::ProjectSourceFolder = Globals::ProjectFolder / TEXT("Source");
    Globals::ProjectCacheFolder = Globals::ProjectFolder / TEXT("Cache");
#endif

#if USE_MONO
    // We must ensure that engine is located in folder which path contains only ANSI characters
    // Why? Mono lib must have etc and lib folders at ANSI path
    // But project can be located on Unicode path
    if (!Globals::StartupFolder.IsANSI())
        Platform::Fatal(TEXT("Cannot start application in directory which name contains non-ANSI characters."));
#endif

#if !PLATFORM_SWITCH && !FLAX_TESTS
    // Setup directories
    if (FileSystem::DirectoryExists(Globals::TemporaryFolder))
        FileSystem::DeleteDirectory(Globals::TemporaryFolder);
    if (FileSystem::CreateDirectory(Globals::TemporaryFolder))
    {
        // Try one more time (Explorer may block it)
        Platform::Sleep(10);
        if (FileSystem::CreateDirectory(Globals::TemporaryFolder))
            Platform::Fatal(TEXT("Cannot create temporary directory."));
    }
#endif
#if USE_EDITOR
    if (!FileSystem::DirectoryExists(Globals::ProjectContentFolder))
        FileSystem::CreateDirectory(Globals::ProjectContentFolder);
    if (!FileSystem::DirectoryExists(Globals::ProjectSourceFolder))
        FileSystem::CreateDirectory(Globals::ProjectSourceFolder);
    if (CommandLine::Options.ClearCache.IsTrue())
        FileSystem::DeleteDirectory(Globals::ProjectCacheFolder, true);
    else if (CommandLine::Options.ClearCookerCache.IsTrue())
        FileSystem::DeleteDirectory(Globals::ProjectCacheFolder / TEXT("Cooker"), true);
    if (!FileSystem::DirectoryExists(Globals::ProjectCacheFolder))
        FileSystem::CreateDirectory(Globals::ProjectCacheFolder);
#endif

    // Setup current working directory to the project root
    Platform::SetWorkingDirectory(Globals::ProjectFolder);
}

void EngineImpl::InitMainWindow()
{
#if PLATFORM_HAS_HEADLESS_MODE
    // Try to use headless mode
    if (CommandLine::Options.Headless.IsTrue())
    {
        LOG(Info, "Running in headless mode.");
        return;
    }
#endif
    PROFILE_CPU_NAMED("Engine::InitMainWindow");

    // Create window
    Engine::MainWindow = Application::CreateMainWindow();
    if (!Engine::MainWindow)
    {
        LOG(Warning, "No main window created.");
        return;
    }

    // Init window rendering output resources
    if (Engine::MainWindow->InitSwapChain())
    {
        Platform::Fatal(TEXT("Cannot init rendering output for a window."));
        return;
    }

#if !USE_EDITOR && !COMPILE_WITHOUT_CSHARP
    // Inform the managed runtime about the window (game can link GUI to it)
    auto scriptingClass = Scripting::GetStaticClass();
    ASSERT(scriptingClass);
    auto setWindowMethod = scriptingClass->GetMethod("SetWindow", 1);
    ASSERT(setWindowMethod);
    void* params[1];
    params[0] = Engine::MainWindow->GetOrCreateManagedInstance();
    MObject* exception = nullptr;
    setWindowMethod->Invoke(nullptr, params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Fatal, TEXT("FlaxEngine.Scripting.SetWindow"));
    }
#endif
}
