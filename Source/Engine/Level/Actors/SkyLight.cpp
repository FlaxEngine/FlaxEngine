// Copyright (c) Wojciech Figat. All rights reserved.

#include "SkyLight.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/ProbesRenderer.h"
#include "Engine/Content/Content.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Level/Scene/Scene.h"

SkyLight::SkyLight(const SpawnParams& params)
    : Light(params)
    , _radius(1000000.0f)
{
    _drawNoCulling = 1;
    Brightness = 2.0f;
    UpdateBounds();
}

void SkyLight::SetRadius(float value)
{
    value = Math::Max(0.0f, value);
    if (value == _radius)
        return;

    _radius = value;
    UpdateBounds();
}

float SkyLight::GetScaledRadius() const
{
    return _radius * _transform.Scale.MaxValue();
}

CubeTexture* SkyLight::GetSource() const
{
    if (Mode == Modes::CaptureScene)
        return _bakedProbe;
    if (Mode == Modes::CustomTexture)
        return CustomTexture.Get();
    return nullptr;
}

void SkyLight::Bake(float timeout)
{
    ProbesRenderer::Bake(this, timeout);
}

void SkyLight::SetProbeData(TextureData& data)
{
    // Validate input data
    ASSERT(data.GetArraySize() == 6);

    // Check if was using custom probe
    if (Mode == Modes::CustomTexture)
    {
        // Set
        Mode = Modes::CaptureScene;
        _bakedProbe = nullptr;
    }

    Guid id = Guid::New();

#if COMPILE_WITH_ASSETS_IMPORTER
    // Create asset file
    const String path = GetScene()->GetDataFolderPath() / TEXT("SkyLights/") / GetID().ToString(Guid::FormatType::N) + ASSET_FILES_EXTENSION_WITH_DOT;
    AssetInfo info;
    if (FileSystem::FileExists(path) && Content::GetAssetInfo(path, info))
        id = info.ID;
    if (AssetsImportingManager::Create(AssetsImportingManager::CreateCubeTextureTag, path, id, &data))
    {
        LOG(Error, "Cannot import generated sky light!");
        return;
    }
#else
    // TODO: create or reuse virtual texture and use it
    LOG(Error, "Changing probes at runtime in game is not supported.");
    return;
#endif

    // Check if has loaded probe and it has different ID
    if (_bakedProbe && _bakedProbe->GetID() != id)
    {
        const auto prevId = _bakedProbe->GetID();
        _bakedProbe = nullptr;
        LOG(Warning, "New sku light cube texture has different ID={0} than old one={1}.", id, prevId);
    }

    // Link probe texture
    _bakedProbe = Content::LoadAsync<CubeTexture>(id);
}

void SkyLight::UpdateBounds()
{
    _sphere = BoundingSphere(GetPosition(), GetScaledRadius());
    BoundingBox::FromSphere(_sphere, _box);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Bounds);
}

void SkyLight::Draw(RenderContext& renderContext)
{
    float brightness = Brightness;
    AdjustBrightness(renderContext.View, brightness);
    const Float3 position = GetPosition() - renderContext.View.Origin;
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::SkyLights)
        && EnumHasAnyFlags(renderContext.View.Pass, DrawPass::GBuffer)
        && brightness > ZeroTolerance
        && (ViewDistance < ZeroTolerance || Vector3::DistanceSquared(renderContext.View.Position, position) < ViewDistance * ViewDistance))
    {
        RenderSkyLightData data;
        data.Position = position;
        data.Color = Color.ToFloat3() * (Color.A * brightness);
        data.VolumetricScatteringIntensity = VolumetricScatteringIntensity;
        data.CastVolumetricShadow = CastVolumetricShadow;
        data.AdditiveColor = AdditiveColor.ToFloat3() * (AdditiveColor.A * brightness);
        data.IndirectLightingIntensity = IndirectLightingIntensity;
        data.Radius = GetScaledRadius();
        data.Image = GetSource();
        data.StaticFlags = GetStaticFlags();
        data.ID = GetID();
        data.ScreenSize = Math::Min(1.0f, Math::Sqrt(RenderTools::ComputeBoundsScreenRadiusSquared(position, (float)_sphere.Radius, renderContext.View)));
        renderContext.List->SkyLights.Add(data);
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void SkyLight::OnDebugDrawSelected()
{
    // Draw influence range
    DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::AliceBlue, 0, true);

    // Base
    Light::OnDebugDrawSelected();
}

#endif

void SkyLight::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Light::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SkyLight);

    SERIALIZE(AdditiveColor);
    SERIALIZE_MEMBER(Radius, _radius);
    SERIALIZE(SkyDistanceThreshold);
    SERIALIZE_MEMBER(BakedProbe, _bakedProbe);
    SERIALIZE(Mode);
    SERIALIZE(CustomTexture);
}

void SkyLight::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Light::Deserialize(stream, modifier);

    DESERIALIZE(AdditiveColor);
    DESERIALIZE_MEMBER(Radius, _radius);
    DESERIALIZE(SkyDistanceThreshold);
    DESERIALIZE_MEMBER(BakedProbe, _bakedProbe);
    DESERIALIZE(Mode);
    DESERIALIZE(CustomTexture);
}

bool SkyLight::HasContentLoaded() const
{
    if (Mode == Modes::CaptureScene)
        return _bakedProbe == nullptr || _bakedProbe->IsLoaded();
    if (Mode == Modes::CustomTexture)
        return CustomTexture == nullptr || CustomTexture->IsLoaded();
    return true;
}

void SkyLight::OnTransformChanged()
{
    // Base
    Light::OnTransformChanged();

    UpdateBounds();
}
