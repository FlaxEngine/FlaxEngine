// Copyright (c) Wojciech Figat. All rights reserved.

#include "TerrainTools.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Cache.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Collections/CollectionPoolCache.h"
#include "Engine/Terrain/TerrainPatch.h"
#include "Engine/Terrain/Terrain.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Graphics/PixelFormatSampler.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Platform/FileSystem.h"
#include "FlaxEngine.Gen.h"

bool TerrainTools::TryGetPatchCoordToAdd(Terrain* terrain, const Ray& ray, Int2& result)
{
    CHECK_RETURN(terrain, true);
    result = Int2::Zero;
    const float patchSize = terrain->GetChunkSize() * TERRAIN_UNITS_PER_VERTEX * Terrain::ChunksCountEdge;

    // Try to pick any of the patch edges
    for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
    {
        const auto patch = terrain->GetPatch(patchIndex);
        const auto x = patch->GetX();
        const auto z = patch->GetZ();
        const auto bounds = patch->GetBounds();

        // TODO: use chunk neighbors to reduce algorithm complexity

#define CHECK_EDGE(dx, dz) \
			if (terrain->GetPatch(x + dx, z + dz) == nullptr) \
			{ \
				if (bounds.MakeOffsetted(Vector3(patchSize * dx, 0, patchSize * dz)).Intersects(ray)) \
				{ \
					result = Int2(x + dx, z + dz); \
					return true; \
				} \
			}

        CHECK_EDGE(1, 0);
        CHECK_EDGE(-1, 0);
        CHECK_EDGE(0, 1);
        CHECK_EDGE(0, -1);
        CHECK_EDGE(1, 1);
        CHECK_EDGE(1, -1);
        CHECK_EDGE(1, 1);
        CHECK_EDGE(-1, -1);
        CHECK_EDGE(-1, 1);

#undef CHECK_EDGE
    }

    // Use the default patch if none added
    if (terrain->GetPatchesCount() == 0)
    {
        return true;
    }

    return false;
}

struct TextureDataResult
{
    FlaxStorage::LockData Lock;
    BytesContainer Mip0Data;
    TextureData Tmp;
    uint32 RowPitch, SlicePitch;
    PixelFormat Format;
    Int2 Mip0Size;
    BytesContainer* Mip0DataPtr;
};

bool GetTextureDataForSampling(Texture* texture, TextureDataResult& data, bool hdr = false)
{
    // Lock asset chunks (if not virtual)
    data.Lock = texture->LockData();

    // Get the highest mip
    {
        PROFILE_CPU_NAMED("GetMipData");

        texture->GetMipDataWithLoading(0, data.Mip0Data);
        if (data.Mip0Data.IsInvalid())
        {
            LOG(Warning, "Failed to get texture data.");
            return true;
        }
        if (!texture->GetMipDataCustomPitch(0, data.RowPitch, data.SlicePitch))
            texture->GetTexture()->ComputePitch(0, data.RowPitch, data.SlicePitch);
    }
    data.Mip0Size = Int2(texture->GetTexture()->Size());
    data.Format = texture->GetTexture()->Format();

    // Decompress or convert data if need to
    data.Mip0DataPtr = &data.Mip0Data;
    if (PixelFormatExtensions::IsCompressed(data.Format) || PixelFormatSampler::Get(data.Format) == nullptr)
    {
        PROFILE_CPU_NAMED("Decompress");

        // Prepare source data descriptor (no data copy, just link the mip data)
        TextureData src;
        src.Width = data.Mip0Size.X;
        src.Height = data.Mip0Size.Y;
        src.Depth = 1;
        src.Format = data.Format;
        src.Items.Resize(1);
        src.Items[0].Mips.Resize(1);
        auto& srcMip = src.Items[0].Mips[0];
        srcMip.Data.Link(data.Mip0Data);
        srcMip.DepthPitch = data.SlicePitch;
        srcMip.RowPitch = data.RowPitch;
        srcMip.Lines = src.Height;

        // Decompress texture
        if (TextureTool::Convert(data.Tmp, src, hdr ? PixelFormat::R16G16B16A16_Float : PixelFormat::R8G8B8A8_UNorm))
        {
            LOG(Warning, "Failed to decompress data.");
            return true;
        }

        // Override source data and format
        data.Format = data.Tmp.Format;
        data.RowPitch = data.Tmp.Items[0].Mips[0].RowPitch;
        data.SlicePitch = data.Tmp.Items[0].Mips[0].DepthPitch;
        data.Mip0DataPtr = &data.Tmp.Items[0].Mips[0].Data;
    }

    // Check if can even sample the given format
    const auto sampler = PixelFormatSampler::Get(data.Format);
    if (sampler == nullptr)
    {
        LOG(Warning, "Texture format {0} cannot be sampled.", (int32)data.Format);
        return true;
    }

    return false;
}

