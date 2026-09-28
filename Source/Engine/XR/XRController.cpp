// Copyright (c) Wojciech Figat. All rights reserved.

#include "XRController.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Serialization/Serialization.h"

namespace
{
    Array<XRController*> Controllers;
}

XRController::XRController(const SpawnParams& params)
    : Actor(params)
    , Hand(XRHand::Right)
    , PoseType(XRPoseType::Grip)
{
}

void XRController::UpdateAll()
{
    if (Controllers.IsEmpty() || !XR::IsActive())
        return;
    const Transform origin = XR::GetTrackingOrigin();
    for (XRController* controller : Controllers)
    {
        Transform pose;
        controller->_isTracked = XR::GetHandPose(controller->Hand, controller->PoseType, pose);
        if (!controller->_isTracked)
            continue;
        Transform transform = origin.LocalToWorld(pose);
        transform.Scale = controller->GetScale();
        controller->SetTransform(transform);
    }
}

void XRController::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(XRController);

    SERIALIZE(Hand);
    SERIALIZE(PoseType);
}

void XRController::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(Hand);
    DESERIALIZE(PoseType);
}

void XRController::OnEnable()
{
    Controllers.Add(this);

    // Base
    Actor::OnEnable();
}

void XRController::OnDisable()
{
    // Base
    Actor::OnDisable();

    Controllers.Remove(this);
    _isTracked = false;
}
