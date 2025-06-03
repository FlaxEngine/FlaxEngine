// Copyright (c) Wojciech Figat. All rights reserved.

#include "ViewportIconsRenderer.h"
#include "Engine/Core/Types/Variant.h"
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
#include "Engine/Video/VideoPlayer.h"

enum class IconTypes
{
    PointLight,
    DirectionalLight,
    EnvironmentProbe,
    Skybox,
    SkyLight,
    AudioListener,
    AudioSource,
    Decal,
    ParticleEffect,
    SceneAnimationPlayer,

    CustomTexture,

    MAX
};

AssetReference<Model> QuadModel;
AssetReference<MaterialInstance> CustomTextureMaterial;
ModelInstanceEntries InstanceBuffers[static_cast<int32>(IconTypes::MAX)];
Dictionary<ScriptingTypeHandle, IconTypes> ActorTypeToIconType;
Dictionary<ScriptingTypeHandle, AssetReference<Texture>> ActorTypeToTexture;
Dictionary<ScriptingObjectReference<Actor>, AssetReference<Texture>> ActorToTexture;
Dictionary<AssetReference<Texture>, AssetReference<MaterialBase>> TextureToMaterial;

class ViewportIconsRendererService : public EngineService
{
public:
    ViewportIconsRendererService()
        : EngineService(TEXT("Viewport Icons Renderer"))
    {
    }

    static void DrawIcons(RenderContext& renderContext, Scene* scene, Mesh::DrawInfo& draw);
    static void DrawIcons(RenderContext& renderContext, Actor* actor, Mesh::DrawInfo& draw);
    bool Init() override;
    void Dispose() override;
};

ViewportIconsRendererService ViewportIconsRendererServiceInstance;
float ViewportIconsRenderer::Scale = 1.0f;

void ViewportIconsRenderer::GetBounds(const Vector3& position, const Vector3& viewPosition, BoundingSphere& bounds)
{
    constexpr Real minSize = 7.0;
    constexpr Real maxSize = 30.0;
    Real scale = Math::Square(Vector3::Distance(position, viewPosition) / 1000.0f);
    Real radius = minSize + Math::Min<Real>(scale, 1.0f) * (maxSize - minSize);
    bounds = BoundingSphere(position, radius * Scale);
}

void ViewportIconsRenderer::DrawIcons(RenderContext& renderContext, Actor* actor)
{
    auto& view = renderContext.View;
    if (!actor || (view.Flags & ViewFlags::EditorSprites) == ViewFlags::None || QuadModel == nullptr || !QuadModel->IsLoaded())
        return;

    Mesh::DrawInfo draw;
    draw.Lightmap = nullptr;
    draw.LightmapUVs = nullptr;
    draw.Flags = StaticFlags::Transform;
    draw.DrawModes = DrawPass::Forward;
    draw.PerInstanceRandom = 0;
    draw.LODBias = 0;
    draw.ForcedLOD = -1;
    draw.SortOrder = 0;
    draw.VertexColors = nullptr;

    if (const auto scene = SceneObject::Cast<Scene>(actor))
    {
        ViewportIconsRendererService::DrawIcons(renderContext, scene, draw);
    }
    else
    {
        ViewportIconsRendererService::DrawIcons(renderContext, actor, draw);
    }
}

void ViewportIconsRenderer::AddCustomIcon(const ScriptingTypeHandle& type, Texture* iconTexture)
{
    CHECK(type && iconTexture);
    ActorTypeToTexture[type] = iconTexture;
}

void ViewportIconsRenderer::AddActor(Actor* actor)
{
    CHECK(actor && actor->GetScene());
    actor->GetSceneRendering()->AddViewportIcon(actor);
}

void ViewportIconsRenderer::AddActorWithTexture(Actor* actor, Texture* iconTexture)
{
    CHECK(actor && actor->GetScene() && iconTexture);
    ActorToTexture[actor] = iconTexture;
    actor->GetSceneRendering()->AddViewportIcon(actor);
}

void ViewportIconsRenderer::RemoveActor(Actor* actor)
{
    CHECK(actor && actor->GetScene());
    actor->GetSceneRendering()->RemoveViewportIcon(actor);
    ActorToTexture.Remove(actor);
}

void ViewportIconsRendererService::DrawIcons(RenderContext& renderContext, Scene* scene, Mesh::DrawInfo& draw)
{
    auto& view = renderContext.View;
    const BoundingFrustum frustum = view.Frustum;
    const auto& icons = scene->GetSceneRendering()->ViewportIcons;
    Matrix m1, m2, world;
    GeometryDrawStateData drawState;
    draw.DrawState = &drawState;
    draw.Deformation = nullptr;
    draw.World = &world;
    AssetReference<Texture> texture;
    for (Actor* icon : icons)
    {
        BoundingSphere sphere;
        ViewportIconsRenderer::GetBounds(icon->GetPosition() - renderContext.View.Origin, renderContext.View.Position, sphere);
        if (!frustum.Intersects(sphere))
            continue;
        IconTypes iconType;
        ScriptingTypeHandle typeHandle = icon->GetTypeHandle();
        draw.Buffer = nullptr;

        if (ActorToTexture.TryGet(icon, texture) || ActorTypeToTexture.TryGet(typeHandle, texture))
        {
            // Use custom texture
            draw.Buffer = &InstanceBuffers[static_cast<int32>(IconTypes::CustomTexture)];
            if (draw.Buffer->Count() == 0)
            {
                // Lazy-init (use in-built icon material with custom texture)
                draw.Buffer->Setup(1);
                draw.Buffer->At(0).ReceiveDecals = false;
                draw.Buffer->At(0).ShadowsMode = ShadowsCastingMode::None;
            }

            AssetReference<MaterialBase> material;

            if (!TextureToMaterial.TryGet(texture, material))
            {
                // Create custom material per custom texture
                TextureToMaterial[texture] = InstanceBuffers[0][0].Material->CreateVirtualInstance();
                TextureToMaterial[texture]->SetParameterValue(TEXT("Image"), Variant(texture));
                material = TextureToMaterial[texture];
            }

            draw.Buffer->At(0).Material = material;
        }
        else if (ActorTypeToIconType.TryGet(typeHandle, iconType))
        {
            // Use predefined material
            draw.Buffer = &InstanceBuffers[static_cast<int32>(iconType)];
        }

        if (draw.Buffer)
        {
            // Create world matrix
            Matrix::Scaling((float)sphere.Radius * 2.0f, m2);
            Matrix::RotationY(PI, world);
            Matrix::Multiply(m2, world, m1);
            Matrix::Billboard(sphere.Center, view.Position, Vector3::Up, view.Direction, m2);
            Matrix::Multiply(m1, m2, world);

            // Draw icon
            draw.Bounds = sphere;
            QuadModel->Draw(renderContext, draw);
        }
    }
}

