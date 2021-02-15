// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Json.h"

/// <summary>
/// Layers and objects tags settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API LayersAndTagsSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(LayersAndTagsSettings);
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
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static LayersAndTagsSettings* Get();

    // [SettingsBase]
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