bool TerrainTools::GenerateTerrain(Terrain* terrain, const Int2& numberOfPatches, Texture* heightmap, float heightmapScale, Texture* splatmap1, Texture* splatmap2)
{
    PROFILE_CPU_NAMED("Terrain.GenerateTerrain");
    CHECK_RETURN(terrain && terrain->GetChunkSize() != 0, true);
    if (numberOfPatches.X < 1 || numberOfPatches.Y < 1)
    {
        LOG(Warning, "Cannot setup terrain with no patches.");
        return false;
    }

    // Wait for assets to be loaded
    if (heightmap && heightmap->WaitForLoaded())
    {
        LOG(Warning, "Loading heightmap texture failed.");
        return true;
    }
    if (splatmap1 && splatmap1->WaitForLoaded())
    {
        LOG(Warning, "Loading splatmap texture failed.");
        return true;
    }
    if (splatmap2 && splatmap2->WaitForLoaded())
    {
        LOG(Warning, "Loading splatmap texture failed.");
        return true;
    }

    // Spawn patches
    terrain->AddPatches(numberOfPatches);

    // Prepare data
    const int32 heightmapSize = terrain->GetChunkSize() * Terrain::ChunksCountEdge + 1;
    const float heightmapSizeInv = 1.0f / (float)(heightmapSize - 1);
    const Float2 uvPerPatch = Float2::One / Float2(numberOfPatches);
    Array<float> heightmapData;
    heightmapData.Resize(heightmapSize * heightmapSize);

    // Get heightmap data
    if (heightmap && !Math::IsZero(heightmapScale))
    {
        // Get data
        TextureDataResult dataHeightmap;
        if (GetTextureDataForSampling(heightmap, dataHeightmap, true))
            return true;
        const auto sampler = PixelFormatSampler::Get(dataHeightmap.Format);

        // Initialize with sub-range of the input heightmap
        for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
        {
            auto patch = terrain->GetPatch(patchIndex);
            const Float2 uvStart = Float2((float)patch->GetX(), (float)patch->GetZ()) * uvPerPatch;

            // Sample heightmap pixels with interpolation to get actual heightmap vertices locations
            for (int32 z = 0; z < heightmapSize; z++)
            {
                for (int32 x = 0; x < heightmapSize; x++)
                {
                    const Float2 uv = uvStart + Float2(x * heightmapSizeInv, z * heightmapSizeInv) * uvPerPatch;
                    const Color color = sampler->SampleLinear(dataHeightmap.Mip0DataPtr->Get(), uv, dataHeightmap.Mip0Size, dataHeightmap.RowPitch);
                    heightmapData[z * heightmapSize + x] = color.R * heightmapScale;
                }
            }

            if (patch->SetupHeightMap(heightmapData.Count(), heightmapData.Get(), nullptr))
                return true;
        }
    }
    else
    {
        // Initialize flat heightmap data
        heightmapData.SetAll(0.0f);
        for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
        {
            auto patch = terrain->GetPatch(patchIndex);
            if (patch->SetupHeightMap(heightmapData.Count(), heightmapData.Get(), nullptr))
                return true;
        }
    }

    // Initialize terrain layers weights
    Texture* splatmaps[2] = { splatmap1, splatmap2 };
    Array<Color32> splatmapData;
    TextureDataResult data1;
    for (int32 index = 0; index < ARRAY_COUNT(splatmaps); index++)
    {
        const auto splatmap = splatmaps[index];
        if (!splatmap)
            continue;

        // Get splatmap data
        if (GetTextureDataForSampling(splatmap, data1))
            return true;
        const auto sampler = PixelFormatSampler::Get(data1.Format);

        // Modify heightmap splatmaps with sub-range of the input splatmaps
        splatmapData.Resize(heightmapSize * heightmapSize);
        for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
        {
            auto patch = terrain->GetPatch(patchIndex);
            const Float2 uvStart = Float2((float)patch->GetX(), (float)patch->GetZ()) * uvPerPatch;

            // Sample splatmap pixels with interpolation to get actual splatmap values
            for (int32 z = 0; z < heightmapSize; z++)
            {
                for (int32 x = 0; x < heightmapSize; x++)
                {
                    const Float2 uv = uvStart + Float2(x * heightmapSizeInv, z * heightmapSizeInv) * uvPerPatch;
                    const Color color = sampler->SampleLinear(data1.Mip0DataPtr->Get(), uv, data1.Mip0Size, data1.RowPitch);

                    Color32 layers;
                    layers.R = (byte)(Math::Min(1.0f, color.R) * 255.0f);
                    layers.G = (byte)(Math::Min(1.0f, color.G) * 255.0f);
                    layers.B = (byte)(Math::Min(1.0f, color.B) * 255.0f);
                    layers.A = (byte)(Math::Min(1.0f, color.A) * 255.0f);

                    splatmapData[z * heightmapSize + x] = layers;
                }
            }

            if (patch->ModifySplatMap(index, splatmapData.Get(), Int2::Zero, Int2(heightmapSize)))
                return true;
        }
    }

    return false;
}

