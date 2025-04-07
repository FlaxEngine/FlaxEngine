// Copyright (c) Wojciech Figat. All rights reserved.

#include "SceneLightmapsData.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/Scene/Lightmap.h"

SceneLightmapsData::SceneLightmapsData(Scene* scene)
    : _lightmaps(4)
    , _scene(scene)
{
}

SceneLightmapsData::~SceneLightmapsData()
{
    // Ensure that lightmaps has been released
    ASSERT(_lightmaps.IsEmpty());
}

Lightmap* SceneLightmapsData::GetReadyLightmap(int32 index)
{
    return index >= 0 && index < _lightmaps.Count() && _lightmaps[index]->IsReady() ? _lightmaps[index] : nullptr;
}

#if USE_EDITOR

void SceneLightmapsData::GetCacheFolder(String* result)
{
    *result = _scene->GetDataFolderPath() / TEXT("Lightmaps");
}

void SceneLightmapsData::GetCachedLightmapPath(String* result, int32 lightmapIndex, int32 textureIndex)
{
    String cacheFolder;
    GetCacheFolder(&cacheFolder);
    *result = cacheFolder / String::Format(TEXT("Lightmap{0:0>2}-{1}"), lightmapIndex, textureIndex) + ASSET_FILES_EXTENSION_WITH_DOT;
}

#endif

void SceneLightmapsData::ClearLightmaps()
{
    UpdateLightmapsCollection(0, 0);
}

void SceneLightmapsData::LoadLightmaps(Array<SavedLightmapInfo>& lightmaps)
{
    // Unload previous
    UnloadLightmaps();

    if (lightmaps.IsEmpty())
        return;
    LOG(Info, "Loading {0} lightmap(s)", lightmaps.Count());

    for (int32 i = 0; i < lightmaps.Count(); i++)
    {
        _lightmaps.Add(New<Lightmap>(this, i, lightmaps[i]));
    }
}

void SceneLightmapsData::UnloadLightmaps()
{
    if (_lightmaps.HasItems())
    {
        LOG(Info, "Unloding {0} lightmap(s)", _lightmaps.Count());

        _lightmaps.ClearDelete();
    }
}

void SceneLightmapsData::SaveLightmaps(Array<SavedLightmapInfo>& lightmaps)
{
    lightmaps.Resize(_lightmaps.Count(), false);

    for (int32 i = 0; i < _lightmaps.Count(); i++)
    {
        _lightmaps[i]->GetInfo(lightmaps[i]);
    }
}

void SceneLightmapsData::UpdateLightmapsCollection(int32 count, int32 size)
{
    // Check if amount will change
    if (_lightmaps.Count() != count)
    {
        LOG(Info, "Changing amount of lightmaps from {0} to {1}", _lightmaps.Count(), count);

        // Remove too many entries
        while (_lightmaps.Count() > count)
        {
            auto lightmap = _lightmaps[count];
            Delete(lightmap);

            _lightmaps.RemoveAt(count);
        }

        // Add missing entries
        while (_lightmaps.Count() < count)
        {
            SavedLightmapInfo info;
            info.Lightmap0 = Guid::Empty;
            info.Lightmap1 = Guid::Empty;
            info.Lightmap2 = Guid::Empty;
            auto lightmap = New<Lightmap>(this, _lightmaps.Count(), info);

            _lightmaps.Add(lightmap);
        }
    }

    // Resize invalid size lightmaps
    for (int32 i = 0; i < _lightmaps.Count(); i++)
    {
        _lightmaps[i]->EnsureSize(size);
    }
}
