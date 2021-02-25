// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ManagedEditor.h"
#include "Editor/Editor.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/MainThreadManagedInvokeAction.h"
#include "Engine/ShadowsOfMordor/Builder.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Content/Assets/VisualScript.h"
#include "Engine/CSG/CSGBuilder.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Renderer/ProbesRenderer.h"
#include "Engine/Animations/Graph/AnimGraph.h"
#include <ThirdParty/mono-2.0/mono/metadata/threads.h>

ManagedEditor::InternalOptions ManagedEditor::ManagedEditorOptions;

bool WasExitCalled = true; // Fake exit flag so we don't call Exit() in situation when object has been created but not yet initialized (we clear flag in Init())
MMethod* UpdateMethod = nullptr;
MMethod* Internal_EnvProbeBake = nullptr;
MMethod* Internal_LightmapsBake = nullptr;
MMethod* Internal_CanReloadScripts = nullptr;
MMethod* Internal_CanAutoBuildCSG = nullptr;
MMethod* Internal_CanAutoBuildNavMesh = nullptr;
MMethod* Internal_HasGameViewportFocus = nullptr;
MMethod* Internal_ScreenToGameViewport = nullptr;
MMethod* Internal_GameViewportToScreen = nullptr;
MMethod* Internal_GetGameWinPtr = nullptr;
MMethod* Internal_GetGameWindowSize = nullptr;
MMethod* Internal_OnAppExit = nullptr;
MMethod* Internal_OnVisualScriptingDebugFlow = nullptr;
MMethod* Internal_OnAnimGraphDebugFlow = nullptr;

void OnLightmapsBake(ShadowsOfMordor::BuildProgressStep step, float stepProgress, float totalProgress, bool isProgressEvent)
{
    if (Internal_LightmapsBake == nullptr)
    {
        Internal_LightmapsBake = ManagedEditor::GetStaticClass()->GetMethod("Internal_LightmapsBake", 4);
        ASSERT(Internal_LightmapsBake);
    }

    MainThreadManagedInvokeAction::ParamsBuilder params;
    params.AddParam(step);
    params.AddParam(stepProgress);
    params.AddParam(totalProgress);
    params.AddParam(isProgressEvent);
    MainThreadManagedInvokeAction::Invoke(Internal_LightmapsBake, params);
}

void OnLightmapsBuildStarted()
{
    OnLightmapsBake(ShadowsOfMordor::BuildProgressStep::Initialize, 0, 0, false);
}

void OnLightmapsBuildProgress(ShadowsOfMordor::BuildProgressStep step, float stepProgress, float totalProgress)
{
    OnLightmapsBake(step, stepProgress, totalProgress, true);
}

void OnLightmapsBuildFinished(bool failed)
{
    if (failed)
        OnLightmapsBake(ShadowsOfMordor::BuildProgressStep::UpdateEntries, 0, 0, false);
    else
        OnLightmapsBake(ShadowsOfMordor::BuildProgressStep::GenerateLightmapCharts, 0, 0, false);
}

void OnBakeEvent(bool started, const ProbesRenderer::Entry& e)
{
    if (Internal_EnvProbeBake == nullptr)
    {
        Internal_EnvProbeBake = ManagedEditor::GetStaticClass()->GetMethod("Internal_EnvProbeBake", 2);
        ASSERT(Internal_EnvProbeBake);
    }

    MonoObject* probeObj = e.Actor ? e.Actor->GetManagedInstance() : nullptr;

    MainThreadManagedInvokeAction::ParamsBuilder params;
    params.AddParam(started);
    params.AddParam(probeObj);
    MainThreadManagedInvokeAction::Invoke(Internal_EnvProbeBake, params);
}

void OnRegisterBake(const ProbesRenderer::Entry& e)
{
    OnBakeEvent(true, e);
}

void OnFinishBake(const ProbesRenderer::Entry& e)
{
    OnBakeEvent(false, e);
}

