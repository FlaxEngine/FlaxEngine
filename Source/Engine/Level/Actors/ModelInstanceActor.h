// Copyright (c) Wojciech Figat. All rights reserved.

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

    /// <summary>
    /// Utility container to reference a single mesh within <see cref="ModelInstanceActor"/>.
    /// </summary>
    API_STRUCT(NoDefault) struct MeshReference : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(MeshReference);
        API_AUTO_SERIALIZATION();

        // Owning actor.
        API_FIELD() ScriptingObjectReference<ModelInstanceActor> Actor;
        // Index of the LOD (Level Of Detail).
        API_FIELD() int32 LODIndex = 0;
        // Index of the mesh (within the LOD).
        API_FIELD() int32 MeshIndex = 0;

        String ToString() const;
    };

protected:
    int32 _sceneRenderingKey = -1; // Uses SceneRendering::DrawCategory::SceneDrawAsync

public:
    /// <summary>
    /// The model instance buffer.
    /// </summary>
    ModelInstanceEntries Entries;

    /// <summary>
    /// Gets the model entries collection. Each entry contains data how to render meshes using this entry (transformation, material, shadows casting, etc.).
    /// </summary>
    API_PROPERTY(Attributes="Serialize, EditorOrder(1000), EditorDisplay(\"Entries\", EditorDisplayAttribute.InlineStyle), Collection(CanReorderItems=false, NotNullItems=true, CanResize=false, Spacing=10)")
    FORCE_INLINE const Array<ModelInstanceEntry>& GetEntries() const
    {
        return Entries;
    }

    /// <summary>
    /// Sets the model entries collection. Each entry contains data how to render meshes using this entry (transformation, material, shadows casting, etc.).
    /// </summary>
    API_PROPERTY() void SetEntries(const Array<ModelInstanceEntry>& value);

    /// <summary>
    /// Gets the material slots array set on the asset (eg. model or skinned model asset).
    /// </summary>
    API_PROPERTY(Sealed) virtual const Span<class MaterialSlot> GetMaterialSlots() const = 0;

    /// <summary>
    /// Gets the material used to draw the meshes which are assigned to that slot (set in Entries or model's default).
    /// </summary>
    /// <param name="entryIndex">The material slot entry index.</param>
    API_FUNCTION(Sealed) virtual MaterialBase* GetMaterial(int32 entryIndex) = 0;

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
    API_FUNCTION() virtual bool IntersectsEntry(int32 entryIndex, API_PARAM(Ref) const Ray& ray, API_PARAM(Out) Real& distance, API_PARAM(Out) Vector3& normal)
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
    API_FUNCTION() virtual bool IntersectsEntry(API_PARAM(Ref) const Ray& ray, API_PARAM(Out) Real& distance, API_PARAM(Out) Vector3& normal, API_PARAM(Out) int32& entryIndex)
    {
        return false;
    }

    /// <summary>
    /// Extracts mesh buffer data from CPU. Might be cached internally (eg. by Model/SkinnedModel).
    /// </summary>
    /// <param name="ref">Mesh reference.</param>
    /// <param name="type">Buffer type</param>
    /// <param name="result">The result data</param>
    /// <param name="count">The amount of items inside the result buffer.</param>
    /// <param name="layout">The result layout of the result buffer (for vertex buffers). Optional, pass null to ignore it.</param>
    /// <returns>True if failed, otherwise false.</returns>
    virtual bool GetMeshData(const MeshReference& ref, MeshBufferType type, BytesContainer& result, int32& count, GPUVertexLayout** layout = nullptr) const
    {
        return true;
    }

    /// <summary>
    /// Gets the mesh deformation utility for this model instance (optional).
    /// </summary>
    /// <returns>Model deformation utility or null if not supported.</returns>
    virtual MeshDeformation* GetMeshDeformation() const
    {
        return nullptr;
    }

    /// <summary>
    /// Updates the model bounds (eg. when mesh has applied significant deformation).
    /// </summary>
    virtual void UpdateBounds() = 0;

protected:
    virtual void WaitForModelLoad();

public:
    // [Actor]
    void OnLayerChanged() override;
    void OnStaticFlagsChanged() override;
    void OnTransformChanged() override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
};
