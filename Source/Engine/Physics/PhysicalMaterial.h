// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Core/ISerializable.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Level/Tags.h"

/// <summary>
/// Physical materials are used to define the response of a physical object when interacting dynamically with the world.
/// </summary>
API_CLASS(Attributes="ContentContextMenu(\"New/Physics/Physical Material\")")
class FLAXENGINE_API PhysicalMaterial final : public ScriptingObject, public ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(PhysicalMaterial, ScriptingObject);
private:
    void* _material = nullptr;

public:
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

    /// <summary>
    /// Physical material density in kilograms per cubic meter (kg/m^3). Higher density means a higher weight of the object using this material. Wood is around 700, water is 1000, steel is around 8000.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Physical Material\")")
    float Density = 1000.0f;

    /// <summary>
    /// Physical material tag used to identify it (eg. `Surface.Wood`). Can be used to play proper footstep sounds when walking over object with that material.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"Physical Material\")")
    Tag Tag;

public:
    /// <summary>
    /// Gets the PhysX material.
    /// </summary>
    /// <returns>The native material object.</returns>
    void* GetPhysicsMaterial();

    /// <summary>
    /// Updates the physics material (after any property change).
    /// </summary>
    void UpdatePhysicsMaterial();
};
