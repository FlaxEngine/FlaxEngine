// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#define SCENE_RENDERING_USE_PROFILER 0

#include "SceneRendering.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/RenderList.h"
#if SCENE_RENDERING_USE_PROFILER
#include "Engine/Profiler/ProfilerCPU.h"
#endif

void SceneRendering::Draw(RenderContext& renderContext)
{
    auto& view = renderContext.View;
    const BoundingFrustum frustum = view.CullingFrustum;
    renderContext.List->Scenes.Add(this);

    // Draw all visual components
    if (view.IsOfflinePass)
    {
        for (int32 i = 0; i < Actors.Count(); i++)
        {
            auto& e = Actors[i];
            if (view.RenderLayersMask.Mask & e.LayerMask && (e.NoCulling || frustum.Intersects(e.Bounds)) && e.Actor->GetStaticFlags() & view.StaticFlagsMask)
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
            auto& e = Actors[i];
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
    if (view.Pass & DrawPass::GBuffer)
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
    Actors.Clear();
#if USE_EDITOR
    PhysicsDebug.Clear();
#endif
}

int32 SceneRendering::AddActor(Actor* a)
{
    int32 key = 0;
    // TODO: track removedCount and skip searching for free entry if there is none
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
    return key;
}

void SceneRendering::UpdateActor(Actor* a, int32 key)
{
    if (Actors.IsEmpty())
        return;
    auto& e = Actors[key];
    ASSERT_LOW_LAYER(a == e.Actor);
    e.LayerMask = a->GetLayerMask();
    e.Bounds = a->GetSphere();
}

void SceneRendering::RemoveActor(Actor* a, int32& key)
{
    if (Actors.HasItems())
    {
        auto& e = Actors[key];
        ASSERT_LOW_LAYER(a == e.Actor);
        e.Actor = nullptr;
        e.LayerMask = 0;
    }
    key = -1;
}
