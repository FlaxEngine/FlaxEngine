// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
#include "Engine/Core/Utilities.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Level/Level.h"
#include "Engine/Renderer/Renderer.h"
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
#include "Engine/Scripting/MException.h"
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
uint64 Engine::FrameCount = 0;
Action Engine::FixedUpdate;
Action Engine::Update;
Action Engine::LateUpdate;
Action Engine::Draw;
Action Engine::Pause;
Action Engine::Unpause;
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

    if (Platform::Init())
    {
        Platform::Fatal(TEXT("Cannot init platform."));
        return -1;
    }

    Platform::SetHighDpiAwarenessEnabled(!CommandLine::Options.LowDPI.IsTrue());
    Time::StartupTime = DateTime::Now();
#if COMPILE_WITH_PROFILER
    ProfilerCPU::Enabled = true;
#endif
    Globals::StartupFolder = Globals::BinariesFolder = Platform::GetMainDirectory();
#if USE_EDITOR
    Globals::StartupFolder /= TEXT("../../../..");
#endif
    StringUtils::PathRemoveRelativeParts(Globals::StartupFolder);
    FileSystem::NormalizePath(Globals::BinariesFolder);

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
    EngineService::OnInit();
    if (Application::Init())
        return -10;

    // Become ready before run
    Platform::BeforeRun();
    EngineImpl::InitMainWindow();
    Application::BeforeRun();
#if !USE_EDITOR && (PLATFORM_WINDOWS || PLATFORM_LINUX)
    EngineImpl::RunInBackground = PlatformSettings::Get()->RunInBackground;
#endif
    Log::Logger::WriteFloor();
    LOG_FLUSH();
    Time::OnBeforeRun();
    EngineImpl::IsReady = true;

    // Main engine loop
    bool canDraw = true; // Prevent drawing 2 or more frames in a row without update or fixed update (nothing will change)
    while (!ShouldExit())
    {
#if 0
        // TODO: test it more and maybe use in future to reduce CPU usage
		// Reduce CPU usage by introducing idle time if the engine is running very fast and has enough time to spend
		{
			float tickFps;
			auto tick = Time->GetHighestFrequency(tickFps);
			double tickTargetStepTime = 1.0 / tickFps;
			double nextTick = tick->LastEnd + tickTargetStepTime;
			double timeToTick = nextTick - Platform::GetTimeSeconds();
			int32 sleepTimeMs = Math::Min(4, Math::FloorToInt(timeToTick * (1000.0 * 0.8))); // Convert seconds to milliseconds and apply adjustment with limit
			if (!Device->WasVSyncUsed() && sleepTimeMs > 0)
			{
				PROFILE_CPU_NAMED("Idle");
				Platform::Sleep(sleepTimeMs);
			}
		}
#endif
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

        // Update game logic
        if (Time::OnBeginUpdate())
        {
            OnUpdate();
            OnLateUpdate();
            Time::OnEndUpdate();
            canDraw = true;
        }

        // Start physics simulation
        if (Time::OnBeginPhysics())
        {
            OnFixedUpdate();
            Time::OnEndPhysics();
            canDraw = true;
        }

        // Draw frame
        if (canDraw && Time::OnBeginDraw())
        {
            OnDraw();
            Time::OnEndDraw();
            canDraw = false;
        }

        // Collect physics simulation results (does nothing if Simulate hasn't been called in the previous loop step)
        Physics::CollectResults();
    }

    // Call on exit event
    OnExit();

    // Delete temporary directory only if Engine is closing normally (after crash user/developer can restore some data)
    if (FileSystem::DirectoryExists(Globals::TemporaryFolder))
    {
        FileSystem::DeleteDirectory(Globals::TemporaryFolder);
    }

    return Globals::ExitCode;
}

void Engine::Exit(int32 exitCode)
{
    ASSERT(IsInMainThread());

    // Call on exit event
    OnExit();

    // Exit application
    exit(exitCode);
}

void Engine::RequestExit(int32 exitCode)
{
#if USE_EDITOR
    // Send to editor (will leave play mode if need to)
    if (Editor::Managed->OnAppExit())
    {
        Globals::IsRequestingExit = true;
        Globals::ExitCode = exitCode;
    }
#else
    Globals::IsRequestingExit = true;
    Globals::ExitCode = exitCode;
#endif
}

void Engine::OnFixedUpdate()
{
    PROFILE_CPU_NAMED("Fixed Update");

    Physics::FlushRequests();

    // Call event
    FixedUpdate();

    // Update services
    EngineService::OnFixedUpdate();

    if (!Time::GetGamePaused() && Physics::AutoSimulation)
    {
        const float dt = Time::Physics.DeltaTime.GetTotalSeconds();
        Physics::Simulate(dt);

        // After this point we should not modify physic objects state (rendering operations is mostly readonly)
        // That's because auto-simulation mode is performing rendering during physics simulation
    }
}

