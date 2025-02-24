// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "PointLight.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
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
    _direction = Float3::Forward;
    _sphere = BoundingSphere(Vector3::Zero, _radius);
    BoundingBox::FromSphere(_sphere, _box);
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
    Float3::Transform(Float3::Forward, _transform.Orientation, _direction);
    _direction.Normalize();

    // Cache bounding box
    _sphere = BoundingSphere(GetPosition(), GetScaledRadius());
    BoundingBox::FromSphere(_sphere, _box);

    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Bounds);
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
    Float3 position;
    const float radius = GetScaledRadius();
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::PointLights)
        && EnumHasAnyFlags(renderContext.View.Pass, DrawPass::GBuffer)
        && brightness > ZeroTolerance
        && radius > ZeroTolerance
        && CheckViewDistance(renderContext.View.Position, renderContext.View.Origin, position, brightness))
    {
        RenderPointLightData data;
        data.Position = position;
        data.MinRoughness = MinRoughness;
        data.ShadowsDistance = ShadowsDistance;
        data.Color = Color.ToFloat3() * (Color.A * brightness);
        data.ShadowsStrength = ShadowsStrength;
        data.Direction = _direction;
        data.ShadowsFadeDistance = ShadowsFadeDistance;
        data.ShadowsNormalOffsetScale = ShadowsNormalOffsetScale;
        data.ShadowsDepthBias = ShadowsDepthBias;
        data.ShadowsSharpness = ShadowsSharpness;
        data.VolumetricScatteringIntensity = VolumetricScatteringIntensity;
        data.CastVolumetricShadow = CastVolumetricShadow;
        data.ShadowsUpdateRate = ShadowsUpdateRate;
        data.ShadowsUpdateRateAtDistance = ShadowsUpdateRateAtDistance;
        data.ShadowFrame = _invalidateShadowFrame;
        data.ShadowsResolution = (int32)ShadowsResolution;
        data.ShadowsMode = ShadowsMode;
        data.Radius = radius;
        data.FallOffExponent = FallOffExponent;
        data.UseInverseSquaredFalloff = UseInverseSquaredFalloff;
        data.SourceRadius = SourceRadius;
        data.SourceLength = SourceLength;
        data.ContactShadowsLength = ContactShadowsLength;
        data.IndirectLightingIntensity = IndirectLightingIntensity;
        data.IESTexture = IESTexture ? IESTexture->GetTexture() : nullptr;
        data.StaticFlags = GetStaticFlags();
        data.ID = GetID();
        data.ScreenSize = Math::Min(1.0f, Math::Sqrt(RenderTools::ComputeBoundsScreenRadiusSquared(position, (float)_sphere.Radius, renderContext.View)));
        renderContext.List->PointLights.Add(data);
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void PointLight::OnDebugDraw()
{
    if (SourceRadius > ZeroTolerance || SourceLength > ZeroTolerance)
    {
        // Draw source capsule
        DEBUG_DRAW_WIRE_CAPSULE(GetPosition(), GetOrientation(), SourceRadius, SourceLength, Color::Orange, 0, true);
    }

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

void PointLight::DrawLightsDebug(RenderView& view)
{
    const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
    if (!view.CullingFrustum.Intersects(sphere) || !EnumHasAnyFlags(view.Flags, ViewFlags::PointLights))
        return;
    
    // Draw influence range
    DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::Yellow, 0, true);
}

#endif

void PointLight::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Layer);
}

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

bool PointLight::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return CollisionsHelper::RayIntersectsSphere(ray, _sphere, distance, normal);
}
