// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    if (Float3::NearEqual(value, _size))
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
#include "Engine/Physics/Colliders/ColliderColorConfig.h"

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
    
    if (DisplayCollider)
    {
        if (GetIsTrigger())
        {
            DEBUG_DRAW_WIRE_BOX(_bounds, FlaxEngine::ColliderColors::TriggerColliderOutline, 0, false);
            DEBUG_DRAW_BOX(_bounds, FlaxEngine::ColliderColors::TriggerCollider, 0, true);
        }
        else
        {
            DEBUG_DRAW_WIRE_BOX(_bounds, FlaxEngine::ColliderColors::NormalColliderOutline, 0, false);
            DEBUG_DRAW_BOX(_bounds, FlaxEngine::ColliderColors::NormalCollider, 0, true);
        }
    }

    // Base
    Collider::OnDebugDraw();
}

void BoxCollider::OnDebugDrawSelected()
{

    if (GetIsTrigger())
    {
        DEBUG_DRAW_WIRE_BOX(_bounds, FlaxEngine::ColliderColors::TriggerColliderOutline, 0, false);
        DEBUG_DRAW_BOX(_bounds, FlaxEngine::ColliderColors::TriggerCollider, 0, true);
    }
    else
    {
        DEBUG_DRAW_WIRE_BOX(_bounds, FlaxEngine::ColliderColors::NormalColliderOutline, 0, false);
        DEBUG_DRAW_BOX(_bounds, FlaxEngine::ColliderColors::NormalCollider, 0, true);
    }

    if (_contactOffset > 0)
    {
        OrientedBoundingBox contactBounds = _bounds;
        contactBounds.Extents += Vector3(_contactOffset) / contactBounds.Transformation.Scale;
        DEBUG_DRAW_WIRE_BOX(contactBounds, Color::Blue.AlphaMultiplied(0.2f), 0, false);
    }


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
    Float3 size = _size * _cachedScale;
    const float minSize = 0.001f;
    size = Float3::Max(size.GetAbsolute() * 0.5f, Float3(minSize));
    collision.SetBox(size.Raw);
}
