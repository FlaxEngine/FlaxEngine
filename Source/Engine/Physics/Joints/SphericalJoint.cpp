// Copyright (c) Wojciech Figat. All rights reserved.

#include "SphericalJoint.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Physics/PhysicsBackend.h"

SphericalJoint::SphericalJoint(const SpawnParams& params)
    : Joint(params)
    , _flags(SphericalJointFlag::Limit)
{
}

void SphericalJoint::SetFlags(const SphericalJointFlag value)
{
    if (_flags == value)
        return;
    _flags = value;
    if (_joint)
        PhysicsBackend::SetSphericalJointFlags(_joint, value);
}

void SphericalJoint::SetLimit(const LimitConeRange& value)
{
    if (_limit == value)
        return;
    _limit = value;
    if (_joint)
        PhysicsBackend::SetSphericalJointLimit(_joint, value);
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"
#include "Engine/Core/Math/Color.h"

void SphericalJoint::OnDebugDrawSelected()
{
    const Vector3 source = GetPosition();
    const Vector3 target = GetTargetPosition();
    const float size = 15.0f;
    const float arrowSize = size / 100.0f * 0.5f;
    const Color color = Color::Green.AlphaMultiplied(0.6f);
    DEBUG_DRAW_WIRE_ARROW(source, GetOrientation(), arrowSize, arrowSize * 0.5f, Color::Red, 0, false);
    if (EnumHasAnyFlags(_flags, SphericalJointFlag::Limit))
    {
        DEBUG_DRAW_CONE(source, GetOrientation(), size, _limit.YLimitAngle * DegreesToRadians, _limit.ZLimitAngle * DegreesToRadians, color, 0, false);
    }
    else
    {
        DEBUG_DRAW_SPHERE(BoundingSphere(source, size), color, 0, false);
    }
    DEBUG_DRAW_LINE(source, target, Color::Green * 0.6f, 0, false);

    // Base
    Joint::OnDebugDrawSelected();
}

#endif

void SphericalJoint::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Joint::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SphericalJoint);

    SERIALIZE_MEMBER(Flags, _flags);
    SERIALIZE_MEMBER(ContactDist, _limit.ContactDist);
    SERIALIZE_MEMBER(Restitution, _limit.Restitution);
    SERIALIZE_MEMBER(Stiffness, _limit.Spring.Stiffness);
    SERIALIZE_MEMBER(Damping, _limit.Spring.Damping);
    SERIALIZE_MEMBER(YLimitAngle, _limit.YLimitAngle);
    SERIALIZE_MEMBER(ZLimitAngle, _limit.ZLimitAngle);
}

void SphericalJoint::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Joint::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Flags, _flags);
    DESERIALIZE_MEMBER(ContactDist, _limit.ContactDist);
    DESERIALIZE_MEMBER(Restitution, _limit.Restitution);
    DESERIALIZE_MEMBER(Stiffness, _limit.Spring.Stiffness);
    DESERIALIZE_MEMBER(Damping, _limit.Spring.Damping);
    DESERIALIZE_MEMBER(YLimitAngle, _limit.YLimitAngle);
    DESERIALIZE_MEMBER(ZLimitAngle, _limit.ZLimitAngle);
}

void* SphericalJoint::CreateJoint(const PhysicsJointDesc& desc)
{
    void* joint = PhysicsBackend::CreateSphericalJoint(desc);
    PhysicsBackend::SetSphericalJointFlags(joint, _flags);
    PhysicsBackend::SetSphericalJointLimit(joint, _limit);
    return joint;
}
