// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "RenderTask.h"
#include "RenderBuffers.h"
#include "GPUDevice.h"
#include "GPUSwapChain.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Level/Actors/PostFxVolume.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Threading/Threading.h"
#if USE_MONO
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>
#endif
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#else
#include "Engine/Engine/Screen.h"
#endif

#if USE_MONO

// TODO: use API for events and remove this manual wrapper code
class RenderContextInternal
{
public:
    MonoObject* Buffers;
    MonoObject* List;
    RenderView View;
    RenderView* LodProxyView;
    MonoObject* Task;
};

// TODO: use API for events and remove this manual wrapper code
namespace
{
    RenderContextInternal ToManaged(const RenderContext& value)
    {
        RenderContextInternal result;
        result.Buffers = ScriptingObject::ToManaged((ScriptingObject*)value.Buffers);
        result.List = ScriptingObject::ToManaged((ScriptingObject*)value.List);
        result.View = value.View;
        result.LodProxyView = value.LodProxyView;
        result.Task = ScriptingObject::ToManaged((ScriptingObject*)value.Task);
        return result;
    }
}

#endif

Array<RenderTask*> RenderTask::Tasks;
CriticalSection RenderTask::TasksLocker;
int32 RenderTask::TasksDoneLastFrame;
MainRenderTask* MainRenderTask::Instance;

void RenderTask::DrawAll()
{
    ScopeLock lock(TasksLocker);

    // Sort tasks (by Order property)
    Sorting::QuickSortObj(Tasks.Get(), Tasks.Count());

    // Render all that shit
    for (auto task : Tasks)
    {
        if (task->CanDraw())
        {
            task->OnDraw();
        }
    }
}

RenderTask::RenderTask(const SpawnParams& params)
    : ScriptingObject(params)
{
    // Register
    TasksLocker.Lock();
    Tasks.Add(this);
    TasksLocker.Unlock();
}

RenderTask::~RenderTask()
{
    // Unregister
    TasksLocker.Lock();
    Tasks.Remove(this);
    TasksLocker.Unlock();
}

bool RenderTask::CanDraw() const
{
    if (SwapChain && SwapChain->GetWindow() && !SwapChain->GetWindow()->IsVisible() && !SwapChain->GetWindow()->GetSettings().ShowAfterFirstPaint)
        return false;
    return Enabled;
}

void RenderTask::OnDraw()
{
    const auto context = GPUDevice::Instance->GetMainContext();
    OnBegin(context);
    OnRender(context);
    OnEnd(context);
}

void RenderTask::OnBegin(GPUContext* context)
{
    Begin(this, context);
    if (SwapChain)
        SwapChain->Begin(this);

    _prevTask = GPUDevice::Instance->CurrentTask;
    GPUDevice::Instance->CurrentTask = this;
    LastUsedFrame = Engine::FrameCount;
    FrameCount++;
}

void RenderTask::OnRender(GPUContext* context)
{
    Render(this, context);

    if (SwapChain)
    {
        PROFILE_GPU_CPU_NAMED("GUI");
        const Viewport viewport(0, 0, static_cast<float>(SwapChain->GetWidth()), static_cast<float>(SwapChain->GetHeight()));
        Render2D::Begin(context, SwapChain->GetBackBufferView(), nullptr, viewport);
        SwapChain->GetWindow()->OnDraw();
        Render2D::End();
    }
}

void RenderTask::OnEnd(GPUContext* context)
{
    GPUDevice::Instance->CurrentTask = _prevTask;

    _prevTask = nullptr;
    if (SwapChain)
        SwapChain->End(this);
    End(this, context);
}

void RenderTask::OnPresent(bool vsync)
{
    if (SwapChain)
        SwapChain->Present(vsync);
    Present(this);
}

bool RenderTask::Resize(int32 width, int32 height)
{
    return false;
}

void ManagedPostProcessEffect::FetchInfo()
{
    ASSERT(Target);

    SetEnabled(Target->GetEnabled());

    static MMethod* FetchInfoManaged = nullptr;
    if (FetchInfoManaged == nullptr)
    {
        auto klass = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly->GetClass("FlaxEngine.PostProcessEffect");
        ASSERT(klass);
        FetchInfoManaged = klass->GetMethod("FetchInfo", 3);
        ASSERT(FetchInfoManaged);
    }

    void* args[3];
    args[0] = Target->GetOrCreateManagedInstance();
    args[1] = &_location;
    args[2] = &_useSingleTarget;
    MObject* exception = nullptr;
    FetchInfoManaged->Invoke(nullptr, args, &exception);
    if (exception)
        DebugLog::LogException(exception);
}

