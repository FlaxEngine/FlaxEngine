// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Lightmap.h"
#include "Scene.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Content/Content.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/SceneLightmapsData.h"
#if USE_EDITOR
#include "Engine/ContentImporters/ImportTexture.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/Graphics/Textures/TextureData.h"
#endif
#include "Engine/Serialization/Serialization.h"

void LightmapSettings::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(LightmapSettings);

    SERIALIZE(IndirectLightingIntensity);
    SERIALIZE(GlobalObjectsScale);
    SERIALIZE(ChartsPadding);
    SERIALIZE(AtlasSize);
    SERIALIZE(BounceCount);
    SERIALIZE(CompressLightmaps);
    SERIALIZE(UseGeometryWithNoMaterials);
    SERIALIZE(Quality);
}

void LightmapSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(IndirectLightingIntensity);
    DESERIALIZE(GlobalObjectsScale);
    DESERIALIZE(ChartsPadding);
    DESERIALIZE(AtlasSize);
    DESERIALIZE(BounceCount);
    DESERIALIZE(CompressLightmaps);
    DESERIALIZE(UseGeometryWithNoMaterials);
    DESERIALIZE(Quality);
}

Lightmap::Lightmap(SceneLightmapsData* manager, int32 index, const SavedLightmapInfo& info)
    : _manager(manager)
    , _index(index)
{
    // Try to load textures with given IDs
    _textures[0] = Content::LoadAsync<Texture>(info.Lightmap0);
    _textures[1] = Content::LoadAsync<Texture>(info.Lightmap1);
    _textures[2] = Content::LoadAsync<Texture>(info.Lightmap2);
}

void Lightmap::UpdateTexture(Texture* texture, int32 index)
{
    auto& prev = _textures[index];
    if (prev.Get() != texture)
    {
        LOG(Info, "Changing lightmap {0} texture {1} from '{2}' to '{3}'", _index, index, prev ? prev->ToString() : String::Empty, texture ? texture->ToString() : String::Empty);
        prev = texture;
    }
}

void Lightmap::EnsureSize(int32 size)
{
    ASSERT(size >= 4 && size <= 4096);
#if USE_EDITOR
    _size = size;
#endif

    // Check every texture
    for (int32 textureIndex = 0; textureIndex < ARRAY_COUNT(_textures); textureIndex++)
    {
        auto& texture = _textures[textureIndex];

        // Check if has texture linked
        if (texture)
        {
            // Wait for the loading
            if (texture->WaitForLoaded())
            {
                // Unlink texture that cannot be loaded
                LOG(Warning, "Lightmap::EnsureSize failed to load texture");
                texture = nullptr;
            }
            else
            {
                // Check if need to resize texture
                if (texture->GetTexture()->Width() != size || texture->GetTexture()->Height() != size)
                {
                    // Unlink texture and import new with valid size
                    LOG(Info, "Changing lightmap {0}:{1} size from {2} to {3}", _index, textureIndex, texture->GetTexture()->Size(), size);
                    texture = nullptr;
                }
            }
        }

        // Check if has missing texture
        if (texture == nullptr)
        {
#if USE_EDITOR

#if COMPILE_WITH_ASSETS_IMPORTER

            Guid id = Guid::New();
            LOG(Info, "Cannot load lightmap {0} ({1}:{2}). Creating new one.", id, _index, textureIndex);
            String assetPath;
            _manager->GetCachedLightmapPath(&assetPath, _index, textureIndex);

            // Import texture with custom options
            ImportTexture::Options options;
            options.Type = TextureFormatType::HdrRGBA;
            options.IndependentChannels = true;
            options.Compress = _manager->GetScene()->GetLightmapSettings().CompressLightmaps;
            options.IsAtlas = false;
            options.sRGB = false;
            options.NeverStream = false;
            options.InternalLoad.Bind<Lightmap, &Lightmap::OnInitLightmap>(this);
            if (AssetsImportingManager::Create(AssetsImportingManager::CreateTextureTag, assetPath, id, &options))
            {
                LOG(Error, "Cannot import empty lightmap {0}:{1}", _index, textureIndex);
            }
            auto result = Content::LoadAsync<Texture>(id);
#else
			auto result = nullptr;
#endif
            if (result == nullptr)
            {
                LOG(Error, "Cannot load new lightmap {0}:{1}", _index, textureIndex);
            }

            // Update asset
            texture = result;
#else
			LOG(Warning, "Cannot create empty lightmap. Saving data to the cooked content is disabled.");
#endif
        }
    }
}

#if USE_EDITOR

bool Lightmap::OnInitLightmap(TextureData& image)
{
    // Initialize with transparent image
    image.Width = image.Height = _size;
    image.Depth = 1;
    image.Format = PixelFormat::R8G8B8A8_UNorm;
    image.Items.Resize(1);
    image.Items[0].Mips.Resize(1);
    auto& mip = image.Items[0].Mips[0];
    mip.RowPitch = image.Width * 4;
    mip.DepthPitch = mip.RowPitch * image.Height;
    mip.Lines = image.Height;
    mip.Data.Allocate(mip.DepthPitch);
    Platform::MemoryClear(mip.Data.Get(), mip.DepthPitch);
    return false;
}

#endif
