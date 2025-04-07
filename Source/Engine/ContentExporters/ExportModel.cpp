// Copyright (c) Wojciech Figat. All rights reserved.

#include "AssetExporters.h"

#if COMPILE_WITH_ASSETS_EXPORTER

#include "Engine/Core/Log.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Graphics/Models/MeshAccessor.h"
#include "Engine/Core/DeleteMe.h"

ExportAssetResult AssetExporters::ExportModel(ExportAssetContext& context)
{
    auto asset = (ModelBase*)context.Asset.Get();
    auto lock = asset->Storage->LockSafe();
    auto path = GET_OUTPUT_PATH(context, "obj");
    const int32 lodIndex = 0;

    // Fetch chunk with data
    const auto chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
    if (asset->LoadChunk(chunkIndex))
        return ExportAssetResult::CannotLoadData;
    const auto chunk = asset->GetChunk(chunkIndex);
    if (!chunk)
        return ExportAssetResult::CannotLoadData;
    MemoryReadStream stream(chunk->Get(), chunk->Size());
    FileWriteStream* output = FileWriteStream::Open(path);
    if (output == nullptr)
        return ExportAssetResult::Error;
    DeleteMe<FileWriteStream> outputDeleteMe(output);

    const auto name = StringUtils::GetFileNameWithoutExtension(asset->GetPath()).ToStringAnsi();
    output->WriteText(StringAnsi::Format("# Exported model {0}\n", name.Get()));

    // Extract all meshes
    if (asset->GetLODsCount() <= lodIndex)
        return ExportAssetResult::Error;
    Array<MeshBase*> meshes;
    asset->GetMeshes(meshes, lodIndex);
    uint32 vertexStart = 1; // OBJ counts vertices from 1 not from 0
    ModelBase::MeshData meshData;
    byte meshVersion = stream.ReadByte();
    for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
    {
        auto mesh = meshes[meshIndex];
        if (asset->LoadMesh(stream, meshVersion, mesh, &meshData))
            return ExportAssetResult::CannotLoadData;
        if (meshData.Vertices == 0 || meshData.Triangles == 0)
            return ExportAssetResult::Error;
        MeshAccessor accessor;
        if (accessor.LoadFromMeshData(&meshData))
            return ExportAssetResult::CannotLoadAsset;
        output->WriteText(StringAnsi::Format("# Mesh {0}\n", meshIndex));

        auto positionStream = accessor.Position();
        if (!positionStream.IsValid())
            return ExportAssetResult::Error;
        for (uint32 i = 0; i < meshData.Vertices; i++)
        {
            auto v = positionStream.GetFloat3(i);
            output->WriteText(StringAnsi::Format("v {0} {1} {2}\n", v.X, v.Y, v.Z));
        }
        output->WriteChar('\n');

        auto texCoordStream = accessor.TexCoord();
        if (texCoordStream.IsValid())
        {
            for (uint32 i = 0; i < meshData.Vertices; i++)
            {
                auto v = texCoordStream.GetFloat2(i);
                output->WriteText(StringAnsi::Format("vt {0} {1}\n", v.X, v.Y));
            }
            output->WriteChar('\n');
        }

        auto normalStream = accessor.Normal();
        if (normalStream.IsValid())
        {
            for (uint32 i = 0; i < meshData.Vertices; i++)
            {
                auto v = normalStream.GetFloat3(i);
                MeshAccessor::UnpackNormal(v);
                output->WriteText(StringAnsi::Format("vn {0} {1} {2}\n", v.X, v.Y, v.Z));
            }
            output->WriteChar('\n');
        }

        if (meshData.IBStride == sizeof(uint16))
        {
            auto t = (const uint16*)meshData.IBData;
            for (uint32 i = 0; i < meshData.Triangles; i++)
            {
                auto i0 = vertexStart + *t++;
                auto i1 = vertexStart + *t++;
                auto i2 = vertexStart + *t++;
                output->WriteText(StringAnsi::Format("f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n", i0, i1, i2));
            }
        }
        else
        {
            auto t = (const uint32*)meshData.IBData;
            for (uint32 i = 0; i < meshData.Triangles; i++)
            {
                auto i0 = vertexStart + *t++;
                auto i1 = vertexStart + *t++;
                auto i2 = vertexStart + *t++;
                output->WriteText(StringAnsi::Format("f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n", i0, i1, i2));
            }
        }
        output->WriteChar('\n');

        vertexStart += meshData.Vertices;
    }

    if (output->HasError())
        return ExportAssetResult::Error;

    return ExportAssetResult::Ok;
}

ExportAssetResult AssetExporters::ExportSkinnedModel(ExportAssetContext& context)
{
    // The same code, except SkinnedModel::LoadMesh will be used to read Blend Shapes data
    return ExportModel(context);
}

#endif
