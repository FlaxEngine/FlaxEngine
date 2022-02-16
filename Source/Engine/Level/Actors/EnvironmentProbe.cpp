// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "EnvironmentProbe.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/ProbesRenderer.h"
#include "Engine/Renderer/ReflectionsPass.h"
#include "Engine/Content/Content.h"
#include "Engine/ContentExporters/AssetExporters.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/Scene.h"

EnvironmentProbe::EnvironmentProbe(const SpawnParams& params)
    : Actor(params)
    , _radius(3000.0f)
    , _isUsingCustomProbe(false)
    , _probe(nullptr)
{
    _sphere = BoundingSphere(Vector3::Zero, _radius);
    BoundingBox::FromSphere(_sphere, _box);
}

float EnvironmentProbe::GetRadius() const
{
    return _radius;
}

void EnvironmentProbe::SetRadius(float value)
{
    value = Math::Max(0.0f, value);
    if (Math::NearEqual(value, _radius))
        return;

    _radius = value;
    UpdateBounds();
}

float EnvironmentProbe::GetScaledRadius() const
{
    return _radius * _transform.Scale.MaxValue();
}

void EnvironmentProbe::SetupProbeData(ProbeData* data) const
{
    const float radius = GetScaledRadius();
    data->Data0 = Vector4(GetPosition(), 0);
    data->Data1 = Vector4(radius, 1.0f / radius, Brightness, 0);
}

CubeTexture* EnvironmentProbe::GetCustomProbe() const
{
    if (IsUsingCustomProbe())
        return GetProbe();
    return nullptr;
}

void EnvironmentProbe::SetCustomProbe(CubeTexture* probe)
{
    // Check if value won't change
    if (probe == _probe)
        return;

    // Set
    _isUsingCustomProbe = probe != nullptr;
    _probe = probe;
}

void EnvironmentProbe::Bake(float timeout)
{
#if COMPILE_WITH_PROBES_BAKING
    ProbesRenderer::Bake(this, timeout);
#else
    LOG(Warning, "Baking probes is not supported.");
#endif
}

void EnvironmentProbe::SetProbeData(TextureData& data)
{
    // Validate input data
    ASSERT(data.GetArraySize() == 6 && data.Height == ENV_PROBES_RESOLUTION && data.Width == ENV_PROBES_RESOLUTION);

    // Check if was using custom probe
    if (_isUsingCustomProbe)
    {
        // Set
        _isUsingCustomProbe = false;
        _probe = nullptr;
    }

    Guid id = Guid::New();

#if COMPILE_WITH_ASSETS_IMPORTER
    // Create asset file
    const String path = GetScene()->GetDataFolderPath() / TEXT("EnvProbes/") / GetID().ToString(Guid::FormatType::N) + ASSET_FILES_EXTENSION_WITH_DOT;
    AssetInfo info;
    if (FileSystem::FileExists(path) && Content::GetAssetInfo(path, info))
        id = info.ID;
    if (AssetsImportingManager::Create(AssetsImportingManager::CreateCubeTextureTag, path, id, &data))
    {
        LOG(Error, "Cannot import generated env probe!");
        return;
    }
#else
    // TODO: create or reuse virtual texture and use it
    LOG(Error, "Changing probes at runtime in game is not supported.");
    return;
#endif

    // Check if has loaded probe and it has different ID
    if (_probe && _probe->GetID() != id)
    {
        const auto prevId = _probe->GetID();
        _probe = nullptr;
        LOG(Warning, "New env probe cube texture has different ID={0} than old one={1}.", id, prevId);
    }

    // Link probe texture
    _probe = Content::LoadAsync<CubeTexture>(id);
}

void EnvironmentProbe::UpdateBounds()
{
    _sphere = BoundingSphere(GetPosition(), GetScaledRadius());
    BoundingBox::FromSphere(_sphere, _box);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateCommon(this, _sceneRenderingKey);
}

void EnvironmentProbe::Draw(RenderContext& renderContext)
{
    if (Brightness > ZeroTolerance && (renderContext.View.Flags & ViewFlags::Reflections) != 0 && HasProbeLoaded())
    {
        renderContext.List->EnvironmentProbes.Add(this);
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void EnvironmentProbe::OnDebugDrawSelected()
{
    // Draw influence range
    DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::CornflowerBlue, 0, true);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void EnvironmentProbe::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateCommon(this, _sceneRenderingKey);
}

void EnvironmentProbe::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(EnvironmentProbe);

    SERIALIZE_MEMBER(Radius, _radius);
    SERIALIZE(Brightness);
    SERIALIZE(AutoUpdate);
    SERIALIZE(CaptureNearPlane);
    SERIALIZE_MEMBER(IsCustomProbe, _isUsingCustomProbe);
    SERIALIZE_MEMBER(ProbeID, _probe);
}

void EnvironmentProbe::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Radius, _radius);
    DESERIALIZE(Brightness);
    DESERIALIZE(AutoUpdate);
    DESERIALIZE(CaptureNearPlane);
    DESERIALIZE_MEMBER(IsCustomProbe, _isUsingCustomProbe);
    DESERIALIZE_MEMBER(ProbeID, _probe);
}

bool EnvironmentProbe::HasContentLoaded() const
{
    return _probe == nullptr || _probe->IsLoaded();
}

bool EnvironmentProbe::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return CollisionsHelper::RayIntersectsSphere(ray, _sphere, distance, normal);
}

void EnvironmentProbe::OnEnable()
{
    _sceneRenderingKey = GetSceneRendering()->AddCommon(this);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    Actor::OnEnable();
}

void EnvironmentProbe::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveCommon(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void EnvironmentProbe::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    UpdateBounds();

    if (AutoUpdate && IsDuringPlay())
    {
        Bake(1.0f);
    }
}
