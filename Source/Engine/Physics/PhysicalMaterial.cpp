// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "PhysicalMaterial.h"
#include "PhysicsSettings.h"
#include "Engine/Serialization/JsonTools.h"
#include "Physics.h"
#include <ThirdParty/PhysX/PxPhysics.h>
#include <ThirdParty/PhysX/PxMaterial.h>

PhysicalMaterial::PhysicalMaterial()
    : _material(nullptr)
{
}

PhysicalMaterial::~PhysicalMaterial()
{
    if (_material)
    {
        Physics::RemoveMaterial(_material);
    }
}

PxMaterial* PhysicalMaterial::GetPhysXMaterial()
{
    if (_material == nullptr && CPhysX)
    {
        _material = CPhysX->createMaterial(Friction, Friction, Restitution);
        _material->userData = this;

        const PhysicsCombineMode useFrictionCombineMode = (OverrideFrictionCombineMode ? FrictionCombineMode : PhysicsSettings::Instance()->FrictionCombineMode);
        _material->setFrictionCombineMode(static_cast<PxCombineMode::Enum>(useFrictionCombineMode));

        const PhysicsCombineMode useRestitutionCombineMode = (OverrideRestitutionCombineMode ? RestitutionCombineMode : PhysicsSettings::Instance()->RestitutionCombineMode);
        _material->setRestitutionCombineMode(static_cast<PxCombineMode::Enum>(useRestitutionCombineMode));
    }

    return _material;
}

void PhysicalMaterial::UpdatePhysXMaterial()
{
    if (_material != nullptr)
    {
        _material->setStaticFriction(Friction);
        _material->setDynamicFriction(Friction);
        const PhysicsCombineMode useFrictionCombineMode = (OverrideFrictionCombineMode ? FrictionCombineMode : PhysicsSettings::Instance()->FrictionCombineMode);
        _material->setFrictionCombineMode(static_cast<PxCombineMode::Enum>(useFrictionCombineMode));

        _material->setRestitution(Restitution);
        const PhysicsCombineMode useRestitutionCombineMode = (OverrideRestitutionCombineMode ? RestitutionCombineMode : PhysicsSettings::Instance()->RestitutionCombineMode);
        _material->setRestitutionCombineMode(static_cast<PxCombineMode::Enum>(useRestitutionCombineMode));
    }
}

void PhysicalMaterial::Serialize(SerializeStream& stream, const void* otherObj)
{
    MISSING_CODE("PhysicalMaterial::Serialize is not implemented");
}

void PhysicalMaterial::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    Friction = JsonTools::GetFloat(stream, "Friction", PhysicalMaterial_Friction);
    FrictionCombineMode = JsonTools::GetEnum(stream, "FrictionCombineMode", PhysicalMaterial_FrictionCombineMode);
    OverrideFrictionCombineMode = JsonTools::GetBool(stream, "OverrideFrictionCombineMode", PhysicalMaterial_OverrideFrictionCombineMode);
    Restitution = JsonTools::GetFloat(stream, "Restitution", PhysicalMaterial_Restitution);
    RestitutionCombineMode = JsonTools::GetEnum(stream, "RestitutionCombineMode", PhysicalMaterial_RestitutionCombineMode);
    OverrideRestitutionCombineMode = JsonTools::GetBool(stream, "OverrideRestitutionCombineMode", PhysicalMaterial_OverrideRestitutionCombineMode);
}
