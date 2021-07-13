// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SceneRendering.h"
#include "Scene.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"

#define SCENE_RENDERING_USE_PROFILER 0
#define SCENE_RENDERING_USE_SIMD 0

#if SCENE_RENDERING_USE_PROFILER
#include "Engine/Profiler/ProfilerCPU.h"
#endif

#if SCENE_RENDERING_USE_SIMD

#include "Engine/Core/SIMD.h"

ALIGN_BEGIN(16) struct CullDataSIMD
{
    float xs[8];
    float ys[8];
    float zs[8];
    float ds[8];
} ALIGN_END(16);

#endif

int32 SceneRendering::DrawEntries::Add(Actor* obj)
{
    int32 key = 0;
    for (; key < List.Count(); key++)
    {
        if (List[key].Actor == nullptr)
            break;
    }
    if (key == List.Count())
        List.AddOne();
    auto& e = List[key];
    e.Actor = obj;
    e.LayerMask = obj->GetLayerMask();
    e.Bounds = obj->GetSphere();
    return key;
}

void SceneRendering::DrawEntries::Update(Actor* obj, int32 key)
{
    if (List.IsEmpty())
        return;
    auto& e = List[key];
    ASSERT_LOW_LAYER(obj == e.Actor);
    e.LayerMask = obj->GetLayerMask();
    e.Bounds = obj->GetSphere();
}

void SceneRendering::DrawEntries::Remove(Actor* obj, int32 key)
{
    if (List.IsEmpty())
        return;
    auto& e = List[key];
    ASSERT_LOW_LAYER(obj == e.Actor);
    e.Actor = nullptr;
    e.LayerMask = 0;
}

void SceneRendering::DrawEntries::Clear()
{
    List.Clear();
}

void SceneRendering::DrawEntries::CullAndDraw(RenderContext& renderContext)
{
    auto& view = renderContext.View;
    const BoundingFrustum frustum = view.CullingFrustum;
#if SCENE_RENDERING_USE_SIMD
    CullDataSIMD cullData;
    {
        // Near
        auto plane = view.Frustum.GetNear();
        cullData.xs[0] = plane.Normal.X;
        cullData.ys[0] = plane.Normal.Y;
        cullData.zs[0] = plane.Normal.Z;
        cullData.ds[0] = plane.D;

        // Far
        plane = view.Frustum.GetFar();
        cullData.xs[1] = plane.Normal.X;
        cullData.ys[1] = plane.Normal.Y;
        cullData.zs[1] = plane.Normal.Z;
        cullData.ds[1] = plane.D;

        // Left
        plane = view.Frustum.GetLeft();
        cullData.xs[2] = plane.Normal.X;
        cullData.ys[2] = plane.Normal.Y;
        cullData.zs[2] = plane.Normal.Z;
        cullData.ds[2] = plane.D;

        // Right
        plane = view.Frustum.GetRight();
        cullData.xs[3] = plane.Normal.X;
        cullData.ys[3] = plane.Normal.Y;
        cullData.zs[3] = plane.Normal.Z;
        cullData.ds[3] = plane.D;

        // Top
        plane = view.Frustum.GetTop();
        cullData.xs[4] = plane.Normal.X;
        cullData.ys[4] = plane.Normal.Y;
        cullData.zs[4] = plane.Normal.Z;
        cullData.ds[4] = plane.D;

        // Bottom
        plane = view.Frustum.GetBottom();
        cullData.xs[5] = plane.Normal.X;
        cullData.ys[5] = plane.Normal.Y;
        cullData.zs[5] = plane.Normal.Z;
        cullData.ds[5] = plane.D;

        // Extra 0
        cullData.xs[6] = 0;
        cullData.ys[6] = 0;
        cullData.zs[6] = 0;
        cullData.ds[6] = 0;

        // Extra 1
        cullData.xs[7] = 0;
        cullData.ys[7] = 0;
        cullData.zs[7] = 0;
        cullData.ds[7] = 0;
    }

    SimdVector4 px = SIMD::Load(cullData.xs);
    SimdVector4 py = SIMD::Load(cullData.ys);
    SimdVector4 pz = SIMD::Load(cullData.zs);
    SimdVector4 pd = SIMD::Load(cullData.ds);
    SimdVector4 px2 = SIMD::Load(&cullData.xs[4]);
    SimdVector4 py2 = SIMD::Load(&cullData.ys[4]);
    SimdVector4 pz2 = SIMD::Load(&cullData.zs[4]);
    SimdVector4 pd2 = SIMD::Load(&cullData.ds[4]);

    for (int32 i = 0; i < List.Count(); i++)
    {
        auto e = List[i];

        SimdVector4 cx = SIMD::Splat(e.Bounds.Center.X);
        SimdVector4 cy = SIMD::Splat(e.Bounds.Center.Y);
        SimdVector4 cz = SIMD::Splat(e.Bounds.Center.Z);
        SimdVector4 r = SIMD::Splat(-e.Bounds.Radius);

        SimdVector4 t = SIMD::Mul(cx, px);
        t = SIMD::Add(t, SIMD::Mul(cy, py));
        t = SIMD::Add(t, SIMD::Mul(cz, pz));
        t = SIMD::Add(t, pd);
        t = SIMD::Sub(t, r);
        if (SIMD::MoveMask(t))
            continue;

        t = SIMD::Mul(cx, px2);
        t = SIMD::Add(t, SIMD::Mul(cy, py2));
        t = SIMD::Add(t, SIMD::Mul(cz, pz2));
        t = SIMD::Add(t, pd2);
        t = SIMD::Sub(t, r);
        if (SIMD::MoveMask(t))
            continue;

        if (view.RenderLayersMask.Mask & e.LayerMask)
        {
#if SCENE_RENDERING_USE_PROFILER
            PROFILE_CPU();
#if TRACY_ENABLE
            ___tracy_scoped_zone.Name(*e.Actor->GetName(), e.Actor->GetName().Length());
#endif
#endif
            e.Actor->Draw(renderContext);
        }
    }
#else
    for (int32 i = 0; i < List.Count(); i++)
    {
        auto e = List[i];
        if (view.RenderLayersMask.Mask & e.LayerMask && frustum.Intersects(e.Bounds))
        {
#if SCENE_RENDERING_USE_PROFILER
            PROFILE_CPU();
#if TRACY_ENABLE
            ___tracy_scoped_zone.Name(*e.Actor->GetName(), e.Actor->GetName().Length());
#endif
#endif
            e.Actor->Draw(renderContext);
        }
    }
#endif
}

