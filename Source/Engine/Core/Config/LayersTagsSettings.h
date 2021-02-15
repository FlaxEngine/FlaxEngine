// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Json.h"

/// <summary>
/// Layers and objects tags settings.
/// </summary>
/// <seealso cref="Settings{LayersAndTagsSettings}" />
class LayersAndTagsSettings : public Settings<LayersAndTagsSettings>
{
public:

    /// <summary>
    /// The tags names.
    /// </summary>
    Array<String> Tags;

    /// <summary>
    /// The layers names.
    /// </summary>
    String Layers[32];

public:

    /// <summary>
    /// Gets or adds the tag (returns the tag index).
    /// </summary>
    /// <param name="tag">The tag.</param>
    /// <returns>The tag index.</returns>
    int32 GetOrAddTag(const String& tag)
    {
        int32 index = Tags.Find(tag);
        if (index == INVALID_INDEX)
        {
            index = Tags.Count();
            Tags.Add(tag);
        }
        return index;
    }

    /// <summary>
    /// Gets the amount of non empty layer names (from the beginning, trims the last ones).
    /// </summary>
    /// <returns>The layers count.</returns>
    int32 GetNonEmptyLayerNamesCount() const
    {
        int32 result = 31;
        while (result >= 0 && Layers[result].IsEmpty())
            result--;
        return result + 1;
    }

public:

    // [Settings]
    void RestoreDefault() override
    {
        Tags.Clear();
        for (int32 i = 0; i < 32; i++)
            Layers[i].Clear();
    }
    
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        const auto tags = stream.FindMember("Tags");
        if (tags != stream.MemberEnd())
        {
            auto& tagsArray = tags->value;
            ASSERT(tagsArray.IsArray());
            Tags.EnsureCapacity(tagsArray.Size());

            // Note: we cannot remove tags at runtime so this should deserialize them in additive mode
            // Tags are stored as tagIndex in actors so collection change would break the linkage

            for (uint32 i = 0; i < tagsArray.Size(); i++)
            {
                auto& v = tagsArray[i];
                if (v.IsString())
                {
                    const String tag = v.GetText();
                    if (!Tags.Contains(tag))
                        Tags.Add(tag);
                }
            }
        }

        const auto layers = stream.FindMember("Layers");
        if (layers != stream.MemberEnd())
        {
            auto& layersArray = layers->value;
            ASSERT(layersArray.IsArray());

            for (uint32 i = 0; i < layersArray.Size() && i < 32; i++)
            {
                auto& v = layersArray[i];
                if (v.IsString())
                    Layers[i] = v.GetText();
                else
                    Layers[i].Clear();
            }
        }
    }
};