void OnBrushModified(CSG::Brush* brush)
{
    if (brush && Editor::Managed && Editor::Managed->CanAutoBuildCSG())
    {
        CSG::Builder::Build(brush->GetBrushScene(), ManagedEditor::ManagedEditorOptions.AutoRebuildCSGTimeoutMs);
    }
}

struct VisualScriptingDebugFlowInfo
{
    MonoObject* Script;
    MonoObject* ScriptInstance;
    uint32 NodeId;
    int32 BoxId;
};

void OnVisualScriptingDebugFlow()
{
    if (Internal_OnVisualScriptingDebugFlow == nullptr)
    {
        Internal_OnVisualScriptingDebugFlow = ManagedEditor::GetStaticClass()->GetMethod("Internal_OnVisualScriptingDebugFlow", 1);
        ASSERT(Internal_OnVisualScriptingDebugFlow);
    }

    const auto stack = VisualScripting::GetThreadStackTop();
    VisualScriptingDebugFlowInfo flowInfo;
    flowInfo.Script = stack->Script->GetOrCreateManagedInstance();
    flowInfo.ScriptInstance = stack->Instance ? stack->Instance->GetOrCreateManagedInstance() : nullptr;
    flowInfo.NodeId = stack->Node->ID;
    flowInfo.BoxId = stack->Box->ID;
    MonoObject* exception = nullptr;
    void* params[1];
    params[0] = &flowInfo;
    Internal_OnVisualScriptingDebugFlow->Invoke(nullptr, params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("OnVisualScriptingDebugFlow"));
    }
}

struct AnimGraphDebugFlowInfo
{
    MonoObject* Asset;
    MonoObject* Object;
    uint32 NodeId;
    int32 BoxId;
};

void OnAnimGraphDebugFlow(Asset* asset, ScriptingObject* object, uint32 nodeId, uint32 boxId)
{
    if (Internal_OnAnimGraphDebugFlow == nullptr)
    {
        Internal_OnAnimGraphDebugFlow = ManagedEditor::GetStaticClass()->GetMethod("Internal_OnAnimGraphDebugFlow", 1);
        ASSERT(Internal_OnAnimGraphDebugFlow);
    }

    AnimGraphDebugFlowInfo flowInfo;
    flowInfo.Asset = asset ? asset->GetOrCreateManagedInstance() : nullptr;
    flowInfo.Object = object ? object->GetOrCreateManagedInstance() : nullptr;
    flowInfo.NodeId = nodeId;
    flowInfo.BoxId = boxId;
    MonoObject* exception = nullptr;
    void* params[1];
    params[0] = &flowInfo;
    Internal_OnAnimGraphDebugFlow->Invoke(nullptr, params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("OnAnimGraphDebugFlow"));
    }
}

void OnLogMessage(LogType type, const StringView& msg);

ManagedEditor::ManagedEditor()
    : PersistentScriptingObject(SpawnParams(ObjectID, ManagedEditor::TypeInitializer))
{
    // Link events
    auto editor = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    editor->Loaded.Bind<ManagedEditor, &ManagedEditor::OnEditorAssemblyLoaded>(this);
    ProbesRenderer::OnRegisterBake.Bind<OnRegisterBake>();
    ProbesRenderer::OnFinishBake.Bind<OnFinishBake>();
    auto lightmapsBuilder = ShadowsOfMordor::Builder::Instance();
    lightmapsBuilder->OnBuildStarted.Bind<OnLightmapsBuildStarted>();
    lightmapsBuilder->OnBuildProgress.Bind<OnLightmapsBuildProgress>();
    lightmapsBuilder->OnBuildFinished.Bind<OnLightmapsBuildFinished>();
    CSG::Builder::OnBrushModified.Bind<OnBrushModified>();
    Log::Logger::OnMessage.Bind<OnLogMessage>();
    VisualScripting::DebugFlow.Bind<OnVisualScriptingDebugFlow>();
    AnimGraphExecutor::DebugFlow.Bind<OnAnimGraphDebugFlow>();
}