void Engine::OnUpdate()
{
    PROFILE_CPU_NAMED("Update");

    // Update application (will gather data and other platform related events)
    {
        PROFILE_CPU_NAMED("Platform.Tick");
        Platform::Tick();
    }

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

    MainThreadTask::RunAll();

    // Call event
    Update();

    // Update services
    EngineService::OnUpdate();
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
    return Globals::IsRequestingExit;
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

bool Engine::HasGameViewportFocus()
{
#if USE_EDITOR
    return Editor::Managed->HasGameViewportFocus();
#else
    return HasFocus;
#endif
}

Vector2 Engine::ScreenToGameViewport(const Vector2& screenPos)
{
#if USE_EDITOR
    return Editor::Managed->ScreenToGameViewport(screenPos);
#else
    return MainWindow ? MainWindow->ScreenToClient(screenPos) : Vector2::Minimum;
#endif
}

Vector2 Engine::GameViewportToScreen(const Vector2& viewportPos)
{
#if USE_EDITOR
    return Editor::Managed->GameViewportToScreen(viewportPos);
#else
    return MainWindow ? MainWindow->ClientToScreen(viewportPos) : Vector2::Minimum;
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

    Time::OnBeforeRun();
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
    LOG(Info, "Compiled: {0} {1}", TEXT(__DATE__), TEXT(__TIME__));
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
    LOG(Info, "Base directory: {0}", Globals::StartupFolder);
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
    // Prepare temp folder path
    FileSystem::GetSpecialFolderPath(SpecialFolder::Temporary, Globals::TemporaryFolder);
    if (Globals::TemporaryFolder.IsEmpty())
        Platform::Fatal(TEXT("Failed to gather temporary folder directory."));
    Globals::TemporaryFolder /= Guid::New().ToString(Guid::FormatType::D);

    // Cache other global paths
    FileSystem::GetSpecialFolderPath(SpecialFolder::LocalAppData, Globals::ProductLocalFolder);
    if (Globals::ProductLocalFolder.IsEmpty())
        Platform::Fatal(TEXT("Failed to gather local app data folder directory."));
#if !PLATFORM_UWP
    Globals::ProductLocalFolder /= Globals::CompanyName / Globals::ProductName;
#endif
#if USE_EDITOR
    Globals::EngineContentFolder = Globals::StartupFolder / TEXT("Content");
#if PLATFORM_WINDOWS
    Globals::MonoPath = Globals::StartupFolder / TEXT("Source/Platforms/Editor/Windows/Mono");
#elif PLATFORM_LINUX
    Globals::MonoPath = Globals::StartupFolder / TEXT("Source/Platforms/Editor/Linux/Mono");
#else
    #error "Please specify the Mono data location for Editor on this platform."
#endif
#else
    Globals::MonoPath = Globals::StartupFolder / TEXT("Mono");
#endif
    Globals::ProjectContentFolder = Globals::ProjectFolder / TEXT("Content");
#if USE_EDITOR
    Globals::ProjectSourceFolder = Globals::ProjectFolder / TEXT("Source");
    Globals::ProjectCacheFolder = Globals::ProjectFolder / TEXT("Cache");
#endif

    // We must ensure that engine is located in folder which path contains only ANSI characters
    // Why? Mono lib must have etc and lib folders at ANSI path
    // But project can be located on Unicode path
    if (!Globals::StartupFolder.IsANSI())
        Platform::Fatal(TEXT("Cannot start application in directory which name contains non-ANSI characters."));

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
#if USE_EDITOR
    if (!FileSystem::DirectoryExists(Globals::ProjectContentFolder))
        FileSystem::CreateDirectory(Globals::ProjectContentFolder);
    if (!FileSystem::DirectoryExists(Globals::ProjectSourceFolder))
        FileSystem::CreateDirectory(Globals::ProjectSourceFolder);
    if (CommandLine::Options.ClearCache)
        FileSystem::DeleteDirectory(Globals::ProjectCacheFolder, true);
    else if (CommandLine::Options.ClearCookerCache)
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

#if !USE_EDITOR
    // Inform the managed runtime about the window (game can link GUI to it)
    auto scriptingClass = Scripting::GetStaticClass();
    ASSERT(scriptingClass);
    auto setWindowMethod = scriptingClass->GetMethod("SetWindow", 1);
    ASSERT(setWindowMethod);
    void* params[1];
    params[0] = Engine::MainWindow->GetOrCreateManagedInstance();
    MonoObject* exception = nullptr;
    setWindowMethod->Invoke(nullptr, params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Fatal, TEXT("FlaxEngine.ClassLibraryInitializer.SetWindow"));
    }
#endif
}
