// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"
#include "Engine/Graphics/Models/ModelInstanceEntry.h"

/// <summary>
/// Base class for actor types that use ModelInstanceEntries for mesh rendering.
/// </summary>
/// <seealso cref="Actor" />
API_CLASS(Abstract) class FLAXENGINE_API ModelInstanceActor : public Actor
{
DECLARE_SCENE_OBJECT_ABSTRACT(ModelInstanceActor);
public:

    /// <summary>
    /// The model instance buffer.
    /// </summary>
    ModelInstanceEntries Entries;

    /// <summary>
    /// Gets the model entries collection. Each entry contains data how to render meshes using this entry (transformation, material, shadows casting, etc.).
    /// </summary>
    API_PROPERTY(Attributes="Serialize, EditorOrder(1000), EditorDisplay(\"Entries\", EditorDisplayAttribute.InlineStyle), Collection(CanReorderItems = false, NotNullItems = true, ReadOnly = true, Spacing = 10)")
    FORCE_INLINE Array<ModelInstanceEntry> GetEntries() const
    {
        return Entries;
    }

    /// <summary>
    /// Sets the model entries collection. Each entry contains data how to render meshes using this entry (transformation, material, shadows casting, etc.).
    /// </summary>
    API_PROPERTY() void SetEntries(const Array<ModelInstanceEntry>& value);

    /// <summary>
    /// Sets the material to the entry slot. Can be used to override the material of the meshes using this slot.
    /// </summary>
    /// <param name="entryIndex">The material slot entry index.</param>
    /// <param name="material">The material to set.</param>
    API_FUNCTION() void SetMaterial(int32 entryIndex, MaterialBase* material);

    /// <summary>
    /// Utility to crate a new virtual Material Instance asset, set its parent to the currently applied material, and assign it to the entry. Can be used to modify the material parameters from code.
    /// </summary>
    /// <param name="entryIndex">The material slot entry index.</param>
    /// <returns>The created virtual material instance.</returns>
    API_FUNCTION() MaterialInstance* CreateAndSetVirtualMaterialInstance(int32 entryIndex);

    /// <summary>
    /// Determines if there is an intersection between the model actor mesh entry and a ray.
    /// If mesh data is available on the CPU performs exact intersection check with the geometry.
    /// Otherwise performs simple <see cref="BoundingBox"/> vs <see cref="Ray"/> test.
    /// For more efficient collisions detection and ray casting use physics.
    /// </summary>
    /// <param name="entryIndex">The material slot entry index to test.</param>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True if the actor is intersected by the ray, otherwise false.</returns>
    API_FUNCTION() virtual bool IntersectsEntry(int32 entryIndex, API_PARAM(Ref) const Ray& ray, API_PARAM(Out) float& distance, API_PARAM(Out) Vector3& normal)
    {
        return false;
    }

    /// <summary>
    /// Determines if there is an intersection between the model actor mesh entry and a ray.
    /// If mesh data is available on the CPU performs exact intersection check with the geometry.
    /// Otherwise performs simple <see cref="BoundingBox"/> vs <see cref="Ray"/> test.
    /// For more efficient collisions detection and ray casting use physics.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes and returns true, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <param name="entryIndex">When the method completes, contains the intersection entry index (if any valid).</param>
    /// <returns>True if the actor is intersected by the ray, otherwise false.</returns>
    API_FUNCTION() virtual bool IntersectsEntry(API_PARAM(Ref) const Ray& ray, API_PARAM(Out) float& distance, API_PARAM(Out) Vector3& normal, API_PARAM(Out) int32& entryIndex)
    {
        return false;
    }

protected:

    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
};
