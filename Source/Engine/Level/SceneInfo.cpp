// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Level/Scene/SceneLightmapsData.h"
#include "Level.h"
#include "Actor.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/Serialization.h"

String SceneInfo::ToString() const
{
    return TEXT("SceneInfo");
}

void SceneInfo::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(SceneInfo);

    SERIALIZE(Title);
    SERIALIZE(Description);
    SERIALIZE(Copyright);

    if (Lightmaps.HasItems())
    {
        stream.JKEY("Lightmaps");
        stream.StartArray();
        const int32 lightmapsCount = Lightmaps.Count();
        for (int32 i = 0; i < lightmapsCount; i++)
        {
            auto& info = Lightmaps[i];

            stream.StartObject();

            stream.JKEY("Lightmap0");
            stream.Guid(info.Lightmap0);

            stream.JKEY("Lightmap1");
            stream.Guid(info.Lightmap1);

            stream.JKEY("Lightmap2");
            stream.Guid(info.Lightmap2);

            stream.EndObject();
        }
        stream.EndArray(lightmapsCount);
    }

    stream.JKEY("LightmapSettings");
    stream.Object(&LightmapSettings, nullptr);
}

void SceneInfo::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    Title = JsonTools::GetString(stream, "Title");
    Description = JsonTools::GetString(stream, "Description");
    Copyright = JsonTools::GetString(stream, "Copyright");

    auto lightmaps = stream.FindMember("Lightmaps");
    if (lightmaps != stream.MemberEnd() && lightmaps->value.IsArray())
    {
        auto& lightmapsArray = lightmaps->value;
        Lightmaps.Resize(lightmapsArray.Size(), false);

        for (int32 i = 0; i < Lightmaps.Count(); i++)
        {
            auto& lightmap = Lightmaps[i];
            auto& lightmapsData = lightmapsArray[i];

            lightmap.Lightmap0 = JsonTools::GetGuid(lightmapsData, "Lightmap0");
            lightmap.Lightmap1 = JsonTools::GetGuid(lightmapsData, "Lightmap1");
            lightmap.Lightmap2 = JsonTools::GetGuid(lightmapsData, "Lightmap2");
        }
    }

    auto lightmapSettings = stream.FindMember("LightmapSettings");
    if (lightmapSettings != stream.MemberEnd())
    {
        LightmapSettings.Deserialize(lightmapSettings->value, modifier);
    }
    else
    {
        // Clear settings
        ::LightmapSettings settings;
        LightmapSettings = settings;
    }
}
