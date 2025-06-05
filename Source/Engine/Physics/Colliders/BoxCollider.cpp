// Copyright (c) Wojciech Figat. All rights reserved.

#include "BoxCollider.h"
#include "Engine/Physics/PhysicsBackend.h"
#include "Engine/Level/Scene/Scene.h"

BoxCollider::BoxCollider(const SpawnParams& params)
    : Collider(params)
    , _size(100.0f)
{
}

void BoxCollider::SetSize(const Float3& value)
{
    if (value == _size)
        return;
    _size = value;

    UpdateGeometry();
    UpdateBounds();
}

void BoxCollider::AutoResize(bool globalOrientation = true)
{
    Actor* parent = GetParent();
    if (Cast<Scene>(parent))
        return;

    // Get bounds of all siblings (excluding itself)
    const Vector3 parentScale = parent->GetScale();
    if (parentScale.IsAnyZero())
        return; // Avoid division by zero

    // Hacky way to get unrotated bounded box of parent.
    const Quaternion parentOrientation = parent->GetOrientation();
    parent->SetOrientation(Quaternion::Identity);
    BoundingBox parentBox = parent->GetBox();
    parent->SetOrientation(parentOrientation);

    for (const Actor* sibling : parent->Children)
    {
        if (sibling != this)
            BoundingBox::Merge(parentBox, sibling->GetBoxWithChildren(), parentBox);
    }
    const Vector3 parentSize = parentBox.GetSize();
    const Vector3 parentCenter = parentBox.GetCenter() - parent->GetPosition();

    // Update bounds
    SetLocalPosition(Vector3::Zero);
    SetSize(parentSize / parentScale);
    SetCenter(parentCenter / parentScale);
    if (globalOrientation)
        SetOrientation(GetOrientation() * Quaternion::Invert(GetOrientation()));
    else
        SetOrientation(parentOrientation);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Graphics/RenderView.h"

void BoxCollider::DrawPhysicsDebug(RenderView& view)
{
    const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
    if (!view.CullingFrustum.Intersects(sphere))
        return;
    if (view.Mode == ViewMode::PhysicsColliders && !GetIsTrigger())
        DEBUG_DRAW_BOX(_bounds, _staticActor ? Color::CornflowerBlue : Color::Orchid, 0, true);
    else
        DEBUG_DRAW_WIRE_BOX(_bounds, Color::GreenYellow * 0.8f, 0, true);
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
        const Vector3 dir = Float3::Normalize(vec);
        Quaternion orientation;
        if (Vector3::Dot(dir, Float3::Up) >= 0.999f)
            Quaternion::RotationAxis(Float3::Left, PI_HALF, orientation);
        else
            Quaternion::LookRotation(dir, Float3::Cross(Float3::Cross(dir, Float3::Up), dir), orientation);
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

void BoxCollider::OnDebugDrawSelected()
{
    const Color color = Color::GreenYellow;
    DEBUG_DRAW_WIRE_BOX(_bounds, color * 0.3f, 0, false);

    if (_contactOffset > 0)
    {
        OrientedBoundingBox contactBounds = _bounds;
        contactBounds.Extents += Vector3(_contactOffset) / contactBounds.Transformation.Scale;
        DEBUG_DRAW_WIRE_BOX(contactBounds, Color::Blue.AlphaMultiplied(0.2f), 0, false);
    }

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

bool BoxCollider::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _bounds.Intersects(ray, distance, normal);
}

void BoxCollider::UpdateBounds()
{
    // Cache bounds
    OrientedBoundingBox::CreateCentered(_center, _size, _bounds);
    _bounds.Transform(_transform);
    _bounds.GetBoundingBox(_box);
    BoundingSphere::FromBox(_box, _sphere);
}

void BoxCollider::GetGeometry(CollisionShape& collision)
{
    Float3 size = _size * _transform.Scale;
    const float minSize = 0.001f;
    size = Float3::Max(size.GetAbsolute() * 0.5f, Float3(minSize));
    collision.SetBox(size.Raw);
}
