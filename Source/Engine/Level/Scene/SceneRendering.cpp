// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#define SCENE_RENDERING_USE_PROFILER_PER_ACTOR 0

#include "SceneRendering.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Threading/JobSystem.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Profiler/ProfilerCPU.h"

ISceneRenderingListener::~ISceneRenderingListener()
{
    for (SceneRendering* scene : _scenes)
    {
        scene->_listeners.Remove(this);
    }
}

void ISceneRenderingListener::ListenSceneRendering(SceneRendering* scene)
{
    if (!_scenes.Contains(scene))
    {
        _scenes.Add(scene);
        scene->_listeners.Add(this);
    }
}

FORCE_INLINE bool FrustumsListCull(const BoundingSphere& bounds, const Array<BoundingFrustum>& frustums)
{
    const int32 count = frustums.Count();
    const BoundingFrustum* data = frustums.Get();
    for (int32 i = 0; i < count; i++)
    {
        if (data[i].Intersects(bounds))
            return true;
    }
    return false;
}

void SceneRendering::Draw(RenderContextBatch& renderContextBatch, DrawCategory category)
{
    ScopeLock lock(Locker);
    if (category == PreRender)
    {
        // Register scene
        for (const auto& renderContext : renderContextBatch.Contexts)
            renderContext.List->Scenes.Add(this);

        // Add additional lock during scene rendering (prevents any Actors cache modifications on content streaming threads - eg. when model residency changes)
        Locker.Lock();
    }
    else if (category == PostRender)
    {
        // Release additional lock
        Locker.Unlock();
    }
    auto& view = renderContextBatch.GetMainContext().View;
    auto& list = Actors[(int32)category];
    _drawListData = list.Get();
    _drawListSize = list.Count();
    _drawBatch = &renderContextBatch;

    // Setup frustum data
    const int32 frustumsCount = renderContextBatch.Contexts.Count();
    _drawFrustumsData.Resize(frustumsCount);
    for (int32 i = 0; i < frustumsCount; i++)
        _drawFrustumsData.Get()[i] = renderContextBatch.Contexts.Get()[i].View.CullingFrustum;

    // Draw all visual components
    _drawListIndex = -1;
    if (_drawListSize >= 64 && category == SceneDrawAsync && renderContextBatch.EnableAsync)
    {
        // Run in async via Job System
        Function<void(int32)> func;
        func.Bind<SceneRendering, &SceneRendering::DrawActorsJob>(this);
        const uint64 waitLabel = JobSystem::Dispatch(func, JobSystem::GetThreadsCount());
        renderContextBatch.WaitLabels.Add(waitLabel);
    }
    else
    {
        // Scene is small so draw on a main-thread
        DrawActorsJob(0);
    }

#if USE_EDITOR
    if (EnumHasAnyFlags(view.Pass, DrawPass::GBuffer) && category == SceneDraw)
    {
        // Draw physics shapes
        if (EnumHasAnyFlags(view.Flags, ViewFlags::PhysicsDebug) || view.Mode == ViewMode::PhysicsColliders)
        {
            PROFILE_CPU_NAMED("PhysicsDebug");
            const PhysicsDebugCallback* physicsDebugData = PhysicsDebug.Get();
            for (int32 i = 0; i < PhysicsDebug.Count(); i++)
            {
                physicsDebugData[i](view);
            }
        }

        // Draw light shapes
        if (EnumHasAnyFlags(view.Flags, ViewFlags::LightsDebug))
        {
            PROFILE_CPU_NAMED("LightsDebug");
            const LightsDebugCallback* lightsDebugData = LightsDebug.Get();
            for (int32 i = 0; i < LightsDebug.Count(); i++)
            {
                lightsDebugData[i](view);
            }
        }
    }
#endif
}

void SceneRendering::CollectPostFxVolumes(RenderContext& renderContext)
{
#if SCENE_RENDERING_USE_PROFILER
    PROFILE_CPU();
#endif
    for (int32 i = 0; i < PostFxProviders.Count(); i++)
    {
        PostFxProviders.Get()[i]->Collect(renderContext);
    }
}

void SceneRendering::Clear()
{
    ScopeLock lock(Locker);
    for (auto* listener : _listeners)
    {
        listener->OnSceneRenderingClear(this);
        listener->_scenes.Remove(this);
    }
    _listeners.Clear();
    for (auto& e : Actors)
        e.Clear();
#if USE_EDITOR
    PhysicsDebug.Clear();
#endif
}