void SceneRendering::DrawEntries::CullAndDrawOffline(RenderContext& renderContext)
{
    auto& view = renderContext.View;
    const BoundingFrustum frustum = view.CullingFrustum;
#if SCENE_RENDERING_USE_SIMD
    CullDataSIMD cullData;
    {
        // Near
        auto plane = view.Frustum.GetNear();
        cullData.xs[0] = plane.Normal.X;
        cullData.ys[0] = plane.Normal.Y;
        cullData.zs[0] = plane.Normal.Z;
        cullData.ds[0] = plane.D;

        // Far
        plane = view.Frustum.GetFar();
        cullData.xs[1] = plane.Normal.X;
        cullData.ys[1] = plane.Normal.Y;
        cullData.zs[1] = plane.Normal.Z;
        cullData.ds[1] = plane.D;

        // Left
        plane = view.Frustum.GetLeft();
        cullData.xs[2] = plane.Normal.X;
        cullData.ys[2] = plane.Normal.Y;
        cullData.zs[2] = plane.Normal.Z;
        cullData.ds[2] = plane.D;

        // Right
        plane = view.Frustum.GetRight();
        cullData.xs[3] = plane.Normal.X;
        cullData.ys[3] = plane.Normal.Y;
        cullData.zs[3] = plane.Normal.Z;
        cullData.ds[3] = plane.D;

        // Top
        plane = view.Frustum.GetTop();
        cullData.xs[4] = plane.Normal.X;
        cullData.ys[4] = plane.Normal.Y;
        cullData.zs[4] = plane.Normal.Z;
        cullData.ds[4] = plane.D;

        // Bottom
        plane = view.Frustum.GetBottom();
        cullData.xs[5] = plane.Normal.X;
        cullData.ys[5] = plane.Normal.Y;
        cullData.zs[5] = plane.Normal.Z;
        cullData.ds[5] = plane.D;

        // Extra 0
        cullData.xs[6] = 0;
        cullData.ys[6] = 0;
        cullData.zs[6] = 0;
        cullData.ds[6] = 0;

        // Extra 1
        cullData.xs[7] = 0;
        cullData.ys[7] = 0;
        cullData.zs[7] = 0;
        cullData.ds[7] = 0;
    }

    SimdVector4 px = SIMD::Load(cullData.xs);
    SimdVector4 py = SIMD::Load(cullData.ys);
    SimdVector4 pz = SIMD::Load(cullData.zs);
    SimdVector4 pd = SIMD::Load(cullData.ds);
    SimdVector4 px2 = SIMD::Load(&cullData.xs[4]);
    SimdVector4 py2 = SIMD::Load(&cullData.ys[4]);
    SimdVector4 pz2 = SIMD::Load(&cullData.zs[4]);
    SimdVector4 pd2 = SIMD::Load(&cullData.ds[4]);

    for (int32 i = 0; i < List.Count(); i++)
    {
        auto e = List[i];

        SimdVector4 cx = SIMD::Splat(e.Bounds.Center.X);
        SimdVector4 cy = SIMD::Splat(e.Bounds.Center.Y);
        SimdVector4 cz = SIMD::Splat(e.Bounds.Center.Z);
        SimdVector4 r = SIMD::Splat(-e.Bounds.Radius);

        SimdVector4 t = SIMD::Mul(cx, px);
        t = SIMD::Add(t, SIMD::Mul(cy, py));
        t = SIMD::Add(t, SIMD::Mul(cz, pz));
        t = SIMD::Add(t, pd);
        t = SIMD::Sub(t, r);
        if (SIMD::MoveMask(t))
            continue;

        t = SIMD::Mul(cx, px2);
        t = SIMD::Add(t, SIMD::Mul(cy, py2));
        t = SIMD::Add(t, SIMD::Mul(cz, pz2));
        t = SIMD::Add(t, pd2);
        t = SIMD::Sub(t, r);
        if (SIMD::MoveMask(t))
            continue;

        if (view.RenderLayersMask.Mask & e.LayerMask && e.Actor->GetStaticFlags() & renderContext.View.StaticFlagsMask)
        {
#if SCENE_RENDERING_USE_PROFILER
            PROFILE_CPU();
#if TRACY_ENABLE
            ___tracy_scoped_zone.Name(*e.Actor->GetName(), e.Actor->GetName().Length());
#endif
#endif
            e.Actor->Draw(renderContext);
        }
    }
#else
    for (int32 i = 0; i < List.Count(); i++)
    {
        auto e = List[i];
        if (view.RenderLayersMask.Mask & e.LayerMask && frustum.Intersects(e.Bounds) && e.Actor->GetStaticFlags() & view.StaticFlagsMask)
        {
#if SCENE_RENDERING_USE_PROFILER
            PROFILE_CPU();
#if TRACY_ENABLE
            ___tracy_scoped_zone.Name(*e.Actor->GetName(), e.Actor->GetName().Length());
#endif
#endif
            e.Actor->Draw(renderContext);
        }
    }
#endif
}

