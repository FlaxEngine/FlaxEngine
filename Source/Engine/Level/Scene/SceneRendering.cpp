// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#define SCENE_RENDERING_USE_PROFILER 0

#include "SceneRendering.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Threading/Threading.h"
#if SCENE_RENDERING_USE_PROFILER
#include "Engine/Profiler/ProfilerCPU.h"
#endif

FORCE_INLINE bool FrustumsListCull(const BoundingSphere& bounds, const BoundingFrustum* frustums, int32 frustumsCount)
{
    for (int32 i = 0; i < frustumsCount; i++)
    {
        if (frustums[i].Intersects(bounds))
            return true;
    }
    return false;
}

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

void SceneRendering::Draw(RenderContextBatch& renderContextBatch, DrawCategory category)
{
    ScopeLock lock(Locker);
    auto& view = renderContextBatch.GetMainContext().View;
    const Vector3 origin = view.Origin;
    for (auto& renderContext : renderContextBatch.Contexts)
        renderContext.List->Scenes.Add(this);
    auto& list = Actors[(int32)category];

    // Setup frustum data
    Array<BoundingFrustum, RenderListAllocation> frustumsData;
    BoundingFrustum* frustums = &view.CullingFrustum;
    int32 frustumsCount = renderContextBatch.Contexts.Count();
    if (frustumsCount != 1)
    {
        frustumsData.Resize(frustumsCount);
        frustums = frustumsData.Get();
        for (int32 i = 0; i < frustumsCount; i++)
            frustums[i] = renderContextBatch.Contexts[i].View.CullingFrustum;
    }

#define CHECK_ACTOR ((view.RenderLayersMask.Mask & e.LayerMask) && (e.NoCulling || FrustumsListCull(e.Bounds, frustums, frustumsCount)))
#define CHECK_ACTOR_SINGLE_FRUSTUM ((view.RenderLayersMask.Mask & e.LayerMask) && (e.NoCulling || frustums->Intersects(e.Bounds)))
#if SCENE_RENDERING_USE_PROFILER
#define DRAW_ACTOR(mode) PROFILE_CPU_ACTOR(e.Actor); e.Actor->Draw(mode)
#else
#define DRAW_ACTOR(mode) e.Actor->Draw(mode)
#endif

    // Draw all visual components
    if (view.IsOfflinePass)
    {
        // Offline pass with additional static flags culling
        for (int32 i = 0; i < list.Count(); i++)
        {
            auto e = list.Get()[i];
            e.Bounds.Center -= origin;
            if (CHECK_ACTOR && e.Actor->GetStaticFlags() & view.StaticFlagsMask)
            {
                DRAW_ACTOR(renderContextBatch);
            }
        }
    }
    else if (origin.IsZero() && frustumsCount == 1)
    {
        // Fast path for no origin shifting with a single context
        auto& renderContext = renderContextBatch.Contexts[0];
        for (int32 i = 0; i < list.Count(); i++)
        {
            auto e = list.Get()[i];
            if (CHECK_ACTOR_SINGLE_FRUSTUM)
            {
                DRAW_ACTOR(renderContext);
            }
        }
    }
    else if (origin.IsZero())
    {
        // Fast path for no origin shifting
        for (int32 i = 0; i < list.Count(); i++)
        {
            auto e = list.Get()[i];
            if (CHECK_ACTOR)
            {
                DRAW_ACTOR(renderContextBatch);
            }
        }
    }
    else
    {
        // Generic case
        for (int32 i = 0; i < list.Count(); i++)
        {
            auto e = list.Get()[i];
            e.Bounds.Center -= origin;
            if (CHECK_ACTOR)
            {
                DRAW_ACTOR(renderContextBatch);
            }
        }
    }

#undef CHECK_ACTOR
#undef DRAW_ACTOR
#if USE_EDITOR
    if (view.Pass & DrawPass::GBuffer && category == SceneDraw)
    {
        // Draw physics shapes
        if (view.Flags & ViewFlags::PhysicsDebug || view.Mode == ViewMode::PhysicsColliders)
        {
            for (int32 i = 0; i < PhysicsDebug.Count(); i++)
            {
                PhysicsDebug[i](view);
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
        PostFxProviders[i]->Collect(renderContext);
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

void SceneRendering::AddActor(Actor* a, int32& key, DrawCategory category)
{
    if (key != -1)
        return;
    ScopeLock lock(Locker);
    auto& list = Actors[(int32)category];
    // TODO: track removedCount and skip searching for free entry if there is none
    key = 0;
    for (; key < list.Count(); key++)
    {
        if (list[key].Actor == nullptr)
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

void SceneRendering::UpdateActor(Actor* a, int32 key, DrawCategory category)
{
    ScopeLock lock(Locker);
    auto& list = Actors[(int32)category];
    if (list.IsEmpty())
        return;
    auto& e = list[key];
    ASSERT_LOW_LAYER(a == e.Actor);
    for (auto* listener : _listeners)
        listener->OnSceneRenderingUpdateActor(a, e.Bounds);
    e.LayerMask = a->GetLayerMask();
    e.Bounds = a->GetSphere();
}

void SceneRendering::RemoveActor(Actor* a, int32& key, DrawCategory category)
{
    ScopeLock lock(Locker);
    auto& list = Actors[(int32)category];
    if (list.HasItems())
    {
        auto& e = list[key];
        ASSERT_LOW_LAYER(a == e.Actor);
        for (auto* listener : _listeners)
            listener->OnSceneRenderingRemoveActor(a);
        e.Actor = nullptr;
        e.LayerMask = 0;
    }
    key = -1;
}
