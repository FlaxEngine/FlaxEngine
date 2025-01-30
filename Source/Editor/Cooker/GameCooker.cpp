// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GameCooker.h"
#include "PlatformTools.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Scripting/ManagedCLR/MTypes.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/Internal/MainThreadManagedInvokeAction.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Globals.h"
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
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Content/AssetReference.h"
#if PLATFORM_TOOLS_WINDOWS
#include "Platform/Windows/WindowsPlatformTools.h"
#include "Engine/Platform/Windows/WindowsPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_UWP
#include "Platform/UWP/UWPPlatformTools.h"
#include "Engine/Platform/UWP/UWPPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_LINUX
#include "Platform/Linux/LinuxPlatformTools.h"
#include "Engine/Platform/Linux/LinuxPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_PS4
#include "Platforms/PS4/Editor/PlatformTools/PS4PlatformTools.h"
#endif
#if PLATFORM_TOOLS_PS5
#include "Platforms/PS5/Editor/PlatformTools/PS5PlatformTools.h"
#endif
#if PLATFORM_TOOLS_XBOX_ONE
#include "Platforms/XboxOne/Editor/PlatformTools/XboxOnePlatformTools.h"
#endif
#if PLATFORM_TOOLS_XBOX_SCARLETT
#include "Platforms/XboxScarlett/Editor/PlatformTools/XboxScarlettPlatformTools.h"
#endif
#if PLATFORM_TOOLS_ANDROID
#include "Platform/Android/AndroidPlatformTools.h"
#endif
#if PLATFORM_TOOLS_SWITCH
#include "Platforms/Switch/Editor/PlatformTools/SwitchPlatformTools.h"
#endif
#if PLATFORM_TOOLS_MAC
#include "Platform/Mac/MacPlatformTools.h"
#include "Engine/Platform/Mac/MacPlatformSettings.h"
#endif
#if PLATFORM_TOOLS_IOS
#include "Platform/iOS/iOSPlatformTools.h"
#include "Engine/Platform/iOS/iOSPlatformSettings.h"
#endif

namespace GameCookerImpl
{
    MMethod* Internal_OnEvent = nullptr;
    MMethod* Internal_OnProgress = nullptr;
    MMethod* Internal_OnCollectAssets = nullptr;

    volatile bool IsRunning = false;
    volatile bool IsThreadRunning = false;
    int64 CancelFlag = 0;
    int64 CancelThreadFlag = 0;
    ConditionVariable ThreadCond;

    CriticalSection ProgressLocker;
    String ProgressMsg;
    float ProgressValue;

    CookingData* Data = nullptr;
    Array<GameCooker::BuildStep*> Steps;
    Dictionary<BuildPlatform, PlatformTools*> Tools;

    BuildPlatform PluginDeployPlatform;
    MAssembly* PluginDeployAssembly;
    bool PluginDeployResult;

    void CallEvent(GameCooker::EventType type);
    void ReportProgress(const String& info, float totalProgress);
    void OnCollectAssets(HashSet<Guid>& assets);
    bool Build();
    int32 ThreadFunction();

    void OnEditorAssemblyUnloading(MAssembly* assembly)
    {
        Internal_OnEvent = nullptr;
        Internal_OnProgress = nullptr;
        Internal_OnCollectAssets = nullptr;
    }
}

using namespace GameCookerImpl;

Delegate<GameCooker::EventType> GameCooker::OnEvent;
Delegate<const String&, float> GameCooker::OnProgress;
Action GameCooker::DeployFiles;
Action GameCooker::PostProcessFiles;
Action GameCooker::PackageFiles;
Delegate<HashSet<Guid>&> GameCooker::OnCollectAssets;

const Char* ToString(const BuildPlatform platform)
{
    switch (platform)
    {
    case BuildPlatform::Windows32:
        return TEXT("Windows x86");
    case BuildPlatform::Windows64:
        return TEXT("Windows x64");
    case BuildPlatform::UWPx86:
        return TEXT("Windows Store x86");
    case BuildPlatform::UWPx64:
        return TEXT("Windows Store x64");
    case BuildPlatform::XboxOne:
        return TEXT("Xbox One");
    case BuildPlatform::LinuxX64:
        return TEXT("Linux x64");
    case BuildPlatform::PS4:
        return TEXT("PlayStation 4");
    case BuildPlatform::XboxScarlett:
        return TEXT("Xbox Scarlett");
    case BuildPlatform::AndroidARM64:
        return TEXT("Android ARM64");
    case BuildPlatform::Switch:
        return TEXT("Switch");
    case BuildPlatform::PS5:
        return TEXT("PlayStation 5");
    case BuildPlatform::MacOSx64:
        return TEXT("Mac x64");
    case BuildPlatform::MacOSARM64:
        return TEXT("Mac ARM64");
    case BuildPlatform::iOSARM64:
        return TEXT("iOS ARM64");
    case BuildPlatform::WindowsARM64:
        return TEXT("Windows ARM64");
    default:
        return TEXT("");
    }
}

