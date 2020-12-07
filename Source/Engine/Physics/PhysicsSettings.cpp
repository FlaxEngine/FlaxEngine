// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "PhysicsSettings.h"
#include "Engine/Serialization/Serialization.h"
#include "Physics.h"

void PhysicsSettings::Apply()
{
    // Set simulation parameters
    Physics::SetGravity(DefaultGravity);
    Physics::SetBounceThresholdVelocity(BounceThresholdVelocity);
    Physics::SetEnableCCD(!DisableCCD);

    // TODO: setting eADAPTIVE_FORCE requires PxScene setup (physx docs: This flag is not mutable, and must be set in PxSceneDesc at scene creation.)

    // Update all shapes

    // TODO: update all shapes filter data
    // TODO: update all shapes flags

    /*
    {
        get all actors and then:
        
        const PxU32 numShapes = actor->getNbShapes();
        PxShape** shapes = (PxShape**)SAMPLE_ALLOC(sizeof(PxShape*)*numShapes);
        actor->getShapes(shapes, numShapes);
        for (PxU32 i = 0; i < numShapes; i++)
        {
            ..
        }
        SAMPLE_FREE(shapes);
    }*/
}

void PhysicsSettings::RestoreDefault()
{
    DefaultGravity = PhysicsSettings_DefaultGravity;
    TriangleMeshTriangleMinAreaThreshold = PhysicsSettings_TriangleMeshTriangleMinAreaThreshold;
    BounceThresholdVelocity = PhysicsSettings_BounceThresholdVelocity;
    FrictionCombineMode = PhysicsSettings_FrictionCombineMode;
    RestitutionCombineMode = PhysicsSettings_RestitutionCombineMode;
    DisableCCD = PhysicsSettings_DisableCCD;
    EnableAdaptiveForce = PhysicsSettings_EnableAdaptiveForce;
    MaxDeltaTime = PhysicsSettings_MaxDeltaTime;
    EnableSubstepping = PhysicsSettings_EnableSubstepping;
    SubstepDeltaTime = PhysicsSettings_SubstepDeltaTime;
    MaxSubsteps = PhysicsSettings_MaxSubsteps;
    QueriesHitTriggers = PhysicsSettings_QueriesHitTriggers;
    SupportCookingAtRuntime = PhysicsSettings_SupportCookingAtRuntime;

    for (int32 i = 0; i < 32; i++)
        LayerMasks[i] = MAX_uint32;
}

void PhysicsSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(DefaultGravity);
    DESERIALIZE(TriangleMeshTriangleMinAreaThreshold);
    DESERIALIZE(BounceThresholdVelocity);
    DESERIALIZE(FrictionCombineMode);
    DESERIALIZE(RestitutionCombineMode);
    DESERIALIZE(DisableCCD);
    DESERIALIZE(EnableAdaptiveForce);
    DESERIALIZE(MaxDeltaTime);
    DESERIALIZE(EnableSubstepping);
    DESERIALIZE(SubstepDeltaTime);
    DESERIALIZE(MaxSubsteps);
    DESERIALIZE(QueriesHitTriggers);
    DESERIALIZE(SupportCookingAtRuntime);

    const auto layers = stream.FindMember("LayerMasks");
    if (layers != stream.MemberEnd())
    {
        auto& layersArray = layers->value;
        ASSERT(layersArray.IsArray());
        for (uint32 i = 0; i < layersArray.Size() && i < 32; i++)
        {
            LayerMasks[i] = layersArray[i].GetUint();
        }
    }
}