bool ManagedPostProcessEffect::IsLoaded() const
{
    return Target != nullptr;
}

void ManagedPostProcessEffect::Render(RenderContext& renderContext, GPUTexture* input, GPUTexture* output)
{
#if USE_MONO
    const auto context = GPUDevice::Instance->GetMainContext();
    auto inputObj = ScriptingObject::ToManaged(input);
    auto outputObj = ScriptingObject::ToManaged(output);

    // Call rendering method (handle base classes)
    ASSERT(Target && Target->GetClass() != nullptr);
    auto mclass = Target->GetClass();
    auto renderMethod = mclass->FindMethod("Render", 4, true);
    if (renderMethod == nullptr)
        return;
    RenderContextInternal tmp = ::ToManaged(renderContext);
    void* params[4];
    params[0] = context->GetOrCreateManagedInstance();
    params[1] = &tmp;
    params[2] = inputObj;
    params[3] = outputObj;
    MObject* exception = nullptr;
    renderMethod->InvokeVirtual(Target->GetOrCreateManagedInstance(), params, &exception);
    if (exception)
        DebugLog::LogException(exception);
#endif
}

SceneRenderTask::SceneRenderTask(const SpawnParams& params)
    : RenderTask(params)
{
    View.Position = Float3::Zero;
    View.Direction = Float3::Forward;
    Buffers = New<RenderBuffers>();
}

SceneRenderTask::~SceneRenderTask()
{
    if (Buffers)
        Buffers->DeleteObjectNow();
    if (_customActorsScene)
        Delete(_customActorsScene);
}

void SceneRenderTask::CameraCut()
{
    IsCameraCut = true;
}

void SceneRenderTask::AddCustomActor(Actor* actor)
{
    CustomActors.Add(actor);
}

void SceneRenderTask::RemoveCustomActor(Actor* actor)
{
    CustomActors.Remove(actor);
}

void SceneRenderTask::ClearCustomActors()
{
    CustomActors.Clear();
}

void SceneRenderTask::CollectPostFxVolumes(RenderContext& renderContext)
{
    if ((ActorsSource & ActorsSources::Scenes) != 0)
    {
        Level::CollectPostFxVolumes(renderContext);
    }
    if ((ActorsSource & ActorsSources::CustomActors) != 0)
    {
        for (auto a : CustomActors)
        {
            auto* postFxVolume = dynamic_cast<PostFxVolume*>(a);
            if (postFxVolume && a->GetIsActive())
            {
                postFxVolume->Collect(renderContext);
            }
        }
    }
}

void AddActorToSceneRendering(SceneRendering* s, Actor* a)
{
    if (a && a->IsActiveInHierarchy())
    {
        int32 key = -1;
        s->AddActor(a, key);
        for (Actor* child : a->Children)
            AddActorToSceneRendering(s, child);
    }
}

void SceneRenderTask::OnCollectDrawCalls(RenderContext& renderContext)
{
    // Draw actors (collect draw calls)
    if ((ActorsSource & ActorsSources::CustomActors) != 0)
    {
        if (_customActorsScene == nullptr)
            _customActorsScene = New<SceneRendering>();
        else
            _customActorsScene->Clear();
        for (Actor* a : CustomActors)
            AddActorToSceneRendering(_customActorsScene, a);
        _customActorsScene->Draw(renderContext);
    }
    if ((ActorsSource & ActorsSources::Scenes) != 0)
    {
        Level::DrawActors(renderContext);
    }

    // External drawing event
    CollectDrawCalls(renderContext);
}

void SceneRenderTask::OnPreRender(GPUContext* context, RenderContext& renderContext)
{
    PreRender(context, renderContext);
}

void SceneRenderTask::OnPostRender(GPUContext* context, RenderContext& renderContext)
{
    PostRender(context, renderContext);
}

Viewport SceneRenderTask::GetViewport() const
{
    Viewport viewport;
    if (Output)
        viewport = Viewport(0, 0, static_cast<float>(Output->Width()), static_cast<float>(Output->Height()));
    else if (SwapChain)
        viewport = Viewport(0, 0, static_cast<float>(SwapChain->GetWidth()), static_cast<float>(SwapChain->GetHeight()));
    else if (Buffers != nullptr)
        viewport = Buffers->GetViewport();
    else
        viewport = Viewport(0, 0, 1280, 720);
    viewport.Width *= RenderingPercentage;
    viewport.Height *= RenderingPercentage;
    return viewport;
}