const Char* ToString(const BuildConfiguration configuration)
{
    switch (configuration)
    {
    case BuildConfiguration::Debug:
        return TEXT("Debug");
    case BuildConfiguration::Development:
        return TEXT("Development");
    case BuildConfiguration::Release:
        return TEXT("Release");
    default:
        return TEXT("");
    }
}

const Char* ToString(const DotNetAOTModes mode)
{
    switch (mode)
    {
    case DotNetAOTModes::None:
        return TEXT("None");
    case DotNetAOTModes::ILC:
        return TEXT("ILC");
    case DotNetAOTModes::MonoAOTDynamic:
        return TEXT("MonoAOTDynamic");
    case DotNetAOTModes::MonoAOTStatic:
        return TEXT("MonoAOTStatic");
    default:
        return TEXT("");
    }
}

bool PlatformTools::IsNativeCodeFile(CookingData& data, const String& file)
{
    const String filename = StringUtils::GetFileName(file);
    if (filename.Contains(TEXT(".CSharp")) ||
        filename.Contains(TEXT("Newtonsoft.Json")))
        return false;
    // TODO: maybe use Mono.Cecil via Flax.Build to read assembly image metadata and check if it contains C#?
    return true;
}

bool CookingData::AssetTypeStatistics::operator<(const AssetTypeStatistics& other) const
{
    if (ContentSize != other.ContentSize)
        return ContentSize > other.ContentSize;
    return Count > other.Count;
}

CookingData::CookingData(const SpawnParams& params)
    : ScriptingObject(params)
{
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

    return GetPlatformBinariesRoot() / TEXT("Game") / archDir / ::ToString(Configuration);
}

String CookingData::GetPlatformBinariesRoot() const
{
    return Globals::StartupFolder / TEXT("Source/Platforms") / Tools->GetName() / TEXT("Binaries");
}

void CookingData::GetBuildPlatformName(const Char*& platform, const Char*& architecture) const
{
    switch (Platform)
    {
    case BuildPlatform::Windows32:
        platform = TEXT("Windows");
        architecture = TEXT("x86");
        break;
    case BuildPlatform::Windows64:
        platform = TEXT("Windows");
        architecture = TEXT("x64");
        break;
    case BuildPlatform::UWPx86:
        platform = TEXT("UWP");
        architecture = TEXT("x86");
        break;
    case BuildPlatform::UWPx64:
        platform = TEXT("UWP");
        architecture = TEXT("x64");
        break;
    case BuildPlatform::XboxOne:
        platform = TEXT("XboxOne");
        architecture = TEXT("x64");
        break;
    case BuildPlatform::LinuxX64:
        platform = TEXT("Linux");
        architecture = TEXT("x64");
        break;
    case BuildPlatform::PS4:
        platform = TEXT("PS4");
        architecture = TEXT("x64");
        break;
    case BuildPlatform::XboxScarlett:
        platform = TEXT("XboxScarlett");
        architecture = TEXT("x64");
        break;
    case BuildPlatform::AndroidARM64:
        platform = TEXT("Android");
        architecture = TEXT("ARM64");
        break;
    case BuildPlatform::Switch:
        platform = TEXT("Switch");
        architecture = TEXT("ARM64");
        break;
    case BuildPlatform::PS5:
        platform = TEXT("PS5");
        architecture = TEXT("x64");
        break;
    case BuildPlatform::MacOSx64:
        platform = TEXT("Mac");
        architecture = TEXT("x64");
        break;
    case BuildPlatform::MacOSARM64:
        platform = TEXT("Mac");
        architecture = TEXT("ARM64");
        break;
    case BuildPlatform::iOSARM64:
        platform = TEXT("iOS");
        architecture = TEXT("ARM64");
        break;
    case BuildPlatform::WindowsARM64:
        platform = TEXT("Windows");
        architecture = TEXT("ARM64");
        break;
    default:
        LOG(Fatal, "Unknown or unsupported build platform.");
    }
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

void CookingData::Error(const StringView& msg)
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

CookingData* GameCooker::GetCurrentData()
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
        case BuildPlatform::WindowsARM64:
            result = New<WindowsPlatformTools>(ArchitectureType::ARM64);
            break;
#endif
#if PLATFORM_TOOLS_UWP
        case BuildPlatform::UWPx86:
            result = New<UWPPlatformTools>(ArchitectureType::x86);
            break;
        case BuildPlatform::UWPx64:
            result = New<UWPPlatformTools>(ArchitectureType::x64);
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
#if PLATFORM_TOOLS_SWITCH
        case BuildPlatform::Switch:
            result = New<SwitchPlatformTools>();
            break;
#endif
#if PLATFORM_TOOLS_PS5
        case BuildPlatform::PS5:
            result = New<PS5PlatformTools>();
            break;
#endif
#if PLATFORM_TOOLS_MAC
        case BuildPlatform::MacOSx64:
            result = New<MacPlatformTools>(ArchitectureType::x64);
            break;
        case BuildPlatform::MacOSARM64:
            result = New<MacPlatformTools>(ArchitectureType::ARM64);
            break;
#endif
#if PLATFORM_TOOLS_IOS
        case BuildPlatform::iOSARM64:
            result = New<iOSPlatformTools>();
            break;
#endif
        }
        Tools.Add(platform, result);
    }
    return result;
}

