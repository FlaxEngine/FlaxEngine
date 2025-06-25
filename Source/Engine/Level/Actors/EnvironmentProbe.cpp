// Copyright (c) Wojciech Figat. All rights reserved.

#include "EnvironmentProbe.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/ProbesRenderer.h"
#include "Engine/Renderer/ReflectionsPass.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Deprecated.h"
#include "Engine/ContentExporters/AssetExporters.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Scene/Scene.h"

EnvironmentProbe::EnvironmentProbe(const SpawnParams& params)
    : Actor(params)
    , _radius(3000.0f)
    , _isUsingCustomProbe(false)
{
    _drawCategory = SceneRendering::PreRender;
    _sphere = BoundingSphere(Vector3::Zero, _radius);
    BoundingBox::FromSphere(_sphere, _box);
}

EnvironmentProbe::~EnvironmentProbe()
{
    SAFE_DELETE_GPU_RESOURCE(_probeTexture);
}

float EnvironmentProbe::GetRadius() const
{
    return _radius;
}

void EnvironmentProbe::SetRadius(float value)
{
    value = Math::Max(0.0f, value);
    if (value == _radius)
        return;

    _radius = value;
    UpdateBounds();
}

float EnvironmentProbe::GetScaledRadius() const
{
    return _radius * _transform.Scale.MaxValue();
}

GPUTexture* EnvironmentProbe::GetProbe() const
{
    return _probe ? _probe->GetTexture() : _probeTexture;
}

bool EnvironmentProbe::IsUsingCustomProbe() const
{
    return _isUsingCustomProbe;
}

CubeTexture* EnvironmentProbe::GetCustomProbe() const
{
    return _isUsingCustomProbe ? _probe : nullptr;
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
    ProbesRenderer::Bake(this, timeout);
}

void EnvironmentProbe::SetProbeData(GPUContext* context, GPUTexture* data)
{
    // Remove probe asset (if used)
    _isUsingCustomProbe = false;
    _probe = nullptr;

    // Allocate probe texture manually
    if (!_probeTexture)
    {
        _probeTexture = GPUTexture::New();
#if !BUILD_RELEASE
        _probeTexture->SetName(GetNamePath());
#endif
    }
    if (_probeTexture->Width() != data->Width() || _probeTexture->Format() != data->Format())
    {
        auto desc = data->GetDescription();
        desc.Usage = GPUResourceUsage::Default;
        desc.Flags = GPUTextureFlags::ShaderResource;
        if (_probeTexture->Init(desc))
            return;
        _probeTexture->SetResidentMipLevels(_probeTexture->MipLevels());
    }

    // Copy probe texture data
    context->CopyResource(_probeTexture, data);
}

void EnvironmentProbe::SetProbeData(TextureData& data)
{
    // Remove custom probe (if used)
    if (_isUsingCustomProbe)
    {
        _isUsingCustomProbe = false;
        _probe = nullptr;
    }

    // Remove probe texture (if used)
    SAFE_DELETE_GPU_RESOURCE(_probeTexture);

#if COMPILE_WITH_ASSETS_IMPORTER
    // Create asset file
    const String path = GetScene()->GetDataFolderPath() / TEXT("EnvProbes/") / GetID().ToString(Guid::FormatType::N) + ASSET_FILES_EXTENSION_WITH_DOT;
    AssetInfo info;
    Guid id = Guid::New();
    if (FileSystem::FileExists(path) && Content::GetAssetInfo(path, info))
        id = info.ID;
    if (AssetsImportingManager::Create(AssetsImportingManager::CreateCubeTextureTag, path, id, &data))
    {
        LOG(Error, "Cannot import generated env probe!");
        return;
    }

    // Check if has loaded probe and it has different ID
    if (_probe && _probe->GetID() != id)
    {
        const auto prevId = _probe->GetID();
        _probe = nullptr;
        LOG(Warning, "New env probe cube texture has different ID={0} than old one={1}.", id, prevId);
    }

    // Link probe texture
    _probe = Content::LoadAsync<CubeTexture>(id);
#else
    // Create virtual asset
    if (!_probe || !_probe->IsVirtual())
        _probe = Content::CreateVirtualAsset<CubeTexture>();
    auto initData = New<TextureBase::InitData>();
    initData->FromTextureData(data);
    if (_probe->Init(initData))
    {
        Delete(initData);
        LOG(Error, "Cannot load generated env probe!");
        return;
    }
#endif
}

