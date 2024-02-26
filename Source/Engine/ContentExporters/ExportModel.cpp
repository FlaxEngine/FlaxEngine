// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "AssetExporters.h"

#if COMPILE_WITH_ASSETS_EXPORTER

#include "Engine/Core/Log.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Core/DeleteMe.h"

ExportAssetResult AssetExporters::ExportModel(ExportAssetContext& context)
{
    // Prepare
    auto asset = (Model*)context.Asset.Get();
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
    const auto& lod = asset->LODs[lodIndex];
    int32 vertexStart = 1; // OBJ counts vertices from 1 not from 0
    for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
    {
        auto& mesh = lod.Meshes[meshIndex];

        // #MODEL_DATA_FORMAT_USAGE
        uint32 vertices;
        stream.ReadUint32(&vertices);
        uint32 triangles;
        stream.ReadUint32(&triangles);
        uint32 indicesCount = triangles * 3;
        bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
        uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
        if (vertices == 0 || triangles == 0)
            return ExportAssetResult::Error;
        auto vb0 = stream.Move<VB0ElementType>(vertices);
        auto vb1 = stream.Move<VB1ElementType>(vertices);
        bool hasColors = stream.ReadBool();
        VB2ElementType18* vb2 = nullptr;
        if (hasColors)
        {
            vb2 = stream.Move<VB2ElementType18>(vertices);
        }
        auto ib = stream.Move<byte>(indicesCount * ibStride);

        output->WriteText(StringAnsi::Format("# Mesh {0}\n", meshIndex));

        for (uint32 i = 0; i < vertices; i++)
        {
            auto v = vb0[i].Position;
            output->WriteText(StringAnsi::Format("v {0} {1} {2}\n", v.X, v.Y, v.Z));
        }

        output->WriteChar('\n');

        for (uint32 i = 0; i < vertices; i++)
        {
            auto v = vb1[i].TexCoord.ToFloat2();
            output->WriteText(StringAnsi::Format("vt {0} {1}\n", v.X, v.Y));
        }

        output->WriteChar('\n');

        for (uint32 i = 0; i < vertices; i++)
        {
            auto v = vb1[i].Normal.ToFloat3() * 2.0f - 1.0f;
            output->WriteText(StringAnsi::Format("vn {0} {1} {2}\n", v.X, v.Y, v.Z));
        }

        output->WriteChar('\n');

        if (use16BitIndexBuffer)
        {
            auto t = (uint16*)ib;
            for (uint32 i = 0; i < triangles; i++)
            {
                auto i0 = vertexStart + *t++;
                auto i1 = vertexStart + *t++;
                auto i2 = vertexStart + *t++;
                output->WriteText(StringAnsi::Format("f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n", i0, i1, i2));
            }
        }
        else
        {
            auto t = (uint32*)ib;
            for (uint32 i = 0; i < triangles; i++)
            {
                auto i0 = vertexStart + *t++;
                auto i1 = vertexStart + *t++;
                auto i2 = vertexStart + *t++;
                output->WriteText(StringAnsi::Format("f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n", i0, i1, i2));
            }
        }

        output->WriteChar('\n');

        vertexStart += vertices;
    }

    if (output->HasError())
        return ExportAssetResult::Error;

    return ExportAssetResult::Ok;
}

ExportAssetResult AssetExporters::ExportSkinnedModel(ExportAssetContext& context)
{
    // Prepare
    auto asset = (SkinnedModel*)context.Asset.Get();
    auto lock = asset->Storage->LockSafe();
    auto path = GET_OUTPUT_PATH(context, "obj");
    const int32 lodIndex = 0;

    // Fetch chunk with data
    const auto chunkIndex = 1;
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
    const auto& lod = asset->LODs[lodIndex];
    int32 vertexStart = 1; // OBJ counts vertices from 1 not from 0
    byte version = stream.ReadByte();
    for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
    {
        auto& mesh = lod.Meshes[meshIndex];

        // #MODEL_DATA_FORMAT_USAGE
        uint32 vertices;
        stream.ReadUint32(&vertices);
        uint32 triangles;
        stream.ReadUint32(&triangles);
        uint16 blendShapesCount;
        stream.ReadUint16(&blendShapesCount);
        for (int32 blendShapeIndex = 0; blendShapeIndex < blendShapesCount; blendShapeIndex++)
        {
            stream.ReadBool();
            uint32 tmp;
            stream.ReadUint32(&tmp);
            stream.ReadUint32(&tmp);
            uint32 blendShapeVertices;
            stream.ReadUint32(&blendShapeVertices);
            stream.Move(blendShapeVertices * sizeof(BlendShapeVertex));
        }
        uint32 indicesCount = triangles * 3;
        bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
        uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
        if (vertices == 0 || triangles == 0)
            return ExportAssetResult::Error;
        auto vb0 = stream.Move<VB0SkinnedElementType>(vertices);
        auto ib = stream.Move<byte>(indicesCount * ibStride);

        output->WriteText(StringAnsi::Format("# Mesh {0}\n", meshIndex));

        for (uint32 i = 0; i < vertices; i++)
        {
            auto v = vb0[i].Position;
            output->WriteText(StringAnsi::Format("v {0} {1} {2}\n", v.X, v.Y, v.Z));
        }

        output->WriteChar('\n');

        for (uint32 i = 0; i < vertices; i++)
        {
            auto v = vb0[i].TexCoord.ToFloat2();
            output->WriteText(StringAnsi::Format("vt {0} {1}\n", v.X, v.Y));
        }

        output->WriteChar('\n');

        for (uint32 i = 0; i < vertices; i++)
        {
            auto v = vb0[i].Normal.ToFloat3() * 2.0f - 1.0f;
            output->WriteText(StringAnsi::Format("vn {0} {1} {2}\n", v.X, v.Y, v.Z));
        }

        output->WriteChar('\n');

        if (use16BitIndexBuffer)
        {
            auto t = (uint16*)ib;
            for (uint32 i = 0; i < triangles; i++)
            {
                auto i0 = vertexStart + *t++;
                auto i1 = vertexStart + *t++;
                auto i2 = vertexStart + *t++;
                output->WriteText(StringAnsi::Format("f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n", i0, i1, i2));
            }
        }
        else
        {
            auto t = (uint32*)ib;
            for (uint32 i = 0; i < triangles; i++)
            {
                auto i0 = vertexStart + *t++;
                auto i1 = vertexStart + *t++;
                auto i2 = vertexStart + *t++;
                output->WriteText(StringAnsi::Format("f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n", i0, i1, i2));
            }
        }

        output->WriteChar('\n');

        vertexStart += vertices;
    }

    if (output->HasError())
        return ExportAssetResult::Error;

    return ExportAssetResult::Ok;
}

#endif