bool GameCooker::Build(BuildPlatform platform, BuildConfiguration configuration, const StringView& outputPath, BuildOptions options, const Array<String>& customDefines, const StringView& preset, const StringView& presetTarget)
{
    if (IsRunning())
    {
        LOG(Warning, "Cannot start a build. Already running.");
        return true;
    }
    PlatformTools* tools = GetTools(platform);
    if (tools == nullptr)
    {
        LOG(Error, "Build platform {0} is not supported.", ::ToString(platform));
        return true;
    }

    // Setup
    CancelFlag = 0;
    ProgressMsg.Clear();
    ProgressValue = 1.0f;
    Data = New<CookingData>();
    CookingData& data = *Data;
    data.Tools = tools;
    data.Platform = platform;
    data.Configuration = configuration;
    data.Options = options;
    data.Preset = preset;
    data.PresetTarget = presetTarget;
    data.CustomDefines = customDefines;
    data.OriginalOutputPath = outputPath;
    FileSystem::NormalizePath(data.OriginalOutputPath);
    data.OriginalOutputPath = FileSystem::ConvertRelativePathToAbsolute(Globals::ProjectFolder, data.OriginalOutputPath);
    data.NativeCodeOutputPath = data.ManagedCodeOutputPath = data.DataOutputPath = data.OriginalOutputPath;
    data.CacheDirectory = Globals::ProjectCacheFolder / TEXT("Cooker") / tools->GetName();
    if (!FileSystem::DirectoryExists(data.CacheDirectory))
    {
        if (FileSystem::CreateDirectory(data.CacheDirectory))
        {
            LOG(Error, "Cannot setup game building cache directory.");
            return true;
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
            return true;
        }
    }
    else
    {
        ThreadCond.NotifyOne();
    }

    return false;
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

void GameCooker::GetCurrentPlatform(PlatformType& platform, BuildPlatform& buildPlatform, BuildConfiguration& buildConfiguration)
{
    platform = PLATFORM_TYPE;
#if BUILD_DEBUG
    buildConfiguration = BuildConfiguration::Debug;
#elif BUILD_DEVELOPMENT
    buildConfiguration = BuildConfiguration::Development;
#elif BUILD_RELEASE
    buildConfiguration = BuildConfiguration::Release;
#endif
    switch (PLATFORM_TYPE)
    {
    case PlatformType::Windows:
        if (PLATFORM_ARCH == ArchitectureType::x64)
            buildPlatform = BuildPlatform::Windows64;
        else if (PLATFORM_ARCH == ArchitectureType::ARM64)
            buildPlatform = BuildPlatform::WindowsARM64;
        else
            buildPlatform = BuildPlatform::Windows32;
        break;
    case PlatformType::XboxOne:
        buildPlatform = BuildPlatform::XboxOne;
        break;
    case PlatformType::UWP:
        buildPlatform = BuildPlatform::UWPx64;
        break;
    case PlatformType::Linux:
        buildPlatform = BuildPlatform::LinuxX64;
        break;
    case PlatformType::PS4:
        buildPlatform = BuildPlatform::PS4;
        break;
    case PlatformType::XboxScarlett:
        buildPlatform = BuildPlatform::XboxScarlett;
        break;
    case PlatformType::Android:
        buildPlatform = BuildPlatform::AndroidARM64;
        break;
    case PlatformType::Switch:
        buildPlatform = BuildPlatform::Switch;
        break;
    case PlatformType::PS5:
        buildPlatform = BuildPlatform::PS5;
        break;
    case PlatformType::Mac:
        buildPlatform = PLATFORM_ARCH_ARM || PLATFORM_ARCH_ARM64 ? BuildPlatform::MacOSARM64 : BuildPlatform::MacOSx64;
        break;
    case PlatformType::iOS:
        buildPlatform = BuildPlatform::iOSARM64;
        break;
    default: ;
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

void GameCookerImpl::OnCollectAssets(HashSet<Guid>& assets)
{
    if (Internal_OnCollectAssets == nullptr)
    {
        auto c = GameCooker::GetStaticClass();
        if (c)
            Internal_OnCollectAssets = c->GetMethod("Internal_OnCollectAssets", 0);
        ASSERT(GameCookerImpl::Internal_OnCollectAssets);
    }

    MCore::Thread::Attach();
    MObject* exception = nullptr;
    auto list = (MArray*)Internal_OnCollectAssets->Invoke(nullptr, nullptr, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("OnCollectAssets"));
    }

    if (list)
    {
        auto ids = MUtils::ToSpan<Guid>(list);
        for (int32 i = 0; i < ids.Length(); i++)
            assets.Add(ids[i]);
    }
}

bool GameCookerImpl::Build()
{
    CookingData& data = *Data;
    LOG(Info, "Starting Game Cooker...");
    LOG(Info, "Platform: {0}, Configuration: {2}, Options: {1}", ::ToString(data.Platform), (int32)data.Options, ::ToString(data.Configuration));
    LOG(Info, "Output Path: {0}", data.OriginalOutputPath);

    // Late init feature
    if (Steps.IsEmpty())
    {
        Steps.Add(New<ValidateStep>());
        Steps.Add(New<CompileScriptsStep>());
        Steps.Add(New<DeployDataStep>());
        Steps.Add(New<PrecompileAssembliesStep>());
        Steps.Add(New<CollectAssetsStep>());
        Steps.Add(New<CookAssetsStep>());
        Steps.Add(New<PostProcessStep>());
    }

    MCore::Thread::Attach();

    // Build Started
    if (!EnumHasAnyFlags(data.Options, BuildOptions::NoCook))
    {
        CallEvent(GameCooker::EventType::BuildStarted);
        data.Tools->OnBuildStarted(data);
        for (int32 stepIndex = 0; stepIndex < Steps.Count(); stepIndex++)
            Steps[stepIndex]->OnBuildStarted(data);
        data.InitProgress(Steps.Count());
    }

    // Execute all steps in a sequence
    bool failed = false;
    for (int32 stepIndex = 0; stepIndex < Steps.Count(); stepIndex++)
    {
        if (GameCooker::IsCancelRequested())
            break;
        if (EnumHasAnyFlags(data.Options, BuildOptions::NoCook))
            continue;
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

        if (EnumHasAnyFlags(data.Options, BuildOptions::ShowOutput))
        {
            FileSystem::ShowFileExplorer(data.OriginalOutputPath);
        }

        if (EnumHasAnyFlags(data.Options, BuildOptions::AutoRun))
        {
            String executableFile, commandLineFormat, workingDir;
            data.Tools->OnRun(data, executableFile, commandLineFormat, workingDir);
            if (executableFile.HasChars())
            {
                const String gameArgs; // TODO: pass custom game run args from Editor? eg. starting map? or client info?
                const String commandLine = commandLineFormat.HasChars() ? String::Format(*commandLineFormat, gameArgs) : gameArgs;
                if (workingDir.IsEmpty())
                    workingDir = data.NativeCodeOutputPath;
                CreateProcessSettings procSettings;
                procSettings.FileName = executableFile;
                procSettings.Arguments = commandLine;
                procSettings.WorkingDirectory = workingDir;
                procSettings.HiddenWindow = false;
                procSettings.WaitForEnd = false;
                procSettings.LogOutput = false;
                procSettings.ShellExecute = true;
                Platform::CreateProcess(procSettings);
            }
            else
            {
                LOG(Warning, "Missing executable to run or platform doesn't support build&run.");
            }
        }
    }
    IsRunning = false;
    CancelFlag = 0;
    if (!EnumHasAnyFlags(data.Options, BuildOptions::NoCook))
    {
        for (int32 stepIndex = 0; stepIndex < Steps.Count(); stepIndex++)
            Steps[stepIndex]->OnBuildEnded(data, failed);
        data.Tools->OnBuildEnded(data, failed);
        CallEvent(failed ? GameCooker::EventType::BuildFailed : GameCooker::EventType::BuildDone);
    }
    Delete(Data);
    Data = nullptr;

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
    GameCooker::OnCollectAssets.Bind(OnCollectAssets);

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
            params.AddParam(ProgressMsg, Scripting::GetScriptsDomain());
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
