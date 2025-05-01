// Copyright (c) Wojciech Figat. All rights reserved.

#include "NavLink.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"

NavLink::NavLink(const SpawnParams& params)
    : Actor(params)
    , Start(Vector3::Zero)
    , End(Vector3::Zero)
    , Radius(30.0f)
    , BiDirectional(true)
{
}

void NavLink::UpdateBounds()
{
    const auto start = _transform.LocalToWorld(Start);
    const auto end = _transform.LocalToWorld(End);
    BoundingBox::FromPoints(start, end, _box);
    BoundingSphere::FromBox(_box, _sphere);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void NavLink::OnDebugDrawSelected()
{
    const auto start = _transform.LocalToWorld(Start);
    const auto end = _transform.LocalToWorld(End);
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(start, 10.0f), Color::BlueViolet, 0, true);
    DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(end, 10.0f), Color::BlueViolet, 0, true);
    DEBUG_DRAW_LINE(start, end, Color::BlueViolet, 0, true);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void NavLink::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(NavLink);

    SERIALIZE(Start);
    SERIALIZE(End);
    SERIALIZE(Radius);
    SERIALIZE(BiDirectional);
}

void NavLink::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(Start);
    DESERIALIZE(End);
    DESERIALIZE(Radius);
    DESERIALIZE(BiDirectional);
}

void NavLink::OnEnable()
{
    GetScene()->Navigation.Actors.Add(this);

    Actor::OnEnable();
}

void NavLink::OnDisable()
{
    Actor::OnDisable();

    GetScene()->Navigation.Actors.Remove(this);
}

void NavLink::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    UpdateBounds();
}