ManagedEditor::~ManagedEditor()
{
    // Unlink events
    auto editor = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    editor->Loaded.Unbind<ManagedEditor, &ManagedEditor::OnEditorAssemblyLoaded>(this);
    ProbesRenderer::OnRegisterBake.Unbind<OnRegisterBake>();
    ProbesRenderer::OnFinishBake.Unbind<OnFinishBake>();
    auto lightmapsBuilder = ShadowsOfMordor::Builder::Instance();
    lightmapsBuilder->OnBuildStarted.Unbind<OnLightmapsBuildStarted>();
    lightmapsBuilder->OnBuildProgress.Unbind<OnLightmapsBuildProgress>();
    lightmapsBuilder->OnBuildFinished.Unbind<OnLightmapsBuildFinished>();
    CSG::Builder::OnBrushModified.Unbind<OnBrushModified>();
    Log::Logger::OnMessage.Unbind<OnLogMessage>();
    VisualScripting::DebugFlow.Unbind<OnVisualScriptingDebugFlow>();
    AnimGraphExecutor::DebugFlow.Unbind<OnAnimGraphDebugFlow>();
}

void ManagedEditor::Init()
{
    // Note: editor modules should perform quite fast init, any longer things should be done in async during 'editor splash screen time
    void* args[2];
    MClass* mclass = GetClass();
    if (mclass == nullptr)
    {
        LOG(Fatal, "Invalid Editor assembly! Missing class.");
    }
    const auto initMethod = mclass->GetMethod("Init", ARRAY_COUNT(args));
    if (initMethod == nullptr)
    {
        LOG(Fatal, "Invalid Editor assembly! Missing initialization method.");
    }
    MonoObject* instance = GetOrCreateManagedInstance();
    if (instance == nullptr)
    {
        LOG(Fatal, "Failed to create editor instance.");
    }
    MonoObject* exception = nullptr;
    bool isHeadless = CommandLine::Options.Headless.IsTrue();
    bool skipCompile = CommandLine::Options.SkipCompile.IsTrue();
    args[0] = &isHeadless;
    args[1] = &skipCompile;
    initMethod->Invoke(instance, args, &exception);
    if (exception)
    {
        // Umm, we could handle this but here we are not sure if it is sth fatal.
        // Editor should catch non-critical exceptions and leave fatal ones
        MException ex(exception);
        ex.Log(LogType::Warning, TEXT("ManagedEditor::Init"));
        LOG_STR(Fatal, TEXT("Failed to initialize editor! ") + ex.Message);
    }

    // Clear flag to ensure to call Exit() on assembly unloading
    WasExitCalled = false;

    // Load scripts if auto-load on startup is disabled
    if (!ManagedEditorOptions.ForceScriptCompilationOnStartup || skipCompile)
    {
        LOG(Info, "Loading managed assemblies (due to disabled compilation on startup)");
        Scripting::Load();
    }

    // Call building if need to (based on CL)
    if (CommandLine::Options.Build.HasValue())
    {
        const auto buildCommandMethod = GetClass()->GetMethod("BuildCommand", 1);
        if (buildCommandMethod == nullptr)
        {
            LOG(Fatal, "Missing build command method!");
        }
        args[0] = MUtils::ToString(CommandLine::Options.Build.GetValue());
        buildCommandMethod->Invoke(GetManagedInstance(), args, &exception);
        if (exception)
        {
            LOG(Fatal, "Build command failed!");
        }
    }
}

void ManagedEditor::Update()
{
    // Skip if managed object is missing
    const auto instance = GetManagedInstance();
    if (instance == nullptr)
        return;

    // Cache update method pointer
    if (UpdateMethod == nullptr)
    {
        UpdateMethod = GetClass()->GetMethod("Update");
        if (UpdateMethod == nullptr)
        {
            LOG(Fatal, "Invalid Editor assembly!");
        }
    }

    // Call update
    MonoObject* exception = nullptr;
    UpdateMethod->Invoke(instance, nullptr, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Warning, TEXT("ManagedEditor::Update"));
    }
}

