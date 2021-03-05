// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ViewportIconsRenderer.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Content/Content.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Actors/PointLight.h"
#include "Engine/Level/Actors/DirectionalLight.h"
#include "Engine/Level/Actors/EnvironmentProbe.h"
#include "Engine/Level/Actors/Skybox.h"
#include "Engine/Level/Actors/Decal.h"
#include "Engine/Level/Actors/ExponentialHeightFog.h"
#include "Engine/Audio/AudioListener.h"
#include "Engine/Audio/AudioSource.h"
#include "Engine/Particles/ParticleEffect.h"
#include "Engine/Animations/SceneAnimations/SceneAnimationPlayer.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Level/Actors/Sky.h"
#include "Engine/Level/Actors/SkyLight.h"
#include "Engine/Level/Actors/SpotLight.h"

#define ICON_RADIUS 7.0f

enum class IconTypes
{
    PointLight,
    DirectionalLight,
    EnvironmentProbe,
    Skybox,
    AudioListener,
    AudioSource,
    Decal,
    ParticleEffect,
    SceneAnimationPlayer,

    MAX
};

AssetReference<Model> QuadModel;
ModelInstanceEntries InstanceBuffers[static_cast<int32>(IconTypes::MAX)];
Dictionary<ScriptingTypeHandle, IconTypes> ActorTypeToIconType;

class ViewportIconsRendererService : public EngineService
{
public:

    ViewportIconsRendererService()
        : EngineService(TEXT("Viewport Icons Renderer"))
    {
    }

    bool Init() override;
    void Dispose() override;
};

ViewportIconsRendererService ViewportIconsRendererServiceInstance;

namespace
{
    void DrawIcons(RenderContext& renderContext, Scene* scene, Mesh::DrawInfo& draw)
    {
        auto& view = renderContext.View;
        const BoundingFrustum frustum = view.Frustum;
        auto& icons = scene->GetSceneRendering()->ViewportIcons;
        Matrix m1, m2, world;
        for (Actor* icon : icons)
        {
            BoundingSphere sphere(icon->GetPosition(), ICON_RADIUS);
            IconTypes iconType;
            if (frustum.Intersects(sphere) && ActorTypeToIconType.TryGet(icon->GetTypeHandle(), iconType))
            {
                // Create world matrix
                Matrix::Scaling(ICON_RADIUS * 2.0f, m2);
                Matrix::RotationY(PI, world);
                Matrix::Multiply(m2, world, m1);
                Matrix::Billboard(sphere.Center, view.Position, Vector3::Up, view.Direction, m2);
                Matrix::Multiply(m1, m2, world);

                // Draw icon
                GeometryDrawStateData drawState;
                draw.DrawState = &drawState;
                draw.Buffer = &InstanceBuffers[static_cast<int32>(iconType)];
                draw.World = &world;
                draw.Bounds = sphere;
                QuadModel->Draw(renderContext, draw);
            }
        }
    }

    void DrawIcons(RenderContext& renderContext, Actor* actor, Mesh::DrawInfo& draw)
    {
        auto& view = renderContext.View;
        const BoundingFrustum frustum = view.Frustum;
        Matrix m1, m2, world;
        BoundingSphere sphere(actor->GetPosition(), ICON_RADIUS);
        IconTypes iconType;
        if (frustum.Intersects(sphere) && ActorTypeToIconType.TryGet(actor->GetTypeHandle(), iconType))
        {
            // Create world matrix
            Matrix::Scaling(ICON_RADIUS * 2.0f, m2);
            Matrix::RotationY(PI, world);
            Matrix::Multiply(m2, world, m1);
            Matrix::Billboard(sphere.Center, view.Position, Vector3::Up, view.Direction, m2);
            Matrix::Multiply(m1, m2, world);

            // Draw icon
            GeometryDrawStateData drawState;
            draw.DrawState = &drawState;
            draw.Buffer = &InstanceBuffers[static_cast<int32>(iconType)];
            draw.World = &world;
            draw.Bounds = sphere;
            QuadModel->Draw(renderContext, draw);
        }

        for (auto child : actor->Children)
            DrawIcons(renderContext, child, draw);
    }
}

void ViewportIconsRenderer::DrawIcons(RenderContext& renderContext, Actor* actor)
{
    auto& view = renderContext.View;
    if (!actor || (view.Flags & ViewFlags::EditorSprites) == 0 || QuadModel == nullptr || !QuadModel->IsLoaded())
        return;

    Mesh::DrawInfo draw;
    draw.Lightmap = nullptr;
    draw.LightmapUVs = nullptr;
    draw.Flags = StaticFlags::Transform;
    draw.DrawModes = DrawPass::Forward;
    draw.PerInstanceRandom = 0;
    draw.LODBias = 0;
    draw.ForcedLOD = -1;
    draw.VertexColors = nullptr;

    if (const auto scene = SceneObject::Cast<Scene>(actor))
    {
        ::DrawIcons(renderContext, scene, draw);
    }
    else
    {
        ::DrawIcons(renderContext, actor, draw);
    }
}

bool ViewportIconsRendererService::Init()
{
    QuadModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Quad"));
#define INIT(type, path) \
	InstanceBuffers[static_cast<int32>(IconTypes::type)].Setup(1); \
	InstanceBuffers[static_cast<int32>(IconTypes::type)][0].ReceiveDecals = false; \
	InstanceBuffers[static_cast<int32>(IconTypes::type)][0].Material = Content::LoadAsyncInternal<MaterialInstance>(TEXT(path))
    INIT(PointLight, "Editor/Icons/PointLight");
    INIT(DirectionalLight, "Editor/Icons/DirectionalLight");
    INIT(EnvironmentProbe, "Editor/Icons/EnvironmentProbe");
    INIT(Skybox, "Editor/Icons/Skybox");
    INIT(AudioListener, "Editor/Icons/AudioListener");
    INIT(AudioSource, "Editor/Icons/AudioSource");
    INIT(Decal, "Editor/Icons/Decal");
    INIT(ParticleEffect, "Editor/Icons/ParticleEffect");
    INIT(SceneAnimationPlayer, "Editor/Icons/SceneAnimationPlayer");
#undef INIT
#define MAP_TYPE(type, iconType) ActorTypeToIconType.Add(type::TypeInitializer, IconTypes::iconType)
    MAP_TYPE(PointLight, PointLight);
    MAP_TYPE(DirectionalLight, DirectionalLight);
    MAP_TYPE(EnvironmentProbe, EnvironmentProbe);
    MAP_TYPE(Skybox, Skybox);
    MAP_TYPE(AudioListener, AudioListener);
    MAP_TYPE(AudioSource, AudioSource);
    MAP_TYPE(Decal, Decal);
    MAP_TYPE(ParticleEffect, ParticleEffect);
    MAP_TYPE(SceneAnimationPlayer, SceneAnimationPlayer);
    MAP_TYPE(ExponentialHeightFog, Skybox);
    MAP_TYPE(Sky, Skybox);
    MAP_TYPE(SkyLight, PointLight);
    MAP_TYPE(SpotLight, PointLight);
#undef MAP_TYPE

    return false;
}

void ViewportIconsRendererService::Dispose()
{
    QuadModel = nullptr;
    for (int32 i = 0; i < ARRAY_COUNT(InstanceBuffers); i++)
        InstanceBuffers[i].Release();
    ActorTypeToIconType.Clear();
}
