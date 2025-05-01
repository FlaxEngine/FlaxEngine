// Copyright (c) Wojciech Figat. All rights reserved.

#include "Skybox.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Content.h"

Skybox::Skybox(const SpawnParams& params)
    : Actor(params)
{
    _drawNoCulling = 1;
    _drawCategory = SceneRendering::PreRender;
}

void Skybox::setupProxy()
{
    if (_proxyMaterial == nullptr)
    {
        // Create virtual material instance for default engine skybox material
        _proxyMaterial = Content::CreateVirtualAsset<MaterialInstance>();
        _proxyMaterial->SetBaseMaterial(Content::LoadAsyncInternal<Material>(TEXT("Engine/SkyboxMaterial")));
    }
}

void Skybox::Draw(RenderContext& renderContext)
{
    bool isReady;
    if (CustomMaterial)
    {
        isReady = CustomMaterial->IsLoaded() && CustomMaterial->IsSurface() && EnumHasAnyFlags(CustomMaterial->GetDrawModes(), DrawPass::GBuffer);
    }
    else
    {
        setupProxy();
        isReady = _proxyMaterial && _proxyMaterial->IsReady();
    }
    if (isReady && EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::Sky))
    {
        renderContext.List->Sky = this;
    }
}

void Skybox::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Skybox);

    SERIALIZE(Color);
    SERIALIZE(Exposure);
    SERIALIZE(CubeTexture);
    SERIALIZE(PanoramicTexture);
    SERIALIZE(CustomMaterial);
}

void Skybox::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(Color);
    DESERIALIZE(Exposure);
    DESERIALIZE(CubeTexture);
    DESERIALIZE(PanoramicTexture);
    DESERIALIZE(CustomMaterial);
}

bool Skybox::HasContentLoaded() const
{
    if (CustomMaterial)
        return CustomMaterial->IsLoaded();
    if (CubeTexture)
        return CubeTexture->IsLoaded();
    if (PanoramicTexture)
        return PanoramicTexture->IsLoaded();
    return true;
}

bool Skybox::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return false;
}

bool Skybox::IsDynamicSky() const
{
    return !IsStatic();
}

float Skybox::GetIndirectLightingIntensity() const
{
    return 1.0f;
}

void Skybox::ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world)
{
    // Prepare mock draw call data
    DrawCall drawCall;
    drawCall.World = world;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.ObjectRadius = (float)_sphere.Radius;
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
    drawCall.PerInstanceRandom = GetPerInstanceRandom();
    MaterialBase::BindParameters bindParams(context, renderContext, drawCall);
    bindParams.BindViewData();
    bindParams.BindDrawData();

    // Check if use custom material
    if (CustomMaterial)
    {
        CustomMaterial->Bind(bindParams);
    }
    else
    {
        setupProxy();
        auto material = _proxyMaterial.Get();
        if (material && material->IsReady())
        {
            // TODO: optimize materials binding -> cache them
            material->SetParameterValue(TEXT("CubeTexture"), CubeTexture.Get(), false);
            material->SetParameterValue(TEXT("PanoramicTexture"), PanoramicTexture.Get(), false);
            material->SetParameterValue(TEXT("Color"), Color * Math::Exp2(Exposure), false);
            material->SetParameterValue(TEXT("IsPanoramic"), PanoramicTexture != nullptr, false);
            material->Bind(bindParams);
        }
    }
}

void Skybox::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    Actor::OnEnable();
}

void Skybox::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void Skybox::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