void ViewportIconsRendererService::DrawIcons(RenderContext& renderContext, Actor* actor, Mesh::DrawInfo& draw)
{
    if (!actor || !actor->IsActiveInHierarchy())
        return;
    auto& view = renderContext.View;
    const BoundingFrustum frustum = view.Frustum;
    Matrix m1, m2, world;
    BoundingSphere sphere;
    ViewportIconsRenderer::GetBounds(actor->GetPosition() - renderContext.View.Origin, renderContext.View.Position, sphere);
    IconTypes iconType;
    AssetReference<Texture> texture;

    if (frustum.Intersects(sphere) && ActorTypeToIconType.TryGet(actor->GetTypeHandle(), iconType))
    {
        // Create world matrix
        Matrix::Scaling((float)sphere.Radius * 2.0f, m2);
        Matrix::RotationY(PI, world);
        Matrix::Multiply(m2, world, m1);
        Matrix::Billboard(sphere.Center, view.Position, Vector3::Up, view.Direction, m2);
        Matrix::Multiply(m1, m2, world);

        // Draw icon
        GeometryDrawStateData drawState;
        draw.DrawState = &drawState;
        draw.Deformation = nullptr;

        // Support custom icons through types, but not ones that were added through actors, since they cant register while in prefab view anyway
        if (ActorTypeToTexture.TryGet(actor->GetTypeHandle(), texture))
        {
            // Use custom texture
            draw.Buffer = &InstanceBuffers[static_cast<int32>(IconTypes::CustomTexture)];
            if (draw.Buffer->Count() == 0)
            {
                // Lazy-init (use in-built icon material with custom texture)
                draw.Buffer->Setup(1);
                draw.Buffer->At(0).ReceiveDecals = false;
                draw.Buffer->At(0).ShadowsMode = ShadowsCastingMode::None;
            }

            AssetReference<MaterialBase> material;

            if (!TextureToMaterial.TryGet(texture, material))
            {
                // Create custom material per custom texture
                TextureToMaterial[texture] = InstanceBuffers[0][0].Material->CreateVirtualInstance();
                TextureToMaterial[texture]->SetParameterValue(TEXT("Image"), Variant(texture));
                material = TextureToMaterial[texture];
            }

            draw.Buffer->At(0).Material = material;
        }
        else
        {
            // Use predefined material
            draw.Buffer = &InstanceBuffers[static_cast<int32>(iconType)];
        }

        draw.World = &world;
        draw.Bounds = sphere;
        QuadModel->Draw(renderContext, draw);
    }

    for (auto child : actor->Children)
        DrawIcons(renderContext, child, draw);
}

bool ViewportIconsRendererService::Init()
{
    QuadModel = Content::LoadAsyncInternal<Model>(TEXT("Engine/Models/Quad"));
#define INIT(type, path) \
    InstanceBuffers[static_cast<int32>(IconTypes::type)].Setup(1); \
    InstanceBuffers[static_cast<int32>(IconTypes::type)][0].ReceiveDecals = false; \
    InstanceBuffers[static_cast<int32>(IconTypes::type)][0].ShadowsMode = ShadowsCastingMode::None; \
    InstanceBuffers[static_cast<int32>(IconTypes::type)][0].Material = Content::LoadAsyncInternal<MaterialInstance>(TEXT(path))
    INIT(PointLight, "Editor/Icons/PointLight");
    INIT(DirectionalLight, "Editor/Icons/DirectionalLight");
    INIT(EnvironmentProbe, "Editor/Icons/EnvironmentProbe");
    INIT(Skybox, "Editor/Icons/Skybox");
    INIT(SkyLight, "Editor/Icons/SkyLight");
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
    MAP_TYPE(SkyLight, SkyLight);
    MAP_TYPE(SpotLight, PointLight);
    MAP_TYPE(VideoPlayer, SceneAnimationPlayer);
#undef MAP_TYPE

    return false;
}

void ViewportIconsRendererService::Dispose()
{
    QuadModel = nullptr;
    CustomTextureMaterial = nullptr;
    for (int32 i = 0; i < ARRAY_COUNT(InstanceBuffers); i++)
        InstanceBuffers[i].Release();
    ActorTypeToIconType.Clear();
    ActorToTexture.Clear();
    TextureToMaterial.Clear();
}
