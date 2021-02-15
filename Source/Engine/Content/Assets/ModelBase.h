// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Models/MaterialSlot.h"
#include "Engine/Streaming/StreamableResource.h"

/// <summary>
/// Base class for asset types that can contain a model resource.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API ModelBase : public BinaryAsset, public StreamableResource
{
DECLARE_ASSET_HEADER(ModelBase);
protected:

    explicit ModelBase(const SpawnParams& params, const AssetInfo* info, StreamingGroup* group)
        : BinaryAsset(params, info)
        , StreamableResource(group)
    {
    }

public:

    /// <summary>
    /// The minimum screen size to draw this model (the bottom limit). Used to cull small models. Set to 0 to disable this feature.
    /// </summary>
    API_FIELD() float MinScreenSize = 0.0f;

    /// <summary>
    /// The list of material slots.
    /// </summary>
    API_FIELD(ReadOnly) Array<MaterialSlot> MaterialSlots;

    /// <summary>
    /// Gets the amount of the material slots used by this model asset.
    /// </summary>
    API_PROPERTY() int32 GetMaterialSlotsCount() const
    {
        return MaterialSlots.Count();
    }

    /// <summary>
    /// Resizes the material slots collection. Updates meshes that were using removed slots.
    /// </summary>
    API_FUNCTION() virtual void SetupMaterialSlots(int32 slotsCount)
    {
        CHECK(slotsCount >= 0 && slotsCount < 4096);
        if (!IsVirtual() && WaitForLoaded())
            return;

        ScopeLock lock(Locker);

        const int32 prevCount = MaterialSlots.Count();
        MaterialSlots.Resize(slotsCount, false);

        // Initialize slot names
        for (int32 i = prevCount; i < slotsCount; i++)
            MaterialSlots[i].Name = String::Format(TEXT("Material {0}"), i + 1);
    }

    /// <summary>
    /// Gets the material slot by the name.
    /// </summary>
    /// <param name="name">The slot name.</param>
    /// <returns>The material slot with the given name or null if cannot find it (asset may be not loaded yet).</returns>
    API_FUNCTION() MaterialSlot* GetSlot(const StringView& name)
    {
        MaterialSlot* result = nullptr;
        for (auto& slot : MaterialSlots)
        {
            if (slot.Name == name)
            {
                result = &slot;
                break;
            }
        }
        return result;
    }
};