void SceneRendering::AddActor(Actor* a, int32& key)
{
    if (key != -1)
        return;
    const int32 category = a->_drawCategory;
    ScopeLock lock(Locker);
    auto& list = Actors[category];
    // TODO: track removedCount and skip searching for free entry if there is none
    key = 0;
    for (; key < list.Count(); key++)
    {
        if (list.Get()[key].Actor == nullptr)
            break;
    }
    if (key == list.Count())
        list.AddOne();
    auto& e = list[key];
    e.Actor = a;
    e.LayerMask = a->GetLayerMask();
    e.Bounds = a->GetSphere();
    e.NoCulling = a->_drawNoCulling;
    for (auto* listener : _listeners)
        listener->OnSceneRenderingAddActor(a);
}

void SceneRendering::UpdateActor(Actor* a, int32& key, ISceneRenderingListener::UpdateFlags flags)
{
    const int32 category = a->_drawCategory;
    ScopeLock lock(Locker);
    auto& list = Actors[category];
    if (list.Count() <= key) // Ignore invalid key softly
        return;
    auto& e = list[key];
    if (e.Actor == a)
    {
        for (auto* listener : _listeners)
            listener->OnSceneRenderingUpdateActor(a, e.Bounds, flags);
        if (flags & ISceneRenderingListener::Layer)
            e.LayerMask = a->GetLayerMask();
        if (flags & ISceneRenderingListener::Bounds)
            e.Bounds = a->GetSphere();
    }
}

void SceneRendering::RemoveActor(Actor* a, int32& key)
{
    const int32 category = a->_drawCategory;
    ScopeLock lock(Locker);
    auto& list = Actors[category];
    if (list.Count() > key) // Ignore invalid key softly (eg. list after batch clear during scene unload)
    {
        auto& e = list.Get()[key];
        if (e.Actor == a)
        {
            for (auto* listener : _listeners)
                listener->OnSceneRenderingRemoveActor(a);
            e.Actor = nullptr;
            e.LayerMask = 0;
        }
    }
    key = -1;
}

#define FOR_EACH_BATCH_ACTOR const int64 count = _drawListSize; while (true) { const int64 index = Platform::InterlockedIncrement(&_drawListIndex); if (index >= count) break; auto e = _drawListData[index];
#define CHECK_ACTOR ((view.RenderLayersMask.Mask & e.LayerMask) && (e.NoCulling || FrustumsListCull(e.Bounds, _drawFrustumsData)))
#define CHECK_ACTOR_SINGLE_FRUSTUM ((view.RenderLayersMask.Mask & e.LayerMask) && (e.NoCulling || view.CullingFrustum.Intersects(e.Bounds)))
#if SCENE_RENDERING_USE_PROFILER_PER_ACTOR
#define DRAW_ACTOR(mode) PROFILE_CPU_ACTOR(e.Actor); e.Actor->Draw(mode)
#else
#define DRAW_ACTOR(mode) e.Actor->Draw(mode)
#endif

void SceneRendering::DrawActorsJob(int32)
{
    PROFILE_CPU();
    auto& mainContext = _drawBatch->GetMainContext();
    const auto& view = mainContext.View;
    if (view.StaticFlagsMask != StaticFlags::None)
    {
        // Static-flags culling
        FOR_EACH_BATCH_ACTOR
            e.Bounds.Center -= view.Origin;
            if (CHECK_ACTOR && (e.Actor->GetStaticFlags() & view.StaticFlagsMask) == view.StaticFlagsCompare)
            {
                DRAW_ACTOR(*_drawBatch);
            }
        }
    }
    else if (view.Origin.IsZero() && _drawFrustumsData.Count() == 1)
    {
        // Fast path for no origin shifting with a single context
        FOR_EACH_BATCH_ACTOR
            if (CHECK_ACTOR_SINGLE_FRUSTUM)
            {
                DRAW_ACTOR(mainContext);
            }
        }
    }
    else if (view.Origin.IsZero())
    {
        // Fast path for no origin shifting
        FOR_EACH_BATCH_ACTOR
            if (CHECK_ACTOR)
            {
                DRAW_ACTOR(*_drawBatch);
            }
        }
    }
    else
    {
        // Generic case
        FOR_EACH_BATCH_ACTOR
            e.Bounds.Center -= view.Origin;
            if (CHECK_ACTOR)
            {
                DRAW_ACTOR(*_drawBatch);
            }
        }
    }
}

#undef FOR_EACH_BATCH_ACTOR
#undef CHECK_ACTOR
#undef DRAW_ACTOR
