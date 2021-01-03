// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Light.h"
#include "../Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"

Light::Light(const SpawnParams& params)
    : Actor(params)
{
}

void Light::AdjustBrightness(const RenderView& view, float& brightness) const
{
    // Indirect light scale during baking
#if USE_EDITOR
    brightness *= IsRunningRadiancePass && view.IsOfflinePass ? IndirectLightingIntensity * GetScene()->Info.LightmapSettings.IndirectLightingIntensity : 1.0f;
#endif
}

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
}