StringAnsi TerrainTools::SerializePatch(Terrain* terrain, const Int2& patchCoord)
{
    CHECK_RETURN(terrain, StringAnsi::Empty);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK_RETURN(patch, StringAnsi::Empty);

    rapidjson_flax::StringBuffer buffer;
    CompactJsonWriter writerObj(buffer);
    JsonWriter& writer = writerObj;
    writer.StartObject();
    patch->Serialize(writer, nullptr);
    writer.EndObject();

    return StringAnsi(buffer.GetString());
}

void TerrainTools::DeserializePatch(Terrain* terrain, const Int2& patchCoord, const StringAnsiView& value)
{
    CHECK(terrain);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK(patch);

    rapidjson_flax::Document document;
    document.Parse(value.Get(), value.Length());
    CHECK(!document.HasParseError());

    auto modifier = Cache::ISerializeModifier.Get();
    modifier->EngineBuild = FLAXENGINE_VERSION_BUILD;
    Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);

    patch->Deserialize(document, modifier.Value);
    patch->UpdatePostManualDeserialization();
}

bool TerrainTools::InitializePatch(Terrain* terrain, const Int2& patchCoord)
{
    CHECK_RETURN(terrain, true);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK_RETURN(patch, true);
    return patch->InitializeHeightMap();
}

bool TerrainTools::ModifyHeightMap(Terrain* terrain, const Int2& patchCoord, const float* samples, const Int2& offset, const Int2& size)
{
    CHECK_RETURN(terrain, true);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK_RETURN(patch, true);
    return patch->ModifyHeightMap(samples, offset, size);
}

bool TerrainTools::ModifyHolesMask(Terrain* terrain, const Int2& patchCoord, const byte* samples, const Int2& offset, const Int2& size)
{
    CHECK_RETURN(terrain, true);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK_RETURN(patch, true);
    return patch->ModifyHolesMask(samples, offset, size);
}

bool TerrainTools::ModifySplatMap(Terrain* terrain, const Int2& patchCoord, int32 index, const Color32* samples, const Int2& offset, const Int2& size)
{
    CHECK_RETURN(terrain, true);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK_RETURN(patch, true);
    CHECK_RETURN(index >= 0 && index < TERRAIN_MAX_SPLATMAPS_COUNT, true);
    return patch->ModifySplatMap(index, samples, offset, size);
}

float* TerrainTools::GetHeightmapData(Terrain* terrain, const Int2& patchCoord)
{
    CHECK_RETURN(terrain, nullptr);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK_RETURN(patch, nullptr);
    return patch->GetHeightmapData();
}

byte* TerrainTools::GetHolesMaskData(Terrain* terrain, const Int2& patchCoord)
{
    CHECK_RETURN(terrain, nullptr);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK_RETURN(patch, nullptr);
    return patch->GetHolesMaskData();
}

