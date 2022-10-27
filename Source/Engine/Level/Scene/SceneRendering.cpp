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

void SceneRendering::Draw(RenderContext& renderContext, DrawCategory category)
{
    ScopeLock lock(Locker);
    auto& view = renderContext.View;
    const BoundingFrustum frustum = view.CullingFrustum;
    const Vector3 origin = view.Origin;
    renderContext.List->Scenes.Add(this);
    auto& list = Actors[(int32)category];

    // Draw all visual components
    if (view.IsOfflinePass)
    {
        for (int32 i = 0; i < list.Count(); i++)
        {
            auto e = list.Get()[i];
            e.Bounds.Center -= origin;
            if (view.RenderLayersMask.Mask & e.LayerMask && (e.NoCulling || frustum.Intersects(e.Bounds)) && e.Actor->GetStaticFlags() & view.StaticFlagsMask)
            {
#if SCENE_RENDERING_USE_PROFILER
                PROFILE_CPU_ACTOR(e.Actor);
#endif
                e.Actor->Draw(renderContext);
            }
        }
    }
    else if (origin.IsZero())
    {
        for (int32 i = 0; i < list.Count(); i++)
        {
            auto e = list.Get()[i];
            if (view.RenderLayersMask.Mask & e.LayerMask && (e.NoCulling || frustum.Intersects(e.Bounds)))
            {
#if SCENE_RENDERING_USE_PROFILER
                PROFILE_CPU_ACTOR(e.Actor);
#endif
                e.Actor->Draw(renderContext);
            }
        }
    }
    else
    {
        for (int32 i = 0; i < list.Count(); i++)
        {
            auto e = list.Get()[i];
            e.Bounds.Center -= origin;
            if (view.RenderLayersMask.Mask & e.LayerMask && (e.NoCulling || frustum.Intersects(e.Bounds)))
            {
#if SCENE_RENDERING_USE_PROFILER
                PROFILE_CPU_ACTOR(e.Actor);
#endif
                e.Actor->Draw(renderContext);
            }
        }
    }
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
