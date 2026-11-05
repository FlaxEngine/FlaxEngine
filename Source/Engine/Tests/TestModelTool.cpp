// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Tools/ModelTool/ModelTool.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Engine/Globals.h"
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
