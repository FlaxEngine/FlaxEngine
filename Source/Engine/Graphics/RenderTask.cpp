// Copyright (c) Wojciech Figat. All rights reserved.

#include "RenderTask.h"
#include "RenderBuffers.h"
#include "GPUDevice.h"
#include "GPUSwapChain.h"
#include "PostProcessEffect.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Level/Actors/PostFxVolume.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Engine/Renderer/Lightmaps.h"
#else
#include "Engine/Engine/Screen.h"
#endif

Array<RenderTask*> RenderTask::Tasks;
CriticalSection RenderTask::TasksLocker;
int32 RenderTask::TasksDoneLastFrame;
Array<PostProcessEffect*> SceneRenderTask::GlobalCustomPostFx;
MainRenderTask* MainRenderTask::Instance;
CriticalSection RenderContext::GPULocker;

PostProcessEffect::PostProcessEffect(const SpawnParams& params)
    : Script(params)
{
}

void RenderTask::DrawAll()
{
    ScopeLock lock(TasksLocker);

    // Sort tasks (by Order property)
    Sorting::QuickSortObj(Tasks.Get(), Tasks.Count());

    // Render all tasks
    for (auto task : Tasks)
    {
        if (task->CanDraw())
            task->OnDraw();
        else
            task->OnIdle();
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

void RenderTask::OnIdle()
{
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

SceneRenderTask::SceneRenderTask(const SpawnParams& params)
    : RenderTask(params)
{
    Buffers = New<RenderBuffers>();

    // Initialize view
    View.Position = Float3::Zero;
    View.Direction = Float3::Forward;
    Matrix::PerspectiveFov(PI_OVER_2, 1.0f, View.Near, View.Far, View.Projection);
    View.NonJitteredProjection = View.Projection;
    Matrix::Invert(View.Projection, View.IP);
    View.SetFace(4);
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
    CustomActors.AddUnique(actor);
}

void SceneRenderTask::RemoveCustomActor(Actor* actor)
{
    CustomActors.Remove(actor);
}

void SceneRenderTask::ClearCustomActors()
{
    CustomActors.Clear();
}

void SceneRenderTask::AddCustomPostFx(PostProcessEffect* fx)
{
    CustomPostFx.AddUnique(fx);
}

void SceneRenderTask::RemoveCustomPostFx(PostProcessEffect* fx)
{
    CustomPostFx.Remove(fx);
}

void SceneRenderTask::AddGlobalCustomPostFx(PostProcessEffect* fx)
{
    GlobalCustomPostFx.AddUnique(fx);
}

void SceneRenderTask::RemoveGlobalCustomPostFx(PostProcessEffect* fx)
{
    GlobalCustomPostFx.Remove(fx);
}

void SceneRenderTask::CollectPostFxVolumes(RenderContext& renderContext)
{
    // Cache WorldPosition used for PostFx volumes blending (RenderView caches it later on)
    renderContext.View.WorldPosition = renderContext.View.Origin + renderContext.View.Position;

    if (EnumHasAllFlags(ActorsSource, ActorsSources::Scenes))
    {
        Level::CollectPostFxVolumes(renderContext);
    }
    if (EnumHasAllFlags(ActorsSource, ActorsSources::CustomActors))
    {
        for (Actor* a : CustomActors)
        {
            auto* postFxVolume = dynamic_cast<PostFxVolume*>(a);
            if (postFxVolume && a->GetIsActive())
                postFxVolume->Collect(renderContext);
        }
    }
    if (EnumHasAllFlags(ActorsSource, ActorsSources::CustomScenes))
    {
        for (Scene* scene : CustomScenes)
        {
            if (scene && scene->IsActiveInHierarchy())
                scene->Rendering.CollectPostFxVolumes(renderContext);
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

bool SortPostFx(PostProcessEffect* const& a, PostProcessEffect* const& b)
{
    return a->Order < b->Order;
}

void SceneRenderTask::OnCollectDrawCalls(RenderContextBatch& renderContextBatch, byte category)
{
    // Setup PostFx in the render list
    if (category == SceneRendering::DrawCategory::PreRender)
    {
        const RenderContext& renderContext = renderContextBatch.GetMainContext();
        auto& postFx = renderContext.List->PostFx;

        // Collect all post effects
        if (AllowGlobalCustomPostFx)
        {
            for (PostProcessEffect* fx : GlobalCustomPostFx)
            {
                if (fx && fx->CanRender(renderContext))
                    postFx.Add(fx);
            }
        }
        for (PostProcessEffect* fx : CustomPostFx)
        {
            if (fx && fx->CanRender(renderContext))
                postFx.Add(fx);
        }
        if (const auto* camera = Camera.Get())
        {
            for (Script* script : camera->Scripts)
            {
                auto* fx = Cast<PostProcessEffect>(script);
                if (fx && fx->CanRender(renderContext))
                    postFx.Add(fx);
            }
        }

        // Sort by order
        Sorting::QuickSort(postFx.Get(), postFx.Count(), &SortPostFx);
    }

    // Draw actors (collect draw calls)
    if (EnumHasAllFlags(ActorsSource, ActorsSources::CustomActors))
    {
        if (category == SceneRendering::DrawCategory::PreRender)
        {
            if (_customActorsScene == nullptr)
                _customActorsScene = New<SceneRendering>();
            else
                _customActorsScene->Clear();
            for (Actor* a : CustomActors)
                AddActorToSceneRendering(_customActorsScene, a);
        }
        ASSERT_LOW_LAYER(_customActorsScene);
        _customActorsScene->Draw(renderContextBatch, (SceneRendering::DrawCategory)category);
    }
    if (EnumHasAllFlags(ActorsSource, ActorsSources::CustomScenes))
    {
        for (Scene* scene : CustomScenes)
        {
            if (scene && scene->IsActiveInHierarchy())
                scene->Rendering.Draw(renderContextBatch, (SceneRendering::DrawCategory)category);
        }
    }
    if (EnumHasAllFlags(ActorsSource, ActorsSources::Scenes))
    {
        Level::DrawActors(renderContextBatch, category);
    }

    // External drawing event
    for (RenderContext& renderContext : renderContextBatch.Contexts)
        CollectDrawCalls(renderContext);
}

void SceneRenderTask::OnPreRender(GPUContext* context, RenderContext& renderContext)
{
    PreRender(context, renderContext);

    // Collect initial draw calls
    renderContext.View.Pass = DrawPass::GBuffer;
    RenderContextBatch renderContextBatch(renderContext);
    OnCollectDrawCalls(renderContextBatch, SceneRendering::PreRender);
}

void SceneRenderTask::OnPostRender(GPUContext* context, RenderContext& renderContext)
{
    // Collect final draw calls
    renderContext.View.Pass = DrawPass::GBuffer;
    RenderContextBatch renderContextBatch(renderContext);
    OnCollectDrawCalls(renderContextBatch, SceneRendering::PostRender);

    PostRender(context, renderContext);

    if (Buffers)
        Buffers->ReleaseUnusedMemory();
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
    if (!IsCustomRendering && Buffers && Buffers->GetWidth() > 0)
        Renderer::Render(this);

    RenderTask::OnRender(context);
}

void SceneRenderTask::OnEnd(GPUContext* context)
{
    TasksDoneLastFrame++;
    IsCameraCut = false;

    RenderTask::OnEnd(context);

    // Swap data
    View.PrevOrigin = View.Origin;
    View.PrevView = View.View;
    View.PrevProjection = View.Projection;
    View.PrevViewProjection = View.ViewProjection();

    // Remove jitter from the projection (in case it's unmodified by gameplay eg. due to missing camera)
    View.Projection = View.NonJitteredProjection;
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

void SceneRenderTask::OnIdle()
{
    RenderTask::OnIdle();

    if (Buffers)
        Buffers->ReleaseUnusedMemory();
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

RenderContext::RenderContext(SceneRenderTask* task) noexcept
{
    Buffers = task->Buffers;
    Task = task;
    View = task->View;
}

RenderContextBatch::RenderContextBatch(SceneRenderTask* task)
{
    Buffers = task->Buffers;
    Task = task;
}

RenderContextBatch::RenderContextBatch(const RenderContext& context)
{
    Buffers = context.Buffers;
    Task = context.Task;
    Contexts.Add(context);
}
