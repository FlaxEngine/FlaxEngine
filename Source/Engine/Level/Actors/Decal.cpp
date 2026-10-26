// Copyright (c) Wojciech Figat. All rights reserved.

#include "Decal.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderContext.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/GPUPipelineCache.h"
#if COMPILE_WITH_PSO_PRECACHING
#include "Engine/Renderer/GBufferPass.h"
#endif

Decal::Decal(const SpawnParams& params)
    : Actor(params)
    , _size(100.0f)
{
    _drawCategory = SceneRendering::PreRender;
    _bounds.Extents = _size * 0.5f;
    _bounds.Transformation = _transform;
    _bounds.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}

Decal::~Decal()
{
    GPU_PSO_WAIT(_precacheHandle);
}

void Decal::SetSize(const Vector3& value)
{
    const auto v = value.GetAbsolute();
    if (v != _size)
    {
        _size = v;
        _bounds.Extents = v * 0.5f;
        _bounds.GetBoundingBox(_box);
        BoundingSphere::FromBox(_box, _sphere);
    }
}

MaterialInstance* Decal::CreateAndSetVirtualMaterialInstance()
{
    auto material = Material.Get();
    CHECK_RETURN(material && !material->WaitForLoaded(), nullptr);
    const auto result = material->CreateVirtualInstance();
    Material = result;
    return result;
}

void Decal::PrecachePSOBatch(GPUPipelineCacheBatch& batch)
{
#if COMPILE_WITH_PSO_PRECACHING
    auto* material = Material.Get();
    if (!material || material->WaitForLoaded())
        return;
    auto& materialPSO = batch.Materials[material->GetMaterial()];
    materialPSO.Domain = GPUPipelineCacheBatch::PassDomains::Decals;
    materialPSO.DrawModes |= DrawPass::GBuffer;
    materialPSO.VertexLayouts.AddUnique(GBufferPass::Instance()->GetDecalVertexLayout());
#endif
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void Decal::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_BOX(_bounds, Color::BlueViolet, 0, true);

    // Base
    Actor::OnDebugDrawSelected();
}

BoundingBox Decal::GetEditorBoundingBox() const
{
    const Vector3 size(10.0f);
    return BoundingBox(_transform.Translation - size, _transform.Translation + size);
}

#endif

void Decal::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Layer);
}

void Decal::Draw(RenderContextBatch& renderContextBatch)
{
    const RenderContext& renderContext = renderContextBatch.GetMainContext();
    MaterialBase* material = Material;
    if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::Decals) &&
        EnumHasAnyFlags(renderContext.View.Pass, DrawPass::GBuffer) &&
        material &&
        material->IsLoaded() &&
        material->IsDecal())
    {
        // Check if decal is being culled
        const auto lodView = (renderContext.LodProxyView ? renderContext.LodProxyView : &renderContext.View);
        const float screenRadiusSquared = RenderTools::ComputeBoundsScreenRadiusSquared(_sphere.Center - renderContext.View.Origin, (float)_sphere.Radius, *lodView) * renderContext.View.ModelLODDistanceFactorSqrt;
        if (Math::Square(DrawMinScreenSize * 0.5f) > screenRadiusSquared)
            return;
        if (!GPU_PSO_READY(_precacheHandle))
            return;

        RenderDecalData data;
        Transform transform = GetTransform();
        transform.Scale *= _size;
        renderContext.View.GetWorldMatrix(transform, data.World);
        data.SortOrder = SortOrder;
        data.RenderLayersMask = RenderLayersMask;
        data.Material = material;
        renderContext.List->Decals.Add(data);
    }
}

void Decal::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Decal);

    SERIALIZE(Material);
    SERIALIZE_MEMBER(Size, _size);
    SERIALIZE(SortOrder);
    SERIALIZE(RenderLayersMask);
    SERIALIZE(DrawMinScreenSize);
}

void Decal::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(Material);
    DESERIALIZE_MEMBER(Size, _size);
    DESERIALIZE(SortOrder);
    DESERIALIZE(RenderLayersMask);
    DESERIALIZE(DrawMinScreenSize);

    _bounds.Extents = _size * 0.5f;

#if COMPILE_WITH_PSO_PRECACHING
    if (GPU_PSO_PRECACHING(Level) && Material && modifier->LoadDepth == 0)
        PrecachePSO();
#endif
}

bool Decal::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _bounds.Intersects(ray, distance, normal);
}

void Decal::PrecachePSO()
{
#if COMPILE_WITH_PSO_PRECACHING
    GPU_PSO_WAIT(_precacheHandle);
    GPUPipelineCache::BatchCallback f;
    f.Bind<Decal, &Decal::PrecachePSOBatch>(this);
    GPUPipelineCache::PrecacheAsync(f, &_precacheHandle);
#endif
}

void Decal::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    Actor::OnEnable();
}

void Decal::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);

    // Base
    Actor::OnDisable();
}

void Decal::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _bounds.Transformation = _transform;
    _bounds.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);

    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
}
