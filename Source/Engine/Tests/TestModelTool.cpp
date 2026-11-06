// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Tools/ModelTool/ModelTool.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Engine/Globals.h"
#if COMPILE_WITH_ASSETS_IMPORTER
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/ContentImporters/ImportModel.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#endif
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("ModelTool")
{
    SECTION("Test DetectLodIndex")
    {
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh LOD")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh LOD0")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh LOD1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_LOD1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_lod1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_lod2")) == 2);
        CHECK(ModelTool::DetectLodIndex(TEXT("lod0")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("lod1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("lod_2")) == 2);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_lod_0")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_lod_1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh lod_2")) == 2);
    }
}

TEST_CASE("ModelTool glTF materials")
{
    SECTION("Test roughness and metalness factors")
    {
        // One triangle drawn with two glTF materials that have only constant factors (no textures)
        const char* gltf = R"({
"asset": { "version": "2.0" },
"scenes": [ { "nodes": [ 0 ] } ],
"nodes": [ { "mesh": 0 } ],
"meshes": [ { "primitives": [ { "attributes": { "POSITION": 0 }, "material": 0 }, { "attributes": { "POSITION": 0 }, "material": 1 } ] } ],
"materials": [
  { "name": "Cloth", "pbrMetallicRoughness": { "roughnessFactor": 0.7, "metallicFactor": 0.0 } },
  { "name": "Metal", "pbrMetallicRoughness": { "roughnessFactor": 0.25, "metallicFactor": 1.0 } }
],
"buffers": [ { "byteLength": 36, "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAA" } ],
"bufferViews": [ { "buffer": 0, "byteLength": 36 } ],
"accessors": [ { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [ 0, 0, 0 ], "max": [ 1, 1, 0 ] } ]
})";
        const String path = Globals::TemporaryFolder / TEXT("TestModelToolMaterials.gltf");
        REQUIRE(!File::WriteAllBytes(path, gltf, (int32)StringUtils::Length(gltf)));
        ModelTool::Options options;
        options.Type = ModelTool::ModelType::Model;
        options.ImportTypes = ImportDataTypes::Geometry | ImportDataTypes::Materials;
        ModelData data;
        String errorMsg;
        const bool failed = ModelTool::ImportData(path, data, options, errorMsg);
        FileSystem::DeleteFile(path);
        REQUIRE(!failed);
        const MaterialSlotEntry* cloth = nullptr;
        const MaterialSlotEntry* metal = nullptr;
        for (const MaterialSlotEntry& e : data.Materials)
        {
            if (e.Name == TEXT("Cloth"))
                cloth = &e;
            else if (e.Name == TEXT("Metal"))
                metal = &e;
        }
        REQUIRE(cloth != nullptr);
        REQUIRE(metal != nullptr);
        CAPTURE(cloth->Roughness.Value, cloth->Metalness.Value, metal->Roughness.Value, metal->Metalness.Value);
        CHECK(Math::NearEqual(cloth->Roughness.Value, 0.7f));
        CHECK(Math::NearEqual(cloth->Metalness.Value, 0.0f));
        CHECK(Math::NearEqual(metal->Roughness.Value, 0.25f));
        CHECK(Math::NearEqual(metal->Metalness.Value, 1.0f));
    }
}

#if COMPILE_WITH_ASSETS_IMPORTER

TEST_CASE("ModelTool create")
{
    SECTION("Test model screen sizes")
    {
        // A model of two levels, one triangle each
        ModelData data;
        data.Materials.Resize(1);
        data.LODs.Resize(2);
        const Float3 positions[] = { Float3(0, 0, 0), Float3(100, 0, 0), Float3(0, 0, 100) };
        const Float3 normals[] = { Float3::Up, Float3::Up, Float3::Up };
        const uint32 indices[] = { 0, 1, 2 };
        for (int32 lod = 0; lod < 2; lod++)
        {
            auto mesh = New<MeshData>();
            mesh->Positions.Set(positions, 3);
            mesh->Normals.Set(normals, 3);
            mesh->Indices.Set(indices, 3);
            data.LODs[lod].Meshes.Add(mesh);
        }
        String path = Globals::TemporaryFolder / TEXT("TestModelScreenSizes.flax");
        ContentStorageManager::FormatPath(path);

        // Written with the caller's screen sizes
        data.LODs[0].ScreenSize = 3.0f;
        data.LODs[1].ScreenSize = 2.0f;
        data.MinScreenSize = 0.0f;
        REQUIRE(!AssetsImportingManager::Create(ImportModel::CreateWithScreenSizes, path, &data));
        AssetReference<Model> model = Content::Load<Model>(path);
        REQUIRE(model);
        CHECK(model->LODs[0].ScreenSize == 3.0f);
        CHECK(model->LODs[1].ScreenSize == 2.0f);
        CHECK(model->MinScreenSize == 0.0f);
        model = nullptr;
        Content::DeleteAsset(path);

        // Written with the sizes Flax computes, whatever the data holds
        REQUIRE(!AssetsImportingManager::Create(AssetsImportingManager::CreateModelTag, path, &data));
        model = Content::Load<Model>(path);
        REQUIRE(model);
        CHECK(model->LODs[0].ScreenSize == 1.0f);
        CHECK(model->LODs[1].ScreenSize == 0.5f);
        CHECK(model->MinScreenSize == 0.01f);
        model = nullptr;
        Content::DeleteAsset(path);
    }
}

#endif
