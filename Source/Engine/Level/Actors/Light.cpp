// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Light.h"
#include "../Scene/Scene.h"
#if USE_EDITOR
#include "Engine/Graphics/RenderView.h"
#endif

Light::Light(const SpawnParams& params)
    : Actor(params)
{
    _drawCategory = SceneRendering::PreRender;
}

void Light::AdjustBrightness(const RenderView& view, float& brightness) const
{
    // Indirect light scale during baking
#if USE_EDITOR
    brightness *= IsRunningRadiancePass && view.IsOfflinePass ? IndirectLightingIntensity * GetScene()->Info.LightmapSettings.IndirectLightingIntensity : 1.0f;
#endif
}

void Light::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
    GetSceneRendering()->AddLightsDebug<Light, &Light::DrawLightsDebug>(this);
#endif

    // Base
    Actor::OnEnable();
}

void Light::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
    GetSceneRendering()->RemoveLightsDebug<Light, &Light::DrawLightsDebug>(this);
#endif
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

#if USE_EDITOR
void Light::DrawLightsDebug(RenderView& view)
{
}
#endif

LightWithShadow::LightWithShadow(const SpawnParams& params)
    : Light(params)
{
}