SceneRendering::SceneRendering(::Scene* scene)
    : Scene(scene)
{
}

void SceneRendering::Draw(RenderContext& renderContext)
{
    // Skip if disabled
    if (!Scene->GetIsActive())
        return;
    auto& view = renderContext.View;

    // Draw all visual components
    if (view.IsOfflinePass)
    {
        Geometry.CullAndDrawOffline(renderContext);
        if (view.Pass & DrawPass::GBuffer)
        {
            Common.CullAndDrawOffline(renderContext);
            for (int32 i = 0; i < CommonNoCulling.Count(); i++)
            {
                auto actor = CommonNoCulling[i];
                if (actor->GetStaticFlags() & view.StaticFlagsMask && view.RenderLayersMask.HasLayer(actor->GetLayer()))
                    actor->Draw(renderContext);
            }
        }
    }
    else
    {
        Geometry.CullAndDraw(renderContext);
        if (view.Pass & DrawPass::GBuffer)
        {
            Common.CullAndDraw(renderContext);
            for (int32 i = 0; i < CommonNoCulling.Count(); i++)
            {
                auto actor = CommonNoCulling[i];
                if (view.RenderLayersMask.HasLayer(actor->GetLayer()))
                {
#if SCENE_RENDERING_USE_PROFILER
                    PROFILE_CPU();
#if TRACY_ENABLE
                    ___tracy_scoped_zone.Name(*actor->GetName(), actor->GetName().Length());
#endif
#endif
                    actor->Draw(renderContext);
                }
            }
        }
    }
#if USE_EDITOR
    if (view.Pass & DrawPass::GBuffer)
    {
        // Draw physics shapes
        if (view.Flags & ViewFlags::PhysicsDebug)
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
    Geometry.Clear();
    Common.Clear();
    CommonNoCulling.Clear();
#if USE_EDITOR
    PhysicsDebug.Clear();
#endif
}
