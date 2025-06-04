// Copyright (c) Wojciech Figat. All rights reserved.

#include "BoxVolume.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Serialization/Serialization.h"

BoxVolume::BoxVolume(const SpawnParams& params)
    : Actor(params)
    , _size(1000.0f)
{
}

void BoxVolume::SetSize(const Vector3& value)
{
    if (value != _size)
    {
        const auto prevBounds = _box;
        _size = value;
        OrientedBoundingBox::CreateCentered(Vector3::Zero, _size, _bounds);
        _bounds.Transform(_transform);
        _bounds.GetBoundingBox(_box);
        BoundingSphere::FromBox(_box, _sphere);
        OnBoundsChanged(prevBounds);
    }
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/Color.h"

Color BoxVolume::GetWiresColor()
{
    return Color::White;
}

void BoxVolume::OnDebugDraw()
{
    const Color color = GetWiresColor();
    DEBUG_DRAW_WIRE_BOX(_bounds, color, 0, true);

    // Base
    Actor::OnDebugDraw();
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
        Matrix world;
        Matrix::CreateWorld(min + vec * 0.5f, dir, up, world);
        world.Decompose(box.Transformation);
        Matrix invWorld;
        Matrix::Invert(world, invWorld);
        Vector3 vecLocal;
        Vector3::TransformNormal(vec * 0.5f, invWorld, vecLocal);
        box.Extents.X = margin;
        box.Extents.Y = margin;
        box.Extents.Z = vecLocal.Z;
        return box;
    }
}

void BoxVolume::OnDebugDrawSelected()
{
    const Color color = GetWiresColor();
    DEBUG_DRAW_WIRE_BOX(_bounds, color * 0.3f, 0, false);

    Vector3 sideLinks[] =
    {
        Vector3(0.5f, 0, 0),
        Vector3(-0.5f, 0, 0),
        Vector3(0, 0.5f, 0),
        Vector3(0, -0.5f, 0),
        Vector3(0, 0, 0.5f),
        Vector3(0, 0, -0.5f),
    };
    Vector3 sideLinksFinal[6];
    for (int32 i = 0; i < 6; i++)
    {
        sideLinksFinal[i] = sideLinks[i] * _size;
        _transform.LocalToWorld(sideLinksFinal[i], sideLinksFinal[i]);
    }
    for (int32 i = 0; i < 6; i++)
    {
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(sideLinksFinal[i], 10.0f), Color::YellowGreen, 0, true);
    }

    Vector3 corners[8];
    _bounds.GetCorners(corners);
    const float margin = 2.0f;
    const Color wiresColor = color.AlphaMultiplied(0.8f);
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
    Actor::OnDebugDrawSelected();
}

#endif

void BoxVolume::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(BoxVolume);

    SERIALIZE_MEMBER(Size, _size);
}

void BoxVolume::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Size, _size);
}

void BoxVolume::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    const auto prevBounds = _box;
    OrientedBoundingBox::CreateCentered(Vector3::Zero, _size, _bounds);
    _bounds.Transform(_transform);
    _bounds.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
    OnBoundsChanged(prevBounds);
}