Viewport SceneRenderTask::GetOutputViewport() const
{
    if (Output && Output->IsAllocated())
        return Viewport(0, 0, static_cast<float>(Output->Width()), static_cast<float>(Output->Height()));
    if (SwapChain)
        return Viewport(0, 0, static_cast<float>(SwapChain->GetWidth()), static_cast<float>(SwapChain->GetHeight()));
    return GetViewport();
}

GPUTextureView* SceneRenderTask::GetOutputView() const
{
    if (Output && Output->IsAllocated())
        return Output->View();
    if (SwapChain)
        return SwapChain->GetBackBufferView();
    return nullptr;
}

void SceneRenderTask::OnBegin(GPUContext* context)
{
    RenderTask::OnBegin(context);

    // Copy view info if camera is specified
    if (Camera)
    {
        auto viewport = GetViewport();
        View.CopyFrom(Camera, &viewport);
    }

    // Get custom and global PostFx
    CustomPostFx.Clear();
#if !COMPILE_WITHOUT_CSHARP
    // TODO: move postFx in SceneRenderTask from C# to C++
    static MMethod* GetPostFxManaged = GetStaticClass()->GetMethod("GetPostFx", 1);
    if (GetPostFxManaged)
    {
        int32 count = 0;
        void* params[1];
        params[0] = &count;
        MObject* exception = nullptr;
#if USE_MONO
        const auto objects = (MonoArray*)GetPostFxManaged->Invoke(GetOrCreateManagedInstance(), params, &exception);
        if (exception)
            DebugLog::LogException(exception);
        CustomPostFx.Resize(count);
        for (int32 i = 0; i < count; i++)
        {
            auto& postFx = CustomPostFx[i];
            postFx.Target = mono_array_get(objects, Script*, i);
            if (postFx.Target)
                postFx.FetchInfo();
        }
#endif
    }
#endif

    // Setup render buffers for the output rendering resolution
    if (Output)
    {
        Buffers->Init((int32)((float)Output->Width() * RenderingPercentage), (int32)((float)Output->Height() * RenderingPercentage));
    }
    else if (SwapChain)
    {
        Buffers->Init((int32)((float)SwapChain->GetWidth() * RenderingPercentage), (int32)((float)SwapChain->GetHeight() * RenderingPercentage));
    }
}

void SceneRenderTask::OnRender(GPUContext* context)
{
    if (Buffers && Buffers->GetWidth() > 0)
        Renderer::Render(this);

    RenderTask::OnRender(context);
}

void SceneRenderTask::OnEnd(GPUContext* context)
{
    TasksDoneLastFrame++;
    IsCameraCut = false;

    RenderTask::OnEnd(context);

    // Swap matrices
    View.PrevView = View.View;
    View.PrevProjection = View.Projection;
    View.PrevViewProjection = View.ViewProjection();
}

bool SceneRenderTask::Resize(int32 width, int32 height)
{
    if (Output && Output->Resize(width, height))
        return true;
    if (Buffers && Buffers->Init((int32)((float)width * RenderingPercentage), (int32)((float)height * RenderingPercentage)))
        return true;
    return false;
}

bool SceneRenderTask::CanDraw() const
{
    if (Output && !Output->IsAllocated())
        return false;
    return RenderTask::CanDraw();
}

MainRenderTask::MainRenderTask(const SpawnParams& params)
    : SceneRenderTask(params)
{
    if (Instance == nullptr)
    {
        Instance = this;
        LOG(Info, "Main render task created");
    }
}

MainRenderTask::~MainRenderTask()
{
    if (Instance == this)
    {
        Instance = nullptr;
    }
}

void MainRenderTask::OnBegin(GPUContext* context)
{
    // Use the main camera for the game (can be later overriden in Begin event by external code)
    Camera = Camera::GetMainCamera();

#if !USE_EDITOR
    // Sync render buffers size with the backbuffer
    const auto size = Screen::GetSize();
    Buffers->Init((int32)(size.X * RenderingPercentage), (int32)(size.Y * RenderingPercentage));
#endif

    SceneRenderTask::OnBegin(context);
}

RenderContext::RenderContext(SceneRenderTask* task)
{
    Buffers = task->Buffers;
    View = task->View;
    Task = task;
}