void ManagedEditor::Exit()
{
    if (WasExitCalled)
    {
        // Ups xD
        LOG(Warning, "Managed Editor exit called after exit or before init.");
        return;
    }

    // Set flag
    WasExitCalled = true;

    // Skip if managed object is missing
    const auto instance = GetManagedInstance();
    if (instance == nullptr)
        return;

    // Call exit
    const auto exitMethod = GetClass()->GetMethod("Exit");
    if (exitMethod == nullptr)
    {
        LOG(Fatal, "Invalid Editor assembly!");
    }
    MonoObject* exception = nullptr;
    exitMethod->Invoke(instance, nullptr, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Warning, TEXT("ManagedEditor::Exit"));
        LOG_STR(Fatal, TEXT("Failed to shutdown editor! ") + ex.Message);
    }
}

Window* ManagedEditor::GetMainWindow()
{
    ASSERT(HasManagedInstance());
    const auto method = GetClass()->GetMethod("GetMainWindowPtr");
    ASSERT(method);
    return (Window*)MUtils::Unbox<void*>(method->Invoke(GetManagedInstance(), nullptr, nullptr));
}

bool ManagedEditor::CanReloadScripts()
{
    if (!HasManagedInstance())
        return false;
    if (Internal_CanReloadScripts == nullptr)
    {
        Internal_CanReloadScripts = GetClass()->GetMethod("Internal_CanReloadScripts");
        ASSERT(Internal_CanReloadScripts);
    }
    return MUtils::Unbox<bool>(Internal_CanReloadScripts->Invoke(GetManagedInstance(), nullptr, nullptr));
}

bool ManagedEditor::CanAutoBuildCSG()
{
    // Skip calls from non-managed thread (eg. physics worker)
    if (!mono_domain_get() || !mono_thread_current())
        return false;

    if (!HasManagedInstance())
        return false;
    if (!ManagedEditorOptions.AutoRebuildCSG)
        return false;
    if (Internal_CanAutoBuildCSG == nullptr)
    {
        Internal_CanAutoBuildCSG = GetClass()->GetMethod("Internal_CanAutoBuildCSG");
        ASSERT(Internal_CanAutoBuildCSG);
    }
    return MUtils::Unbox<bool>(Internal_CanAutoBuildCSG->Invoke(GetManagedInstance(), nullptr, nullptr));
}

bool ManagedEditor::CanAutoBuildNavMesh()
{
    // Skip calls from non-managed thread (eg. physics worker)
    if (!mono_domain_get() || !mono_thread_current())
        return false;

    if (!HasManagedInstance())
        return false;
    if (!ManagedEditorOptions.AutoRebuildNavMesh)
        return false;
    if (Internal_CanAutoBuildNavMesh == nullptr)
    {
        Internal_CanAutoBuildNavMesh = GetClass()->GetMethod("Internal_CanAutoBuildNavMesh");
        ASSERT(Internal_CanAutoBuildNavMesh);
    }
    return MUtils::Unbox<bool>(Internal_CanAutoBuildNavMesh->Invoke(GetManagedInstance(), nullptr, nullptr));
}

bool ManagedEditor::HasGameViewportFocus() const
{
    bool result = false;
    if (HasManagedInstance())
    {
        if (Internal_HasGameViewportFocus == nullptr)
        {
            Internal_HasGameViewportFocus = GetClass()->GetMethod("Internal_HasGameViewportFocus");
            ASSERT(Internal_HasGameViewportFocus);
        }
        result = MUtils::Unbox<bool>(Internal_HasGameViewportFocus->Invoke(GetManagedInstance(), nullptr, nullptr));
    }
    return result;
}

