// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    _world = Matrix::Scaling(_size);
    _bounds.Extents = Vector3::Half;
    _bounds.Transformation = _world;
    _bounds.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}

void Decal::SetSize(const Vector3& value)
{
    const auto v = value.GetAbsolute();
    if (v != _size)
    {
        _size = v;

        Transform t = _transform;
        t.Scale *= _size;
        t.GetWorld(_world);
        _bounds.Extents = Vector3::Half;
        _bounds.Transformation = _world;
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

#endif

void Decal::Draw(RenderContext& renderContext)
{
    if ((renderContext.View.Flags & ViewFlags::Decals) != 0 &&
        Material &&
        Material->IsLoaded() &&
        Material->IsDecal())
    {
        const auto lodView = (renderContext.LodProxyView ? renderContext.LodProxyView : &renderContext.View);
        const float screenRadiusSquared = RenderTools::ComputeBoundsScreenRadiusSquared(_sphere.Center, _sphere.Radius, *lodView) * renderContext.View.ModelLODDistanceFactorSqrt;

        // Check if decal is being culled
        if (Math::Square(DrawMinScreenSize * 0.5f) > screenRadiusSquared)
            return;

        renderContext.List->Decals.Add(this);
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
}

bool Decal::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return _bounds.Intersects(ray, distance, normal);
}

void Decal::OnEnable()
{
    GetSceneRendering()->AddCommon(this);
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
    GetSceneRendering()->RemoveCommon(this);

    // Base
    Actor::OnDisable();
}

void Decal::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    Transform t = _transform;
    t.Scale *= _size;
    t.GetWorld(_world);

    _bounds.Extents = Vector3::Half;
    _bounds.Transformation = _world;
    _bounds.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}