void EnvironmentProbe::UpdateBounds()
{
    _sphere = BoundingSphere(GetPosition(), GetScaledRadius());
    BoundingBox::FromSphere(_sphere, _box);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Bounds);
}

void EnvironmentProbe::Draw(RenderContext& renderContext)
{
    if (Brightness > ZeroTolerance &&
        EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::Reflections) &&
        EnumHasAnyFlags(renderContext.View.Pass, DrawPass::GBuffer))
    {
        // Size culling
        const Float3 position = _sphere.Center - renderContext.View.Origin;
        const float radius = GetScaledRadius();
        const float drawMinScreenSize = 0.02f;
        const auto lodView = (renderContext.LodProxyView ? renderContext.LodProxyView : &renderContext.View);
        const float screenRadiusSquared = RenderTools::ComputeBoundsScreenRadiusSquared(position, radius, *lodView) * renderContext.View.ModelLODDistanceFactorSqrt;
        if (Math::Square(drawMinScreenSize * 0.5f) > screenRadiusSquared)
            return;

        // Realtime probe update
        if (UpdateMode == ProbeUpdateMode::Realtime)
            ProbesRenderer::Bake(this, 0.0f);

        GPUTexture* texture = GetProbe();
        if (texture)
        {
            RenderEnvironmentProbeData data;
            data.Texture = texture;
            data.Position = position;
            data.Radius = radius;
            data.Brightness = Brightness;
            data.SortOrder = SortOrder;
            data.HashID = GetHash(_id);
            renderContext.List->EnvironmentProbes.Add(data);
        }
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
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Layer);
}

void EnvironmentProbe::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(EnvironmentProbe);

    SERIALIZE_MEMBER(Radius, _radius);
    SERIALIZE(CubemapResolution);
    SERIALIZE(Brightness);
    SERIALIZE(SortOrder);
    SERIALIZE(UpdateMode);
    SERIALIZE(CaptureNearPlane);
    SERIALIZE_MEMBER(IsCustomProbe, _isUsingCustomProbe);
    SERIALIZE_MEMBER(ProbeID, _probe);
}

void EnvironmentProbe::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Radius, _radius);
    DESERIALIZE(CubemapResolution);
    DESERIALIZE(Brightness);
    DESERIALIZE(SortOrder);
    DESERIALIZE(UpdateMode);
    DESERIALIZE(CaptureNearPlane);
    DESERIALIZE_MEMBER(IsCustomProbe, _isUsingCustomProbe);
    DESERIALIZE_MEMBER(ProbeID, _probe);

    // [Deprecated on 18.07.2022, expires on 18.07.2022]
    if (modifier->EngineBuild <= 6332)
    {
        MARK_CONTENT_DEPRECATED();
        const auto member = stream.FindMember("AutoUpdate");
        if (member != stream.MemberEnd() && member->value.IsBool() && member->value.GetBool())
            UpdateMode = ProbeUpdateMode::WhenMoved;
    }
}

bool EnvironmentProbe::HasContentLoaded() const
{
    return _probe == nullptr || _probe->IsLoaded();
}

bool EnvironmentProbe::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return CollisionsHelper::RayIntersectsSphere(ray, _sphere, distance, normal);
}

void EnvironmentProbe::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
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
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void EnvironmentProbe::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    UpdateBounds();

    if (IsActiveInHierarchy() && IsDuringPlay() && UpdateMode == ProbeUpdateMode::WhenMoved)
    {
        Bake(1.0f);
    }
}
