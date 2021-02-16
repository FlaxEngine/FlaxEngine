// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "BoxCollider.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/Utilities.h"
#include <ThirdParty/PhysX/PxShape.h>

BoxCollider::BoxCollider(const SpawnParams& params)
    : Collider(params)
    , _size(100.0f)
{
}

void BoxCollider::SetSize(const Vector3& value)
{
    if (Vector3::NearEqual(value, _size))
        return;

    _size = value;

    UpdateGeometry();
    UpdateBounds();
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void BoxCollider::DrawPhysicsDebug(RenderView& view)
{
    const Color color = Color::GreenYellow;
    DEBUG_DRAW_WIRE_BOX(_bounds, color * 0.8f, 0, true);
}

void BoxCollider::OnDebugDraw()
{
    if (GetIsTrigger())
    {
        const Color color = Color::GreenYellow;
        DEBUG_DRAW_WIRE_BOX(_bounds, color, 0, true);
    }

    // Base
    Collider::OnDebugDraw();
}

namespace
{
    OrientedBoundingBox GetWriteBox(const Vector3& min, const Vector3& max, const float margin)
    {
        OrientedBoundingBox box;
        const Vector3 vec = max - min;
        const Vector3 dir = Vector3::Normalize(vec);
        Quaternion orientation;
        if (Vector3::Dot(dir, Vector3::Up) >= 0.999f)
            Quaternion::RotationAxis(Vector3::Left, PI_HALF, orientation);
        else
            Quaternion::LookRotation(dir, Vector3::Cross(Vector3::Cross(dir, Vector3::Up), dir), orientation);
        const Vector3 up = orientation * Vector3::Up;
        Matrix::CreateWorld(min + vec * 0.5f, dir, up, box.Transformation);
        Matrix inv;
        Matrix::Invert(box.Transformation, inv);
        Vector3 vecLocal;
        Vector3::TransformNormal(vec * 0.5f, inv, vecLocal);
        box.Extents.X = margin;
        box.Extents.Y = margin;
        box.Extents.Z = vecLocal.Z;
        return box;
    }
}

void BoxCollider::OnDebugDrawSelected()
{
    const Color color = Color::GreenYellow;
    DEBUG_DRAW_WIRE_BOX(_bounds, color * 0.3f, 0, false);

    Vector3 corners[8];
    _bounds.GetCorners(corners);
    const float margin = 1.0f;
    const Color wiresColor = color.AlphaMultiplied(0.6f);
    DEBUG_DRAW_BOX(GetWriteBox(corners[0], corners[1], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[0], corners[3], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[0], corners[4], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[1], corners[2], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[1], corners[5], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[2], corners[3], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[2], corners[6], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[3], corners[7], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[4], corners[5], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[4], corners[7], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[5], corners[6], margin), wiresColor, 0, true);
    DEBUG_DRAW_BOX(GetWriteBox(corners[6], corners[7], margin), wiresColor, 0, true);

    // Base
    Collider::OnDebugDrawSelected();
}

#endif

bool BoxCollider::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return _bounds.Intersects(ray, distance, normal);
}

void BoxCollider::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Collider::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(BoxCollider);

    SERIALIZE_MEMBER(Size, _size);
}

void BoxCollider::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Collider::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Size, _size);
}

void BoxCollider::UpdateBounds()
{
    // Cache bounds
    OrientedBoundingBox::CreateCentered(_center, _size, _bounds);
    _bounds.Transform(_transform.GetWorld());
    _bounds.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}

void BoxCollider::GetGeometry(PxGeometryHolder& geometry)
{
    Vector3 size = _size * _cachedScale;
    size.Absolute();
    const float minSize = 0.001f;
    const PxBoxGeometry box(Math::Max(size.X * 0.5f, minSize), Math::Max(size.Y * 0.5f, minSize), Math::Max(size.Z * 0.5f, minSize));
    geometry.storeAny(box);
}
