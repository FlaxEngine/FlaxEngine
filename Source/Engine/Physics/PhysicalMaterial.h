// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Serialization/ISerializable.h"

/// <summary>
/// Physical materials are used to define the response of a physical object when interacting dynamically with the world.
/// </summary>
API_CLASS() class FLAXENGINE_API PhysicalMaterial final : public ISerializable
{
API_AUTO_SERIALIZATION();
DECLARE_SCRIPTING_TYPE_MINIMAL(PhysicalMaterial);
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
    API_FIELD(Attributes="EditorOrder(0), Limit(0), EditorDisplay(\"Physical Material\")")
    float Friction = 0.7f;

    /// <summary>
    /// The friction combine mode, controls how friction is computed for multiple materials.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1), EditorDisplay(\"Physical Material\")")
    PhysicsCombineMode FrictionCombineMode = PhysicsCombineMode::Average;

    /// <summary>
    /// If set we will use the FrictionCombineMode of this material, instead of the FrictionCombineMode found in the Physics settings. 
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    bool OverrideFrictionCombineMode = false;

    /// <summary>
    /// The restitution or 'bounciness' of this surface, between 0 (no bounce) and 1 (outgoing velocity is same as incoming).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(3), Range(0, 1), EditorDisplay(\"Physical Material\")")
    float Restitution = 0.3f;

    /// <summary>
    /// The restitution combine mode, controls how restitution is computed for multiple materials.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(4), EditorDisplay(\"Physical Material\")")
    PhysicsCombineMode RestitutionCombineMode = PhysicsCombineMode::Average;

    /// <summary>
    /// If set we will use the RestitutionCombineMode of this material, instead of the RestitutionCombineMode found in the Physics settings.
    /// </summary>
    API_FIELD(Attributes="HideInEditor")
    bool OverrideRestitutionCombineMode = false;

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
};
