// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GameCooker.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Scripting/MainThreadManagedInvokeAction.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/ThreadSpawner.h"
#include "Engine/Platform/FileSystem.h"
#include "Steps/ValidateStep.h"
#include "Steps/CompileScriptsStep.h"
#include "Steps/PrecompileAssembliesStep.h"
#include "Steps/DeployDataStep.h"
#include "Steps/CollectAssetsStep.h"
#include "Steps/CookAssetsStep.h"
#include "Steps/PostProcessStep.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Content/AssetReference.h"
#if PLATFORM_TOOLS_WINDOWS
#include "Platform/Windows/WindowsPlatformTools.h"
#include "Engine/Platform/Windows/WindowsPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_UWP || PLATFORM_TOOLS_XBOX_ONE
#include "Platform/UWP/UWPPlatformTools.h"
#include "Engine/Platform/UWP/UWPPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_LINUX
#include "Platform/Linux/LinuxPlatformTools.h"
#include "Engine/Platform/Linux/LinuxPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_PS4
#include "Platforms/PS4/Editor/PlatformTools/PS4PlatformTools.h"
#include "Platforms/PS4/Engine/Platform/PS4PlatformSettings.h"
#endif
#if PLATFORM_TOOLS_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Editor/PlatformTools/XboxScarlettPlatformTools.h"
#endif
#if PLATFORM_TOOLS_ANDROID
#include "Platform/Android/AndroidPlatformTools.h"
#endif

namespace GameCookerImpl
{
    MMethod* Internal_OnEvent = nullptr;
    MMethod* Internal_OnProgress = nullptr;
    MMethod* Internal_CanDeployPlugin = nullptr;

    bool IsRunning = false;
    bool IsThreadRunning = false;
    int64 CancelFlag = 0;
    int64 CancelThreadFlag = 0;
    ConditionVariable ThreadCond;

    CriticalSection ProgressLocker;
    String ProgressMsg;
    float ProgressValue;

    CookingData Data;
    Array<GameCooker::BuildStep*> Steps;
    Dictionary<BuildPlatform, PlatformTools*> Tools;

    BuildPlatform PluginDeployPlatform;
    MAssembly* PluginDeployAssembly;
    bool PluginDeployResult;

    void CallEvent(GameCooker::EventType type);
    void ReportProgress(const String& info, float totalProgress);
    bool Build();
    int32 ThreadFunction();

    void OnEditorAssemblyUnloading(MAssembly* assembly)
    {
        Internal_OnEvent = nullptr;
        Internal_OnProgress = nullptr;
    }
}

using namespace GameCookerImpl;

Delegate<GameCooker::EventType> GameCooker::OnEvent;
Delegate<const String&, float> GameCooker::OnProgress;

bool CookingData::AssetTypeStatistics::operator<(const AssetTypeStatistics& other) const
{
    if (ContentSize != other.ContentSize)
        return ContentSize > other.ContentSize;
    return Count > other.Count;
}

CookingData::Statistics::Statistics()
{
    TotalAssets = 0;
    CookedAssets = 0;
    ContentSizeMB = 0;
}

String CookingData::GetGameBinariesPath() const
{
    const Char* archDir;
    switch (Tools->GetArchitecture())
    {
    case ArchitectureType::AnyCPU:
        archDir = TEXT("AnyCPU");
        break;
    case ArchitectureType::x86:
        archDir = TEXT("x86");
        break;
    case ArchitectureType::x64:
        archDir = TEXT("x64");
        break;
    case ArchitectureType::ARM:
        archDir = TEXT("ARM");
        break;
    case ArchitectureType::ARM64:
        archDir = TEXT("ARM64");
        break;
    default:
    CRASH;
        return String::Empty;
    }

    return GetPlatformBinariesRoot() / TEXT("Game") / archDir / ToString(Configuration);
}

String CookingData::GetPlatformBinariesRoot() const
{
    return Globals::StartupFolder / TEXT("Source/Platforms") / Tools->GetName() / TEXT("Binaries");
}

void CookingData::StepProgress(const String& info, const float stepProgress) const
{
    const float singleStepProgress = 1.0f / (StepsCount + 1);
    const float totalProgress = (CurrentStepIndex + stepProgress) * singleStepProgress;
    ReportProgress(info, totalProgress);
}

void CookingData::AddRootAsset(const Guid& id)
{
    RootAssets.Add(id);
}

void CookingData::AddRootAsset(const String& path)
{
    AssetInfo info;
    if (Content::GetAssetInfo(path, info))
    {
        RootAssets.Add(info.ID);
    }
}

void CookingData::AddRootEngineAsset(const String& internalPath)
{
    const String path = Globals::EngineContentFolder / internalPath + ASSET_FILES_EXTENSION_WITH_DOT;
    AssetInfo info;
    if (Content::GetAssetInfo(path, info))
    {
        RootAssets.Add(info.ID);
    }
}

