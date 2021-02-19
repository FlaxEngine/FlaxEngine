// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SceneRendering.h"
#include "Scene.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Level/Actors/PostFxVolume.h"

#define SCENE_RENDERING_USE_SIMD 0

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

SceneRendering::SceneRendering(::Scene* scene)
    : Scene(scene)
{
}

void CullAndDraw(const BoundingFrustum& frustum, RenderContext& renderContext, const Array<Actor*>& actors)
{
    auto& view = renderContext.View;
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

	float4 px = SIMD::Load(cullData.xs);
	float4 py = SIMD::Load(cullData.ys);
	float4 pz = SIMD::Load(cullData.zs);
	float4 pd = SIMD::Load(cullData.ds);
	float4 px2 = SIMD::Load(&cullData.xs[4]);
	float4 py2 = SIMD::Load(&cullData.ys[4]);
	float4 pz2 = SIMD::Load(&cullData.zs[4]);
	float4 pd2 = SIMD::Load(&cullData.ds[4]);

	for (int32 i = 0; i < actors.Count(); i++)
	{
		const auto& sphere = actors[i]->GetSphere();
		float4 cx = SIMD::Splat(sphere.Center.X);
		float4 cy = SIMD::Splat(sphere.Center.Y);
		float4 cz = SIMD::Splat(sphere.Center.Z);
		float4 r = SIMD::Splat(-sphere.Radius);

		float4 t = SIMD::Mul(cx, px);
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

		actors[i]->Draw(renderContext);
	}
#else
    for (int32 i = 0; i < actors.Count(); i++)
    {
        auto actor = actors[i];
        if (view.RenderLayersMask.HasLayer(actor->GetLayer()) && frustum.Intersects(actor->GetSphere()))
            actor->Draw(renderContext);
    }
#endif
}

void CullAndDrawOffline(const BoundingFrustum& frustum, RenderContext& renderContext, const Array<Actor*>& actors)
{
    auto& view = renderContext.View;
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

	float4 px = SIMD::Load(cullData.xs);
	float4 py = SIMD::Load(cullData.ys);
	float4 pz = SIMD::Load(cullData.zs);
	float4 pd = SIMD::Load(cullData.ds);
	float4 px2 = SIMD::Load(&cullData.xs[4]);
	float4 py2 = SIMD::Load(&cullData.ys[4]);
	float4 pz2 = SIMD::Load(&cullData.zs[4]);
	float4 pd2 = SIMD::Load(&cullData.ds[4]);

	for (int32 i = 0; i < actors.Count(); i++)
	{
		const auto& sphere = actors[i]->GetSphere();
		float4 cx = SIMD::Splat(sphere.Center.X);
		float4 cy = SIMD::Splat(sphere.Center.Y);
		float4 cz = SIMD::Splat(sphere.Center.Z);
		float4 r = SIMD::Splat(-sphere.Radius);

		float4 t = SIMD::Mul(cx, px);
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

		if (actors[i]->GetStaticFlags() & renderContext.View.StaticFlagsMask)
			actors[i]->Draw(renderContext);
	}
#else
    for (int32 i = 0; i < actors.Count(); i++)
    {
        auto actor = actors[i];
        if (actor->GetStaticFlags() & view.StaticFlagsMask && view.RenderLayersMask.HasLayer(actor->GetLayer()) && frustum.Intersects(actor->GetSphere()))
            actor->Draw(renderContext);
    }
#endif
}

void SceneRendering::Draw(RenderContext& renderContext)
{
    // Skip if disabled
    if (!Scene->GetIsActive())
        return;
    auto& view = renderContext.View;

    // Draw all visual components
    const BoundingFrustum frustum = view.CullingFrustum;
    if (view.IsOfflinePass)
    {
        CullAndDrawOffline(frustum, renderContext, Geometry);
        if (view.Pass & DrawPass::GBuffer)
        {
            CullAndDrawOffline(frustum, renderContext, Common);
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
        CullAndDraw(frustum, renderContext, Geometry);
        if (view.Pass & DrawPass::GBuffer)
        {
            CullAndDraw(frustum, renderContext, Common);
            for (int32 i = 0; i < CommonNoCulling.Count(); i++)
            {
                auto actor = CommonNoCulling[i];
                if (view.RenderLayersMask.HasLayer(actor->GetLayer()))
                    actor->Draw(renderContext);
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
