// Copyright (c) Wojciech Figat. All rights reserved.

#include "Light.h"
#include "../Scene/Scene.h"
#if USE_EDITOR
#include "Engine/Graphics/RenderView.h"
#endif
#include "Engine/Serialization/Serialization.h"

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

void Light::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Light);

    SERIALIZE(Color);
    SERIALIZE(Brightness);
    SERIALIZE(ViewDistance);
    SERIALIZE(IndirectLightingIntensity);
    SERIALIZE(VolumetricScatteringIntensity);
    SERIALIZE(CastVolumetricShadow);
}

void Light::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(Color);
    DESERIALIZE(Brightness);
    DESERIALIZE(ViewDistance);
    DESERIALIZE(IndirectLightingIntensity);
    DESERIALIZE(VolumetricScatteringIntensity);
    DESERIALIZE(CastVolumetricShadow);
}

LightWithShadow::LightWithShadow(const SpawnParams& params)
    : Light(params)
{
}

void LightWithShadow::InvalidateShadow()
{
    _invalidateShadowFrame++;
}

void LightWithShadow::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Light::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(LightWithShadow);

    SERIALIZE(MinRoughness);
    SERIALIZE(ShadowsDistance);
    SERIALIZE(ShadowsFadeDistance);
    SERIALIZE(ShadowsSharpness);
    SERIALIZE(ShadowsMode);
    SERIALIZE(ShadowsStrength);
    SERIALIZE(ShadowsDepthBias);
    SERIALIZE(ShadowsNormalOffsetScale);
    SERIALIZE(ContactShadowsLength);
    SERIALIZE(ShadowsUpdateRate);
    SERIALIZE(ShadowsUpdateRateAtDistance);
}

void LightWithShadow::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Light::Deserialize(stream, modifier);

    DESERIALIZE(MinRoughness);
    DESERIALIZE(ShadowsDistance);
    DESERIALIZE(ShadowsFadeDistance);
    DESERIALIZE(ShadowsSharpness);
    DESERIALIZE(ShadowsMode);
    DESERIALIZE(ShadowsStrength);
    DESERIALIZE(ShadowsDepthBias);
    DESERIALIZE(ShadowsNormalOffsetScale);
    DESERIALIZE(ContactShadowsLength);
    DESERIALIZE(ShadowsUpdateRate);
    DESERIALIZE(ShadowsUpdateRateAtDistance);
}