void CookingData::Error(const String& msg)
{
    LOG_STR(Error, msg);
}

class GameCookerService : public EngineService
{
public:

    GameCookerService()
        : EngineService(TEXT("Game Cooker"))
    {
    }

    bool Init() override;
    void Update() override;
    void Dispose() override;
};

GameCookerService GameCookerServiceInstance;

const CookingData& GameCooker::GetCurrentData()
{
    return Data;
}

bool GameCooker::IsRunning()
{
    return GameCookerImpl::IsRunning;
}

bool GameCooker::IsCancelRequested()
{
    return Platform::AtomicRead(&CancelFlag) != 0;
}

PlatformTools* GameCooker::GetTools(BuildPlatform platform)
{
    PlatformTools* result = nullptr;
    if (!Tools.TryGet(platform, result))
    {
        switch (platform)
        {
#if PLATFORM_TOOLS_WINDOWS
        case BuildPlatform::Windows32:
            result = New<WindowsPlatformTools>(ArchitectureType::x86);
            break;
        case BuildPlatform::Windows64:
            result = New<WindowsPlatformTools>(ArchitectureType::x64);
            break;
#endif
#if PLATFORM_TOOLS_UWP
        case BuildPlatform::UWPx86:
            result = New<WSAPlatformTools>(ArchitectureType::x86);
            break;
        case BuildPlatform::UWPx64:
            result = New<WSAPlatformTools>(ArchitectureType::x64);
            break;
#endif
#if PLATFORM_TOOLS_XBOX_ONE
        case BuildPlatform::XboxOne:
            result = New<XboxOnePlatformTools>();
            break;
#endif
#if PLATFORM_TOOLS_LINUX
        case BuildPlatform::LinuxX64:
            result = New<LinuxPlatformTools>();
            break;
#endif
#if PLATFORM_TOOLS_PS4
        case BuildPlatform::PS4:
            result = New<PS4PlatformTools>();
            break;
#endif
#if PLATFORM_TOOLS_XBOX_SCARLETT
        case BuildPlatform::XboxScarlett:
            result = New<XboxScarlettPlatformTools>();
            break;
#endif
#if PLATFORM_TOOLS_ANDROID
        case BuildPlatform::AndroidARM64:
            result = New<AndroidPlatformTools>(ArchitectureType::ARM64);
            break;
#endif
        }
        Tools.Add(platform, result);
    }
    return result;
}

void GameCooker::Build(BuildPlatform platform, BuildConfiguration configuration, const StringView& outputPath, BuildOptions options, const Array<String>& customDefines)
{
    if (IsRunning())
    {
        LOG(Warning, "Cannot start a build. Already running.");
        return;
    }
    PlatformTools* tools = GetTools(platform);
    if (tools == nullptr)
    {
        LOG(Error, "Build platform {0} is not supported.", ::ToString(platform));
        return;
    }

    // Setup
    CancelFlag = 0;
    ProgressMsg.Clear();
    ProgressValue = 1.0f;
    CookingData& data = Data;
    data.Init();
    data.Tools = tools;
    data.Platform = platform;
    data.Configuration = configuration;
    data.Options = options;
    data.CustomDefines = customDefines;
    data.OutputPath = outputPath;
    FileSystem::NormalizePath(data.OutputPath);
    data.OutputPath = data.OriginalOutputPath = FileSystem::ConvertRelativePathToAbsolute(Globals::ProjectFolder, data.OutputPath);
    data.CacheDirectory = Globals::ProjectCacheFolder / TEXT("Cooker") / tools->GetName();
    if (!FileSystem::DirectoryExists(data.CacheDirectory))
    {
        if (FileSystem::CreateDirectory(data.CacheDirectory))
        {
            LOG(Error, "Cannot setup game building cache directory.");
            return;
        }
    }

    // Start
    GameCookerImpl::IsRunning = true;

    // Start thread if need to
    if (!IsThreadRunning)
    {
        Function<int32()> f;
        f.Bind(ThreadFunction);
        const auto thread = ThreadSpawner::Start(f, GameCookerServiceInstance.Name, ThreadPriority::Highest);
        if (thread == nullptr)
        {
            GameCookerImpl::IsRunning = false;
            LOG(Error, "Failed to start a build thread.");
            return;
        }
    }
    else
    {
        ThreadCond.NotifyOne();
    }
}

void GameCooker::Cancel(bool waitForEnd)
{
    if (!IsRunning())
        return;

    // Set flag
    Platform::InterlockedIncrement(&CancelFlag);

    if (waitForEnd)
    {
        LOG(Warning, "Waiting for the Game Cooker end...");

        // Wait for the end
        while (GameCookerImpl::IsRunning)
        {
            Platform::Sleep(10);
        }
    }
}

