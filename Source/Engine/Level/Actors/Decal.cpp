// Copyright (c) Wojciech Figat. All rights reserved.

#include "Decal.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"

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

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void Decal::OnDebugDrawSelected()
{
    DEBUG_DRAW_WIRE_BOX(_bounds, Color::BlueViolet, 0, true);

    // Base
    Actor::OnDebugDrawSelected();
}

BoundingBox Decal::GetEditorBox() const
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

void Decal::Draw(RenderContext& renderContext)
{
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

        RenderDecalData data;
        Transform transform = GetTransform();
        transform.Scale *= _size;
        renderContext.View.GetWorldMatrix(transform, data.World);
        data.SortOrder = SortOrder;
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
    SERIALIZE(DrawMinScreenSize);
}

void Decal::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(Material);
    DESERIALIZE_MEMBER(Size, _size);
    DESERIALIZE(SortOrder);
    DESERIALIZE(DrawMinScreenSize);

    _bounds.Extents = _size * 0.5f;
}

bool Decal::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _bounds.Intersects(ray, distance, normal);
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
