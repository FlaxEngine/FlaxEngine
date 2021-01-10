// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Serialization/ISerializable.h"

// Default values for the physical material

#define PhysicalMaterial_Friction 0.7f
#define PhysicalMaterial_FrictionCombineMode PhysicsCombineMode::Average
#define PhysicalMaterial_OverrideFrictionCombineMode false
#define PhysicalMaterial_Restitution 0.3f
#define PhysicalMaterial_RestitutionCombineMode PhysicsCombineMode::Average
#define PhysicalMaterial_OverrideRestitutionCombineMode false

/// <summary>
/// Physical materials are used to define the response of a physical object when interacting dynamically with the world.
/// </summary>
class FLAXENGINE_API PhysicalMaterial : public ISerializable
{
private:

    PxMaterial* _material;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="PhysicalMaterial"/> class.
    /// </summary>
    PhysicalMaterial();

    /// <summary>
    /// Finalizes an instance of the <see cref="PhysicalMaterial"/> class.
    /// </summary>
    ~PhysicalMaterial();

public:

    /// <summary>
    /// The friction value of surface, controls how easily things can slide on this surface.
    /// </summary>
    float Friction = PhysicalMaterial_Friction;

    /// <summary>
    /// The friction combine mode, controls how friction is computed for multiple materials.
    /// </summary>
    PhysicsCombineMode FrictionCombineMode = PhysicalMaterial_FrictionCombineMode;

    /// <summary>
    /// If set we will use the FrictionCombineMode of this material, instead of the FrictionCombineMode found in the Physics settings. 
    /// </summary>
    bool OverrideFrictionCombineMode = PhysicalMaterial_OverrideFrictionCombineMode;

    /// <summary>
    /// The restitution or 'bounciness' of this surface, between 0 (no bounce) and 1 (outgoing velocity is same as incoming).
    /// </summary>
    float Restitution = PhysicalMaterial_Restitution;

    /// <summary>
    /// The restitution combine mode, controls how restitution is computed for multiple materials.
    /// </summary>
    PhysicsCombineMode RestitutionCombineMode = PhysicalMaterial_RestitutionCombineMode;

    /// <summary>
    /// If set we will use the RestitutionCombineMode of this material, instead of the RestitutionCombineMode found in the Physics settings.
    /// </summary>
    bool OverrideRestitutionCombineMode = PhysicalMaterial_OverrideRestitutionCombineMode;

public:

    /// <summary>
    /// Gets the PhysX material.
    /// </summary>
    /// <returns>The native material object.</returns>
    PxMaterial* GetPhysXMaterial();

    /// <summary>
    /// Updates the PhysX material (after any property change).
    /// </summary>
    void UpdatePhysXMaterial();

public:

    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
