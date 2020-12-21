// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "PhysicalMaterial.h"
#include "PhysicsSettings.h"
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