void GameCookerImpl::CallEvent(GameCooker::EventType type)
{
    if (Internal_OnEvent == nullptr)
    {
        auto c = GameCooker::GetStaticClass();
        if (c)
            Internal_OnEvent = c->GetMethod("Internal_OnEvent", 1);
        ASSERT(GameCookerImpl::Internal_OnEvent);
    }

    MainThreadManagedInvokeAction::ParamsBuilder params;
    params.AddParam((int32)type);
    MainThreadManagedInvokeAction::Invoke(Internal_OnEvent, params);

    GameCooker::OnEvent(type);
}

void GameCookerImpl::ReportProgress(const String& info, float totalProgress)
{
    ScopeLock lock(ProgressLocker);

    ProgressMsg = info;
    ProgressValue = totalProgress;
}

bool GameCookerImpl::Build()
{
    CookingData& data = Data;
    LOG(Info, "Starting Game Cooker...");
    LOG(Info, "Platform: {0}, Configuration: {2}, Options: {1}", ::ToString(data.Platform), (int32)data.Options, ::ToString(data.Configuration));
    LOG(Info, "Output Path: {0}", data.OutputPath);

    // Late init feature
    if (Steps.IsEmpty())
    {
        // Create steps
        Steps.Add(New<ValidateStep>());
        Steps.Add(New<CompileScriptsStep>());
        Steps.Add(New<DeployDataStep>());
        Steps.Add(New<PrecompileAssembliesStep>());
        Steps.Add(New<CollectAssetsStep>());
        Steps.Add(New<CookAssetsStep>());
        Steps.Add(New<PostProcessStep>());
    }

    MCore::Instance()->AttachThread();

    // Build Started
    CallEvent(GameCooker::EventType::BuildStarted);
    for (int32 stepIndex = 0; stepIndex < Steps.Count(); stepIndex++)
        Steps[stepIndex]->OnBuildStarted(data);
    Data.Tools->OnBuildStarted(data);
    data.InitProgress(Steps.Count());

    // Execute all steps in a sequence
    bool failed = false;
    for (int32 stepIndex = 0; stepIndex < Steps.Count(); stepIndex++)
    {
        if (GameCooker::IsCancelRequested())
            break;
        auto step = Steps[stepIndex];
        data.NextStep();

        // Execute step
        failed = step->Perform(data);
        if (failed)
            break;
    }

    // Process result
    if (GameCooker::IsCancelRequested())
    {
        LOG(Warning, "Game building cancelled!");
        failed = true;
    }
    else if (failed)
    {
        LOG(Error, "Game building failed!");
    }
    else
    {
        LOG(Info, "Game building done!");

        if (data.Options & BuildOptions::ShowOutput)
        {
            FileSystem::ShowFileExplorer(data.OriginalOutputPath);
        }
    }
    IsRunning = false;
    CancelFlag = 0;
    Data.Tools->OnBuildEnded(data, failed);
    for (int32 stepIndex = 0; stepIndex < Steps.Count(); stepIndex++)
        Steps[stepIndex]->OnBuildEnded(data, failed);
    CallEvent(failed ? GameCooker::EventType::BuildFailed : GameCooker::EventType::BuildDone);

    return failed;
}

int32 GameCookerImpl::ThreadFunction()
{
    IsThreadRunning = true;

    CriticalSection mutex;
    while (Platform::AtomicRead(&CancelThreadFlag) == 0)
    {
        if (IsRunning)
        {
            Build();
        }

        ThreadCond.Wait(mutex);
    }

    IsThreadRunning = false;
    return 0;
}

bool GameCookerService::Init()
{
    auto editorAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    editorAssembly->Unloading.Bind(OnEditorAssemblyUnloading);

    return false;
}

void GameCookerService::Update()
{
    if (IsRunning)
    {
        ScopeLock lock(ProgressLocker);

        if (ProgressMsg.HasChars())
        {
            if (Internal_OnProgress == nullptr)
            {
                auto c = GameCooker::GetStaticClass();
                if (c)
                    Internal_OnProgress = c->GetMethod("Internal_OnProgress", 2);
                ASSERT(GameCookerImpl::Internal_OnProgress);
            }

            MainThreadManagedInvokeAction::ParamsBuilder params;
            params.AddParam(ProgressMsg, Scripting::GetScriptsDomain()->GetNative());
            params.AddParam(ProgressValue);
            MainThreadManagedInvokeAction::Invoke(Internal_OnProgress, params);
            GameCooker::OnProgress(ProgressMsg, ProgressValue);

            ProgressMsg.Clear();
            ProgressValue = 1.0f;
        }
    }
}

void GameCookerService::Dispose()
{
    // Always stop on exit
    GameCooker::Cancel(true);

    // End thread
    if (IsThreadRunning)
    {
        LOG(Warning, "Waiting for the Game Cooker thread end...");

        Platform::AtomicStore(&CancelThreadFlag, 1);
        ThreadCond.NotifyOne();
        while (IsThreadRunning)
        {
            Platform::Sleep(1);
        }
    }

    // Cleanup
    Steps.ClearDelete();
    Tools.ClearDelete();
}