Vector2 ManagedEditor::ScreenToGameViewport(const Vector2& screenPos) const
{
    Vector2 result = screenPos;
    if (HasManagedInstance())
    {
        if (Internal_ScreenToGameViewport == nullptr)
        {
            Internal_ScreenToGameViewport = GetClass()->GetMethod("Internal_ScreenToGameViewport", 1);
            ASSERT(Internal_ScreenToGameViewport);
        }
        void* params[1];
        params[0] = &result;
        Internal_ScreenToGameViewport->Invoke(GetManagedInstance(), params, nullptr);
    }
    return result;
}

Vector2 ManagedEditor::GameViewportToScreen(const Vector2& viewportPos) const
{
    Vector2 result = viewportPos;
    if (HasManagedInstance())
    {
        if (Internal_GameViewportToScreen == nullptr)
        {
            Internal_GameViewportToScreen = GetClass()->GetMethod("Internal_GameViewportToScreen", 1);
            ASSERT(Internal_GameViewportToScreen);
        }
        void* params[1];
        params[0] = &result;
        Internal_GameViewportToScreen->Invoke(GetManagedInstance(), params, nullptr);
    }
    return result;
}

Window* ManagedEditor::GetGameWindow(bool forceGet)
{
    if (HasManagedInstance())
    {
        if (Internal_GetGameWinPtr == nullptr)
        {
            Internal_GetGameWinPtr = GetClass()->GetMethod("Internal_GetGameWinPtr", 2);
            ASSERT(Internal_GetGameWinPtr);
        }
        Window* win = nullptr;
        void* params[2];
        params[0] = &forceGet;
        params[1] = &win;
        Internal_GetGameWinPtr->Invoke(GetManagedInstance(), params, nullptr);
        return win;
    }
    return nullptr;
}

Vector2 ManagedEditor::GetGameWindowSize()
{
    if (HasManagedInstance())
    {
        if (Internal_GetGameWindowSize == nullptr)
        {
            Internal_GetGameWindowSize = GetClass()->GetMethod("Internal_GetGameWindowSize", 1);
            ASSERT(Internal_GetGameWindowSize);
        }
        Vector2 size;
        void* params[1];
        params[0] = &size;
        Internal_GetGameWindowSize->Invoke(GetManagedInstance(), params, nullptr);
        return size;
    }
    return Vector2::Zero;
}

bool ManagedEditor::OnAppExit()
{
    if (!HasManagedInstance())
        return true;
    if (Internal_OnAppExit == nullptr)
    {
        Internal_OnAppExit = GetClass()->GetMethod("Internal_OnAppExit");
        ASSERT(Internal_OnAppExit);
    }
    return MUtils::Unbox<bool>(Internal_OnAppExit->Invoke(GetManagedInstance(), nullptr, nullptr));
}

void ManagedEditor::OnEditorAssemblyLoaded(MAssembly* assembly)
{
    ASSERT(!HasManagedInstance());

    // FlaxEditor.CSharp.dll has been loaded, let's create managed object for C# editor

    CreateManaged();
}

String ManagedEditor::ToString() const
{
    return TEXT("ManagedEditor");
}

void ManagedEditor::DestroyManaged()
{
    // Ensure to cleanup managed stuff
    if (!WasExitCalled)
    {
        Exit();
    }

    Internal_EnvProbeBake = nullptr;
    Internal_LightmapsBake = nullptr;
    Internal_CanReloadScripts = nullptr;
    Internal_ScreenToGameViewport = nullptr;
    Internal_GameViewportToScreen = nullptr;
    Internal_GetGameWinPtr = nullptr;
    Internal_OnAppExit = nullptr;
    Internal_OnVisualScriptingDebugFlow = nullptr;
    Internal_OnAnimGraphDebugFlow = nullptr;

    // Base
    PersistentScriptingObject::DestroyManaged();
}
