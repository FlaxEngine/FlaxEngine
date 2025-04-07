// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Core/ISerializable.h"
#include "Types.h"

/// <summary>
/// The model instance entry that describes how to draw it.
/// </summary>
API_STRUCT() struct FLAXENGINE_API ModelInstanceEntry : ISerializable
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE_MINIMAL(ModelInstanceEntry);

    /// <summary>
    /// The mesh surface material used for the rendering. If not assigned the default value will be used from the model asset.
    /// </summary>
    API_FIELD() AssetReference<MaterialBase> Material;

    /// <summary>
    /// The shadows casting mode.
    /// </summary>
    API_FIELD() ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

    /// <summary>
    /// Determines whenever this mesh is visible.
    /// </summary>
    API_FIELD() bool Visible = true;

    /// <summary>
    /// Determines whenever this mesh can receive decals.
    /// </summary>
    API_FIELD() bool ReceiveDecals = true;

public:
    bool operator==(const ModelInstanceEntry& other) const;
    FORCE_INLINE bool operator!=(const ModelInstanceEntry& other) const
    {
        return !operator==(other);
    }
};

/// <summary>
/// Collection of model instance entries.
/// </summary>
class FLAXENGINE_API ModelInstanceEntries : public Array<ModelInstanceEntry>, public ISerializable
{
public:
    /// <summary>
    /// Determines whether buffer is valid for the given model.
    /// </summary>
    /// <param name="model">The model.</param>
    /// <returns>True if this buffer is valid for the specified model object.</returns>
    bool IsValidFor(const Model* model) const;

    /// <summary>
    /// Determines whether buffer is valid ofr the given skinned model.
    /// </summary>
    /// <param name="model">The skinned model.</param>
    /// <returns>True if this buffer is valid for the specified skinned model object.</returns>
    bool IsValidFor(const SkinnedModel* model) const;

public:
    /// <summary>
    /// Setup buffer for given model
    /// </summary>
    /// <param name="model">Model to setup for</param>
    void Setup(const Model* model);

    /// <summary>
    /// Setup buffer for given skinned model
    /// </summary>
    /// <param name="model">Model to setup for</param>
    void Setup(const SkinnedModel* model);

    /// <summary>
    /// Setup buffer for given amount of material slots
    /// </summary>
    /// <param name="slotsCount">Amount of material slots</param>
    void Setup(int32 slotsCount);

    /// <summary>
    /// Setups the buffer if is invalid (has different amount of entries).
    /// </summary>
    /// <param name="model">The model.</param>
    void SetupIfInvalid(const Model* model);

    /// <summary>
    /// Setups the buffer if is invalid (has different amount of entries).
    /// </summary>
    /// <param name="model">The skinned model.</param>
    void SetupIfInvalid(const SkinnedModel* model);

    /// <summary>
    /// Clones the other buffer data
    /// </summary>
    /// <param name="other">The other buffer to clone.</param>
    void Clone(const ModelInstanceEntries* other)
    {
        *this = *other;
    }

    /// <summary>
    /// Releases the buffer data.
    /// </summary>
    void Release()
    {
        Resize(0);
    }

    bool HasContentLoaded() const;

public:
    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
