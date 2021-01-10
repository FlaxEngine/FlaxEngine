// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "DirectionalLight.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/SceneRendering.h"

DirectionalLight::DirectionalLight(const SpawnParams& params)
    : LightWithShadow(params)
{
    Brightness = 8.0f;
}

void DirectionalLight::Draw(RenderContext& renderContext)
{
    float brightness = Brightness;
    AdjustBrightness(renderContext.View, brightness);
    if (Brightness > ZeroTolerance
        && (renderContext.View.Flags & ViewFlags::DirectionalLights) != 0
        && (ViewDistance < ZeroTolerance || Vector3::DistanceSquared(renderContext.View.Position, GetPosition()) < ViewDistance * ViewDistance))
    {
        RendererDirectionalLightData data;
        data.Position = GetPosition();
        data.MinRoughness = MinRoughness;
        data.ShadowsDistance = ShadowsDistance;
        data.Color = Color.ToVector3() * (Color.A * brightness);
        data.ShadowsStrength = ShadowsStrength;
        data.Direction = GetDirection();
        data.ShadowsFadeDistance = ShadowsFadeDistance;
        data.ShadowsNormalOffsetScale = ShadowsNormalOffsetScale;
        data.ShadowsDepthBias = ShadowsDepthBias;
        data.ShadowsSharpness = ShadowsSharpness;
        data.VolumetricScatteringIntensity = VolumetricScatteringIntensity;
        data.CastVolumetricShadow = CastVolumetricShadow;
        data.RenderedVolumetricFog = 0;
        data.ShadowsMode = ShadowsMode;
        data.CascadeCount = CascadeCount;
        data.ContactShadowsLength = ContactShadowsLength;
        renderContext.List->DirectionalLights.Add(data);
    }
}

void DirectionalLight::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    LightWithShadow::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(DirectionalLight);

    SERIALIZE(CascadeCount);
}

void DirectionalLight::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    LightWithShadow::Deserialize(stream, modifier);

    DESERIALIZE(CascadeCount);
}

bool DirectionalLight::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return false;
}

void DirectionalLight::OnEnable()
{
    GetSceneRendering()->AddCommonNoCulling(this);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    LightWithShadow::OnEnable();
}

void DirectionalLight::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveCommonNoCulling(this);

    // Base
    LightWithShadow::OnDisable();
}

void DirectionalLight::OnTransformChanged()
{
    // Base
    LightWithShadow::OnTransformChanged();

    _box = BoundingBox(_transform.Translation, _transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
