// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "PointLight.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/SceneRendering.h"

PointLight::PointLight(const SpawnParams& params)
    : LightWithShadow(params)
    , _radius(1000.0f)
{
    CastVolumetricShadow = false;
    ShadowsDistance = 2000.0f;
    ShadowsFadeDistance = 100.0f;
    ShadowsDepthBias = 0.5f;
    UpdateBounds();
}

float PointLight::ComputeBrightness() const
{
    float result = Brightness;

    if (IESTexture)
    {
        if (UseIESBrightness)
        {
            result = IESTexture->Brightness * IESBrightnessScale;
        }

        result *= IESTexture->TextureMultiplier;
    }

    //if (UseInverseSquaredFalloff)
    //	result *= 16.0f;

    return result;
}

float PointLight::GetScaledRadius() const
{
    return _radius * _transform.Scale.MaxValue();
}

void PointLight::SetRadius(float value)
{
    value = Math::Max(0.0f, value);
    if (Math::NearEqual(value, _radius))
        return;

    _radius = value;
    UpdateBounds();
}

void PointLight::UpdateBounds()
{
    // Cache light direction
    Vector3::Transform(Vector3::Forward, _transform.Orientation, _direction);
    _direction.Normalize();

    // Cache bounding box
    _sphere = BoundingSphere(GetPosition(), GetScaledRadius());
    BoundingBox::FromSphere(_sphere, _box);
}

void PointLight::OnEnable()
{
    GetSceneRendering()->AddCommon(this);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    LightWithShadow::OnEnable();
}

void PointLight::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveCommon(this);

    // Base
    LightWithShadow::OnDisable();
}

void PointLight::OnTransformChanged()
{
    // Base
    LightWithShadow::OnTransformChanged();

    UpdateBounds();
}

void PointLight::Draw(RenderContext& renderContext)
{
    float brightness = ComputeBrightness();
    AdjustBrightness(renderContext.View, brightness);
    const float radius = GetScaledRadius();
    if ((renderContext.View.Flags & ViewFlags::PointLights) != 0
        && brightness > ZeroTolerance
        && radius > ZeroTolerance
        && (ViewDistance < ZeroTolerance || Vector3::DistanceSquared(renderContext.View.Position, GetPosition()) < ViewDistance * ViewDistance))
    {
        RendererPointLightData data;
        data.Position = GetPosition();
        data.MinRoughness = MinRoughness;
        data.ShadowsDistance = ShadowsDistance;
        data.Color = Color.ToVector3() * (Color.A * brightness);
        data.ShadowsStrength = ShadowsStrength;
        data.Direction = _direction;
        data.ShadowsFadeDistance = ShadowsFadeDistance;
        data.ShadowsNormalOffsetScale = ShadowsNormalOffsetScale;
        data.ShadowsDepthBias = ShadowsDepthBias;
        data.ShadowsSharpness = ShadowsSharpness;
        data.VolumetricScatteringIntensity = VolumetricScatteringIntensity;
        data.CastVolumetricShadow = CastVolumetricShadow;
        data.RenderedVolumetricFog = 0;
        data.ShadowsMode = ShadowsMode;
        data.Radius = radius;
        data.FallOffExponent = FallOffExponent;
        data.UseInverseSquaredFalloff = UseInverseSquaredFalloff;
        data.SourceRadius = SourceRadius;
        data.SourceLength = SourceLength;
        data.ContactShadowsLength = ContactShadowsLength;
        data.IESTexture = IESTexture ? IESTexture->GetTexture() : nullptr;
        renderContext.List->PointLights.Add(data);
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void PointLight::OnDebugDraw()
{
    // Draw source tube
    DEBUG_DRAW_WIRE_TUBE(GetPosition(), GetOrientation(), SourceRadius, SourceLength, Color::Orange, 0, true);

    // Base
    LightWithShadow::OnDebugDraw();
}

void PointLight::OnDebugDrawSelected()
{
    // Draw influence range
    DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::Yellow, 0, true);

    // Base
    LightWithShadow::OnDebugDrawSelected();
}

#endif

void PointLight::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    LightWithShadow::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(PointLight);

    SERIALIZE_MEMBER(Radius, _radius);
    SERIALIZE(IESTexture);
    SERIALIZE(SourceRadius);
    SERIALIZE(SourceLength);
    SERIALIZE(FallOffExponent);
    SERIALIZE(UseInverseSquaredFalloff);
    SERIALIZE(UseIESBrightness);
    SERIALIZE(IESBrightnessScale);
}

void PointLight::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    LightWithShadow::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Radius, _radius);
    DESERIALIZE(IESTexture);
    DESERIALIZE(SourceRadius);
    DESERIALIZE(SourceLength);
    DESERIALIZE(FallOffExponent);
    DESERIALIZE(UseInverseSquaredFalloff);
    DESERIALIZE(UseIESBrightness);
    DESERIALIZE(IESBrightnessScale);
}

bool PointLight::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return CollisionsHelper::RayIntersectsSphere(ray, _sphere, distance, normal);
}
