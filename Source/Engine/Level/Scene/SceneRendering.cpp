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

void SceneRendering::Draw(RenderContext& renderContext)
{
    ScopeLock lock(Locker);
    auto& view = renderContext.View;
    const BoundingFrustum frustum = view.CullingFrustum;
    const Vector3 origin = view.Origin;
    renderContext.List->Scenes.Add(this);

    // Draw all visual components
    if (view.IsOfflinePass)
    {
        for (int32 i = 0; i < Actors.Count(); i++)
        {
            auto e = Actors[i];
            e.Bounds.Center -= origin;
            if (view.RenderLayersMask.Mask & e.LayerMask && (e.NoCulling || frustum.Intersects(e.Bounds)) && static_cast<int32>(e.Actor->GetStaticFlags() & view.StaticFlagsMask))
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
        for (int32 i = 0; i < Actors.Count(); i++)
        {
            auto e = Actors[i];
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
    if (static_cast<int32>(view.Pass & DrawPass::GBuffer))
    {
        // Draw physics shapes
        if (static_cast<int32>(view.Flags & ViewFlags::PhysicsDebug) || view.Mode == ViewMode::PhysicsColliders)
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
    Actors.Clear();
#if USE_EDITOR
    PhysicsDebug.Clear();
#endif
}

void SceneRendering::AddActor(Actor* a, int32& key)
{
    ScopeLock lock(Locker);
    if (key == -1)
    {
        // TODO: track removedCount and skip searching for free entry if there is none
        key = 0;
        for (; key < Actors.Count(); key++)
        {
            if (Actors[key].Actor == nullptr)
                break;
        }
        if (key == Actors.Count())
            Actors.AddOne();
        auto& e = Actors[key];
        e.Actor = a;
        e.LayerMask = a->GetLayerMask();
        e.Bounds = a->GetSphere();
        e.NoCulling = a->_drawNoCulling;
        for (auto* listener : _listeners)
            listener->OnSceneRenderingAddActor(a);
    }
}

void SceneRendering::UpdateActor(Actor* a, int32 key)
{
    ScopeLock lock(Locker);
    if (Actors.IsEmpty())
        return;
    auto& e = Actors[key];
    ASSERT_LOW_LAYER(a == e.Actor);
    for (auto* listener : _listeners)
        listener->OnSceneRenderingUpdateActor(a, e.Bounds);
    e.LayerMask = a->GetLayerMask();
    e.Bounds = a->GetSphere();
}

void SceneRendering::RemoveActor(Actor* a, int32& key)
{
    ScopeLock lock(Locker);
    if (Actors.HasItems())
    {
        auto& e = Actors[key];
        ASSERT_LOW_LAYER(a == e.Actor);
        for (auto* listener : _listeners)
            listener->OnSceneRenderingRemoveActor(a);
        e.Actor = nullptr;
        e.LayerMask = 0;
    }
    key = -1;
}
