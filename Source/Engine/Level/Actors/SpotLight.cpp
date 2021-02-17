// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SpotLight.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Content/Assets/IESProfile.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/SceneRendering.h"

SpotLight::SpotLight(const SpawnParams& params)
    : LightWithShadow(params)
    , _radius(1000.0f)
    , _outerConeAngle(43.0f)
    , _innerConeAngle(10.0f)
{
    CastVolumetricShadow = false;
    ShadowsDistance = 2000.0f;
    ShadowsFadeDistance = 100.0f;
    ShadowsDepthBias = 0.5f;
    UpdateBounds();
}

float SpotLight::ComputeBrightness() const
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

float SpotLight::GetScaledRadius() const
{
    return _radius * _transform.Scale.MaxValue();
}

void SpotLight::SetRadius(float value)
{
    value = Math::Max(0.0f, value);
    if (Math::NearEqual(value, _radius))
        return;

    _radius = value;
    UpdateBounds();
}

void SpotLight::SetOuterConeAngle(float value)
{
    // Clamp value
    value = Math::Clamp(value, 0.0f, 89.0f);

    // Check if value will change
    if (!Math::NearEqual(value, _outerConeAngle))
    {
        // Change values
        _innerConeAngle = Math::Min(_innerConeAngle, value - ZeroTolerance);
        _outerConeAngle = value;

        UpdateBounds();
    }
}

void SpotLight::SetInnerConeAngle(float value)
{
    // Clamp value
    value = Math::Clamp(value, 0.0f, 89.0f);

    // Check if value will change
    if (!Math::NearEqual(value, _innerConeAngle))
    {
        // Change values
        _innerConeAngle = value;
        _outerConeAngle = Math::Max(_outerConeAngle + ZeroTolerance, value);

        UpdateBounds();
    }
}

void SpotLight::UpdateBounds()
{
    // Cache light direction
    Vector3::Transform(Vector3::Forward, _transform.Orientation, _direction);
    _direction.Normalize();

    // Cache cone angles
    _cosOuterCone = Math::Cos(_outerConeAngle * DegreesToRadians);
    _cosInnerCone = Math::Cos(_innerConeAngle * DegreesToRadians);
    _invCosConeDifference = 1.0f / (_cosInnerCone - _cosOuterCone);

    // Cache bounds
    // Note: we use the law of cosines to find the distance to the furthest edge of the spotlight cone from a position that is halfway down the spotlight direction
    const float radius = GetScaledRadius();
    const float boundsRadius = Math::Sqrt(1.25f * radius * radius - radius * radius * _cosOuterCone);
    _sphere = BoundingSphere(GetPosition() + 0.5f * GetDirection() * radius, boundsRadius);
    BoundingBox::FromSphere(_sphere, _box);
}

void SpotLight::OnEnable()
{
    GetSceneRendering()->AddCommon(this);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    LightWithShadow::OnEnable();
}

void SpotLight::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveCommon(this);

    // Base
    LightWithShadow::OnDisable();
}

void SpotLight::OnTransformChanged()
{
    // Base
    LightWithShadow::OnTransformChanged();

    UpdateBounds();
}

void SpotLight::Draw(RenderContext& renderContext)
{
    float brightness = ComputeBrightness();
    AdjustBrightness(renderContext.View, brightness);
    const float radius = GetScaledRadius();
    const float outerConeAngle = GetOuterConeAngle();
    if ((renderContext.View.Flags & ViewFlags::SpotLights) != 0
        && brightness > ZeroTolerance
        && radius > ZeroTolerance
        && outerConeAngle > ZeroTolerance
        && (ViewDistance < ZeroTolerance || Vector3::DistanceSquared(renderContext.View.Position, GetPosition()) < ViewDistance * ViewDistance))
    {
        RendererSpotLightData data;
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
        data.CosOuterCone = _cosOuterCone;
        data.InvCosConeDifference = _invCosConeDifference;
        data.ContactShadowsLength = ContactShadowsLength;
        data.IESTexture = IESTexture ? IESTexture->GetTexture() : nullptr;
        Vector3::Transform(Vector3::Up, GetOrientation(), data.UpVector);
        data.OuterConeAngle = outerConeAngle;
        renderContext.List->SpotLights.Add(data);
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void SpotLight::OnDebugDraw()
{
    // Draw source tube
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(GetPosition(), SourceRadius), Color::Orange, 0, true);

    // Base
    LightWithShadow::OnDebugDraw();
}

void SpotLight::OnDebugDrawSelected()
{
    const auto color = Color::Yellow;
    Vector3 right = _transform.GetRight();
    Vector3 up = _transform.GetUp();
    Vector3 forward = GetDirection();
    float radius = GetScaledRadius();
    float discRadius = radius * Math::Tan(_outerConeAngle * DegreesToRadians);
    float falloffDiscRadius = radius * Math::Tan(_innerConeAngle * DegreesToRadians);
    Vector3 position = GetPosition();

    DEBUG_DRAW_LINE(position, position + forward * radius + up * discRadius, color, 0, true);
    DEBUG_DRAW_LINE(position, position + forward * radius - up * discRadius, color, 0, true);
    DEBUG_DRAW_LINE(position, position + forward * radius + right * discRadius, color, 0, true);
    DEBUG_DRAW_LINE(position, position + forward * radius - right * discRadius, color, 0, true);

    DEBUG_DRAW_CIRCLE(position + forward * radius, forward, discRadius, color, 0, true);
    DEBUG_DRAW_CIRCLE(position + forward * radius, forward, falloffDiscRadius, color * 0.6f, 0, true);

    // Base
    LightWithShadow::OnDebugDrawSelected();
}

#endif

void SpotLight::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    LightWithShadow::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SpotLight);

    SERIALIZE_MEMBER(Radius, _radius);
    SERIALIZE_MEMBER(OuterAngle, _outerConeAngle);
    SERIALIZE_MEMBER(InnerAngle, _innerConeAngle);
    SERIALIZE(IESTexture);
    SERIALIZE(SourceRadius);
    SERIALIZE(FallOffExponent);
    SERIALIZE(UseInverseSquaredFalloff);
    SERIALIZE(UseIESBrightness);
    SERIALIZE(IESBrightnessScale);
}

void SpotLight::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    LightWithShadow::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Radius, _radius);
    DESERIALIZE_MEMBER(OuterAngle, _outerConeAngle);
    DESERIALIZE_MEMBER(InnerAngle, _innerConeAngle);
    DESERIALIZE(IESTexture);
    DESERIALIZE(SourceRadius);
    DESERIALIZE(FallOffExponent);
    DESERIALIZE(UseInverseSquaredFalloff);
    DESERIALIZE(UseIESBrightness);
    DESERIALIZE(IESBrightnessScale);
}

bool SpotLight::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return CollisionsHelper::RayIntersectsSphere(ray, _sphere, distance, normal);
}