Color32* TerrainTools::GetSplatMapData(Terrain* terrain, const Int2& patchCoord, int32 index)
{
    CHECK_RETURN(terrain, nullptr);
    auto patch = terrain->GetPatch(patchCoord);
    CHECK_RETURN(patch, nullptr);
    return patch->GetSplatMapData(index);
}

bool TerrainTools::ExportTerrain(Terrain* terrain, String outputFolder)
{
    PROFILE_CPU_NAMED("Terrain.ExportTerrain");
    CHECK_RETURN(terrain && terrain->GetPatchesCount() != 0, true);

    // Find size of heightmap in patches
    const auto firstPatch = terrain->GetPatch(0);
    Int2 start(firstPatch->GetX(), firstPatch->GetZ());
    Int2 end(start);
    for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
    {
        const auto patch = terrain->GetPatch(patchIndex);
        const Int2 pos(patch->GetX(), patch->GetZ());
        start = Int2::Min(start, pos);
        end = Int2::Max(end, pos);
    }
    const Int2 size = (end + 1) - start;

    // Allocate heightmap for a whole terrain (NumberOfPatches * 4x4 * ChunkSize + 1)
    const Int2 heightmapSize = size * Terrain::ChunksCountEdge * terrain->GetChunkSize() + 1;
    Array<float> heightmap;
    heightmap.Resize(heightmapSize.X * heightmapSize.Y);
    heightmap.SetAll(firstPatch->GetHeightmapData()[0]);

    // Fill heightmap with data from all patches
    const int32 rowSize = terrain->GetChunkSize() * Terrain::ChunksCountEdge + 1;
    for (int32 patchIndex = 0; patchIndex < terrain->GetPatchesCount(); patchIndex++)
    {
        const auto patch = terrain->GetPatch(patchIndex);
        const Int2 pos(patch->GetX() - start.X, patch->GetZ() - start.Y);
        const float* src = patch->GetHeightmapData();
        float* dst = heightmap.Get() + pos.X * (rowSize - 1) + pos.Y * heightmapSize.X * (rowSize - 1);
        for (int32 row = 0; row < rowSize; row++)
            Platform::MemoryCopy(dst + row * heightmapSize.X, src + row * rowSize, rowSize * sizeof(float));
    }

    // Interpolate to 16-bit int
    float maxHeight, minHeight;
    maxHeight = minHeight = heightmap[0];
    for (int32 i = 1; i < heightmap.Count(); i++)
    {
        float h = heightmap.Get()[i];
        if (maxHeight < h)
            maxHeight = h;
        else if (minHeight > h)
            minHeight = h;
    }
    const float alpha = MAX_uint16 / (maxHeight - minHeight);

    // Storage for pixel data
    Array<uint16> byteHeightmap;
    byteHeightmap.Resize(heightmap.Count());
    for (int32 i = 0; i < heightmap.Count(); i++)
    {
        float height = heightmap.Get()[i];
        byteHeightmap.Get()[i] = static_cast<uint16>(alpha * (height - minHeight));
    }

    // Create texture
    TextureData textureData;
    textureData.Width = heightmapSize.X;
    textureData.Height = heightmapSize.Y;
    textureData.Depth = 1;
    textureData.Format = PixelFormat::R16_UNorm;
    textureData.Items.Resize(1);
    textureData.Items[0].Mips.Resize(1);
    TextureMipData* srcMip = textureData.GetData(0, 0);
    srcMip->Data.Link(byteHeightmap.Get());
    srcMip->Lines = textureData.Height;
    srcMip->RowPitch = textureData.Width * sizeof(uint16);
    srcMip->DepthPitch = srcMip->Lines * srcMip->RowPitch;

    // Find next non-existing file heightmap file
    FileSystem::NormalizePath(outputFolder);
    const String baseFileName(TEXT("heightmap"));
    String outputPath;
    for (int32 i = 0; i < 100; i++)
    {
        outputPath = outputFolder / baseFileName + StringUtils::ToString(i) + TEXT(".png");
        if (!FileSystem::FileExists(outputPath))
            break;
    }

    return TextureTool::ExportTexture(outputPath, textureData);
}
