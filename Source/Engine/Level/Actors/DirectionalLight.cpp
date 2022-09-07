// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "DirectionalLight.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/SceneRendering.h"

DirectionalLight::DirectionalLight(const SpawnParams& params)
    : LightWithShadow(params)
{
    _drawNoCulling = 1;
    Brightness = 8.0f;
}

void DirectionalLight::Draw(RenderContext& renderContext)
{
    float brightness = Brightness;
    AdjustBrightness(renderContext.View, brightness);
    const Float3 position = GetPosition() - renderContext.View.Origin;
    if (Brightness > ZeroTolerance
        && (renderContext.View.Flags & ViewFlags::DirectionalLights) != 0
        && renderContext.View.Pass & DrawPass::GBuffer
        && (ViewDistance < ZeroTolerance || Float3::DistanceSquared(renderContext.View.Position, position) < ViewDistance * ViewDistance))
    {
        RendererDirectionalLightData data;
        data.Position = position;
        data.MinRoughness = MinRoughness;
        data.ShadowsDistance = ShadowsDistance;
        data.Color = Color.ToFloat3() * (Color.A * brightness);
        data.ShadowsStrength = ShadowsStrength;
        data.Direction = GetDirection();
        data.ShadowsFadeDistance = ShadowsFadeDistance;
        data.ShadowsNormalOffsetScale = ShadowsNormalOffsetScale;
        data.ShadowsDepthBias = ShadowsDepthBias;
        data.ShadowsSharpness = ShadowsSharpness;
        data.VolumetricScatteringIntensity = VolumetricScatteringIntensity;
        data.IndirectLightingIntensity = IndirectLightingIntensity;
        data.CastVolumetricShadow = CastVolumetricShadow;
        data.RenderedVolumetricFog = 0;
        data.ShadowsMode = ShadowsMode;
        data.CascadeCount = CascadeCount;
        data.ContactShadowsLength = ContactShadowsLength;
        data.StaticFlags = GetStaticFlags();
        data.ID = GetID();
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

bool DirectionalLight::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return false;
}

void DirectionalLight::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
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
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    LightWithShadow::OnDisable();
}

void DirectionalLight::OnTransformChanged()
{
    // Base
    LightWithShadow::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
